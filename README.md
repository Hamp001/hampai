# MicroAI

MicroAI is an offline-first Windows assistant focused on practical automation.
The project starts with a lightweight native system monitor and will later add
a local language-model runtime and a safe, deterministic tool layer.

## Current milestone: CLI system monitor

The first milestone is a small C++20 dashboard that reports:

- Total CPU utilization, sampled every 500 ms.
- Physical memory usage and available memory.
- Logical processor count.
- AVX2 CPU and OS-state support.
- Inference metrics as unavailable until a model runtime is integrated.

No inference performance is fabricated or simulated.

## Build (Windows)

Requirements:
- Windows 10/11
- CMake 3.20+
- Visual Studio 2022 with the Desktop development with C++ workload.

From the repository root, run:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\microai.exe
```

Press `Q` to quit.

## Planned direction

1. Establish and test the native CLI monitor.
2. Add a benchmark harness for CPU inference.
3. Integrate a compact local model through a proven runtime.
4. Add validated, allowlisted automation tools.
5. Profile before adding AVX2-specific kernels or assembly.

The language model will interpret requests; deterministic native tools will
perform actions. Destructive operations will require explicit confirmation.
