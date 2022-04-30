#pragma once
#ifndef _CAMERA_H_
#define _CAMERA_H_

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory.h>
#include <vector>

class Scene;
class Image;

class Hit
{
public:
    Hit() : x(0), n(0), t(0) {}
    Hit(const glm::vec3 &x, const glm::vec3 &n, float t) { this->x = x; this->n = n; this->t = t; }
    glm::vec3 x; // position
    glm::vec3 n; // normal
    float t; // distance
};

class Ray
{
public:
    Ray(){}
    Ray(const glm::vec3 & o, const glm::vec3 &d) :
    origin(o),
    direction(d)
    {}
    glm::vec3 origin;
    glm::vec3 direction;
};

class Camera
{
public:
    Camera(const int &w, const int &h);
    
    // compute the ray at a given pixel
    Ray generateRay(int row, int col);
    
    // compute the ray color function
    glm::vec3 computeRayColor(Scene &s, const Ray &r, const float &t0, const float &t1, const int &depth);
    
    // ray tracer function (Take Picture)
    void rayTrace(Scene &s, std::shared_ptr<Image> image);
    
    int width, height;
    float aspect;
    float fov;
    glm::vec3 Postion;
    glm::vec3 LookAt;
    glm::vec3 Up;
    glm::vec2 Rotation;
    std::vector<Ray> rays;
};

#endif
