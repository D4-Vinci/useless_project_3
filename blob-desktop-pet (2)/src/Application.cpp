#include "Application.h"
#include <SDL2/SDL.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <random>
#include <climits>

namespace {
Application* g_appForSignals = nullptr;
volatile sig_atomic_t g_wantShutdown = 0;

void HandleSignal(int) { g_wantShutdown = 1; }
} // namespace

int Application::Run(int argc, char** argv) {
    std::string arg = (argc > 1) ? argv[1] : "";

    if (arg == "--remove") return RunAsClientCommand("remove");
    if (arg == "--status") return RunAsClientCommand("status");
    if (arg == "--toggle") {
        if (CommandInterface::IsRunning()) return RunAsClientCommand("remove");
        return RunServer();
    }
    if (!arg.empty()) {
        std::fprintf(stderr,
                      "blob: unknown argument '%s'\n"
                      "usage: blob [--remove|--toggle|--status]\n",
                      arg.c_str());
        return 2;
    }

    // Plain `blob` launch.
    if (CommandInterface::IsRunning()) {
        std::fprintf(stderr, "blob: already running (use --remove to stop it, "
                              "or --status to check)\n");
        return 1;
    }
    return RunServer();
}

int Application::RunAsClientCommand(const std::string& command) {
    auto reply = CommandInterface::SendCommand(command);
    if (!reply) {
        std::fprintf(stdout, "blob: no instance is currently running\n");
        return command == "status" ? 0 : 1;
    }
    if (command == "status") {
        std::fprintf(stdout, "blob: running (%s)\n", reply->c_str());
    } else {
        std::fprintf(stdout, "blob: %s\n", reply->c_str());
    }
    return 0;
}

void Application::InstallSignalHandlers() {
    g_appForSignals = this;
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);
}

int Application::RunServer() {
    if (!commandInterface_.BecomeServer()) {
        std::fprintf(stderr, "blob: another instance appears to be starting up; "
                              "try again in a moment\n");
        return 1;
    }

    InstallSignalHandlers();

    window_ = std::make_unique<DesktopWindow>();

    // Open a throwaway X display first just to size the overlay window to
    // the full virtual desktop (bounding box of all monitors), so the blob
    // can freely roam without the window itself needing to move.
    Display* probe = XOpenDisplay(nullptr);
    if (!probe) {
        std::fprintf(stderr, "blob: cannot connect to X display (is a display "
                              "server running?)\n");
        commandInterface_.Shutdown();
        return 1;
    }
    ScreenInfo probeScreen(probe);
    int totalW = 0, totalH = 0;
    for (const auto& m : probeScreen.Monitors()) {
        totalW = std::max(totalW, m.x + m.w);
        totalH = std::max(totalH, m.y + m.h);
    }
    XCloseDisplay(probe);
    if (totalW <= 0 || totalH <= 0) {
        totalW = 1920;
        totalH = 1080;
    }

    if (!window_->Initialize(totalW, totalH)) {
        std::fprintf(stderr, "blob: failed to create desktop overlay window\n");
        commandInterface_.Shutdown();
        return 1;
    }

    screen_ = std::make_unique<ScreenInfo>(window_->X11Display());
    cursor_ = std::make_unique<CursorTracker>(window_->X11Display());

    std::random_device rd;
    uint32_t seed = rd();

    movement_ = std::make_unique<MovementController>();
    // Start roughly centered on the primary monitor.
    const MonitorRect& primary = screen_->Monitors().front();
    movement_->SetPosition(static_cast<float>(primary.CenterX()),
                            static_cast<float>(primary.CenterY()));

    emotion_ = std::make_unique<EmotionSystem>(seed);
    blob_ = std::make_unique<Blob>(seed);
    behavior_ = std::make_unique<BehaviorController>(seed, *movement_, *blob_, *emotion_,
                                                       *screen_);

    MainLoop();

    window_->Shutdown();
    commandInterface_.Shutdown();
    return 0;
}

void Application::MainLoop() {
    using clock = std::chrono::steady_clock;
    auto lastTime = clock::now();
    float monitorRefreshTimer = 0.0f;

    // Frame pacing: run briskly while the blob is active, drop to a much
    // lower rate while it's fully idle/still so the process barely touches
    // the CPU over long periods.
    const double kActiveFrameSeconds = 1.0 / 60.0;
    const double kIdleFrameSeconds = 1.0 / 24.0; // still responsive enough to catch clicks

    while (running_ && !g_wantShutdown) {
        auto now = clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        if (dt > 0.25f) dt = 0.25f; // clamp huge hitches (e.g. laptop sleep/resume)

        // Drain the X11/SDL event queue so it never backs up; the window
        // takes no input, but Expose/ConfigureNotify still arrive.
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running_ = false;
        }

        // Cheap non-blocking IPC poll for --remove / --toggle / --status.
        std::string cmd = commandInterface_.PollIncoming();
        if (cmd == "remove") {
            running_ = false;
        }

        monitorRefreshTimer += dt;
        if (monitorRefreshTimer > 5.0f) {
            screen_->Refresh(); // pick up monitor/resolution changes, not every frame
            monitorRefreshTimer = 0.0f;
        }

        // Global cursor awareness: lets the blob glance toward a nearby
        // mouse, and lets a click register as a "bonk" if it lands on the
        // blob. This never intercepts the click itself — it's a read-only
        // poll of the pointer, so the click still passes through to
        // whatever's underneath the (click-through) overlay window.
        cursor_->Poll();
        behavior_->SetCursorInfo(static_cast<float>(cursor_->X()),
                                  static_cast<float>(cursor_->Y()), cursor_->Valid());
        if (cursor_->Valid() && cursor_->JustPressed()) {
            behavior_->NotifyClick(static_cast<float>(cursor_->X()),
                                    static_cast<float>(cursor_->Y()));
        }

        behavior_->Update(dt);
        movement_->Update(dt);

        float dirX = 0.0f, dirY = 0.0f;
        float speed = movement_->Speed();
        bool isMoving = speed > 2.0f;
        if (isMoving) {
            dirX = movement_->VelX() / speed;
            dirY = movement_->VelY() / speed;
        }

        Emotion dominant = emotion_->Dominant();
        blob_->Update(dt, movement_->SpeedFraction(), isMoving, dirX, dirY, dominant);

        window_->Clear();
        blob_->Render(window_->Renderer(), movement_->X(), movement_->Y());
        window_->Present();

        bool restful = behavior_->IsRestful();
        double targetFrameSeconds = restful ? kIdleFrameSeconds : kActiveFrameSeconds;

        auto frameEnd = clock::now();
        double elapsed = std::chrono::duration<double>(frameEnd - now).count();
        double remaining = targetFrameSeconds - elapsed;
        if (remaining > 0) {
            SDL_Delay(static_cast<Uint32>(remaining * 1000.0));
        }
    }
}
