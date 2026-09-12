#include "DesktopWindow.h"
#include <SDL2/SDL_syswm.h>
#include <X11/extensions/shape.h>
#include <X11/Xatom.h>
#include <cstdio>
#include <cstdlib>

DesktopWindow::DesktopWindow() = default;

DesktopWindow::~DesktopWindow() { Shutdown(); }

bool DesktopWindow::CreateTransparentX11Window(int width, int height) {
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        std::fprintf(stderr, "blob: cannot open X display\n");
        return false;
    }

    int screen = DefaultScreen(display_);
    XVisualInfo vinfo;
    if (!XMatchVisualInfo(display_, screen, 32, TrueColor, &vinfo)) {
        std::fprintf(stderr, "blob: no 32-bit ARGB visual available (compositor "
                              "required for a transparent overlay)\n");
        return false;
    }
    visual_ = vinfo.visual;

    Window root = RootWindow(display_, screen);
    colormap_ = XCreateColormap(display_, root, visual_, AllocNone);

    XSetWindowAttributes attrs;
    attrs.colormap = colormap_;
    attrs.border_pixel = 0;
    attrs.background_pixel = 0; // fully transparent background
    attrs.override_redirect = True; // bypass the WM entirely: no decorations,
                                     // no taskbar entry, immune to WM quirks
    attrs.event_mask = ExposureMask | StructureNotifyMask;

    x11Window_ = XCreateWindow(
        display_, root, 0, 0, static_cast<unsigned>(width), static_cast<unsigned>(height),
        0, vinfo.depth, InputOutput, visual_,
        CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect | CWEventMask,
        &attrs);

    if (!x11Window_) {
        std::fprintf(stderr, "blob: XCreateWindow failed\n");
        return false;
    }

    width_ = width;
    height_ = height;
    return true;
}

void DesktopWindow::ApplyEwmhHints() {
    Atom netWmWindowType = XInternAtom(display_, "_NET_WM_WINDOW_TYPE", False);
    Atom netWmWindowTypeUtility =
        XInternAtom(display_, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    XChangeProperty(display_, x11Window_, netWmWindowType, XA_ATOM, 32,
                     PropModeReplace,
                     reinterpret_cast<unsigned char*>(&netWmWindowTypeUtility), 1);

    Atom netWmState = XInternAtom(display_, "_NET_WM_STATE", False);
    Atom states[3] = {
        XInternAtom(display_, "_NET_WM_STATE_SKIP_TASKBAR", False),
        XInternAtom(display_, "_NET_WM_STATE_SKIP_PAGER", False),
        XInternAtom(display_, "_NET_WM_STATE_ABOVE", False),
    };
    XChangeProperty(display_, x11Window_, netWmState, XA_ATOM, 32, PropModeReplace,
                     reinterpret_cast<unsigned char*>(states), 3);

    // Never accept input focus.
    XWMHints wmHints;
    wmHints.flags = InputHint;
    wmHints.input = False;
    XSetWMHints(display_, x11Window_, &wmHints);

    const char* title = "blob";
    XStoreName(display_, x11Window_, title);
}

void DesktopWindow::MakeClickThrough() {
    // An empty input shape means the window receives no pointer events at
    // all — clicks pass through to whatever is beneath it on the desktop.
    int shapeEvent, shapeError;
    if (!XShapeQueryExtension(display_, &shapeEvent, &shapeError)) {
        std::fprintf(stderr, "blob: XShape extension unavailable; blob may "
                              "intercept clicks\n");
        return;
    }
    XShapeCombineRectangles(display_, x11Window_, ShapeInput, 0, 0, nullptr, 0,
                             ShapeSet, 0);
}

bool DesktopWindow::Initialize(int width, int height) {
    // Must select the X11 driver explicitly so SDL_CreateWindowFrom can
    // adopt a raw Xlib window handle.
    setenv("SDL_VIDEODRIVER", "x11", 1);

    if (!CreateTransparentX11Window(width, height)) return false;

    ApplyEwmhHints();
    XMapWindow(display_, x11Window_);
    MakeClickThrough();
    XFlush(display_);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "blob: SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    sdlWindow_ = SDL_CreateWindowFrom(reinterpret_cast<void*>(x11Window_));
    if (!sdlWindow_) {
        std::fprintf(stderr, "blob: SDL_CreateWindowFrom failed: %s\n", SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(
        sdlWindow_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        // Fall back to software rendering rather than failing outright —
        // still fine for a handful of filled rects per frame.
        renderer_ = SDL_CreateRenderer(sdlWindow_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        std::fprintf(stderr, "blob: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    return true;
}

void DesktopWindow::Clear() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    SDL_RenderClear(renderer_);
}

void DesktopWindow::Present() { SDL_RenderPresent(renderer_); }

void DesktopWindow::MoveTo(int x, int y) {
    if (display_ && x11Window_) {
        XMoveWindow(display_, x11Window_, x, y);
        XFlush(display_);
    }
}

void DesktopWindow::Shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (sdlWindow_) {
        // SDL_DestroyWindow would also try to destroy the underlying X11
        // window it adopted; that's fine since we're tearing everything
        // down together.
        SDL_DestroyWindow(sdlWindow_);
        sdlWindow_ = nullptr;
        x11Window_ = 0; // already destroyed via SDL
    }
    if (display_) {
        if (x11Window_) {
            XDestroyWindow(display_, x11Window_);
            x11Window_ = 0;
        }
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}
