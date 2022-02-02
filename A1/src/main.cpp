#include <iostream>
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Image.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;

double RANDOM_COLORS[7][3] = {
	{0.0000,    0.4470,    0.7410},
	{0.8500,    0.3250,    0.0980},
	{0.9290,    0.6940,    0.1250},
	{0.4940,    0.1840,    0.5560},
	{0.4660,    0.6740,    0.1880},
	{0.3010,    0.7450,    0.9330},
	{0.6350,    0.0780,    0.1840},
};
// bring back all the datastructures from the labs
struct Point_t {
    int x, y , z; // is in three dimensions
    unsigned char r, g, b; // define the colors
    
    // constructor
    Point_t() : x(0), y(0), z(0) {}
    Point_t(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
    
    void setColors(unsigned char _r, unsigned char _g, unsigned char _b){
        r = _r, g = _g, b = _b;
    }
    
    Point_t operator -(Point_t _v2){
        return {x - _v2.x, y - _v2.y, z - _v2.z};
    }
    
    Point_t crossProduct(Point_t _v2){
        return {
                       ((y * _v2.z) - (z * _v2.z)),
                       -((x * _v2.z) - (z * _v2.x)),
                       ((x * _v2.y) - (y * _v2.x)) };
    }
    
    float lenght(){
        return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    }
    
    float dotProduct(Point_t _v2){
        return (x * _v2.x) + (y * _v2.y) + (z * _v2.z);
    }
};

// Triangle DS
struct Triangle_t{
    Point_t vertex[3];
    float area;
    float u, v, w; // the bycentric coordinates at that point
    
    // constructors
    Triangle_t() : area(0) {}
    Triangle_t(Point_t _v1, Point_t _v2, Point_t _v3){
        vertex[0] = _v1, vertex[1] = _v2, vertex[2] = _v3;
        area = 0.5 * (((_v2 - _v1)).crossProduct((_v3 - _v1))).lenght();
    }
    
    // calculate the bycentric coordinates
    bool inTrinagle(Point_t P){
        // get the vertex PB x PC / area
        Triangle_t PBC (P, vertex[1], vertex[2]);
        Triangle_t PCA (P, vertex[2], vertex[0]);
        Triangle_t PAB (P, vertex[0], vertex[1]);
        
        u = PBC.area / area;
        v = PCA.area / area;
        w = PAB.area / area;
        
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
    
};

struct BoundedBox_t{
    int xmin, xmax;
    int ymin, ymax;
    int zmin, zmax;
    
    // constructor
    BoundedBox_t() : xmin(0), xmax(0), ymin(0), ymax(0), zmin(0), zmax(0) {}
    BoundedBox_t(const Triangle_t T){
        xmin = INT_MAX, ymin = INT_MAX, zmin = INT_MAX;
        xmax = INT_MIN, ymin = INT_MIN, zmax = INT_MIN;
        for(auto const v : T.vertex){
            if(v.x < xmin) xmin = v.x;
            if(v.y < ymin) ymin = v.y;
            if(v.z < zmin) zmin = v.z;
            
            if(v.x > xmax) xmax = v.x;
            if(v.y > ymax) ymax = v.y;
            if(v.z > zmax) zmax =  v.z;
        }
    }
};

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A1 meshfile" << endl;
		return 0;
	}
	string meshName(argv[1]);

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
	
	return 0;
}
