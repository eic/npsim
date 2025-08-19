# Development Container for NPSim

This development container is based on the `eicweb/eic_ci:nightly` image, which provides a complete EIC software development environment.

## Features

- Based on the official EIC CI container
- Pre-configured with DD4hep, ROOT, and other EIC software dependencies
- Includes essential VS Code extensions for C++ and CMake development
- Configured for seamless development of NPSim

## Getting Started

1. Install VS Code and the "Remote - Containers" extension
2. Open this repository in VS Code
3. When prompted, click "Reopen in Container"
4. VS Code will build and start the development container

## Build Instructions

Once inside the container:

```bash
mkdir build
cd build
cmake ..
make
```

## Included Tools and Software

- DD4hep
- ROOT
- Geant4
- CMake
- GCC/G++
- Essential development tools

## VS Code Extensions

The following extensions are automatically installed:
- C/C++ tools
- CMake/CMake Tools
- Doxygen Documentation Generator

## Environment Variables

Key environment variables are pre-configured:
- DD4HEP_DIR
- LD_LIBRARY_PATH
- ROOT_INCLUDE_PATH