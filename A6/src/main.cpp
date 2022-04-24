#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <string>
#include <map>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Image.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;
using namespace glm;

shared_ptr<Image> image;

// testing function
void printVec(const vec3 &v){
    cout << v.x << " " << v.y << " " << v.z << endl;
}

// needed classes for the assignment

class Hit
{
public:
    Hit() : x(0), n(0), t(0) {}
    Hit(const vec3 &x, const vec3 &n, float t) { this->x = x; this->n = n; this->t = t; }
    vec3 x; // position
    vec3 n; // normal
    float t; // distance
};

class Ray
{
public:
    Ray(){}
    Ray(const vec3 & o, const vec3 &d) :
    origin(o),
    direction(d)
    {}
    vec3 origin;
    vec3 direction;
};

class Light
{
public:
    Light() :
    lightPos(0),
    intensity(0)
    {}
    
    Light(const vec3 &lp,const float &i) :
    lightPos(lp),
    intensity(i)
    {}
    
    vec3 lightPos;
    float intensity;
};

class Shape
{
public:
    Shape(){}
    ~Shape(){}
    // functions for each object
    virtual float Intersect(const Ray &r,const float &t0,const float &t1) const { return 0;};
    virtual vec3 getColor(const Hit &h, const Light &l, vec3 cPos) const { return vec3(0); };
    
    vec3 Position;
    vec3 Scale;
    vec3 rotation;
    vec3 Diffuse;
    vec3 Specular;
    vec3 Ambient;
    float Exponent;
};

class Sphere : public Shape{
public:
    Sphere(const vec3 &p, const vec3 &s, const vec3 &r, const vec3 &d, const vec3 &sp, const vec3 &a, const float &e){
        Position = p; Scale = s; rotation =  r; Diffuse = d; Specular = sp; Ambient = a; Exponent = e;
    }
    
    float Intersect(const Ray &r,const float &t0,const float &t1) const override{
        vec3 pc =  r.origin - Position; // ray center - sphere position vec(0,0,4)
        float a = dot(r.direction, r.direction);
        float b = 2*(dot(r.direction,pc)); // 2 * dot(v^, pc)
        float c = dot(pc,pc) - (Scale.x * Scale.x); // dot(pc,pc) - r^2
        float d = (b*b) - (4 * a * c);
        if(d > 0){
            // solve for the distances
            float _t0,_t1;
            _t0 = (-b + sqrt(d)) / (2*a);
            _t1 = (-b - sqrt(d)) / (2*a);
            return std::min(_t0,_t1);
        }
        return INT_MAX;
    }

    vec3 getColor(const Hit &hi, const Light &li, vec3 cPos) const override{
        // compute bling phong
        vec3 Pos = hi.x; // hit position (point position)
        // compute the normal for the translated sphere
        vec3 n = normalize(Pos - Position / Scale);
        vec3 l = normalize(li.lightPos -  Pos);
        vec3 e = normalize(cPos - Pos); // camera - point
        vec3 h = normalize(l + e);

        // compute the colors
        vec3 color =  Ambient;
        vec3 diff = Diffuse * std::max(0.0f,dot(l,n));
        vec3 spec = Specular * pow(std::max(0.0f,dot(h,n)), Exponent);
        color += diff + spec;
        return color;
    }
};


class Scene
{
public:
    Scene(){}
    
    int Hit(const Ray &r, const float &t0, const float &t1, Hit &h) const{
        float t = INT_MAX;
        int index = -1;
        for(int i = 0; i < shapes.size(); ++i){
            float it = shapes[i]->Intersect(r, t0, t1);
            if(it < t){
                t = it;
                index = i;
            }
        }
        if(index == -1) return index;
        // compute the point and the normal of the hit with respect to the shape
        // position of the hit
        vec3 x = r.origin + (t * r.direction);
        vec3 n = r.direction; // compute the normal with respect to the given object
        // change the hit
        h.x = x; h.n = n; h.t = t;
        return index;
    }
    
    // compute bling phong for all the lights using ith object as reference
    vec3 computeBlingPhong(const class Hit &h, const int index) const{
        vec3 pixelColor(0); // set it to the normal
        
        for(int i = 0; i < lights.size(); ++i){
            vec3 color = shapes[index]->getColor(h, lights[i], vec3(0,0,5.0));
            pixelColor += color;
        }
        // clamp the values of the colors
        pixelColor.r = std::clamp(pixelColor.r, 0.0f, 1.0f);
        pixelColor.g = std::clamp(pixelColor.g, 0.0f, 1.0f);
        pixelColor.b = std::clamp(pixelColor.b, 0.0f, 1.0f);
        return pixelColor * 255.0f;
    }
    
    // setters
    void addShape(Shape *s){ shapes.push_back(s); }
    void addLight(const Light &l){ lights.push_back(l); }
    
    vector<Shape*> shapes;
    vector<Light> lights;
};

class Camera
{
public:
    Camera(const int &w, const int &h) :
    width(w),
    height(h),
    aspect(1.0f),
    fov((float)(45.0*M_PI/180.0)),
    Postion(0.0,0.0,5.0),
    Rotation(0.0,0.0)
    {}
    
    // compute the ray at a given pixel
    Ray generateRay(int row, int col){
        float x = (-width + ((row * 2) + 1)) / (float) width;
        float y = (-height + ((col * 2) + 1)) / (float) height;
        vec3 v(tan(-fov/2.0) * x, tan(-fov/2.0) * y, -1.0f);
        v = normalize(v);
        return Ray(Postion, v);
    }
    
    // compute the ray color function
    vec3 computeRayColor(const Scene &s, const Ray &r, const float &t0, const float &t1){
        vec3 color(0); // background color
        Hit h;
        int hit = s.Hit(r, t0, t1, h);
        if(hit > -1){ // ray hit the scene
            color = s.computeBlingPhong(h, hit);
        } // else return the background color
        return color;
    }
    
    // ray tracer function (Take Picture)
    void rayTrace(const Scene &s){
        for(int j = 0; j < height; ++j){
            for(int i = 0; i < width; ++i){
                // compute the primary array
                Ray r = generateRay(i, j);
                // compute the ray colors
                vec3 c = computeRayColor(s, r, 0, INFINITY);
                // set the color pixel to the ray color
                image->setPixel(i, j, c.r, c.g, c.b);
            }
        }
    }
    
    int width, height;
    float aspect;
    float fov;
    vec3 Postion;
    vec3 LookAt;
    vec3 Up;
    vec2 Rotation;
    vector<Ray> rays;
};

shared_ptr<Camera> camera;

// make scene 1
void task1(){
    // make a scene
    Scene scene;
    
    // make the light
    Light l(vec3(-2.0,1.0,1.0), 1.0f);
    
    scene.addLight(l);
    
    // create the scene with the specifictations
    vec3 position, rotation, diffuse, specular, ambient, scale;
    float exp;
    
    position = vec3(-0.5, -1.0, 1.0);
    scale = vec3(1.0f);
    rotation = vec3(0);
    diffuse = vec3(1.0,0,0);
    specular =  vec3(1.0,1.0,0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    Sphere * redS = new Sphere(position,scale,rotation,diffuse,specular,ambient,exp);
    position = vec3(0.5, -1.0, -1.0);
    scale = vec3(1.0f);
    rotation = vec3(0);
    diffuse = vec3(0.0, 1.0, 0.0);
    specular =  vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1, 0.1, 0.1);
    exp = 100.0f;
    Sphere * greenS = new Sphere(position,scale,rotation,diffuse,specular,ambient,exp);
    position = vec3(0.0, 1.0, 0.0);
    scale = vec3(1.0f);
    rotation = vec3(0);
    diffuse = vec3(0.0, 0.0, 1.0);
    specular =  vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1, 0.1, 0.1);
    exp = 100.0f;
    Sphere * blueS = new Sphere(position,scale,rotation,diffuse,specular,ambient,exp);
    scene.addShape(redS);
    scene.addShape(greenS);
    scene.addShape(blueS);
    
    camera->rayTrace(scene);
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		cout << "Error: A6 Not Enough Arguments" << endl;
		return 0;
	}
    
    //int sceneNumber = atoi(argv[1]);
    
	string meshName = "../../resources/bunny.obj";
    // Output filename
    string filename(argv[3]);
    // Image width
    int width = atoi(argv[2]);
    // image height
    int height = atoi(argv[2]);
    // task to be executed
    //int task = atoi(argv[5]);
    // Create the image like in the labs
    image = make_shared<Image>(width, height);
    
    camera = make_shared<Camera>(width,height);
    
    task1();
    
    image->writeToFile(filename);
    
	return 0;
}
