# Mandelbrot MPI MPFR
This program utilizes MPI along with MPFR to generate Mandelbrot zooms at high speed and arbitrary percision. Tasks are delegated in such a way that each process will be processing a single frame at a time.

## Examples
[![Mandelbroot zoom example video](https://img.youtube.com/vi/5sWrd5T_MvA/0.jpg)](https://youtu.be/5sWrd5T_MvA)

## Known Issues
1. Buffer overflows with `frame` and other buffers. 
2. Speed. Will begin to investigate with Score-P
3. MPFR cannot find libs, simple fix of appending to $LD_(something)
