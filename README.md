<img width="1280" height="640" alt="git (1)" src="https://github.com/user-attachments/assets/8920b256-2ba8-4988-b824-5351134eb4bd" />

# Model 1 — A Tiny Autonomous Desktop Creature 🎯 (A. K. A. blob)

## Basic Details

### Team Name: G-0

### Team Members

- Team Lead: Tino Tyson
- Member 2: Adhithyan P B

### Project Description

**blob** is a tiny autonomous desktop companion designed specifically for Linux.

It is a small pixel-art creature that lives on your desktop, wandering around, resting, looking at the mouse, blinking, getting bonked when clicked, and occasionally becoming inexplicably happy.

It is not a game and does not follow a predefined animation script. Its behavior is generated in real time using a lightweight behavior and emotion system.

### The Problem (that doesn't exist)

Have you ever looked at your perfectly functional Linux desktop and thought:

> "This operating system is missing a small blob that does absolutely nothing useful."

No?

We fixed that anyway.

Modern operating systems provide terminals, window managers, editors, browsers, development tools, and approximately seventeen ways to configure your keyboard.

But they still don't provide a tiny creature sitting on your desktop judging your productivity.

### The Solution (that nobody asked for)

We created **blob**.

Blob is a small autonomous desktop creature that:

- Wanders around your desktop on its own.
- Looks around when it gets curious.
- Becomes tired and rests.
- Blinks and changes its expression.
- Notices when your mouse gets close.
- Reacts when you click on it.
- Occasionally performs random happiness animations.
- Runs directly as a lightweight native Linux application.

Its behavior is generated dynamically rather than being a simple collection of prerecorded animations.

Basically, your Linux desktop now has a tiny roommate that contributes nothing to your work.

Exactly as intended.

---

## Technical Details

### Technologies/Components Used

For Software:

- **Language:** C++17
- **Graphics:** SDL2
- **Window System:** X11
- **X11 Extensions:** XShape, XRandR
- **Build System:** CMake
- **Compiler:** Any C++17-compatible compiler
- **Build Tools:** Make, pkg-config
- **Platform:** Linux with X11

The project uses SDL2 for rendering and Xlib/XShape/XRandR for the transparent, click-through desktop overlay.

For Hardware:

- No special hardware required.
- Any Linux computer capable of running an X11 desktop environment should be sufficient.

---

### Implementation

For Software:

# Installation

## Linux

> **Current availability:** Linux/X11 only. Windows and macOS are not supported at this time. Wayland is also not currently supported.

### Downloading and Installing Blob

Follow these steps to download and run **blob** on a Linux computer.

### 1. Open a Terminal

On most Linux desktops, you can:

- Right-click on the desktop and select **Open Terminal**, or
- Open your applications menu and search for **Terminal**.

### 2. Navigate to Where You Want the Project

Choose where you want the project folder to be created.

For example:

```bash
cd ~/Projects
```

Or simply use your home directory:

```bash
cd ~
```

This is only the location where the project will be downloaded.

### 3. Clone the Repository

Run:

```bash
git clone https://github.com/D4-Vinci/useless_project_3.git
```

This downloads the complete project from GitHub into a new folder named:

```text
useless_project_3
```

> You can also copy the repository URL directly from the green **Code** button on the GitHub repository page.

### 4. Move Into the Project Folder

Run:

```bash
cd useless_project_3
```

You are now inside your local copy of the project.

Enter the blob project directory:

```bash
cd "blob-desktop-pet (2)"
```

### 5. Install Build Dependencies

#### Debian / Ubuntu

Run:

```bash
sudo apt-get update
```

Then install the required dependencies:

```bash
sudo apt-get install build-essential cmake pkg-config \
    libsdl2-dev libx11-dev libxext-dev libxrandr-dev
```

#### Fedora

Run:

```bash
sudo dnf install SDL2-devel libX11-devel libXext-devel \
    libXrandr-devel cmake gcc-c++
```

These packages provide the compiler, build tools, SDL2, and the X11 libraries required by blob.

### 6. Build the Project

From inside the `blob-desktop-pet (2)` directory, run:

```bash
mkdir build
cd build
```

Then configure and compile the project:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Or, as a single command:

```bash
mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
```

After a successful build, the compiled executable will be located at:

```text
build/blob
```

### 7. Run Blob

From inside the `build` folder, run:

```bash
./blob
```

Blob should appear on your desktop and start wandering around autonomously.

---

## Updating Blob

If the project receives updates on GitHub, you do **not** need to download the entire project again.

Go back to the repository folder:

```bash
cd ~/Projects/useless_project_3
```

> If you cloned the project somewhere else, replace the path above with the location you chose during installation.

Then pull the latest changes:

```bash
git pull
```

After updating, rebuild the project:

```bash
cd "blob-desktop-pet (2)"
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Then run:

```bash
./blob
```

---

## Optional: Install Globally

If you want to run `blob` from anywhere in the terminal, you can install the compiled executable system-wide.

From the `build` directory:

```bash
sudo install -m755 blob /usr/local/bin/blob
```

You can then start blob from anywhere with:

```bash
blob
```

### Available Commands

Start blob:

```bash
blob
```

Remove a running blob:

```bash
blob --remove
```

Toggle blob:

```bash
blob --toggle
```

Check whether blob is running:

```bash
blob --status
```

Only one blob instance can run at a time. The application uses a Unix domain socket for communication between commands and the running instance.

---

## Troubleshooting

### `git: command not found`

Install Git using your distribution's package manager.

For Debian/Ubuntu:

```bash
sudo apt-get install git
```

### `cmake: command not found`

Install CMake:

```bash
sudo apt-get install cmake
```

For Fedora:

```bash
sudo dnf install cmake
```

### SDL2 or X11 errors during CMake

Make sure the required development packages are installed:

```bash
sudo apt-get install libsdl2-dev libx11-dev libxext-dev libxrandr-dev
```

### Blob does not appear transparently

Make sure you are running an **X11 session** with a working compositor.

Wayland is currently unsupported.

---

> **Note:** At the moment, blob is available for **Linux/X11 users only**. Support for other operating systems may be added in the future.

---

# Run

From the `build` directory:

```bash
./blob
```

Or, if you installed it globally:

```bash
blob
```

Blob will appear as a desktop-level overlay and begin behaving autonomously.

### Available Commands

Start blob:

```bash
blob
```

Remove a running blob:

```bash
blob --remove
```

Toggle blob:

```bash
blob --toggle
```

Check whether blob is running:

```bash
blob --status
```

Only one blob instance can run at a time. The application uses a Unix domain socket for communication between commands and the running instance.

---

## System Requirements

Blob currently requires:

- Linux
- X11
- A C++17-compatible compiler
- CMake 3.10 or newer
- SDL2
- X11
- XShape
- XRandR
- A compositing window manager

A compositor such as **picom, KWin, or GNOME Shell/Mutter** is required for the transparent appearance of the blob.

### Important: Wayland

**Wayland is not currently supported.**

The desktop overlay relies on X11-specific functionality for the transparent, click-through window. XWayland may work in some configurations, but it is not guaranteed.

---

### Project Documentation

For Software:

# Screenshots

[https://drive.google.com/file/d/1ENgtJM7JVrlBykgJhr92AgJPbLkq\_6sF/view?usp=sharing](https://drive.google.com/file/d/1ENgtJM7JVrlBykgJhr92AgJPbLkq_6sF/view?usp=sharing)

*Blob initial version*

[https://drive.google.com/file/d/1JK5C6YIb6543shUcPo0A-WMz7kTQNfRh/view?usp=sharing](https://drive.google.com/file/d/1JK5C6YIb6543shUcPo0A-WMz7kTQNfRh/view?usp=sharing)

*Blob final version *

[https://drive.google.com/file/d/1kvpB\_W97PXiJ\_9s2rt2nIT\_2uuuFBPSe/view?usp=sharing](https://drive.google.com/file/d/1kvpB_W97PXiJ_9s2rt2nIT_2uuuFBPSe/view?usp=sharing)

*Blob performing one of its autonomous animations.*

# Diagrams

*High-level architecture showing the application loop, behavior system, movement controller, animation system, and desktop window.*

The application is divided into several components, including:

```text
Application
├── CommandInterface
│   └── Single-instance lock + IPC
│
├── DesktopWindow
│   └── Transparent / click-through X11 window
│
├── ScreenInfo
│   └── Monitor geometry using XRandR
│
├── MovementController
│   └── Position, velocity, acceleration
│
├── EmotionSystem
│   └── Curiosity, sleepiness, boredom, etc.
│
├── BehaviorController
│   └── State machine + weighted decisions
│
└── Blob
    ├── BodyAnimation
    └── Eye
```

The `BehaviorController` acts as the creature's "brain", while the movement and animation components execute the decisions made by it.

---

### Project Demo

# Video

[https://drive.google.com/file/d/1z0BSN\_ek4YjeuU5t0DG93y\_I7dqQjea5/view?usp=sharing](https://drive.google.com/file/d/1z0BSN_ek4YjeuU5t0DG93y_I7dqQjea5/view?usp=sharing)

*The video demonstrates blob running on a Linux desktop, autonomously moving around, reacting to the mouse, and performing its various animations.*

---

## Team Contributions

- **Tino Tyson:** Project development, C++ implementation, behavior system, desktop integration, animation system, and Linux/X11 implementation.
- **Adhithyan P B:** Ideation, Design and theme of the blob, Documentation

---

## Why Blob?

Blob is intentionally useless.

It does not manage your files.

It does not increase your productivity.

It does not automate your workflow.

It does not solve an important problem.

It just exists on your desktop, occasionally looks at your mouse, gets bonked, and continues living its tiny digital life.

And somehow that is enough.

---

Made with ❤️ at TinkerHub Useless Projects



