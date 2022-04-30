#pragma once
#ifndef _SHAPE_H_
#define _SHAPE_H_

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

class Ray;
class Hit;
class Light;

class Shape
{
public:
    Shape(){}
    virtual ~Shape(){}
    // functions for each object
    virtual float Intersect(const Ray &r,const float &t0,const float &t1){return 0.0f;}
    glm::vec3 getColor(const Hit &hi, const Light &li, glm::vec3 cPos);
    virtual void computeHit(const Ray &r, const float &t, Hit &h){};
    
    glm::vec3 Position;
    glm::vec3 Scale;
    glm::vec3 rotation;
    glm::vec3 Diffuse;
    glm::vec3 Specular;
    glm::vec3 Ambient;
    float angle;
    bool reflective = false;
    float Exponent;
};

#endif
