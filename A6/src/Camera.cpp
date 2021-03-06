#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <string>
#include <map>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "Scene.h"
#include "Image.h"

using namespace glm;

#define MAX_DEPTH 6

Camera::Camera(const int &w, const int &h, bool s8) :
width(w),
height(h),
aspect(1.0f),
fov((float)(45.0*M_PI/180.0)),
Postion(0.0,0.0,5.0),
Rotation(0.0,0.0),
scene8(s8)
{}
    
// compute the ray at a given pixel
Ray Camera::generateRay(int row, int col){
    float wp = 2 * tan(fov / 2) / width;
    float hp = 2 * tan(fov / 2) / height;
    
    float b = -tan(fov/2);
    float x = row * wp + b +  (wp / 2.0f);
    float y = col * hp + b + (hp / 2.0f);
    vec3 v;
    // get the direction with respect to the virtual plane
    if(scene8){
        v = vec3(-2.0,y,x) - Postion;
    }else{
        v = vec3(x,y,4.0) - Postion;
    }
    v = normalize(v);
    return Ray(Postion, v);
}
    
// compute the ray color function
vec3 Camera::computeRayColor(Scene &s, const Ray &r, const float &t0, const float &t1, const int &depth){
    vec3 color(0); // background color
    Hit h;
    int hit = s.Hit(r, t0, t1, h,0);
    if(hit > -1){ // ray hit the scene
        color = s.shapes[hit]->Ambient;
        if(s.shapes[hit]->reflective && depth < MAX_DEPTH){
            vec3 refDir = reflect(r.direction, h.n);
            Ray refray(h.x,refDir);
            return color + computeRayColor(s, refray, 0.001, INFINITY, depth+1);
        }
        Hit srec;
        for(Light l : s.lights){
            vec3 lightDir =  normalize(l.lightPos - h.x);
            float lightDist = distance(l.lightPos, h.x);
            Ray sray(h.x,lightDir);
            if(s.Hit(sray, 0.001, lightDist, srec,0) == -1){ // no hits
                color += l.intensity * s.shapes[hit]->getColor(h, l, r.origin);
            }
        }
    } // else return the background color
    color.r = std::clamp(color.r, 0.0f, 1.0f);
    color.g = std::clamp(color.g, 0.0f, 1.0f);
    color.b = std::clamp(color.b, 0.0f, 1.0f);
    return color * 255.0f;
}
    
// ray tracer function (Take Picture)
void Camera::rayTrace(Scene &s, std::shared_ptr<Image> image){
    for(int j = 0; j < height; ++j){
        std::cout << (j / (float) height) * 100.0f << "%" << std::endl;
        for(int i = 0; i < width; ++i){
            // compute the primary array
            Ray r = generateRay(i, j);
            // compute the ray colors
            vec3 c = computeRayColor(s, r, 0, INFINITY, 0);
            // set the color pixel to the ray color
            image->setPixel(i, j, c.r, c.g, c.b);
        }
    }
}
