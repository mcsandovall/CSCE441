#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include <vector>
#include <random>
#include "Shader.h"
#include "Texture.h"

using namespace std;


GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Shader> shader;

shared_ptr<Texture> floorTexture;
shared_ptr<Shape> floorShape;
shared_ptr<Program> floorProgram;

vector<float> posBuf;
vector<float> texBuf;
vector<unsigned int> indBuf;
map<string,GLuint> bufIDs;
int indCount;

/**
 Assigment 4 start code. Implement in different file once it works
 */

vector<int> object_t;
vector<float> scales;
vector<glm::vec3> colors;
vector<Light> lights;
vector<glm::vec3> lightsPos;
vector<glm::vec3> lightsColors;

class Object{
public:
    glm::vec3 Translate, Rotate, Scale;
    glm::mat4 Shear;
    double angle;
    shared_ptr<Shape> shape;
    Material material;
    float y_min, x_min;
    
    Object() : Translate(glm::vec3(0)), Rotate(glm::vec3(1.0)), Scale(glm::vec3(1.0)), Shear(glm::mat4(1.0f)), angle(1.0){}
    ~Object(){}
    Object(string object_name): Translate(glm::vec3(0)), Rotate(glm::vec3(1.0)), Scale(glm::vec3(1.0)), Shear(glm::mat4(1.0f)), angle(0) { // load the object into the shape
        shape = make_shared<Shape>();
        shape->loadMesh(RESOURCE_DIR + object_name);
        y_min = shape->min_y;
        // make the material with a random color
        glm::vec3 _ka, _kd, _ks;
        float s;
        // use the first material as a reference for the ambient light and the shine wanted for assigment 4
        _ka = glm::vec3(0.2,0.2,0.2);
        _kd = glm::vec3(0.8,0.7,0.7);
        _ks = glm::vec3(1.0,0.9,0.8);
        s = 200;
        material = Material(_ka,_kd,_ks,s);
        shape->init();
    }
    void draw_shape(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, vector<glm::vec3> lightsPost){
        MV->pushMatrix();
        
        for(int i = 0; i < lightsPost.size(); ++i){
            lightsPost[i] = glm::vec3(MV->topMatrix() * glm::vec4(lightsPost[i],1.0f));
        }
        
        MV->multMatrix(Shear);
        MV->translate(Translate);
        MV->scale(Scale);
        MV->rotate(angle, Rotate);
        MV->multMatrix(Shear);
        shader->bind(P, MV, material, lightsPost, lightsColors);
        shape->draw(shader->program);
        shader->program_unbind();
        MV->popMatrix();
    }
    void scale_obj(float x){
        Scale = glm::vec3(x);
        y_min = shape->min_y * x;
    }
};

void random_color(vector<glm::vec3> & colors){
    float r,g,b;
    for(int i = 0; i < 100; ++i){
        r = (float) rand() / (float) RAND_MAX;
        g = (float) rand() / (float) RAND_MAX;
        b = (float) rand() / (float) RAND_MAX;
        colors.push_back(glm::vec3(r,g,b));
    }
}

void random_obj(vector<int> & obj_t){
    for(int i = 0; i < 100; ++i){
        obj_t.push_back(rand() % 2);
    }
}

void random_scales(vector<float> &scales){
    for(int i = 0; i < 100; ++i){
        scales.push_back((float) rand() / (float) RAND_MAX);
    }
}

void createLights(vector<glm::vec3> &lightspos, vector<glm::vec3> &lightscolors, const int n = 10){
    default_random_engine gen;
    uniform_int_distribution<float> dist(-5.0f,5.0f);
    uniform_int_distribution<int> colr(0,99);
    for(int i = 0; i < n; ++i){
        lightspos.push_back(glm::vec3(dist(gen), dist(gen), dist(gen)));
        lightscolors.push_back(glm::vec3(colors[colr(gen)]));
    }
}

// class for the free look camera
class FreeLook{
public:
    glm::vec3 position, orientation = glm::vec3(0.0,0.0,-1.0f), up, right;
    const glm::vec3 worldup;
    float aspect,fovy,znear,zfar, speed;
    float yaw = 0, pitch = 0;
    glm::vec2 prevMouse;
    float angle = 45.0f;
    
    FreeLook(glm::vec3 _pos) :
    position(_pos),
    up(0.0,1.0f,0.0),
    worldup(0.0,1.0f,0.0),
    aspect(1.0),
    znear(0.1f),
    zfar(1000.0f),
    speed(0.2)
    {
        right = glm::normalize(glm::cross(orientation, worldup));
        fovy = ((float)(angle*M_PI/180.0));
    }
    
    void setAspect(float a){
        aspect = a;
    }
    
    void updateVectors(){
        glm::vec3 front;
        front.x = -glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
        front.y = glm::sin(glm::radians(pitch));
        front.z = -glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
        orientation = glm::normalize(front);
        right = glm::normalize(glm::cross(orientation, worldup));
        up = glm::normalize(glm::cross(right, orientation));
    }
    
    void processKeyboard(char key){
        // the speed a player can go is restricted by 0.5
        switch (key) {
            case 'w':
                position += 0.5f * glm::vec3(orientation.x, 0.0, orientation.z);
                break;
            case 's': // x and z values since we want to remain in same position regardles of camera view
                position -= 0.5f * glm::vec3(orientation.x, 0.0, orientation.z);
                break;
            case 'd':
                position += 0.5f * right;
                break;
            case 'a':
                position -= 0.5f * right;
                break;
            // zoom functionality
            case 'z':
                // zoom in
                angle -= 8.0f;
                if(angle < 4){
                    angle = 4;
                }
                fovy = ((float)(angle*M_PI/180.0));
                break;
            case 'Z':
                angle += 8.0f;
                if(angle > 114){
                    angle = 114;
                }
                fovy = ((float)(angle*M_PI/180.0));
                break;
            default:
                break;
        }
    }
    
    void mouseClicked(float x, float y){
        prevMouse.x = x;
        prevMouse.y = y;
    }
    
    void mouseMoved(float x, float y){
        glm::vec2 currMouse(x,y);
        glm::vec2 dv = currMouse - prevMouse;
        
        // the mouse clicked latency is 0.01f
        yaw -= 0.01f * dv.x;
        pitch -= 0.01f * dv.y;
        
        // restrictions on the pitch
        if(pitch > 60.0f){
            pitch = 60.0f;
        }else if (pitch < -60.0f){
            pitch = -60.0f;
        }
        
        updateVectors();
        
    }
    
    void applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const{
        P->multMatrix(glm::perspective(fovy, aspect, znear, zfar));
    }
    
    void applyViewMatrix(std::shared_ptr<MatrixStack> MV) const{
        MV->multMatrix(glm::lookAt(position, position + orientation, up));
    }
};

shared_ptr<FreeLook> FLCamera;

bool keyToggles[256] = {false}; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		//camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
        FLCamera->mouseClicked(xmouse, ymouse);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		//camera->mouseMoved((float)xmouse, (float)ymouse);
        FLCamera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

// This function is called keyboard letter is pressed
static void char_callback(GLFWwindow *window, unsigned int key)
{
    FLCamera->processKeyboard((char) key);
    keyToggles[key] = !keyToggles[key];
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
	}
}

void createLights(vector<Light> &lights){
    for(int x  = -5; x < 5; ++x){
        for(int z = -5; z < 5; ++z){
            glm::vec3 pos(x,1.0f,z);
            glm::vec3 color(colors[10 * (x+5) + (z+5)]);
            lights.push_back(Light(pos,color));
        }
    }
}

shared_ptr<Object> bunny,teapot, Floor, sun, Frustum;

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);
    
	//camera = make_shared<Camera>();
	//camera->setInitDistance(2.0f); // Camera's initial Z translation
    
    // initialize the free look camera
    FLCamera = make_shared<FreeLook>(glm::vec3(1.0f,0.5f,2.0f));
	
    shader = make_shared<Shader>(RESOURCE_DIR + "phong_vert.glsl",RESOURCE_DIR + "phong_frag.glsl");
    // load the object only once
    bunny = make_shared<Object>("bunny.obj");
    teapot = make_shared<Object>("teapot.obj");
    Floor = make_shared<Object>("cube.obj");
    sun = make_shared<Object>("sphere.obj");
    createLights(lights);
    GLSL::checkError(GET_FILE_LINE);
}

void drawScene(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, float t){
    // render the multiple lights
    sun->scale_obj(0.2);
    for(int i = 0; i < lightsPos.size();++i){
        lightsPos[i].y = -sun->y_min;
        sun->Translate = lightsPos[i];
        sun->material.kd = glm::vec3(0);
        sun->material.ka = lightsColors[i];
        sun->material.ks = glm::vec3(0);
        
        sun->draw_shape(P, MV, lightsPos);
    }

    Floor->scale_obj(5);
    Floor->Translate = (glm::vec3(0,-Floor->y_min,0));
    Floor->material.kd = glm::vec3(0.0,0.3,0.0);
    Floor->draw_shape(P, MV, lightsPos);
    
    // define the shear matrix
    glm::mat4 shear(1.0f);
    shear[1][0] = 0.3f * cos(t);
    
    glm::vec3 emmisive_color(0);
    for(int i = -5; i < 5; ++i){
        for(int x = -5; x < 5; ++x){
            if(object_t[10 * (i+5) + (x+5)] == 1){//draw the bunny
                bunny->scale_obj(scales[10 * (i+5) + (x+5)]);
                bunny->Translate = glm::vec3(x,-bunny->y_min,i);
                bunny->Rotate = glm::vec3(0.0f,1.0f,0.0f);
                bunny->angle = t;
                bunny->material.ka = emmisive_color;
                bunny->material.kd = colors[10 * (i+5) + (x+5)];
                bunny->material.ks = glm::vec3(1.0f,1.0f,1.0f);
                bunny->draw_shape(P, MV, lightsPos);
            }else{
                teapot->Shear = shear;
                teapot->scale_obj(scales[10 * (i+5) + (x+5)]);
                teapot->Translate = glm::vec3(x,-teapot->y_min,i);
                teapot->Rotate = glm::vec3(0.0f,1.0f,0.0f);
                teapot->angle = 0.0;
                teapot->material.ka = emmisive_color;
                teapot->material.kd = colors[10 * (i+5) + (x+5)];
                teapot->material.ks = glm::vec3(1.0f,1.0f,1.0f);
                teapot->draw_shape(P, MV, lightsPos);
            }
        }
    }
}
// This function is called every frame to draw the scene.
static void render()
{
    // test the changes 
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
    FLCamera->setAspect((float)width / (float)height);
	
	double t = glfwGetTime();
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
    glViewport(0, 0, width, height);
	// Apply camera transforms
	P->pushMatrix();
    FLCamera->applyProjectionMatrix(P);
    MV->pushMatrix();
    
	FLCamera->applyViewMatrix(MV);
    
    drawScene(P, MV, t);
    
    MV->popMatrix();
    P->popMatrix();
    
    shader->program_unbind();
	GLSL::checkError(GET_FILE_LINE);
	
	if(OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
    
	// Optional argument
	if(argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "Mario Sandoval", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
    random_color(colors);
    random_obj(object_t);
    random_scales(scales);
    createLights(lightsPos, lightsColors); // initialize the color buffers
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
