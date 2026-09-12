#include "ScreenInfo.h"
#include <X11/extensions/Xrandr.h>

void ScreenInfo::Refresh() {
    monitors_.clear();
    Window root = DefaultRootWindow(display_);

    int numMonitors = 0;
    XRRMonitorInfo* mons = XRRGetMonitors(display_, root, True, &numMonitors);
    if (mons && numMonitors > 0) {
        for (int i = 0; i < numMonitors; ++i) {
            MonitorRect r;
            r.x = mons[i].x;
            r.y = mons[i].y;
            r.w = mons[i].width;
            r.h = mons[i].height;
            monitors_.push_back(r);
        }
        XRRFreeMonitors(mons);
    }

    if (monitors_.empty()) {
        // Fallback: treat the whole root window as a single monitor.
        XWindowAttributes attrs;
        XGetWindowAttributes(display_, root, &attrs);
        monitors_.push_back(MonitorRect{0, 0, attrs.width, attrs.height});
    }
}

const MonitorRect& ScreenInfo::MonitorAt(int x, int y) const {
    for (const auto& m : monitors_) {
        if (x >= m.x && x < m.x + m.w && y >= m.y && y < m.y + m.h) {
            return m;
        }
    }
    return monitors_.front();
}

MonitorRect ScreenInfo::Inset(const MonitorRect& r, int margin) {
    MonitorRect out = r;
    out.x += margin;
    out.y += margin;
    out.w -= margin * 2;
    out.h -= margin * 2;
    if (out.w < 1) out.w = 1;
    if (out.h < 1) out.h = 1;
    return out;
}
