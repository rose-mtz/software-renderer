#pragma once

#include <vector>
#include "Vec.h"

class Model 
{
private:
	std::vector<Vec3f> local_positions;
	std::vector<std::vector<int>> faces;

public:
	Model(const char *filename);

	int get_face_count();
	Vec3f get_local_position(int i);
	std::vector<int> get_face(int idx);
};