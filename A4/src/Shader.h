#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <vector>
#include <memory>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

enum SHADER_TYPE{
    PHONG,
    TEXTURE
};

class Program;
class MatrixStack;

// make the class for the materials
class Material{
public:
    glm::vec3 ka,kd,ks;
    float s;
    Material() : ka(0.0,0.0,0.0), kd(0.0,0.0,0.0), ks(0.0,0.0,0.0), s(0) {} // constructor
    Material(glm::vec3 _ka, glm::vec3 _kd, glm::vec3 _ks, float _s) : ka(_ka), kd(_kd), ks(_ks), s(_s) {}
};
// class for the lights
class Light{
public:
    glm::vec3 lightPos, lightColor;
    Light() : lightPos(1.0,1.0,1.0), lightColor(1.0,1.0,1.0){}
    Light(glm::vec3 _lp, glm::vec3 _lc) : lightPos(_lp), lightColor(_lc){}
};
// class for the shaders
class Shader{
public:
    shared_ptr<Program> program;
    SHADER_TYPE type;
    Shader(string vertex_file, string frag_file);
    void bind(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, Material M, bool twoD);
    void program_unbind();
};
// class that has the collection of shaders
class Shader_Collection{
public:
    shared_ptr<Shader> header, tail, current;
    Shader_Collection() : header(nullptr),tail(nullptr), current(nullptr){}
    void push_back(string vert_file, string frag_file, SHADER_TYPE _type);
    void move_forward();
    void move_backward();
};

#endif
