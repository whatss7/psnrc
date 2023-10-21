# psnrc

A simple PSNR calculator for PPM files. Supports binary format (P6) and plain format (P3).

## Building

```
cmake . -B build
cmake --build build
```

or simply...

```
g++ psnrc.cpp -o psnrc -O2
```



## Usage

```
psnrc <path-to-1st-ppm> <path-to-2nd-ppm> [<path-to-diff-ppm-output>]
```

