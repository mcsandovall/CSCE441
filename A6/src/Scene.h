#pragma once
#ifndef _SCENE_H_
#define _SCENE_H_

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "Shape.h"


class Light
{
public:
    Light() :
    lightPos(0),
    intensity(0)
    {}
    
    Light(const glm::vec3 &lp,const float &i) :
    lightPos(lp),
    intensity(i)
    {}
    
    glm::vec3 lightPos;
    float intensity;
};

class Scene
{
public:
    Scene();
    ~Scene();
    int Hit(const Ray &r, const float &t0, const float &t1, Hit &h, const int &depth) const;
    // setters
    void addShape(Shape *s){ shapes.push_back(s); }
    void addLight(const Light &l){ lights.push_back(l); }
    
    std::vector<Shape*> shapes;
    std::vector<Light> lights;
};

#endif
