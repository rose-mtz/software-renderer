#pragma once

#include <vector>
#include "Vec.h"

class Mesh 
{
private:
	std::vector<Vec3f> local_positions;
	std::vector<std::vector<int>> faces;

public:
	Mesh(const char *filename);

	int get_face_count();
	Vec3f get_local_position(int i);
	std::vector<int> get_face(int idx);
};