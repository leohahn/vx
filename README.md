# The project
This is a "fork" of the same project written in C (https://github.com/leohahn/voxl). It was rewritten in C++ because I was annoyed by the lack of things like function overloading, operator overloading, etc. This is therefore a 1-to-1 translation.

# What was implemented
- Rendering block into chunks (group of blocks) to reduce the number of draw calls.
- Merging faces of adjacent cubes to send less vertices to the gpu.
- Frustum culling
- Depth buffer renderer (in the CPU, not optimized) for the implementation of occlusion culling and drawing occluders. (not finished)

An album showing a few gifs is located here: https://imgur.com/a/lULV3.

# How to run
*Note*: The code was only tested under linux, and has glfw3 and glew as external dependencies.

```
make all
make run
```
