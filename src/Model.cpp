#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "Mesh.h"

Mesh::Mesh(const char *filename) : local_positions(), faces()
{
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) 
    {
        std::cerr << "Error:: could not open model file " << filename << '\n';
        return;
    }
    std::string line;
    while (!in.eof())
    {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v "))
        {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v.raw[i];
            local_positions.push_back(v);
        } 
        else if (!line.compare(0, 2, "f "))
        {
            iss >> trash;
            
            std::vector<int> f;
            int v_idx;
            while (iss >> v_idx)
            {
                v_idx--;
                f.push_back(v_idx);
            }
            faces.push_back(f);
        }
    }
}

int Mesh::get_face_count()
{
    return faces.size();
}

Vec3f Mesh::get_local_position(int i)
{
    return local_positions[i];
}

std::vector<int> Mesh::get_face(int idx)
{
    return faces[idx];
}