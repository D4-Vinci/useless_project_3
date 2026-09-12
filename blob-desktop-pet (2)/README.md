# blob — a tiny autonomous desktop creature

A small pixel-art blob with one expressive eye that lives on top of your
Linux desktop, wandering, resting, glancing around, and reacting on its own.
It's not a game and not a scripted animation loop — it's a lightweight
native process with its own real-time behavior simulation.

Built in C++17 with SDL2 for rendering and raw Xlib/XShape/Xrandr for the
transparent, click-through, desktop-level overlay window.

## Building

### Dependencies (Debian/Ubuntu)

```bash
sudo apt-get install build-essential cmake pkg-config \
    libsdl2-dev libx11-dev libxext-dev libxrandr-dev
```

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

This produces a single `blob` binary in `build/`. Optionally install it
somewhere on your `PATH`:

```bash
sudo install -m755 blob /usr/local/bin/blob
```

A compositor (picom, KWin, GNOME Shell/Mutter, etc.) must be running for the
window's per-pixel transparency to render correctly — without one, X11 will
still create the window, but the compositor is what actually blends the
alpha channel against your desktop.

## Usage

```
blob             Summon the blob (does nothing if one is already running)
blob --remove    Ask the running blob to shut down gracefully
blob --toggle    Start it if absent, stop it if running
blob --status    Report whether a blob is currently running
```

Only one blob instance can run at a time; this is enforced via a Unix
domain socket at `$XDG_RUNTIME_DIR/blob-pet-<uid>.sock` (falls back to
`/tmp` if `XDG_RUNTIME_DIR` isn't set). `--remove`/`--toggle`/`--status`
talk to the running instance over that same socket rather than sending
signals, so shutdown is always a clean, cooperative request handled inside
the blob's own event loop.

## How it behaves

The blob has no script. Every frame it evaluates a small internal state —
energy, sleepiness, curiosity, restlessness, boredom, surprise — and a
handful of weighted probabilities to decide what to do next: keep idling,
look around and then pick a destination, travel there with natural
acceleration/deceleration, rest for a while, or occasionally change its
mind mid-journey. Its eye is animated independently of its body: it leads
the direction of travel, wanders on its own during idle moments, blinks on
a semi-random schedule, and widens or droops depending on mood.

Destinations, trajectories, blink timing, and body squash/stretch are all
computed live every frame — nothing is precomputed or replayed.

### Reacting to you

The blob is aware of the mouse without ever intercepting it. Every frame it
reads the global cursor position and button state via a read-only X11
pointer query (`CursorTracker`) — it never grabs input or consumes the
click, so clicks still pass straight through the window to whatever's
underneath, exactly as before.

- **Mouse nearby:** if the cursor comes within a modest radius, the blob
  will occasionally glance toward it, as if it's noticed you.
- **Click on the blob:** if the mouse button goes down while the cursor is
  over the blob, it plays a "bonk on the head" reaction — a quick
  compress-and-overshoot squash, a small knockback that naturally
  decelerates back to rest, a wince, and a few stars orbiting briefly above
  its head — then shakes it off and goes back to whatever it was doing.
- **Random happiness bursts:** independent of anything you do, the blob
  will spontaneously play a little joy animation every so often (a happy
  squint, a few quick cheerful hops, small hearts drifting up) — more
  frequent the happier its current mood.

## Look & feel

The blob is a big, round, glossy goo creature with a generously-sized eye,
a soft pastel palette, blush cheeks, and a per-row jelly wobble so its
whole silhouette jiggles like soft-bodied goo rather than moving as one
rigid block.

## Architecture

```
Application            CLI dispatch, main loop, frame pacing
├── CommandInterface    single-instance lock + IPC (Unix domain socket)
├── DesktopWindow       transparent/click-through/always-on-top X11+SDL window
├── ScreenInfo          Xrandr monitor geometry (multi-monitor, DPI-agnostic)
├── MovementController  position/velocity integration, acceleration/deceleration
├── EmotionSystem       slow-drifting mood values (curiosity, sleepiness, ...)
├── BehaviorController  the "brain": state machine + weighted random choices
└── Blob                visible creature
    ├── BodyAnimation   procedural squash/stretch/wiggle/hop
    └── Eye              independent pupil, blink, and expression animation
```

`BehaviorController` is the only piece that makes decisions; `Movement`,
`Blob`, and `Eye` just carry out what they're told (a destination, a look
target, a hop) using their own smooth, physically-plausible motion —
this keeps "what the blob wants" cleanly separated from "how it moves".

## Performance

- Behavior/physics/rendering run in one loop using real delta-time, not a
  fixed-step assumption.
- Frame rate is throttled from ~60 FPS down to ~12 FPS whenever the blob is
  fully idle and stationary (`BehaviorController::IsRestful()`), which is
  most of the time in practice.
- No per-frame heap allocation, no window enumeration, no screenshotting,
  no filesystem polling. Monitor geometry is only re-queried every 5
  seconds via Xrandr, not every frame.
- The blob is drawn as a few dozen filled rectangles (pixel-art blocks) per
  frame — there are no textures, sprite sheets, or asset loading.

This is intended to be left running for days without becoming a CPU or
memory concern.

## Notes / limitations

- X11 only (Wayland is not supported here — the transparent/click-through
  override-redirect window technique this relies on is X11-specific;
  XWayland may or may not work depending on your compositor).
- Requires a compositing window manager for the alpha channel to actually
  look transparent, and the `XShape` extension for click-through (present
  on effectively all X servers).
- If your window manager aggressively manages override-redirect windows in
  an unusual way, EWMH hints (`_NET_WM_STATE_ABOVE` / `SKIP_TASKBAR` /
  `SKIP_PAGER`) are applied as a best-effort — most WMs respect them.
