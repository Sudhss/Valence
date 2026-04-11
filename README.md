<p align="center">
  <img src="https://your-logo-link.png" width="120"/>
</p>

<h1 align="center">Valence</h1>
<p align="center">
  <i>Write code at the speed of thought.</i>
</p>

<p align="center">
  <a href="https://valence-website.vercel.app">
    <img src="https://img.shields.io/badge/Live-Demo-0A0A0A?style=for-the-badge&logo=vercel&logoColor=white"/>
  </a>
  <a href="https://github.com/your-repo">
    <img src="https://img.shields.io/github/stars/your-repo?style=for-the-badge"/>
  </a>
</p>

---

<p align="center">
  <img src="https://your-hero-screenshot.png" width="100%"/>
</p>

---

## Blazing Fast. Zero Bloat.

```0ms startup • ~2k LOC • 0 dependencies • 60 FPS```


- Built entirely in C++ and Qt  
- No Electron. No frameworks  
- Direct control over rendering and input  

---

## Demo

<p align="center">
  <img src="https://your-demo.gif" width="100%"/>
</p>

---

## Core Architecture

Five clean subsystems. Each with a single responsibility.

<p align="center">
  <img src="https://your-architecture-image.png" width="100%"/>
</p>

### Text Buffer
- `std::vector<std::string>`
- One string per line
- Insert, delete, split, merge
- O(1) line access

### Cursor System
- Logical `(row, col)` model  
- Pixel position derived at render time  
- No visual drift issues  

### Rendering Engine
- Custom QPainter loop  
- Draws only visible viewport  
- Consistent performance regardless of file size  

### Input Handler
- `keyPressEvent` driven  
- Routes input to buffer and cursor  
- Single repaint per event  

---

## Features

- Zero-overhead rendering  
- Logical cursor model  
- C++ syntax highlighting  
- File open and save  
- Smooth vertical scrolling  
- Line numbers (in progress)  

---

## Technical Decisions

Why every choice matters.

- Array of lines over rope  
  Simpler, predictable, sufficient for real-world file sizes  

- Logical cursor over pixel cursor  
  Eliminates desync bugs across fonts and scrolling  

- Viewport-based rendering  
  Constant FPS independent of file size  

---

## Build Roadmap

| Week | Focus |
|------|------|
| 1 | Text buffer and rendering |
| 2 | Cursor and typing |
| 3 | Navigation and scrolling |
| 4 | File I/O and syntax highlighting |
| 5 | Polish and MVP |

---

## Getting Started

```bash
git clone https://github.com/your-repo.git
cd valence
mkdir build && cd build
cmake ..
make
./valence
```

## Download
- macOS: available
- Windows: in progress
- Linux: in progress

## Philosophy
### Systems software should not depend on heavy abstractions.

## Valence focuses on:

- Control over performance
- Simplicity in design
- Clear separation of responsibilities


## Author and Creater
---

<p align="center">
  Built by <a href="https://github.com/Sudhss"><b>Sudhanshu Shukla</b></a>
</p>
