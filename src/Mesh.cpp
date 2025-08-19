#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "Mesh.h"

Mesh::Mesh(const char *filename) : vertices(), faces()
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
            vertices.push_back(v);
        }
        else if (!line.compare(0, 2, "vt"))
        {
            iss >> trash; iss >> trash;
            Vec2f uv;
            for (int i=0;i<2;i++) iss >> uv.raw[i];
            uvs.push_back(uv);
        }
        else if (!line.compare(0, 2, "f "))
        {
            iss >> trash;
            
            std::vector<int> f;
            int v_idx, uv_idx;
            while (iss >> v_idx >> trash >> uv_idx)
            {
                v_idx--;
                uv_idx--;
                f.push_back(v_idx);
                f.push_back(uv_idx);
            }
            faces.push_back(f);
        }
    }
}