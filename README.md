
# Amusement Park Layout (Computer Graphics Project)

## Overview

This project presents a 3D isometric amusement park environment developed using OpenGL (GLUT) in C++. It focuses on designing a structured and visually organized park layout by applying core computer graphics concepts such as transformations, lighting, and scene composition.

The current implementation emphasizes layout planning, including roads, zones, and environmental elements, along with initial interactive components and animations.

---

## Features

* Isometric 3D view of the amusement park
* Structured road network with proper alignment and spacing
* Clearly defined zones (rides, food court, stalls, central plaza)
* Decorative boundary wall with entrance gate
* Central roundabout with fountain area
* Environmental elements such as trees, benches, lamps, and sky
* Animated components:

  * Moving train system
  * Rotating wheels and rides
  * Balloon and lighting effects
* User interaction:

  * Camera rotation (mouse and keyboard)
  * Zoom control
  * Toggle animations for rides

---

## Technologies Used

* C++
* OpenGL
* GLUT (FreeGLUT)

---

## Project Structure

```
Project/
├── main.cpp          # Main OpenGL source code
├── README.md         # Documentation
├── grass.bmp         # Ground texture
├── burger.bmp        # Food texture
├── pizza.bmp         # Food texture
├── icecream.bmp      # Food texture
├── water.bmp         # Water texture

---

## Controls

| Input        | Action                        |
| ------------ | ----------------------------- |
| Arrow Keys   | Rotate camera                 |
| Mouse Drag   | Orbit camera                  |
| Scroll Wheel | Zoom in/out                   |
| + / -        | Zoom in/out                   |
| W            | Toggle Ferris wheel animation |
| R            | Toggle roller coaster         |
| C            | Toggle merry-go-round         |
| T            | Start/Stop train |

---

## How to Run

### Linux

1. Install dependencies:

```bash
sudo apt update
sudo apt install freeglut3-dev
```

2. Compile:

```bash
g++ main.cpp -o main -lglut -lGL -lGLU
```

3. Run:

```bash
./main
```

---

### Windows (MinGW)

1. Compile:

```bash
g++ main.cpp -o main -lfreeglut -lopengl32 -lgdi32
```

2. Run:

```bash
./main
```

---

## Output

The application opens a window displaying a 3D amusement park scene that includes:

* Entrance gate with detailed design
* Road network and roundabout
* Multiple functional zones
* Food court and stalls
* Train track with moving train
* Trees, lamps, benches, and decorative elements
* Animated rides and environmental effects

---

