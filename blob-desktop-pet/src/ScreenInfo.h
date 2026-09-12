#pragma once
#include <vector>
#include <X11/Xlib.h>

struct MonitorRect {
    int x, y, w, h;
    int CenterX() const { return x + w / 2; }
    int CenterY() const { return y + h / 2; }
};

// Queries monitor geometry via Xrandr so the blob adapts to arbitrary
// resolutions, multi-monitor setups, and layout changes without any
// hard-coded resolution assumptions. Re-query periodically (not every
// frame) since XRRGetMonitors is a real round-trip to the X server.
class ScreenInfo {
public:
    explicit ScreenInfo(Display* display) : display_(display) { Refresh(); }

    void Refresh();

    const std::vector<MonitorRect>& Monitors() const { return monitors_; }

    // Returns the monitor whose bounds contain (x, y), or the primary/first
    // monitor if the point falls outside all of them.
    const MonitorRect& MonitorAt(int x, int y) const;

    // Inset by `margin` px so the blob never sits flush against an edge.
    static MonitorRect Inset(const MonitorRect& r, int margin);

private:
    Display* display_;
    std::vector<MonitorRect> monitors_;
};
