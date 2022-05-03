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
#include "Shape.h"
#include "Scene.h"
#include "Camera.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;
using namespace glm;

#define MAX_DEPTH 6

shared_ptr<Image> image;
shared_ptr<Camera> camera;

// testing function
void printVec(const vec3 &v){
    cout << v.x << " " << v.y << " " << v.z << endl;
}

class Sphere : public Shape{
public:
    Sphere(const vec3 &p, const vec3 &s, const vec3 &r,const float &ang, const vec3 &d, const vec3 &sp, const vec3 &a, const float &e, const bool &ref = false){
        Position = p; Scale = s; rotation =  r; Diffuse = d; Specular = sp; Ambient = a; Exponent = e, angle = ang; reflective = ref;
    }
    
    float Intersect(const Ray &r, const float &t0, const float &t1) override{
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
    
    void computeHit(const Ray &r, const float &t, Hit &h) override{
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
    
    float Intersect(const Ray &r,const float &t0,const float &t1) override{
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
    
    void computeHit(const Ray &r, const float &t, Hit &h) override{
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
    
    float Intersect(const Ray &r,const float &t0,const float &t1) override{
        return  dot(normal, (Position - r.origin))/dot(normal,r.direction);
    }
    
    void computeHit(const Ray &r, const float &t, Hit &h) override{
        h.x =  r.origin +  (t * r.direction);
        h.n = normal;
        h.t = 0;
    }
    vec3 normal;
};



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
    
    camera->rayTrace(scene, image);
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
    
    camera->rayTrace(scene,image);
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

    camera->rayTrace(scene, image);
}

// copy the code from assigment 1

class Point{
public:
    vec3 position;
    vec3 normal;
    
    // constructor
    Point() : position(0) {}
    Point(const vec3 &v, const vec3 &n) : position(v), normal(n) {}
    
    void setNormal(const vec3 &n){ normal = n;}
    
    void set_positions(const vec3 &p){ position = p;}
    
    void set_toImage(const float &scalar, const vec2 &translate){
        position.x = (scalar * position.x) + translate.x;
        position.y = (scalar * position.y) + translate.y;
    }
};

class BoundedBox{
public:
    vec3 min, max;
    vec3 center;
    float radius;
    
    // constructor
    BoundedBox() : min(INT_MAX), max(INT_MIN) {}
    BoundedBox(const vector<Point> &vtx){
        for(int i = 0; i < 3; ++i){
            set_extremas(vtx[i].position);
        }
    }
    
    void set_extremas(const vec3 &v){
        min.x = std::min(min.x,v.x), min.y = std::min(min.y,v.y), min.z = std::min(min.z,v.z);
        max.x = std::max(max.x,v.x), max.y = std::max(max.y,v.y), max.z = std::max(max.z,v.z);
        sphere();
    }
    
    void sphere(){
        center = (max + min) / 2.0f;
        radius = distance(center, max);
    }
    
    bool inside(const Ray &r){
        vec3 pc =  r.origin - center; // ray center - sphere position vec(0,0,4)
        float a = dot(r.direction, r.direction);
        float b = 2*(dot(r.direction,pc)); // 2 * dot(v^, pc)
        float c = dot(pc,pc) - (radius* radius); // dot(pc,pc) - r^2
        float d = (b*b) - (4 * a * c);
        return (d > 0);
    }
};

class Triangle : public Shape{
public:
    vector<Point> vertex;
    float u, v, w; // the bycentric coordinates at that point
    BoundedBox * bb;
    
    // constructors
    Triangle(){}
    Triangle(const Point &_v1,const Point &_v2,const Point &_v3){
        vertex.push_back(_v1), vertex.push_back(_v2), vertex.push_back(_v3);
        bb = new BoundedBox(vertex);
    }
    
    float Intersect(const Ray &r, const float &t0, const float &t1) override{
       vec3 edge1, edge2, tvec, pvec, qvec;
       float det,inv_det;

       /* find vectors for two edges sharing vert0 */
        edge1 = vertex[1].position - vertex[0].position;
        edge2 = vertex[2].position - vertex[0].position;
       /* begin calculating determinant - also used to calculate U parameter */
        pvec = cross(r.direction, edge2);

       /* if determinant is near zero, ray lies in plane of triangle */
        det = dot(edge1, pvec);//DOT(edge1, pvec);

       if (det > -0.0001f && det < 0.0001f) return INT_MAX;
       inv_det = 1.0 / det;

       /* calculate distance from vert0 to ray origin */
        tvec = r.origin - vertex[0].position;

       /* calculate U parameter and test bounds */
        u = dot(tvec, pvec) * inv_det; //DOT(tvec, pvec) * inv_det;
       if (u < 0.0 || u > 1.0) return INT_MAX;

       /* prepare to test V parameter */
        qvec = cross(tvec, edge1);

       /* calculate V parameter and test bounds */
        v = dot(r.direction, qvec) * inv_det;//DOT(dir, qvec) * inv_det;
       if (v < 0.0 || u + v > 1.0f) return INT_MAX;
        w = 1.0f - (u + v);
       /* calculate t, ray intersects triangle */
        float t  = dot(edge2, qvec) * inv_det;
        if(t > t0 && t < t1) return t;
       return INT_MAX; //DOT(edge2, qvec) * inv_det;
    }
    
    vec3 getNormal(){
        vec3 n(0);
        n.x = w * vertex[0].normal.x + u * vertex[1].normal.x + v * vertex[2].normal.x;
        n.y = w * vertex[0].normal.y + u * vertex[1].normal.y + v * vertex[2].normal.y;
        n.z = w * vertex[0].normal.z + u * vertex[1].normal.z + v * vertex[2].normal.z;
        n.x = std::clamp(n.x, -1.0f, 1.0f);
        n.y = std::clamp(n.y, -1.0f, 1.0f);
        n.z = std::clamp(n.z, -1.0f, 1.0f);
        return n;
    }
    
    void computeHit(const Ray &r, const float &t, Hit &h) override{
        h.x = r.origin + (t * r.direction);
        h.n = getNormal();
        h.t = t;
    }
};

class Object : public Shape{
public:
    mat4 mat_E;
    Hit thit;
    bool transform;
    Object(const vec3 &d, const vec3 &s, const vec3 &a, const float &e){
        Diffuse = d, Specular = s, Ambient = a, Exponent = e;
        transform = false;
    }
    Object(const vec3 &p, const vec3 &s, const vec3 &r,const float &angle, const vec3 &d, const vec3 &sp, const vec3 &a, const float &e)
    {
        // load the matrix E = T * R * S
        mat_E = mat4(1.0); // load identity
        mat_E *= glm::translate(mat4(1.0), p);
        mat_E *= glm::rotate(mat4(1.0), angle, r);
        mat_E *= glm::scale(mat4(1.0), s);
        Diffuse = d;
        Specular = sp;
        Ambient = a;
        Exponent = e;
        transform = true;
    }
    vector<Triangle*> triangles;
    Triangle * triangle;
    BoundedBox * bb;
    Object(){}
    
    float Intersect(const Ray &r, const float &t0, const float &t1) override{
        float t = INT_MAX;
        float tp = INT_MAX;
        Ray rp(r.origin,r.direction);
        vec3 p,v,x,n;
        if(transform){
            p = inverse(mat_E) * vec4(r.origin,1.0);
            v = inverse(mat_E) * vec4(r.direction,0.0);
            v = normalize(v);
            rp.origin = p;
            rp.direction = v;
        }
        if(bb->inside(rp)){
            for(Triangle *T : triangles){
                if((tp = T->Intersect(rp, t0, t1)) != INT_MAX){
                    if(transform){
                        x = p + (tp * v);
                        x = mat_E * vec4(x,1.0f);
                        n = T->getNormal();
                        n = inverse(transpose(mat_E)) * vec4(n,0.0);
                        n = normalize(n);
                        tp = abs(distance(x, r.origin));
                        if(dot(v, x - r.origin) < 0){
                            tp = -tp;
                        }
                    }
                    if(tp < t){
                        t = tp;
                        triangle = T;
                        if(transform){
                            thit.x = x;
                            thit.n = n;
                            thit.t = t;
                        }
                    }
                }
            }
        }
        return t;
    }
    
    void computeHit(const Ray &r, const float &t, Hit &h) override{
        if(!triangle) return;
        if(transform){
            h.x = thit.x;
            h.n = thit.n;
            h.t = thit.t;
        }else{
            triangle->computeHit(r, t, h);
        }
    }
    
    void loadTriangles(const string &meshName, const int &width, const int &height){
        // Load geometry
        vector<float> posBuf; // list of vertex positions
        vector<float> norBuf; // list of vertex normals
        vector<float> texBuf; // list of vertex texture coords
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        string errStr;
        bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &errStr, meshName.c_str());
        if(!rc) {
            cerr << errStr << endl;
        } else {
            // Some OBJ files have different indices for vertex positions, normals,
            // and texture coordinates. For example, a cube corner vertex may have
            // three different normals. Here, we are going to duplicate all such
            // vertices.
            // Loop over shapes
            for(size_t s = 0; s < shapes.size(); s++) {
                // Loop over faces (polygons)
                size_t index_offset = 0;
                for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                    size_t fv = shapes[s].mesh.num_face_vertices[f];
                    // Loop over vertices in the face.
                    for(size_t v = 0; v < fv; v++) {
                        // access to vertex
                        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                        posBuf.push_back(attrib.vertices[3*idx.vertex_index+0]);
                        posBuf.push_back(attrib.vertices[3*idx.vertex_index+1]);
                        posBuf.push_back(attrib.vertices[3*idx.vertex_index+2]);
                        if(!attrib.normals.empty()) {
                            norBuf.push_back(attrib.normals[3*idx.normal_index+0]);
                            norBuf.push_back(attrib.normals[3*idx.normal_index+1]);
                            norBuf.push_back(attrib.normals[3*idx.normal_index+2]);
                        }
                        if(!attrib.texcoords.empty()) {
                            texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
                            texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
                        }
                    }
                    index_offset += fv;
                    // per-face material (IGNORE)
                    shapes[s].mesh.material_ids[f];
                }
            }
        }
        cout << "Number of vertices: " << posBuf.size()/3 << endl;
        
        
        // construct the points in 3D
        vector<Point> points;
        // make the bounding box for the shape
        bb = new BoundedBox();
        for(int p = 0; p < posBuf.size(); p+= 3){
            vec3 pos(posBuf[p], posBuf[p+1], posBuf[p+2]);
            vec3 n(norBuf[p], norBuf[p+1], norBuf[p+2]);
            points.push_back(Point(pos,n));
            // set the extremas for the bouding box ---
            bb->set_extremas(points[points.size()-1].position);
        }
        
        for(int p = 0; p < points.size(); p += 3){
            triangles.push_back(new Triangle(points[p], points[p+1], points[p+2]));
        }
    }
};

// make the function for task 6
void task6(const string &meshName, const int &width, const int &height){
    // make the scene
    Scene scene;
    // make the light
    Light l(vec3(-1.0, 1.0, 1.0), 1.0f);
    scene.addLight(l);
    // make the object
    vec3 position, rotation, diffuse, specular, ambient, scale;
    float exp, angle;
    
    position = vec3(0);
    rotation = vec3(0);
    scale = vec3(1.0);
    angle = 0.0;
    diffuse = vec3(0.0, 0.0, 1.0);
    specular = vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    
    Object * bunny = new Object(diffuse, specular, ambient, exp);
    // load the triangles onto the object
    bunny->loadTriangles(meshName, width, height);
    cout << "load done..." << endl;
    scene.addShape(bunny);
    camera->rayTrace(scene, image);
}

// make the function for task 7
void task7(const string &meshName, const int &width, const int &height){
    // make the scene
    Scene scene;
    // make the light
    Light l(vec3(1.0, 1.0, 2.0), 1.0f);
    scene.addLight(l);
    // make the object
    vec3 position, rotation, diffuse, specular, ambient, scale;
    float exp, angle;
    
    position = vec3(0.3, -1.5, 0.0);
    rotation = vec3(1.0,0.0,0.0);
    scale = vec3(1.5);
    angle = (float) (20.0f*M_PI/180);
    diffuse = vec3(0.0, 0.0, 1.0);
    specular = vec3(1.0, 1.0, 0.5);
    ambient = vec3(0.1);
    exp = 100.0f;
    
    Object * bunny = new Object(position,scale,rotation,angle, diffuse,specular,ambient,exp);
    // load the triangles onto the object
    bunny->loadTriangles(meshName, width, height);
    cout << "load done..." << endl;
    scene.addShape(bunny);
    camera->rayTrace(scene, image);
}

void task8(){
    camera->fov = (float)(60*M_PI/180.0);
    camera->Postion = vec3(-3.0,0,0);
    camera->scene8 = true;
    
    task1();
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		cout << "Error: A6 Not Enough Arguments" << endl;
		return 0;
	}
    
    int sceneNumber = atoi(argv[1]);
    
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
    
    if(sceneNumber == 1 || sceneNumber == 2){
        task1();
    }
    else if(sceneNumber == 3){
        task3();
    }
    else if(sceneNumber == 4 || sceneNumber == 5){
        task4();
    }
    else if(sceneNumber == 6){
        task6(meshName, width, height);
    }
    else if(sceneNumber == 7){
        task7(meshName, width, height);
    }
    else if (sceneNumber == 8){
        task8();
    }
    
    image->writeToFile(filename);
    
	return 0;
}
