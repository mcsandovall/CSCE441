#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "Camera.h"
#include "Scene.h"
#include "Shape.h"


Scene::Scene(){}
Scene::~Scene(){ for(int i =0; i < shapes.size(); ++i){ delete shapes[i]; }}
int Scene::Hit(const Ray &r, const float &t0, const float &t1, class Hit &h, const int &depth){
    float t = INT_MAX;
    int index = -1;
    for(int i = 0; i < shapes.size(); ++i){
        float it = shapes[i]->Intersect(r, t0, t1);
        if(it < t && it > t0 && it < t1){
            t = it;
            index = i;
        }
    }
    if(index == -1) return index;
    // compute the point and the normal of the hit with respect to the shape
    shapes[index]->computeHit(r, t, h);
    return index;
}
