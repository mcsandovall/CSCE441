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

// make normal
typedef struct normal{
    double x, y , z;
    
    normal() : x(0), y(0), z(0) {}
    normal(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
    void scaleNormal(double Scale){
        x *= Scale, y *= Scale, z *= Scale;
    }
    double operator*(const normal n2){ // dot product
        return (x * n2.x) + (y * n2.y) + (z * n2.z);
    }
} normal;

struct Point_t {
    double x, y , z; // is in three dimensions
    double r, g, b; // define the colors
    normal nml;
    
    // constructor
    Point_t() : x(0), y(0), z(0), r(0), g(0), b(0) {}
    Point_t(double _x, double _y, double _z) : x(_x), y(_y), z(_z), r(0), g(0), b(0) {}
    
    void setColors(unsigned char _r, unsigned char _g, unsigned char _b){
        r = _r, g = _g, b = _b;
    }
    void setNormal(double _x, double _y, double _z){
        nml.x = _x, nml.y = _y, nml.z = _z;
    }
    
    Point_t operator -(const Point_t _v2) const{
        return {x - _v2.x, y - _v2.y, z - _v2.z};
    }
    
    Point_t operator +(const Point_t _v2) const{
        return {x + _v2.x, y + _v2.y, z + _v2.z };
    }
    
    Point_t operator *(const double number) const{
        return {x * number, y * number, z * number};
    }
    
    Point_t operator *(const Point_t _v2) const{
        return {x * _v2.x, y * _v2.y, z * _v2.z};
    }
    
    Point_t crossProduct(Point_t _v2){
        return {
                       ((y * _v2.z) - (z * _v2.z)),
                       -((x * _v2.z) - (z * _v2.x)),
                       ((x * _v2.y) - (y * _v2.x)) };
    }
    
    void set_positions(double _x, double _y, double _z){
        x = _x, y = _y, z = _z;
    }
    
    double lenght(){
        return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    }
    
    double dotProduct(Point_t _v2){
        return (x * _v2.x) + (y * _v2.y) + (z * _v2.z);
    }
    
    void set_toImage(double scalar, double * translate){
        x = (scalar * x) + translate[0];
        y = (scalar * y) + translate[1];
    }
};

struct BoundedBox_t{
    double xmin, xmax;
    double ymin, ymax;
    double zmin, zmax;
    
    // constructor
    BoundedBox_t() : xmin(INT_MAX), xmax(INT_MIN), ymin(INT_MAX), ymax(INT_MIN), zmin(INT_MAX), zmax(INT_MIN) {}
    BoundedBox_t(const Point_t * vtx){
        xmin = INT_MAX, ymin = INT_MAX, zmin = INT_MAX;
        xmax = INT_MIN, ymax = INT_MIN, zmax = INT_MIN;
        for(int i = 0; i < 3; ++i){
            if(vtx[i].x < xmin) xmin = vtx[i].x;
            if(vtx[i].x > xmax) xmax = vtx[i].x;
            
            if(vtx[i].y < ymin) ymin = vtx[i].y;
            if(vtx[i].y > ymax) ymax = vtx[i].y;
            
            if(vtx[i].z < zmin) zmin = vtx[i].z;
            if(vtx[i].z > zmax) zmax = vtx[i].z;
        }
    }
    
    void draw_2D(std::shared_ptr<Image> image, unsigned char r, unsigned char g, unsigned char b){
        for(int x = xmin; x <= xmax; ++x){
            for(int y = ymin; y <= ymax; ++y){
                image->setPixel(x, y, r, g, b);
            }
        }
    }
    
    void set_extremas(const Point_t * v){
        if(v->x < xmin) xmin = v->x;
        if(v->x > xmax) xmax = v->x;
        
        if(v->y < ymin) ymin = v->y;
        if(v->y > ymax) ymax = v->y;
        
        if(v->z < zmin) zmin = v->z;
        if(v->z > zmax) zmax = v->z;
    }
};

// Triangle DS
struct Triangle_t{
    Point_t * vertex;
    float area;
    float u, v, w; // the bycentric coordinates at that point
    BoundedBox_t * bb;
    
    // constructors
    Triangle_t() : area(0) {}
    Triangle_t(Point_t _v1, Point_t _v2, Point_t _v3){
        vertex = new Point_t[3];
        vertex[0] = _v1, vertex[1] = _v2, vertex[2] = _v3;
        area = 0.5 * ((vertex[1].x - vertex[0].x) * (vertex[2].y - vertex[0].y) - (vertex[2].x - vertex[0].x) * (vertex[1].y - vertex[0].y));
        //area = 0.5 * (((_v2 - _v1)).crossProduct((_v3 - _v1))).lenght(); area in 3D
        bb = new BoundedBox_t(vertex);
    }
    
    // calculate the bycentric coordinates
    bool inTrinagle(const Point_t * P){
        Triangle_t PBC (*P, vertex[1], vertex[2]);
        Triangle_t PCA (*P, vertex[2], vertex[0]);
        Triangle_t PAB (*P, vertex[0], vertex[1]);
        
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
    
};

// Z buffer
class ZBuffer{
private:
    int width, height;
    float ** pbuffer;
public:
    ZBuffer(int _w, int _h){
        width = _w, height = _h;
        pbuffer = new float*[width+1];
        for(int i = 0; i <= width; ++i){
            pbuffer[i] = new float[height+1];
        }
    }
    ~ZBuffer(){
        for(int i = 0; i <= width; ++i){
            delete[] pbuffer[i];
        }
        delete[] pbuffer;
    }
    ZBuffer( const ZBuffer& ) = delete;
    ZBuffer& operator=( const ZBuffer& ) = delete;
    void Clear(){
        for(int x = 0; x <= width; ++x){
            for(int y = 0; y <= height; ++y){
                pbuffer[x][y] = -1 * std::numeric_limits<float>::infinity();
            }
        }
    }
    float& At(int x, int y){
        return pbuffer[x][y];
    }
    bool TestAndSet(int x, int y, float Depth){
        float & depthInBuffer = At(x, y);
        if(Depth > depthInBuffer){
            depthInBuffer = Depth;
            return true;
        }
        return false;
    }
};

// get color function for blending the colors of the bycentric coordinates
double * getColor(const Point_t * P, const Triangle_t * T){
    double * color = new double[3];
    color[0] = floor(T->vertex[0].r * T->u + T->vertex[1].r * T->v + T->vertex[2].r * T->w);
    color[1] = floor(T->vertex[0].g * T->u + T->vertex[1].g * T->v + T->vertex[2].g * T->w);
    color[2] = floor(T->vertex[0].b * T->u + T->vertex[1].b * T->v + T->vertex[2].b * T->w);
    return color;
}

// return the scalar factor for the image
double scalar_factor(int width, int height,const BoundedBox_t * bb){
    // assume the image always begins at (0,0) -> (w,h)
    double s_x = width / (bb->xmax - bb->xmin);
    double s_y = height / (bb->ymax - bb->ymin);
    return min(s_x,s_y);
}

double * translation_factor(int width, int height, double scalar, const BoundedBox_t * bb){
    // get the translation factor for box x and y
    double * t_x_y = new double[2];
    t_x_y[0] = (0.5 * width) - (scalar * ( 0.5 * (bb->xmax + bb->xmin)));
    t_x_y[1] = (0.5 * height) - (scalar * ( 0.5 * (bb->ymax + bb->ymin)));
    return t_x_y;
}

// task 1 draw all the bounded boxes
void draw_BoundingBoxes(vector<Triangle_t *> triangles, std::shared_ptr<Image> image){
    for(int t = 0; t < triangles.size(); ++t){
        triangles[t]->bb->draw_2D(image, RANDOM_COLORS[t%7][0] * 255, RANDOM_COLORS[t%7][1] * 255, RANDOM_COLORS[t%7][2] * 255);
    }
}

// task 2
void draw_triangles(vector<Triangle_t *> triangles, std::shared_ptr<Image> image){
    Point_t * P = new Point_t();
    for(int t= 0; t < triangles.size();++t){
        for(int x = triangles[t]->bb->xmin; x <= triangles[t]->bb->xmax; ++x){
            for(int y = triangles[t]->bb->ymin; y<= triangles[t]->bb->ymax; ++y){
                P->set_positions(x, y, 0);
                if(triangles[t]->inTrinagle(P)){
                    image->setPixel(x, y, RANDOM_COLORS[t%7][0]*255, RANDOM_COLORS[t%7][1]*255, RANDOM_COLORS[t%7][2]*255);
                }
            }
        }
    }
    delete P;
}

// task 3
void per_vertex_color(vector<Triangle_t *> triangles, std::shared_ptr<Image> image){
    Point_t *  P = new Point_t();
    for(auto T : triangles){
        for(int x = T->bb->xmin; x<= T->bb->xmax; ++x){
            for(int y = T->bb->ymin; y<= T->bb->ymax; ++y){
                P->set_positions(x, y, 0);
                if(T->inTrinagle(P)){
                    double * c = getColor(P, T);
                    image->setPixel(x, y, c[0], c[1], c[2]);
                    delete[] c;
                }
            }
        }
    }
    delete P;
}

// linear interpolation between a line
double * linar_interpolation(const Point_t * P, const Point_t * A, const Point_t * B){
    double * color = new double[3];
    // get alpha
    double a = (*P - *A).lenght() / ( (*A * -1.0) + *B).lenght();
    color[0] = floor((1 - a) * A->r + a * B->r);
    color[1] = floor((1 - a) * A->g + a * B->g);
    color[2] = floor((1 - a) * A->b + a * B->b);
    return color;
}

// task 4
void vertical_color(vector<Triangle_t *> triangles, std::shared_ptr<Image> image, BoundedBox_t * bb){
    Point_t * Ymax = new Point_t(0, bb->ymax, 0);
    Ymax->setColors(255, 0, 0);
    Point_t * Ymin = new Point_t(0, bb->ymin, 0);
    Ymin->setColors(0, 0, 255);
    Point_t * P = new Point_t();
    
    for(auto T : triangles){
        for(int y = T->bb->ymin; y<= T->bb->ymax; ++y){
            for(int x = T->bb->xmin; x <= T->bb->xmax; ++x){
                P->set_positions(x, y, 0);
                if(T->inTrinagle(P)){
                    P->set_positions(0, y, 0); // only care abot the y here
                    double * c = linar_interpolation(P, Ymax, Ymin);
                    image->setPixel(x, y, c[0], c[1], c[2]);
                    delete[] c;
                }
            }
        }
    }
    delete Ymax; delete Ymin; delete P;
}

//
float calculateDepth(Triangle_t * T){
    return ((T->u * T->vertex[0].z) + (T->v * T->vertex[1].z) + (T->w * T->vertex[2].z));
}

// task 5 z buffers
void Z_buffer(vector<Triangle_t *> triangles, std::shared_ptr<Image> image, BoundedBox_t &bb){
    // task 5
    ZBuffer zbuff(bb.xmax, bb.ymax);
    zbuff.Clear();
    
    // make the gradient for linear interpolation like before
    Point_t * Zmax = new Point_t(0, 0, bb.zmax);
    Zmax->setColors(255, 0, 0);
    Point_t * Zmin = new Point_t(0, 0, bb.zmin);
    Zmin->setColors(0, 0, 0);
    Point_t * P = new Point_t();
    float z;
    
    for(auto T : triangles){
        for(int y = T->bb->ymin; y<= T->bb->ymax; ++y){
            for(int x = T->bb->xmin; x <= T->bb->xmax; ++x){
                P->set_positions(x, y, 0);
                if(T->inTrinagle(P)){
                    z = calculateDepth(T);
                    if(zbuff.TestAndSet(x, y, z)){
                        P->set_positions(0, 0, z); // only care abot z
                        double * c = linar_interpolation(P, Zmax, Zmin);
                        image->setPixel(x, y, c[0], c[1], c[2]);
                        delete[] c;
                    }
                }
            }
        }
    }
    delete Zmax; delete Zmin; delete P;
}

normal getNormalColor(Triangle_t * T){
    return {
      (T->vertex[0].nml.x * T->u + T->vertex[1].nml.x * T->v + T->vertex[2].nml.x * T->w),
      (T->vertex[0].nml.y * T->u + T->vertex[1].nml.y * T->v + T->vertex[2].nml.y * T->w),
      (T->vertex[0].nml.z * T->u + T->vertex[1].nml.z * T->v + T->vertex[2].nml.z * T->w)
    };
}

// task 6 normal coloring
void normalColoring(vector<Triangle_t *> triangles, std::shared_ptr<Image> image, BoundedBox_t &bb){
    ZBuffer zbuff(bb.xmax, bb.ymax);
    zbuff.Clear();

    Point_t * P = new Point_t();
    float z; normal n;

    for(auto T : triangles){
        for(int y = T->bb->ymin; y<= T->bb->ymax; ++y){
            for(int x = T->bb->xmin; x <= T->bb->xmax; ++x){
                P->set_positions(x, y, 0);
                if(T->inTrinagle(P)){
                    z = calculateDepth(T);
                    if(zbuff.TestAndSet(x, y, z)){
                        n = getNormalColor(T);
                        image->setPixel(x, y, 255 * (0.5 * n.x + 0.5), 255 * (0.5 * n.y + 0.5), 255 * (0.5 * n.z + 0.5));
                    }
                }
            }
        }
    }
    delete P;
}

// void task 7 simple lighting
void SimpleLight(vector<Triangle_t *> triangles, std::shared_ptr<Image> image, BoundedBox_t &bb){
    ZBuffer zbuff(bb.xmax, bb.ymax);
    zbuff.Clear();

    Point_t * P = new Point_t(); float z; normal n; normal light(1,1,1);
    light.scaleNormal(1/sqrt(3));
    for(Triangle_t * T : triangles){
        for(int x = T->bb->xmin; x <= T->bb->xmax; ++x){
            for(int y = T->bb->ymin; y <= T->bb->ymax; ++y){
                P->set_positions(x, y, 0);
                if(T->inTrinagle(P)){
                    z = calculateDepth(T);
                    if(zbuff.TestAndSet(x, y, z)){
                        n =  getNormalColor(T);
                        double c = max((light * n),0.0);
                        image->setPixel(x, y, c*255, c*255, c*255);
                    }
                }
            }
        }
    }
}
int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A1 meshfile" << endl;
		return 0;
	}
	string meshName(argv[1]);
    // Output filename
    string filename(argv[2]);
    // Image width
    int width = atoi(argv[3]);
    // image height
    int height = atoi(argv[4]);
    // task to be executed
    //int task = atoi(argv[5]);
    // Create the image like in the labs
    auto image = make_shared<Image>(width, height);

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
    vector<Point_t *> points;
    // make the bounding box for the shape
    BoundedBox_t bb;
    for(int p = 0; p < posBuf.size(); p+= 3){
        points.push_back(new Point_t(posBuf[p], posBuf[p+1], posBuf[p+2]));
        points[points.size()-1]->setNormal(norBuf[p], norBuf[p+1], norBuf[p+2]);
        // set the extremas for the bouding box ---
        bb.set_extremas(points[points.size()-1]);
    }
    
    // add the colors to the vectors for task 3
    for(int p =0; p < points.size(); ++p){
        points[p]->setColors(RANDOM_COLORS[p%7][0]*255, RANDOM_COLORS[p%7][1]*255, RANDOM_COLORS[p%7][2]*255);
    }
    
    // compute the scalar and the translation and apply to all the points
    double scalar = scalar_factor(width, height, &bb);
    double * translation = translation_factor(width, height, scalar, &bb);
    
    // apply the scalar to all the points in the shape and create the triangles
    vector<Triangle_t *> triangles;
    for(int p = 0; p < points.size(); p += 3){
        points[p]->set_toImage(scalar, translation);
        points[p+1]->set_toImage(scalar, translation);
        points[p+2]->set_toImage(scalar, translation);

        triangles.push_back(new Triangle_t(*(points[p]), *(points[p+1]), *(points[p+2])));
    }
    // resize the bounded box
    bb.xmin = (scalar * bb.xmin) + translation[0], bb.xmax = (scalar * bb.xmax) + translation[0];
    bb.ymin = (scalar * bb.ymin) + translation[1], bb.ymax = (scalar * bb.ymax) + translation[1];
    
    // make a switch for each test case to be tested
    switch (atoi(argv[5])) {
        case 1:
            draw_BoundingBoxes(triangles, image);
            break;
        case 2:
            draw_triangles(triangles, image);
            break;
        case 3:
            per_vertex_color(triangles, image);
            break;
        case 4:
            vertical_color(triangles, image, &bb);
            break;
        case 5:
            Z_buffer(triangles, image, bb);
            break;
        case 6:
            normalColoring(triangles, image, bb);
            break;
        case 7:
            SimpleLight(triangles, image, bb);
            break;
        default:
            break;
    }
    
    image->writeToFile(filename);
    
	return 0;
}
