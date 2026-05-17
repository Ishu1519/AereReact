AereReact
> Minimal reactive live wallpaper engine for Windows 11 — pure C, OpenGL, zero bloat.
![Platform](https://img.shields.io/badge/platform-Windows%2011-blue)
![Language](https://img.shields.io/badge/language-C-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
---
What is this?
AereReact is a lightweight live wallpaper engine built from scratch in pure C using Win32 API and OpenGL. No Electron. No browser. No framework overhead.
It renders an animated spike-ring directly on your desktop — and reacts to your mouse and keyboard in real time.
Built as a direct response to how bloated existing solutions like Lively Wallpaper are. This runs at ~24 FPS, uses under 1% CPU at idle, and starts silently with Windows.
---
Demo


https://github.com/user-attachments/assets/3d7a2bb3-2879-434b-874c-9c90fab69f81


Features
Spike-ring animation on pure black background
Mouse reactive — ring distorts toward cursor position
Keyboard reactive — spike burst on any keypress
Embeds into Windows desktop shell layer — survives Win+D and Show Desktop gestures
Auto-registers to Windows startup
Single `.exe` — no installer, no dependencies, no runtime
155KB total size
---
## System Requirements

| Component | Spec |
|-----------|------|
| OS | Windows 11 (tested on 25H2, build 26200) |
| GPU | Any with OpenGL support (Intel UHD / NVIDIA / AMD) |
| RAM | Negligible |
| Dependencies | None |
---
How to Run
Download `AereReact.exe` from Releases
Right-click → Run as Administrator (required for shell embedding)
Done — wallpaper starts immediately and registers to autostart
To stop it: Task Manager → `AereReact.exe` → End Task
To remove autostart: Settings → Apps → Startup → disable `AereReact`
---
How it Works
The engine attaches to the Windows desktop shell (`Progman` / `WorkerW`) as a child window, rendering behind all apps and icons but above the static wallpaper layer. OpenGL renders via a pixel-format descriptor on the native device context — no external graphics library needed.
Input is polled per frame using `GetCursorPos` and `GetAsyncKeyState` — no global hooks, zero system-wide overhead.
Animation is a ring of 160 radial spike points. Each spike lerps toward a target value at 7x per second. Mouse proximity is calculated per-spike using angular distance with Gaussian falloff. Keypress events burst randomized targets across 60% of spikes simultaneously.
Frame timing uses `QueryPerformanceCounter` for sub-millisecond precision, capped at 24 FPS to minimize GPU load.
---
Build from Source
Requires `mingw-w64` or any Windows C compiler.
```bash
x86_64-w64-mingw32-gcc AereReact.c -o AereReact.exe \
  -mwindows -O2 -static \
  -lopengl32 -lgdi32 -luser32 -ladvapi32 -lm
```
---
Why not Lively Wallpaper?
Lively is a full UWP app with a browser engine underneath — heavy RAM usage and regular CPU spikes. AereReact does one thing and does it with the least overhead physically possible on Windows.
---
## Desktop Workflow Compatible

AereReact is designed to work *with* your workflow, not against it.

- **Win+D** and **3-finger swipe down** still work normally — use them to instantly hide/show all your open apps
- When you show desktop, AereReact is your background — the animation is always running underneath
- When you restore your apps, AereReact continues silently behind them

This means AereReact effectively gives you an animated desktop that you can reveal instantly at any time — just like a live wallpaper should work.
Author
Ishu1519 — B.Tech Robotics & Automation student, builder of things that shouldn't exist.
---
Built with AI-assisted development (ClaudeCode). Iterated through 6+ versions to solve Windows 11 25H2 shell embedding quirks.
