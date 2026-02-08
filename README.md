# Maxine: The Surgical Tensor Editor

**Maxine** is a lightweight (~65KB), standalone TUI (Terminal User Interface) for inspecting, editing, and debugging tensor data directly over SSH.

It is designed for the "Microscope" workflow: diagnosing why a specific layer in a remote 9GB model is failing, without the need to download the entire file or set up complex local environments.

## Why Maxine?

* **SSH Native:** Runs entirely in the terminal. No X11 forwarding, no dependencies.
* **Instant Deployment:** A single ~65KB binary. Fits in L1 cache.
* **Visual Debugging:** * **Heatmaps:** Instantly spot "hot" or "dead" neurons using ANSI colors.
    * **Diffing:** Compare two tensor files to see exactly which weights changed.
    * **Histograms:** Visual ASCII bar charts of value distributions.
* **Big Data Ready:** Supports 64-bit addressing to handle massive tensor files (100GB+).
<img width="1007" height="591" alt="Screenshot 2026-02-07 065013" src="https://github.com/user-attachments/assets/8f7e2960-748a-4fb5-b061-cdce2560e032" />
<img width="861" height="468" alt="Screenshot 2026-02-07 062433" src="https://github.com/user-attachments/assets/ce50536f-4ea9-4a10-8592-cf9f886c3282" />
<img width="890" height="574" alt="Screenshot 2026-02-07 061047" src="https://github.com/user-attachments/assets/d8e43530-f4a3-413d-9607-94295da5a941" />

## Key Features

### 1. The "Ghost" Diff
Load a checkpoint, then load a reference file (`:diff`) to see the *delta* in real-time.
* **Red:** Weight increased.
* **Cyan:** Weight decreased.

### 2. Diagnostic Suite
* **`:health`** - Scans layer for `NaNs`, `Infs`, and dead neurons.
* **`:hist`** - Plots an ASCII histogram of data distribution.
* **`:stats`** - Quick min/max/mean analysis.

### 3. Surgical Editing
* **`:clip [min] [max]`** - Clamp outliers.
* **`:norm`** - Normalize layer to 0.0 - 1.0.
* **`:zero`** - Manually kill a specific neuron.
* **`:goto [l] [r] [c]`** - Teleport to specific coordinates.

## Installation

Maxine is a single C++ binary with no external library dependencies.

```bash
# Clone
git clone [https://github.com/fhoth2/maxine.git](https://github.com/fhoth2/maxine.git)
cd maxine

# Build (Requires CMake)
mkdir build && cd build
cmake ..
make

# Run
./maxine_tensor
