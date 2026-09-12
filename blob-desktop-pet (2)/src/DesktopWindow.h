#pragma once
#include <SDL2/SDL.h>
#include <X11/Xlib.h>
#include <string>

// Creates and owns a transparent, borderless, click-through, always-on-top
// X11 window (wrapped by SDL for rendering) that behaves like a desktop
// overlay rather than a normal application window:
//   - 32-bit ARGB visual so per-pixel alpha compositing works with the
//     window manager's compositor.
//   - override_redirect + EWMH hints so it has no taskbar/pager entry and
//     isn't touched by normal window management (move/resize/minimize of
//     other windows never affects it).
//   - An empty XShape input region so mouse clicks pass straight through
//     to whatever is beneath the blob.
//   - Never grabs keyboard focus.
class DesktopWindow {
public:
    DesktopWindow();
    ~DesktopWindow();

    // Returns false on failure (caller should abort startup).
    bool Initialize(int width, int height);
    void Shutdown();

    void Clear();   // clears to fully transparent
    void Present();

    SDL_Renderer* Renderer() { return renderer_; }
    Display* X11Display() { return display_; }

    int Width() const { return width_; }
    int Height() const { return height_; }

    // Repositions the underlying X11 window (used so window-space and
    // desktop-space can be treated as one: we keep the SDL window as large
    // as the full virtual desktop, so no repositioning is normally needed,
    // but this is here for robustness / future use).
    void MoveTo(int x, int y);

private:
    bool CreateTransparentX11Window(int width, int height);
    void ApplyEwmhHints();
    void MakeClickThrough();

    Display* display_ = nullptr;
    Window x11Window_ = 0;
    Visual* visual_ = nullptr;
    Colormap colormap_ = 0;

    SDL_Window* sdlWindow_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    int width_ = 0;
    int height_ = 0;
};
