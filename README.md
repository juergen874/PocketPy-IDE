# ⚡ PocketPy IDE for Android

<p align="center">
  <img src="https://img.shields.io/badge/Release-v2.0-brightgreen.svg" alt="Release v2.0" />
  <img src="https://img.shields.io/badge/Platform-Android%20%7C%20Windows%20%7C%20Linux-blue.svg" alt="Platform" />
  <img src="https://img.shields.io/badge/Engine-PocketPy%20C11-orange.svg" alt="Engine" />
  <img src="https://img.shields.io/badge/Functions-160%2B%20Native%20APIs-purple.svg" alt="Functions" />
  <img src="https://img.shields.io/badge/Networking-Sockets%20%7C%20Webserver-success.svg" alt="Networking" />
  <img src="https://img.shields.io/badge/APK%20Size-2.93%20MB-success.svg" alt="APK Size" />
  <img src="https://img.shields.io/badge/UI-Jetpack%20Compose%20%7C%20Material%203-purple.svg" alt="UI" />
  <img src="https://img.shields.io/badge/Architecture-arm64--v8a%20%7C%20x86__64-informational.svg" alt="Architecture" />
  <img src="https://img.shields.io/badge/License-MIT-lightgrey.svg" alt="License" />
</p>

<p align="center">
  <a href="https://github.com/juergen874/PocketPy-IDE/releases/download/v2.0/PocketPy-IDE-2.0.apk">
    <img src="https://img.shields.io/badge/Download_APK-v2.0-brightgreen?style=for-the-badge&logo=android" alt="Download APK" />
  </a>
  <a href="https://github.com/juergen874/PocketPy-IDE/releases/download/v2.0/pocketpy-windows-x64.zip">
    <img src="https://img.shields.io/badge/Download_Windows-x64-blue?style=for-the-badge&logo=windows" alt="Download Windows Binary" />
  </a>
</p>

**PocketPy IDE** is an ultra-fast, lightweight, cross-platform Python environment and Android IDE powered by the embeddable **[PocketPy](https://github.com/pocketpy/pocketpy)** C11 engine with **over 160+ native system, network, crypto, and hardware extensions**. Everything runs identically on Android, Windows, and Linux.

While traditional Python Android runtimes (like Chaquopy or Termux CPython) result in massive **80–120 MB** downloads and sluggish interpreter startup, **PocketPy IDE 2.0** provides full Python script execution, syntax highlighting, an ANSI terminal, live HTML/SVG preview, and comprehensive system access in an astonishing **2.93 MB APK footprint with sub-20ms instant boot**.

---

## 📥 Download & Installation

Pre-compiled signed releases are available directly via GitHub Releases:

* 📱 **Android APK:** [**PocketPy-IDE-2.0.apk** (2.93 MB)](https://github.com/juergen874/PocketPy-IDE/releases/download/v2.0/PocketPy-IDE-2.0.apk) — Android 7.0+ (API 24+) • `arm64-v8a`, `x86_64`
* 🪟 **Windows x64 Executable:** [**pocketpy-windows-x64.zip**](https://github.com/juergen874/PocketPy-IDE/releases/download/v2.0/pocketpy-windows-x64.zip) — Standalone `pocketpy.exe` CLI runner with full Winsock2 and Win32 support
* 📦 **Release Overview:** [PocketPy IDE v2.0 Release](https://github.com/juergen874/PocketPy-IDE/releases/tag/v2.0)

---

## 🚀 Key Highlights in v2.0

* **🪶 Ultra-Lightweight (2.93 MB):** Advanced R8 code shrinking and resource optimization dropped the APK size from 18 MB down to 2.93 MB while multiplying capabilities.
* **⚡ Instant Cold Startup:** Starts executing Python code in milliseconds with minimal RAM (< 30 MB) and zero background battery drain.
* **🔌 160+ Native Extensions Built-In:**
  * **`socket`**: Full TCP Client/Server (`bind`, `listen`, `accept`), UDP (`sendto`, `recvfrom`), DNS resolver, socket options (`SO_REUSEADDR`).
  * **`os` & `os.path`**: 50 POSIX filesystem and process functions (`listdir`, `stat`, `statvfs`, `mkdir`, `remove`, `access`, `chmod`, `getcwd`, `chdir`, `getpid`, `pipe`, `open`, `read`, `write`, `close`, `abspath`, `join`, etc.).
  * **`sysinfo`**: Hardware telemetry (`ram_total`, `ram_free`, `ram_avail`, `uptime`, `cpu_count`, device model, Android version).
  * **`android`**: Hardware bridge (`toast`, `vibrate`, `notify`, `speak` TTS, `get_battery_level`, `is_battery_charging`, `copy_to_clipboard`, `get_clipboard`, `beep`).
  * **`hashlib`**: High-performance Modbus CRC16, CRC32, Adler32, MD5, SHA-256, hex conversions, and cryptographic randomness.
  * **`struct`**: Binary packing & unpacking for 8, 16, 32, and 64-bit integers and floats (Little- & Big-Endian).
  * **`storage` & `sqlite3`**: In-memory persistent KV store and SQLite3 database engine.
  * **`math_ext`**: Extended math (`clamp`, `lerp`, `map_range`, `degrees`, `radians`, `gcd`, `lcm`, `sign`).
* **✍️ Touch-Optimized Code Editor:**
  * Real-time Python syntax highlighting (keywords, builtins, string literals, numbers, comments).
  * Monospace typography with line numbering.
  * One-touch symbol toolbar (`:`, `(`, `)`, `[`, `]`, `{`, `}`, `_`, `=`, `"`, `'`, `#`, `tab`) for frictionless mobile coding.
* **📟 Streaming ANSI Terminal:**
  * Real-time stdout & stderr streaming directly from C11 interpreter callbacks.
  * ANSI color and format parser.
  * Instant Copy, Clear, and Execution Timer badges.
  * Background execution cancellation (Stop button).
* **🌐 Visual Output (WebView):**
  * Live HTML5 and SVG rendering tab.
  * Preview interactive dashboards, animated SVG plots, and web UI directly inside the app.
* **📁 Workspace File Manager:**
  * Create, open, rename, and delete `.py`, `.html`, and `.txt` files.
  * Scoped persistent storage in app sandbox.
  * One-tap restore for built-in sample scripts.

---

## 📊 Comparison: PocketPy IDE vs Traditional Android Python

| Metric | Traditional Python IDE (Chaquopy/CPython) | PocketPy IDE 2.0 (Native C11) |
| :--- | :--- | :--- |
| **APK Footprint** | ~80 MB – 120 MB | **2.93 MB** *(97% reduction)* |
| **Startup Overhead** | 1,500 ms – 3,000 ms | **< 20 ms** |
| **Runtime Memory** | ~150 MB – 250 MB | **~20 MB – 30 MB** |
| **Built-in Functions** | Standard library (large disk footprint) | **160+ C11 extensions in < 250 KB binary code** |
| **Android Bridge** | PyJNIus / complex reflection | **Native JNI hooks (TTS, Vibrate, Notify, Battery, Audio)** |
| **Host Build Requirement** | Python 3.10 host, pip wheels | **Standard NDK / CMake (Pure C11)** |
| **Offline Capability** | Yes | **100% Standalone & Offline** |
| **Ideal For** | Heavy scientific libraries (numpy/scipy) | **Fast algorithms, IoT scripting, Solar/Modbus calculators, HTTP microservers, automation** |

---

## 🧩 Included Demo Scripts

PocketPy IDE comes preloaded with production-ready samples:

1. **`1_pocketpy_speed_test.py`**  
   Benchmarks list comprehensions, math computations, and primes to showcase PocketPy's blazing C11 execution speed.
2. **`2_ascii_sine_waves.py`**  
   Generates animated ASCII sine/cosine waveforms directly in the terminal using standard math functions.
3. **`3_interactive_dashboard.html`**  
   A modern, dark-themed responsive HTML5/SVG status dashboard demonstrated in the Visual tab.
4. **`4_deye_reader.py`**  
   Live Modbus TCP telemetry reader for Deye hybrid inverters (monitoring PV power, battery charge/discharge, grid feed-in, and home consumption in real-time).
5. **`5_system_and_hardware.py`**  
   Inspects device model, Android release, RAM utilization, CPU cores, uptime, and filesystem status via `sysinfo` and `os`.
6. **`6_android_power.py`**  
   Demonstrates Android hardware integration: native Toast, haptic vibration, audible beep, status-bar notification, battery telemetry, and Text-to-Speech (TTS).
7. **`7_mini_webserver.py`**  
   Runs an embedded native HTTP server on port 8080 delivering dynamic HTML telemetry to any browser on the network.

---

## 🛠️ Architecture & Tech Stack

```
PocketPy IDE
 ├── Native Core (C11)
 │    ├── pocketpy.c & pocketpy.h (Amalgamated v2.2.0)
 │    ├── pocketpy_ext.h (160+ native C extensions: sockets, POSIX, sysinfo, crypto, struct, sqlite3)
 │    └── pocketpy_jni.c (JNI bridge, Android system hooks, stdout streaming)
 ├── Kotlin Engine & Services
 │    ├── PocketPyEngine.kt (JNI wrapper, TTS, Haptics, Notifications, Battery, Audio)
 │    ├── MainViewModel.kt (MVI / StateFlow state management)
 │    └── FileManager.kt (Scoped local storage management)
 └── Modern Jetpack Compose UI
      ├── CodeEditorView.kt (Syntax highlighter & quick touch symbols)
      ├── TerminalView.kt (ANSI parser, streaming log buffer)
      ├── VisualWebView.kt (Isolated Android WebView)
      └── WorkspaceDrawer.kt (Material 3 navigation sidebar)
```

---

## 🏗️ Building from Source

### 1. Android APK
**Prerequisites:** Android SDK (API 36), Android NDK 25+, CMake 3.22.1+, JDK 17.

```bash
git clone https://github.com/juergen874/PocketPy-IDE.git
cd PocketPy-IDE

# Make gradlew executable
chmod +x gradlew

# Build Optimized Release APK (with R8 tree-shaking & resource shrinking)
./gradlew assembleRelease
```
The compiled APK will be at `app/build/outputs/apk/release/PocketPy-IDE-2.0.apk`.

### 2. Windows Executable (`pocketpy.exe`)
**Prerequisites:** Visual Studio 2022 (MSVC) or MinGW with GCC/Clang, CMake 3.20+.

```powershell
git clone https://github.com/juergen874/PocketPy-IDE.git
cd PocketPy-IDE

# Configure with CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build PocketPy executable
cmake --build build --config Release

# Run REPL or scripts
.\build\Release\pocketpy.exe
.\build\Release\pocketpy.exe tests\test_full_suite.py
.\build\Release\pocketpy.exe script.py
```

### 3. Linux / macOS Executable (`pocketpy`)
```bash
cmake -B build -S .
cmake --build build
./build/pocketpy tests/test_full_suite.py
```

---

## 📄 License

PocketPy IDE is distributed under the **MIT License**.  
PocketPy engine is copyright (c) [blakehuang](https://github.com/pocketpy/pocketpy) and licensed under the MIT License.
