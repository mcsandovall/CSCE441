# Assigment 1 Rasterizer 

Author: Mario Sandoval
email: mcsandoval@tamu.edu

## Goal 

Create a program that reads in and renders a triangle mesh (of type .obj) to an image via software rasterization. You may use existing resources to load in the mesh and to write out an image. You must write your own rasterizer. In general the required steps for the program are:

- Read in triangles.
- Compute colors per vertex.
- Convert triangles to image coordinates.
- Rasterize each triangle using barycentric coordinates for linear interpolations and in-triangle test.
- Write interpolated color values per pixel using a z-buffer test to resolve depth.

### Setting up the code

Add a command line argument to accept the following command line arguments:

- Input filename of the .obj file to rasterize
- Output image filename (should be png)
- Image width
- Image height
- Task number (1 through 7)

Excute 
```sh
./A1 ../resources/bunny.obj output.png 512 512 1
```

### Resources

Instructions
https://people.engr.tamu.edu/sueda/courses/CSCE441/2022S/assignments/A1/index.html

Bycentric coordinates
https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates

Triangular interpolation color
https://observablehq.com/@toja/triangular-color-interpolation

ZBuffering:
https://www.youtube.com/watch?v=HyVc0X9JKpg

Additional info: Course Slides
