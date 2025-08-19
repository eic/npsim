# Development Container for NPSim

This development container is based on the `eicweb/eic_ci:nightly` image, which provides a complete EIC software development environment.

## Features

- Based on the official EIC CI container
- Pre-configured with DD4hep, ROOT, and other EIC software dependencies
- Includes essential VS Code extensions for C++ and CMake development
- Configured for seamless development of NPSim

## Getting Started

You can develop with this repository in two ways:

### Option 1: GitHub Codespaces (Recommended)

1. Click the green "Code" button on the repository
2. Select "Open with Codespaces"
3. Click "New codespace"
4. Wait for the environment to be ready
5. Start developing in your browser!

### Option 2: Local VS Code with Dev Containers

1. Install VS Code and the "Dev Containers" extension
2. Clone this repository
3. Open the repository in VS Code
4. When prompted, click "Reopen in Container"
5. VS Code will build and start the development container

## Build Instructions

Once inside the container (either Codespaces or local):

```bash
mkdir build
cd build
cmake ..
make
```

## Included Tools and Software

- DD4hep with Geant4 integration
- ROOT 
- CMake
- GCC/G++
- Essential development tools

## VS Code Extensions

The following extensions are automatically installed:
- C/C++ tools for intellisense and debugging
- CMake Tools for project configuration and building
- Doxygen Documentation Generator