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

#define MAX_DEPTH 6

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
    virtual ~Shape(){}
    // functions for each object
    virtual float Intersect(const Ray &r,const float &t0,const float &t1) const { return 0;};
    vec3 getColor(const Hit &hi, const Light &li, vec3 cPos) const {
        // compute bling phong
        vec3 Pos = hi.x; // hit position (point position)
        // compute the normal for the translated sphere
        vec3 n = normalize(hi.n);
        vec3 l = normalize(li.lightPos -  Pos);
        vec3 e = normalize(cPos - Pos); // camera - point
        vec3 h = normalize(l + e);

        // compute the colors
        vec3 diff = Diffuse * std::max(0.0f,dot(l,n));
        vec3 spec = Specular * pow(std::max(0.0f,dot(h,n)), Exponent);
        return diff + spec;
    };
    virtual void computeHit(const Ray &r, const float &t, Hit &h) const{}
    
    vec3 Position;
    vec3 Scale;
    vec3 rotation;
    vec3 Diffuse;
    vec3 Specular;
    vec3 Ambient;
    float angle;
    bool reflective = false;
    float Exponent;
};

class Sphere : public Shape{
public:
    Sphere(const vec3 &p, const vec3 &s, const vec3 &r,const float &ang, const vec3 &d, const vec3 &sp, const vec3 &a, const float &e, const bool &ref = false){
        Position = p; Scale = s; rotation =  r; Diffuse = d; Specular = sp; Ambient = a; Exponent = e, angle = ang; reflective = ref;
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
    
    void computeHit(const Ray &r, const float &t, Hit &h) const override{
        h.x = r.origin + (t * r.direction);
        h.n = normalize((h.x - Position) / Scale);
        h.t = t;
    }
};

class Ellipsoid : public Shape{
public:
    Ellipsoid(const vec3 &p, const vec3 &s, const vec3 &r,const float &angle, const vec3 &d, const vec3 &sp, const vec3 &a, const float &e){
        Position = p; Scale = s; rotation =  r; Diffuse = d; Specular = sp; Ambient = a; Exponent = e;
        // load the matrix E = T * R * S
        ellipsoid_E = mat4(1.0); // load identity
        ellipsoid_E *= glm::translate(mat4(1.0), p);
        ellipsoid_E *= glm::rotate(mat4(1.0), angle, r);
        ellipsoid_E *= glm::scale(mat4(1.0), s);
    }
    
    float Intersect(const Ray &r,const float &t0,const float &t1) const override{
        vec3 p = inverse(ellipsoid_E) * vec4(r.origin,1.0);
        vec3 v = inverse(ellipsoid_E) * vec4(r.direction,0.0);
        v = normalize(v);
        float a = dot(v,v);
        float b = 2 * dot(v,p);
        float c = dot(p,p) - 1;
        float d = (b * b) - (4 * a * c);
        if(d > 0){
            // solve for the distances
            float _t0,_t1;
            vec3 x;
            _t0 = (-b + sqrt(d)) / (2*a);
            x = p + (_t0 * v);
            x = ellipsoid_E * vec4(x,1.0);
            _t0 = abs(distance(x, r.origin));
            if(dot(r.direction, x - r.origin) < 0){
                _t0 = -_t0;
            }
            _t1 = (-b - sqrt(d)) / (2*a);
            x = p + (_t1 * v);
            x = ellipsoid_E * vec4(x,1.0);
            _t1 = abs(distance(x, r.origin));
            if(dot(r.direction, x - r.origin) < 0){
                _t1 = -_t1;
            }
            return std::min(_t0,_t1);
        }
        return INT_MAX;
    }
    
    void computeHit(const Ray &r, const float &t, Hit &h) const override{
        vec3 p = inverse(ellipsoid_E) * vec4(r.origin,1.0);
        vec3 v = inverse(ellipsoid_E) * vec4(r.direction,0.0);
        v = normalize(v);
        float a = dot(v,v);
        float b = 2 * dot(v,p);
        float c = dot(p,p) - 1;
        float d = (b * b) - (4 * a * c);
        if(d > 0){
            // solve for the distances
            float _t0,_t1;
            vec3 x, n;
            _t0 = (-b + sqrt(d)) / (2*a);
            x = p + (_t0 * v);
            n = normalize(inverse(transpose(ellipsoid_E)) * vec4(x,0.0));
            x = ellipsoid_E * vec4(x,1.0);
            _t0 = abs(distance(x, r.origin));
            if(dot(r.direction, x - r.origin) < 0){
                _t0 = -_t0;
            }
            if(t == _t0){
                h.x = x;
                h.n = n;
            }
            _t1 = (-b - sqrt(d)) / (2*a);
            x = p + (_t1 * v);
            n = normalize(inverse(transpose(ellipsoid_E)) * vec4(x,0.0));
            x = ellipsoid_E * vec4(x,1.0);
            _t1 = abs(distance(x, r.origin));
            if(dot(r.direction, x - r.origin) < 0){
                _t1 = -_t1;
            }
            h.x = x; h.n = n;
        }
    }
    
    mat4 ellipsoid_E;
};

class Plane : public Shape{
public:
    Plane(const vec3 &p, const vec3 &s, const vec3 &r,const float &ang, const vec3 &d, const vec3 &sp, const vec3 &a, const float &e){
        Position = p; Scale = s; rotation =  r; Diffuse = d; Specular = sp; Ambient = a; Exponent = e, angle =  ang;
        if(Position.x < 0.0){ normal = vec3(1.0,0.0,0.0); }
        else if (Position.y < 0.0){normal = vec3(0.0,1.0,0.0); }
        else if (Position.z < 0.0) {normal =  vec3(0.0,0.0,1.0); }
    }
    
    float Intersect(const Ray &r,const float &t0,const float &t1) const override{
        return  dot(normal, (Position - r.origin))/dot(normal,r.direction);
    }
    
    void computeHit(const Ray &r, const float &t, Hit &h) const override{
        h.x =  r.origin +  (t * r.direction);
        h.n = normal;
        h.t = 0;
    }
    vec3 normal;
};


class Scene
{
public:
    Scene(){}
    ~Scene(){ for(int i =0; i < shapes.size(); ++i){ delete shapes[i]; }}
    int Hit(const Ray &r, const float &t0, const float &t1, Hit &h, const int &depth) const{
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
        
        if(shapes[index]->reflective == true && depth < MAX_DEPTH){
            vec3 refDir = reflect(r.direction, h.n);
            Ray refray(h.x,refDir);
            int rhit = this->Hit(refray, 0.001, INFINITY, h, depth+1);
            if(rhit != -1) return rhit;
        }
        return index;
    }
    
    // compute bling phong for all the lights using ith object as reference
    vec3 computeBlingPhong(const class Hit &h, const int index) const{
        vec3 pixelColor = shapes[index]->Ambient;
        class Hit srec;
        for(Light l : lights){
            vec3 lightDir =  normalize(l.lightPos - h.x);
            float lightDist = distance(l.lightPos, h.x);
            Ray sray(h.x,lightDir);
            if(this->Hit(sray, 0.001, lightDist, srec,0) == -1){ // no hits
                pixelColor += l.intensity * shapes[index]->getColor(h, l, vec3(0,0,5.0));
            }
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
    fov((float)(-45.0*M_PI/180.0)),
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
        int hit = s.Hit(r, t0, t1, h,0);
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
    float exp, angle;
    
    position = vec3(-0.5, -1.0, 1.0);
    scale = vec3(1.0f);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(1.0,0,0);
    specular =  vec3(1.0,1.0,0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    Sphere * redS = new Sphere(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    position = vec3(0.5, -1.0, -1.0);
    scale = vec3(1.0f);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(0.0, 1.0, 0.0);
    specular =  vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1, 0.1, 0.1);
    exp = 100.0f;
    Sphere * greenS = new Sphere(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    position = vec3(0.0, 1.0, 0.0);
    scale = vec3(1.0f);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(0.0, 0.0, 1.0);
    specular =  vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1, 0.1, 0.1);
    exp = 100.0f;
    Sphere * blueS = new Sphere(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    scene.addShape(redS);
    scene.addShape(greenS);
    scene.addShape(blueS);
    
    camera->rayTrace(scene);
}

void task3(){
    // make a scene
    Scene scene;
    
    // make the light
    Light l1(vec3(1.0, 2.0, 2.0), 0.5);
    Light l2(vec3(-1.0, 2.0, -1.0), 0.5);
    scene.addLight(l1);
    scene.addLight(l2);
    
    // create the scene with the specifictations
    vec3 position, rotation, diffuse, specular, ambient, scale;
    float exp, angle;
    
    // red ellipsoid
    position = vec3(0.5, 0.0, 0.5);
    scale = vec3(0.5, 0.6, 0.2);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(1.0, 0.0, 0.0);
    specular =  vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    
    Ellipsoid * redE = new Ellipsoid(position,scale,rotation,angle,diffuse,specular,ambient,exp);
    scene.addShape(redE);
    
    // green sphere
    position = vec3(-0.5, 0.0, -0.5);
    scale = vec3(1.0, 1.0, 1.0);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(0.0, 1.0, 0.0);
    specular =  vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    
    Sphere * greenS = new Sphere(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    scene.addShape(greenS);
    
    // plane
    position = vec3(0.0, -1.0, 0.0);
    scale = vec3(1.0, 1.0, 1.0);
    rotation = vec3(0,1.0,0);
    angle = 90;
    diffuse = vec3(1.0, 1.0, 1.0);
    specular =  vec3(0.0, 0.0, 0.0);
    ambient = vec3(0.1, 0.1, 0.1);
    exp = 0.0f;
    
    Plane * plane = new Plane(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    
    scene.addShape(plane);
    
    camera->rayTrace(scene);
}

void task4(){
    Scene scene;
    
    Light l1(vec3(-1.0, 2.0, 1.0), 0.5);
    Light l2(vec3(0.5, -0.5, 0.0), 0.5);
    
    scene.addLight(l1);
    scene.addLight(l2);
    
    vec3 position, rotation, diffuse, specular, ambient, scale;
    float exp, angle;
    
    //red sphere
    position = vec3(0.5, -0.7, 0.5);
    scale = vec3(0.3, 0.3, 0.3);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(1.0, 0.0, 0.0);
    specular =  vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    
    Sphere* redS =  new Sphere(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    scene.addShape(redS);
    
    // blue sphere
    position = vec3(1.0, -0.7, 0.0);
    scale = vec3(0.3, 0.3, 0.3);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(0.0, 0.0, 1.0);
    specular =  vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    Sphere* blueS =  new Sphere(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    scene.addShape(blueS);
    
    // floor
    position = vec3(0.0, -1.0, 0.0);
    scale = vec3(1.0);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(1.0, 1.0, 1.0);
    specular =  vec3(0.0, 0.0, 0.0);
    ambient = vec3(0.1);
    exp = 0.0f;
    Plane* floor =  new Plane(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    scene.addShape(floor);
    
    // wall
    position = vec3(0.0, 0.0, -3.0);
    scale = vec3(1.0);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(1.0, 1.0, 1.0);
    specular =  vec3(0.0, 0.0, 0.0);
    ambient = vec3(0.1);
    exp = 0.0f;
    Plane* wall =  new Plane(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    scene.addShape(wall);
    
    //reflective sphere1
    position = vec3(-0.5, 0.0, -0.5);
    scale = vec3(1.0);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(0);
    specular =  vec3(0);
    ambient = vec3(0);
    exp = 0.0f;
    Sphere* reS1 =  new Sphere(position,scale,rotation,angle, diffuse,specular,ambient,exp, true);
    scene.addShape(reS1);
    
    //reflective sphere2
    position = vec3(1.5, 0.0, -1.5);
    scale = vec3(1.0);
    rotation = vec3(1.0);
    angle = 0;
    diffuse = vec3(0);
    specular =  vec3(0);
    ambient = vec3(0);
    exp = 0.0f;
    Sphere* reS2 =  new Sphere(position,scale,rotation,angle, diffuse,specular,ambient,exp, true);
    scene.addShape(reS2);
    
    
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
    
    //task1();
    //task3();
    task4();
    
    image->writeToFile(filename);
    
	return 0;
}
