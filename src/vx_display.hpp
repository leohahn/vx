#ifndef VX_DISPLAY_HPP
#define VX_DISPLAY_HPP

#include <GLFW/glfw3.h>
#include "um.hpp"

namespace vx
{
struct Display
{
    u16           width;
    u16           height;
    GLFWwindow*   window;

    Display(u16 width, u16 height, GLFWwindow* window)
        : width(width), height(height), window(window) {}
};

}

#endif // VX_DISPLAY_HPP
