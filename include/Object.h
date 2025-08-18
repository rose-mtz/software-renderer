#pragma once
#include "Vec.h"
#include "Mesh.h"
#include "Buffer.h"

struct Object
{
    // IDEA: this object is solely for rendering, no need to include 
    //       complex data structures, only the bare minumum to render it
    //       you can have the complex data structures in game/app specific code

    Vec3f translation;
    Vec3f scale;
    float yaw, pitch, roll;

    Mesh* mesh;
    Buffer* texture;
};