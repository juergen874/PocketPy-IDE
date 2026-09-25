# ⚡ PocketPy IDE for Android

<p align="center">
  <img src="https://img.shields.io/badge/Release-v1.0-brightgreen.svg" alt="Release v1.0" />
  <img src="https://img.shields.io/badge/Platform-Android%207.0%2B%20(API%2024%2B)-blue.svg" alt="Platform" />
  <img src="https://img.shields.io/badge/Engine-PocketPy%20C11-orange.svg" alt="Engine" />
  <img src="https://img.shields.io/badge/APK%20Size-~2.5%20MB-success.svg" alt="APK Size" />
  <img src="https://img.shields.io/badge/UI-Jetpack%20Compose%20%7C%20Material%203-purple.svg" alt="UI" />
  <img src="https://img.shields.io/badge/Architecture-arm64--v8a%20%7C%20x86__64-informational.svg" alt="Architecture" />
  <img src="https://img.shields.io/badge/License-MIT-lightgrey.svg" alt="License" />
</p>

<p align="center">
  <a href="https://github.com/juergen874/PocketPy-IDE/releases/download/v1.0/app-debug.apk">
    <img src="https://img.shields.io/badge/Download_APK-v1.0-brightgreen?style=for-the-badge&logo=android" alt="Download APK" />
  </a>
</p>

**PocketPy IDE** is an ultra-fast, lightweight, and fully offline Python IDE for Android powered by the embeddable **[PocketPy](https://github.com/pocketpy/pocketpy)** C11 engine.

While traditional Python Android solutions (like Chaquopy or PyTorch Mobile) result in massive **80–120 MB** APK downloads and noticeable interpreter cold starts, **PocketPy IDE** delivers full Python script execution, syntax highlighting, an ANSI terminal, and a visual HTML/SVG webview in an astonishing **~2.5 MB** footprint with sub-20ms cold boot!

---

## 📥 Download & Installation

Pre-compiled Android APKs are available on the GitHub Releases page:

* 📱 **Direct Download:** [**app-debug.apk (v1.0)**](https://github.com/juergen874/PocketPy-IDE/releases/download/v1.0/app-debug.apk)
* 📦 **Release Overview:** [PocketPy IDE v1.0 Release](https://github.com/juergen874/PocketPy-IDE/releases/tag/v1.0)
* ⚙️ **Compatibility:** Android 7.0+ (API 24+) • Architectures: `arm64-v8a`, `x86_64`

---

## 🚀 Key Highlights

* **🪶 Ultra-Lightweight (~2.5 MB):** Pure native C11 amalgamation linked via JNI. No heavy CPython runtime, no bulky wheel dependencies.
* **⚡ Instant Cold Startup:** Starts executing Python code in milliseconds with minimal RAM (< 30 MB) and battery drain.
* **✍️ Touch-Optimized Code Editor:**
  * Real-time Python syntax highlighting (keywords, builtins, string literals, numbers, comments).
  * Monospace typography with line numbering.
  * One-touch symbol toolbar (`:`, `(`, `)`, `[`, `]`, `{`, `}`, `_`, `=`, `"`, `'`, `#`, `tab`) for frictionless mobile coding.
* **📟 Streaming ANSI Terminal:**
  * Real-time stdout & stderr streaming directly from the C11 interpreter callbacks.
  * ANSI color and format parser.
  * Instant Copy, Clear, and Execution Timer badges.
  * Background execution cancellation (Stop button).
* **🌐 Visual Output (WebView):**
  * Live HTML5 and SVG rendering tab.
  * Preview interactive dashboards, animated SVG plots, and web UI directly inside the app.
* **📁 Workspace File Manager:**
  * Create, open, rename, and delete `.py`, `.html`, and `.txt` files.
  * Persistent storage in app sandbox.
  * One-tap restore for built-in sample scripts.

---

## 📊 Comparison: PocketPy IDE vs Traditional Android Python

| Metric | Traditional Python IDE (Chaquopy/CPython) | PocketPy IDE (Native C11) |
| :--- | :--- | :--- |
| **APK Footprint** | ~80 MB – 120 MB | **~2.5 MB** *(97% reduction)* |
| **Startup Overhead** | 1,500 ms – 3,000 ms | **< 20 ms** |
| **Runtime Memory** | ~150 MB – 250 MB | **~20 MB – 30 MB** |
| **Host Build Requirement** | Python 3.10 host, pip wheels | **Standard NDK / CMake (Pure C11)** |
| **Offline Capability** | Yes | **100% Standalone & Offline** |
| **Ideal For** | Heavy scientific libraries (numpy/scipy) | **Fast algorithms, IoT scripting, Solar/Modbus calculators, logic, automation** |

---

## 🧩 Included Demo Scripts

PocketPy IDE comes preloaded with production-ready samples:

1. **`1_pocketpy_speed_test.py`**  
   Benchmarks list comprehensions, math computations, and primes to showcase PocketPy's blazing C11 execution speed.
2. **`2_ascii_sine_waves.py`**  
   Generates animated ASCII sine/cosine waveforms directly in the terminal using standard math functions.
3. **`3_interactive_dashboard.html`**  
   A modern, dark-themed responsive HTML5/SVG status dashboard demonstrated in the Visual tab.
4. **`4_deye_pv_calc.py`**  
   Real-world industrial Modbus telemetry decoder designed for Deye hybrid inverters (calculating solar power, battery state-of-charge, grid feed-in, and self-consumption efficiency).

---

## 🛠️ Architecture & Tech Stack

```
PocketPy IDE
 ├── Native Core (C11)
 │    ├── pocketpy.c & pocketpy.h (Amalgamated v2.2.0)
 │    └── pocketpy_jni.c (JNI bridge, stdout streaming, exception formatting)
 ├── Kotlin Coroutines & Architecture
 │    ├── PocketPyEngine.kt (JNI wrapper, Dispatchers.IO execution)
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

### Prerequisites
- Android Studio Ladybug or newer
- Android SDK (API 36 / Android 16)
- Android NDK (Version 25+)
- CMake 3.22.1+
- Java JDK 17

### Build Commands
```bash
git clone https://github.com/juergen874/PocketPy-IDE.git
cd PocketPy-IDE

# Make gradlew executable
chmod +x gradlew

# Build Debug APK
./gradlew assembleDebug
```

The compiled APK will be located at:
```
app/build/outputs/apk/debug/app-debug.apk
```

---

## 📄 License

PocketPy IDE is distributed under the **MIT License**.  
PocketPy engine is copyright (c) [blakehuang](https://github.com/pocketpy/pocketpy) and licensed under the MIT License.
