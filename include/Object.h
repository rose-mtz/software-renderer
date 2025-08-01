#pragma once
#include "Vec.h"
#include "Mesh.h"

struct Object
{
    Vec3f world_pos;
    Vec3f orientation; // euler angles

    // Temporary
    Vec3f color;

    Mesh* mesh;
};