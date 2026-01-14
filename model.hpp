#pragma once
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>

#include "geometry.hpp"

using vertex_t = vec<float,  3>;
using face_t   = vec<size_t, 3>;

using model_t =
struct model {
    model(std::string path){
        std::ifstream file(path);
        if(!file) std::cout << "Model Not Found" << std::endl;
        std::string line;
        while(std::getline(file, line)){
            std::istringstream iss(line);
            std::string tag;
            iss >> tag;

            if(tag == "v"){
                float x, y, z;
                iss >> x >> y >> z;
                vertices.emplace_back((vertex_t){x, y, z});
            } else 

            if (tag == "f"){
                size_t a, b, c;
                iss >> a;
                iss.ignore(std::numeric_limits<std::streamsize>::max(), ' '); iss >> b;
                iss.ignore(std::numeric_limits<std::streamsize>::max(), ' '); iss >> c;
                faces.emplace_back((face_t){a, b, c});
            }
        }
    }
    std::vector<vertex_t> vertices;
    std::vector<face_t> faces;
};
