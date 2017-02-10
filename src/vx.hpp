#ifndef VX_HPP
#define VX_HPP

#include <stdbool.h>
#include "um.hpp"

struct GLFWwindow;

namespace vx
{

struct Memory;
struct Camera;

void main_render(Memory& memory, Camera& camera, bool* keyboard);
void main_update(Memory& memory, Camera& camera, bool* keyboard, f64 delta);

}

#endif // VX_HPP
