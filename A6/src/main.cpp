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
    
    Light(const vec3 &lp, float &i) :
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
    virtual void print(){ cout << "shape" << endl;}
    virtual float Intersect(const Ray &r,const float &t0,const float &t1) const { return 0;};
    virtual vec3 getColor(const Hit &h, const Light &l, vec3 cPos) const { return vec3(0); };
    
    vec3 Position;
    float Scale;
    vec3 rotation;
    vec3 Diffuse;
    vec3 Specular;
    vec3 Ambient;
    float Exponent;
};

class Sphere : public Shape{
public:
    Sphere(const vec3 &p, const float &s, const vec3 &r, const vec3 &d, const vec3 &sp, const vec3 &a, const float &e){
        Position = p; Scale = s; rotation =  r; Diffuse = d; Specular = sp; Ambient = a; Exponent = e;
    }
    void print() override{
        cout << "sphere" << endl;
    }
    float Intersect(const Ray &r,const float &t0,const float &t1) const override{
        vec3 pc =  r.origin - Position; // ray center - sphere position
        float a = dot(r.direction, r.direction);
        float b = 2*(dot(r.direction,pc)); // 2 * dot(v^, pc)
        float c = dot(pc,pc) - (Scale * Scale); // dot(pc,pc) - r^2
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

    vec3 getColor(const Hit &h, const Light &li, vec3 cPos) const override{
        // compute bling phong at that hit with given light
        vec3 l = normalize(li.lightPos -  h.x);
        // difuse
        float diffuse = std::max(0.0f, (float)dot(l,h.n));
        //specular color
        vec3 e = normalize(cPos -  h.x);
        vec3 hn = normalize(l + e);
        float specular = pow(std::max(0.0f, (float)dot(h.n,hn)), Exponent);
        vec3 color = Ambient + (Diffuse * diffuse + Specular * specular);
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
            int it = shapes[i]->Intersect(r, t0, t1);
            if(t < it){
                t = it;
                index = i;
            }
        }
        if(index == -1) return false;
        cout << "there is an intersection" << endl;
        // compute the point and the normal of the hit with respect to the shape
        // position of the hit
        vec3 x = r.origin * (t * r.direction);
        // calculate the normal w/ radius and center of the sphere
        vec3 n = normalize( (x - shapes[index]->Position) / shapes[index]->Scale);
        // change the hit
        h.x = x; h.n = n; h.t = t;
        return index;
    }
    
    // compute bling phong for all the lights using ith object as reference
    vec3 computeBlingPhong(const class Hit &h, const int index) const{
        vec3 cPos = h.x; // hit point position
        vec3 cNor = h.n; // hit point normal
        vec3 pixelColor = shapes[index]->Ambient;
        for(int i = 0; i < lights.size(); ++i){
            vec3 n = normalize(cNor);
            vec3 e = normalize(-cPos); // e vector
            vec3 l = normalize(lights[i].lightPos - cPos);
            vec3 h = normalize(l + e);
            float diffuse = glm::max(0.0f, dot(l,n));
            float specular = glm::pow(glm::max(0.0f,dot(h,n)), shapes[index]->Exponent);
            vec3 color = (shapes[index]->Diffuse * diffuse + shapes[index]->Specular * specular);
            pixelColor += color;
        }
        return (pixelColor * 255.0f);
    }
    
    void test() const{
        if(shapes[0]){
            shapes[0]->print();
        }
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
    Postion(0.0,0.0,5.0f),
    Rotation(0.0,0.0)
    {}
    
    // compute the ray at a given pixel
    Ray generateRay(int row, int col){
        float x = (-width + ((row * 2) + 1)) / (float) width;
        float y = (-height + ((col * 2) + 1)) / (float) height;
        vec3 v(tan(-fov/2.0) * x, tan(-fov/2.0) * y, -1.0f);
        v = normalize(v);
        vec3 p = Postion;
        p.z = 4.0;
        return Ray(p, v);
    }
    
    // compute the ray color function
    vec3 computeRayColor(const Scene &s, const Ray &r, const float &t0, const float &t1){
        vec3 color(0); // background color
        Hit h;
        int hit = s.Hit(r, t0, t1, h);
        cout << hit << endl;
        if(hit != -1){ // ray didnt hit an object in scene
            color = s.computeBlingPhong(h, hit);
        }
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
    
    void test(const Scene &s){
        s.test();
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
    // create the scene with the specifictations
    vec3 position, rotation, diffuse, specular, ambient;
    float exp,scale;
    
    position = vec3(-0.5, -1.0, 1.0);
    scale = 1.0f;
    rotation = vec3(0);
    diffuse = vec3(1.0,0,0);
    specular =  vec3(1.0,1.0,0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    
    Sphere * redS = new Sphere(position,scale,rotation,diffuse,specular,ambient,exp);
    
    scene.addShape(redS);
    
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
    
    camera = make_shared<Camera>(80,80);
    
    task1();
    
    image->writeToFile(filename);
    
	return 0;
}
