# Emergency Response Simulator (ERS)

C++17 / Qt 6 / SQLite / CMake OOP project. Simulation only - not for real emergencies.

## Build (Windows, Developer PowerShell for VS 2022)

    cmake -S . -B build -A x64
    cmake --build build --config Debug
    ctest --test-dir build -C Debug --output-on-failure