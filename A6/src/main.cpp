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

class Triangle{
public:
    Triangle(){}
    Triangle(const vec3 &v1, const vec3 &v2, const vec3 &v3){
        vertices.push_back(v1), vertices.push_back(v2),vertices.push_back(v3);
    }
    void setVertices(const vec3 &v1, const vec3 &v2, const vec3 &v3){
        vertices.push_back(v1), vertices.push_back(v2),vertices.push_back(v3);
    }
    void setNormals(const vec3 &n1, const vec3 &n2, const vec3 &n3){
        normals.push_back(n1), normals.push_back(n2), normals.push_back(n3);
    }
    
    void computeArea(){
        // compute the area of the triangle if it has all the vertices
        if(vertices.size() != 3) return;
        area = 0.5 * cross((vertices[1] - vertices[0]), (vertices[2] - vertices[0])).length();
    }
    
    bool inTriangle(const vec3 &P){
        Triangle PBC (P, vertices[1], vertices[2]);
        Triangle PCA (P, vertices[2], vertices[0]);
        Triangle PAB (P, vertices[0], vertices[1]);
        
        u = PBC.area / area;
        v = PCA.area / area;
        w = 1.0f - u - v;
        
        if(u < 0.0 || u > 1.0f){
            return false;
        }
        
        if(v < 0.0 || v > 1.0f){
            return false;
        }
        
        if(w < 0.0 || w > 1.0f)
        {
            return false;
        }
        
        return true;
    }
    
    vector<vec3> vertices;
    vector<vec3> normals;
    float area;
    float u,v,w;
};

// load the obj file function
vector<Triangle> loadTriangles(const string &meshName){
    vector<Triangle> triangles;
    vector<vec3> vertices;
    vector<vec3> normals;
    
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
    
    // add the points into a vector of vec3
    for(int i = 0; i < posBuf.size(); i+=3){
        vertices.push_back(vec3(posBuf[i], posBuf[i+1], posBuf[i+2]));
        normals.push_back(vec3(norBuf[i], norBuf[i+1], norBuf[i+2]));
    }
    
    // create triangles with the vertices and normals
    for(int i = 0; i < vertices.size(); i+=3){
        Triangle t;
        t.setVertices(vertices[i], vertices[i+1], vertices[i+2]);
        t.setNormals(normals[i], normals[i+1], normals[i+2]);
        t.computeArea();
        triangles.push_back(t);
    }
    return triangles;
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
    //loadTriangles(meshName);
    
    image->writeToFile(filename);
    
	return 0;
}
