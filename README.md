# Gravity Simulator

An interactive N-body gravity simulator in C++ with real-time 3D rendering,
written as my A-level Computer Science NEA. It scored 69/70 — the highest mark
the school had awarded for the coursework.

The distinctive feature is the **spacetime grid**: a deformable mesh rendered
beneath the bodies that curves in response to their masses, giving a visual
analogue of gravitational potential alongside the Newtonian simulation driving
the motion.

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
prototypes/             development history — see below
imgui*.h, imconfig.h    vendored Dear ImGui headers
```

### prototypes/

Twenty-two incremental programs written while building the simulator, kept
because they document how it was developed rather than just where it ended up.
They run roughly in this order:

`IntroducingVector` → `IntroducingGravity` → `circle` → `OneSphere` →
`CreateSphere` → `2_Planet` → `Planet_Class_1..3` → `TrailPathClass` →
`STGrid` → `STGridVersion4` → `3D` → `3D2` → `Label_Class` → `FrontEND1` →
`Stage5` → `Emscripten2`

`PseudoCode.pseudo` holds the original design notes.

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
