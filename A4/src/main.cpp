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
#include "Shader.h"

using namespace std;


GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Shader> shader;
// make a pointer to a shared collection

// Shader class function implementation
Shader::Shader(string vertex_file, string frag_file){
    program = make_shared<Program>();
    program->setShaderNames(RESOURCE_DIR + vertex_file, RESOURCE_DIR + frag_file);
    program->setVerbose(true);
    program->init();
    program->addAttribute("aPos");
    program->addAttribute("aNor");
    program->addUniform("MV");
    program->addUniform("P");
    program->addUniform("MVit"); // add the uniform
    program->addUniform("lightPos");
    program->addUniform("ka");
    program->addUniform("kd");
    program->addUniform("ks");
    program->addUniform("s");
    program->setVerbose(false);
}
void Shader::bind(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, Material M){
        program->bind();
        glUniformMatrix4fv(program->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(program->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        // make the MVit
        glm::mat4 MVit = glm::inverse(glm::transpose(MV->topMatrix()));
        // make the light position
        glm::vec3 lightPos(8.0,10.0,1.0);
        lightPos = glm::vec3(MV->topMatrix() * glm::vec4(lightPos,1.0));
        glUniformMatrix4fv(program->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(MVit));
        glUniform3f(program->getUniform("lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(program->getUniform("ka"), M.ka.x,M.ka.y,M.ka.z);
        glUniform3f(program->getUniform("kd"), M.kd.x,M.kd.y,M.kd.z);
        glUniform3f(program->getUniform("ks"), M.ks.x,M.ks.y,M.ks.z);
        glUniform1f(program->getUniform("s"),  M.s);
        
    }
void Shader::program_unbind(){
    program->unbind();
}

/**
 Assigment 4 start code. Implement in different file once it works
 */

vector<int> object_t;
vector<glm::vec3> colors;

enum OBJECT_TYPE{
    BUNNY,
    TEAPOT,
    FLOOR,
    SUN
};

class Object{
public:
    glm::vec3 Translate, Rotate, Scale;
    glm::mat4 Shear;
    double angle;
    OBJECT_TYPE type;
    shared_ptr<Shape> shape;
    Material material;
    float y_min, x_min;
    
    Object() : Translate(glm::vec3(0)), Rotate(glm::vec3(1.0)), Scale(glm::vec3(1.0)), Shear(glm::mat4(1.0f)), angle(1.0){}
    ~Object(){}
    Object(OBJECT_TYPE _type): Translate(glm::vec3(0)), Rotate(glm::vec3(1.0)), Scale(glm::vec3(1.0)), Shear(glm::mat4(1.0f)), angle(0) { // load the object into the shape
        shape = make_shared<Shape>();
        if(_type == BUNNY){
            shape->loadMesh(RESOURCE_DIR + "bunny.obj");
        }else if(_type == TEAPOT){
            shape->loadMesh(RESOURCE_DIR + "teapot.obj");
        }else if(_type == FLOOR){
            shape->loadMesh(RESOURCE_DIR + "cube.obj");
        }else{
            shape->loadMesh(RESOURCE_DIR + "sphere.obj");
        }
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
    void draw_shape(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV){
        MV->pushMatrix();
        MV->translate(Translate);
        MV->scale(Scale);
        MV->rotate(angle, Rotate);
        MV->multMatrix(Shear);
        shader->bind(P, MV, material);
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

// class for the free look camera
class FreeLook{
public:
    glm::vec3 position, orientation = glm::vec3(0.0,0.0,-1.0f);
    const glm::vec3 up;
    float aspect,fovy,znear,zfar, speed;
    
    FreeLook(glm::vec3 _pos) :
    position(_pos),
    up(0.0,1.0f,0.0),
    aspect(1.0),
    fovy((float)(45.0*M_PI/180.0)),
    znear(0.1f),
    zfar(1000.0f),
    speed(0.2)
    {}
    
    void setAspect(float a){
        aspect = a;
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
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

// This function is called keyboard letter is pressed
static void char_callback(GLFWwindow *window, unsigned int key)
{
    if(keyToggles[(unsigned)'w']){
        FLCamera->position += FLCamera->speed * FLCamera->orientation;
    }
    if(keyToggles[(unsigned)'s']){
        FLCamera->position += FLCamera->speed * -FLCamera->orientation;
    }
    if(keyToggles[(unsigned)'a']){
        FLCamera->position += FLCamera->speed * -glm::normalize(glm::cross(FLCamera->orientation, FLCamera->up));
    }
    if(keyToggles[(unsigned)'d']){
        FLCamera->position += FLCamera->speed * glm::normalize(glm::cross(FLCamera->orientation, FLCamera->up));
    }
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

shared_ptr<Object> bunny,teapot, Floor, sun;

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);
    
	//camera = make_shared<Camera>();
	//camera->setInitDistance(2.0f); // Camera's initial Z translation
    
    // initialize the free look camera
    FLCamera = make_shared<FreeLook>(glm::vec3(1.0f,0.5f,2.0f));
	
    shader = make_shared<Shader>("phong_vert.glsl", "phong_frag.glsl");
    // load the object only once
    bunny = make_shared<Object>(BUNNY);
    teapot = make_shared<Object>(TEAPOT);
    Floor = make_shared<Object>(FLOOR);
    sun = make_shared<Object>(SUN);
	
	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	//camera->setAspect((float)width/(float)height);
    FLCamera->setAspect((float)width / (float)height);
	
	double t = glfwGetTime();
	if(!keyToggles[(unsigned)' ']) {
		// Spacebar turns animation on/off
		t = 0.0f;
	}
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
    
	// Apply camera transforms
	P->pushMatrix();
//	camera->applyProjectionMatrix(P);
    FLCamera->applyProjectionMatrix(P);
	MV->pushMatrix();
	FLCamera->applyViewMatrix(MV);
    
    sun->scale_obj(0.5);
    sun->Translate = glm::vec3(8.0,10.0,1.0);
    sun->material.kd = glm::vec3(0);
    sun->material.ka = glm::vec3(1.0,1.0,0.0);
    sun->material.ks = glm::vec3(0);
    sun->draw_shape(P, MV);

    Floor->scale_obj(12);
    Floor->Translate = glm::vec3(0.0,-Floor->y_min,0.0);
    Floor->material.kd = glm::vec3(0.0,0.3,0.0);
    Floor->draw_shape(P, MV);
    
    float scalar = abs(sin(t)/cos(t));
    if(scalar > 1){
        scalar = 1;
    }else if (scalar < 0.5){
        scalar = 0.5;
    }
    bunny->scale_obj(scalar); teapot->scale_obj(scalar);
    for(int i = -5; i < 5; ++i){
        for(int x = -5; x < 5; ++x){
            if(object_t[10 * (i+5) + (x+5)] == 1){//draw the bunny
                bunny->Translate = glm::vec3(x,-bunny->y_min,i);
                bunny->Rotate = glm::vec3(0.0f,1.0f,0.0f);
                bunny->angle = t;
                bunny->material.kd = colors[10 * (i+5) + (x+5)];
                bunny->draw_shape(P, MV);
            }else{
                teapot->Translate = glm::vec3(x,-teapot->y_min,i);
                teapot->Rotate = glm::vec3(0.0f,1.0f,0.0f);
                teapot->angle = t;
                teapot->material.kd = colors[10 * (i+5) + (x+5)];
                teapot->draw_shape(P, MV);
            }
        }
    }
    Floor->scale_obj(12);
    Floor->Translate = (glm::vec3(0,-Floor->y_min,0));
    Floor->material.kd = glm::vec3(0.0,0.3,0.0);
    Floor->draw_shape(P, MV);
    
//    // A3 bunny coordinates for testing
//    bunny->Scale = glm::vec3(0.5);
//    bunny->Translate = glm::vec3(-0.5f,-0.5f,-5.0f);
//    // make the bunny rotate
//    bunny->Rotate = glm::vec3(0.0f,1.0f,0.0f);
//    bunny->angle = t;
//
//    bunny->draw_shape(P, MV);
    
    P->popMatrix();
    MV->popMatrix();
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
