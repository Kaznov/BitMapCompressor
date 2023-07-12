# BitMapCompressor

A simple tool for compressing BMPs with limited number of colors

### Usage

```
./bitmap_compressor file_in file_out
```

### Description

Bitmap is converted into an indexed format - with 1/4/8 bits per pixel, depending on the number of colors.
It results in a 32x/8x/4x smaller files.

### Requirements

- CMake,
- A C++ compiler supporting std::expected (GCC(libstdc++) >= 12, Clang(libc++) >= 16, MSVC >= 19.33)

### Should I use this program?

Most likely - not.
- If you want to just reduce the size of the image using a lossless conversion - use a PNG file format,
- If you need to convert only a few images into such indexed format - use GIMP: Image -> Mode -> Indexed.

However, if you need a small, command-line tool that converts a 24bpp BMP into a reduced bpp BMP, this program is for you!
