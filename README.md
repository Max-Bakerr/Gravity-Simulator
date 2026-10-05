A-level computing NEA
Scored 69/70 - highest score ever awarded by my school:

The full write-up — analysis, design, development and evaluation, 213 pages — is in
[docs/nea-writeup.pdf](docs/nea-writeup.pdf).

## What it does
- N-body gravitational simulation with configurable masses, positions and velocities
- A deformable spacetime grid visualising the potential well around each body
- Persistent orbital trails, so the shape of each orbit is visible as it develops
- Screen-space labels that track their body through the 3D projection
- Live parameter control through an ImGui panel — masses, velocities and the
  gravitational constant can be changed while the simulation runs
- Free camera over the scene

Roughly 1,000 lines of C++, organised into `Planet`, `TrailPath`,
`SpaceTimeGrid` and `Label` classes over a small `vec2` type.

The gravitational constant is scaled (`G = 750000.0f`) rather than SI, so that
orbits evolve at a watchable rate at screen distances.

## Repository layout

```
gravity_simulator.cpp   the simulator
prototypes/             22 incremental programs written while building it
imgui*.h, imconfig.h    vendored Dear ImGui headers
docs/                   the A-level write-up
```


## Building
Requires a C++17 compiler and, as system libraries:

- [GLFW](https://www.glfw.org/) — windowing and input
- [GLM](https://github.com/g-truc/glm) — vector and matrix maths
- OpenGL 3.3+ (macOS system framework)

On macOS with Homebrew:

```sh
brew install glfw glm
```

Dear ImGui is used for the control panel. Only its headers are vendored here —
clone [Dear ImGui](https://github.com/ocornut/imgui) alongside this repository
and compile `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`,
`imgui_widgets.cpp`, `imgui_impl_glfw.cpp` and `imgui_impl_opengl3.cpp` with
the simulator:

```sh
g++ -std=c++17 gravity_simulator.cpp path/to/imgui/*.cpp \
    -I. -Ipath/to/imgui \
    -lglfw -framework OpenGL -framework Cocoa -framework IOKit \
    -o gravity_simulator
```

## Notes

Bodies are treated as point masses; there is no collision handling or merging,
so a sufficiently close encounter will slingshot rather than collide.
