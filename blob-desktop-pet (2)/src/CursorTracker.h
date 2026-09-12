#pragma once
#include <X11/Xlib.h>

// Polls the global mouse pointer position and button state via
// XQueryPointer. This is a plain query — it does not intercept, consume,
// or require focus/grabs — so it works fine alongside the blob's
// click-through window (which never receives any input events itself).
// Cheap enough to call once per frame (a single round trip to the X
// server).
class CursorTracker {
public:
    explicit CursorTracker(Display* display);

    void Poll();

    int X() const { return x_; }
    int Y() const { return y_; }
    bool Valid() const { return valid_; }

    // True only on the frame the primary button transitioned from up to
    // down — a single "click started" edge, not held-down state.
    bool JustPressed() const { return justPressed_; }

private:
    Display* display_;
    Window root_;
    int x_ = 0, y_ = 0;
    bool down_ = false;
    bool justPressed_ = false;
    bool valid_ = false;
};
