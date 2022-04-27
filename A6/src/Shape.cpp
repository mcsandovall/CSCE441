#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <string>
#include <map>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shape.h"
#include "Camera.h"
#include "Scene.h"

glm::vec3 Shape::getColor(const Hit &hi, const Light &li, glm::vec3 cPos) const {
    // compute bling phong
    glm::vec3 Pos = hi.x; // hit position (point position)
    // compute the normal for the translated sphere
    glm::vec3 n = normalize(hi.n);
    glm::vec3 l = normalize(li.lightPos -  Pos);
    glm::vec3 e = normalize(cPos - Pos); // camera - point
    glm::vec3 h = normalize(l + e);

    // compute the colors
    glm::vec3 diff = Diffuse * std::max(0.0f,dot(l,n));
    glm::vec3 spec = Specular * pow(std::max(0.0f,dot(h,n)), Exponent);
    return diff + spec;
};
