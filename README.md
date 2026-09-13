<p align="center">
<img width="347" height="177" alt="image" src="https://github.com/user-attachments/assets/845bc142-8340-4c8f-94ff-5c03bcf7f0d6" />
</p>

<p align="center">
  <i>Write code at the speed of thought.</i>
</p>

<p align="center">
  <a href="https://valence-website.vercel.app">
    <img src="https://img.shields.io/badge/Live-Demo-0A0A0A?style=for-the-badge&logo=vercel&logoColor=white"/>
  </a>
  <a href="https://github.com/Valence">
    <img src="https://img.shields.io/github/stars/your-repo?style=for-the-badge"/>
  </a>
</p>

---

<p align="center">
  <img width="1879" height="884" alt="image" src="https://github.com/user-attachments/assets/71957b5d-d5b5-4517-ac7e-02622eeb1218" />
</p>

---

## Blazing Fast. Zero Bloat.

- Built entirely in C++ and Qt  
- No Electron. No frameworks  
- Direct control over rendering and input  

<img width="1459" height="155" alt="image" src="https://github.com/user-attachments/assets/6d959a07-5862-4d70-83a1-1eecb55221ac" />

---

## Demo

<p align="center">
  <video src = "C:\Users\shukl\OneDrive\Documents\Projects\Valence\Assets\demo.mp4" width = "100%">
</p>

---

## Core Architecture

Five clean subsystems. Each with a single responsibility.

<p align="center">
  <img width="1270" height="806" alt="image" src="https://github.com/user-attachments/assets/717f7358-c362-4150-94c5-1c84dacdfbb4" />
</p>

---

## Features

<img width="1310" height="812" alt="image" src="https://github.com/user-attachments/assets/eb46d9e8-3219-46cc-94bd-d3c7d49ebbf1" />

---

## Technical Decisions

<img width="1240" height="766" alt="image" src="https://github.com/user-attachments/assets/7ca3c8b9-8f14-493c-ba19-68ec4d16b318" />

---

## Build Roadmap
<img width="1464" height="624" alt="image" src="https://github.com/user-attachments/assets/69b01f85-187c-4bc9-b964-a66bcd6ae96a" />

---

## Competitive Programming

Valence ships a built-in judge, so the compile → run → diff loop never leaves the editor.

Open it with `Ctrl+J`, or from **Run → Run Test Cases**.

- Add test cases with **+**. Each holds an input and the expected output.
- **Ctrl+Enter** saves the file, compiles it once with `g++ -O2 -std=gnu++17`,
  then pipes every case through the binary and reports a verdict: `AC`, `WA`,
  `TLE`, `RE` or `CE`.
- Cards collapse to one row, so ten tests stay readable. A failing case expands
  and scrolls its output to the first line that differs.
- Comparison ignores trailing whitespace and trailing blank lines, and is exact
  otherwise. Time limit is 3s per case; a runaway solution is killed, never the UI.
- Tests are saved next to the source as `<name>.valence-tests.json`, so reopening
  a problem restores them.

`g++` must be on your `PATH`.

### Keyboard

| | |
|---|---|
| `Ctrl+J` | Judge panel |
| `Ctrl+Enter` | Run all test cases |
| `F5` | Build and run in the terminal |
| `` Ctrl+` `` | Terminal |
| `Ctrl+B` | Sidebar |
| `Tab` / `Shift+Tab` | Indent / unindent selection |
| `F2` | Rename in the explorer |
| `Delete` | Move to Recycle Bin |

---

## Getting Started

Requires **Qt 6** (Widgets), **CMake 3.16+**, and a C++17 compiler.

```bash
git clone https://github.com/Sudhss/Valence.git
cd Valence
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/mingw_64
cmake --build build
./build/Valence
```

The build defaults to `Release`; pass `-DCMAKE_BUILD_TYPE=Debug` if you want symbols.

### Packaging a release

Needs [Inno Setup 6](https://jrsoftware.org/isinfo.php) and Qt's `windeployqt`.

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/mingw_64
cmake --build build-release

mkdir -p dist/Valence-3.0 && cp build-release/Valence.exe dist/Valence-3.0/
windeployqt --release --no-translations --compiler-runtime --dir dist/Valence-3.0 dist/Valence-3.0/Valence.exe

ISCC ValenceV3.iss
```

The installer lands in `Output/`. `ValenceV3.iss` stages from `dist/` rather than
straight out of the build tree — the V2 script shipped `CMakeCache.txt`,
`CMakeFiles/` and `build.ninja` to every user who installed it.

Before publishing, check the staged build runs on a machine without Qt:

```bash
cd dist/Valence-3.0 && ./Valence.exe
```

## Download
- macOS: not sure, might do it
- Windows: Shipped — v3.0
- Linux: Future advancement

## Philosophy
### Systems software should not depend on heavy abstractions.
<img width="736" height="437" alt="image" src="https://github.com/user-attachments/assets/e8322bab-627b-45c7-b9eb-aec5ba945c56" />

## Valence focuses on:

- Control over performance
- Simplicity in design
- Clear separation of responsibilities

---

<p align="center">
  Built by <a href="https://github.com/Sudhss"><b>Sudhanshu Shukla</b></a>
</p>
