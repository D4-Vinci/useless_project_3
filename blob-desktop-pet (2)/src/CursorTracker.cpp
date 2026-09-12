#include "CursorTracker.h"

CursorTracker::CursorTracker(Display* display)
    : display_(display), root_(DefaultRootWindow(display)) {}

void CursorTracker::Poll() {
    Window returnedRoot, childWindow;
    int rootX, rootY, winX, winY;
    unsigned int mask;

    Bool ok = XQueryPointer(display_, root_, &returnedRoot, &childWindow, &rootX, &rootY,
                             &winX, &winY, &mask);
    if (!ok) {
        valid_ = false;
        justPressed_ = false;
        return;
    }

    valid_ = true;
    x_ = rootX;
    y_ = rootY;

    bool nowDown = (mask & Button1Mask) != 0;
    justPressed_ = nowDown && !down_;
    down_ = nowDown;
}
