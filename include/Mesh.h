#pragma once

#include <vector>
#include "Vec.h"

class Mesh 
{
public:
	std::vector<Vec3f> vertices;
	std::vector<Vec2f> uvs;
	std::vector<std::vector<int>> faces;

	Mesh(const char *filename);
};