# Emergency Response Simulator (ERS)

C++17 / Qt 6 / SQLite / CMake object-oriented programming project. Educational simulation only - not for real emergencies.

A fictional city is modelled as a weighted road graph. Medical, fire, road-accident and crime incidents are prioritised and served by ambulances, fire trucks and police vehicles. The simulator chooses the best available vehicle by estimated travel time (route minutes scaled by vehicle speed), moves it along the route, and records dispatches, an event log and statistics. Runs can be saved to SQLite and reviewed later.

## Features

- Dijkstra routing with traffic factors and blocked roads
- Priority queue, compatibility filtering, partial allocation and retry of unmet resource types
- Qt GUI with an interactive map, tables, dialogs, step-by-step scenario playback and saved runs
- Four prepared scenarios and a 12-node sample city
- 23 automated test executables (21 plain, 2 Qt Test)

## Build (Windows, Developer PowerShell for VS 2022)

    cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
    cmake --build build --config Debug
    ctest --test-dir build -C Debug --output-on-failure

The Qt path is cached after the first configure. Without Qt the GUI targets are skipped and the core library with its plain tests still builds.

## Run

    $env:PATH = "C:\Qt\6.12.0\msvc2022_64\bin;" + $env:PATH
    .\build\src\ui\Debug\ers_gui.exe

## Package

    cmake --build build --config Release --target ers_gui
    New-Item -ItemType Directory -Force -Path .\dist\ERS | Out-Null
    Copy-Item .\build\src\ui\Release\ers_gui.exe .\dist\ERS\
    & C:\Qt\6.12.0\msvc2022_64\bin\windeployqt.exe --release --no-translations .\dist\ERS\ers_gui.exe

windeployqt also copies vc_redist.x64.exe for machines without the Visual C++ runtime. Saved runs go to %APPDATA%\ERS\EmergencyResponseSimulator\ers_gui.db.

## Documentation

- docs/ARCHITECTURE.md - layers, ownership, timing model, persistence, tests
- docs/VIVA.md - every OOP concept mapped to files, demo script, likely questions
- docs/USER_GUIDE.md - controls, map, shortcuts, scenarios
- Software Requirements Specification (SRS v1.4 and later)

## Layout

    include/ers/{common,models,services,repositories,ui}   headers
    src/{common,models,services,repositories,ui}           sources
    src/ui/tests                                           Qt Test executables
    tests                                                  plain test executables
    third_party/sqlite                                     SQLite amalgamation
    database/schema.sql                                    reference schema