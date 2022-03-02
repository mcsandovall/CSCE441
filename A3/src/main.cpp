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

using namespace std;

enum SHADER_TYPE{
    NORMAL,
    PHONG
};

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Program> prog2; 
shared_ptr<Shape> shape;

// make the class for the materials
class Material{
public:
    glm::vec3 ka,kd,ks;
    float s;
    Material() : ka(0.0,0.0,0.0), kd(0.0,0.0,0.0), ks(0.0,0.0,0.0), s(0) {} // constructor
    Material(glm::vec3 _ka, glm::vec3 _kd, glm::vec3 _ks, float _s) : ka(_ka), kd(_kd), ks(_ks), s(_s) {}
};

class Shader{
public:
    shared_ptr<Program> program;
    shared_ptr<Shader> next, prev;
    SHADER_TYPE type;
    Shader(string vertex_file, string frag_file, SHADER_TYPE _type) : next(nullptr), prev(nullptr){
        program = make_shared<Program>();
        program->setShaderNames(RESOURCE_DIR + vertex_file, RESOURCE_DIR + frag_file);
        program->setVerbose(true);
        program->init();
        program->addAttribute("aPos");
        program->addAttribute("aNor");
        program->addUniform("MV");
        program->addUniform("P");
        if(type == PHONG){
            prog->addUniform("MVit"); // add the uniform
            prog->addUniform("lightPos");
            prog->addUniform("ka");
            prog->addUniform("kd");
            prog->addUniform("ks");
            prog->addUniform("s");
            type = _type;
        }
        type = _type;
        program->setVerbose(false);
    }
    void render(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV){
        program->bind();
        glUniformMatrix4fv(program->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(program->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        if(type == PHONG){
            // make the MVit
            glm::mat4 MVit = glm::inverse(glm::transpose(MV->topMatrix()));
            glUniformMatrix4fv(program->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(MVit));
            glUniform3f(prog->getUniform("lightPos"), 1.0f, 1.0f, 1.0f);
            glUniform3f(prog->getUniform("ka"), 0.1f, 0.1f, 0.1f);
            glUniform3f(prog->getUniform("kd"), 0.5f, 0.5f, 0.7f);
            glUniform3f(prog->getUniform("ks"), 0.1f, 0.1f, 0.1f);
            glUniform1f(prog->getUniform("s"), 200.0f);
        }
        shape->draw(program);
        program->unbind();
    }
};

class Shader_Collection{
public:
    shared_ptr<Shader> header, current;
    Shader_Collection() : header(nullptr), current(nullptr){}
    void push_back(string vert_file, string frag_file, SHADER_TYPE _type){
        shared_ptr<Shader> newShader = make_shared<Shader>(vert_file,frag_file, _type);
        if(!header){
            header = newShader;
            current = newShader;
            return;
        }
        shared_ptr<Shader> _current  = header;
        while(_current->next != nullptr){
            _current = current->next;
        }
        header->prev = newShader;
        newShader->prev = _current;
        newShader->next = header;
        _current->next = newShader;
    }
    void move_forward(){
        current = current->next;
    }
    void move_backward(){
        current = current->prev;
    }
};

// make a pointer to a shared collection
shared_ptr<Shader_Collection> shader_collection;

// define the db for the materials
vector<Material> Materials;
int material_index = 0;

static void define_materials(){
    // in here i will define the materials for the objects
    glm::vec3 _ka, _kd, _ks;
    float s;

    // first material
    _ka = glm::vec3(0.2,0.2,0.2);
    _kd = glm::vec3(0.8,0.7,0.7);
    _ks = glm::vec3(1.0,0.9,0.8);
    s = 200;
    Materials.push_back(Material(_ka,_kd,_ks,s));

    // second material blue
    _ka = glm::vec3(0.1,0.1,0.1);
    _kd = glm::vec3(0.1,0.1,1.0);
    _ks = glm::vec3(0.1,1.0,0.1);
    s = 100;
    Materials.push_back(Material(_ka,_kd,_ks,s));

    // third material grey
    _ka = glm::vec3(0.1,0.1,0.1);
    _kd = glm::vec3(0.5,0.5,0.7);
    _ks = glm::vec3(0.1,0.1,0.1);
    s = 200;
    Materials.push_back(Material(_ka,_kd,_ks,s));
}

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
    
    if(keyToggles[(unsigned) 'm']){ // cycle material
        // forward material
        material_index = ++material_index % (Materials.size());
    }
    if(keyToggles[(unsigned) 'M']){
        // backwards material
        if(material_index == 0) material_index = Materials.size()-1;
        --material_index;
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

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

    shader_collection = make_shared<Shader_Collection>();
    shader_collection->push_back("normal_vert.glsl", "normal_frag.glsl",NORMAL);
	
	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation
	
	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();
	
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
	camera->setAspect((float)width/(float)height);
	
	double t = glfwGetTime();
	if(!keyToggles[(unsigned)' ']) {
		// Spacebar turns animation on/off
		t = 0.0f;
	}
	
	// Matrix stacks
    glm::mat4 MVit;
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
    // Task 1 transform the bunny
    MV->scale(0.5);
    MV->translate(0.0f,-1.0f,0.0f);
    
    // Get the inverse transpose
    MVit = glm::inverse(glm::transpose(MV->topMatrix()));
    
    // draw the bunny
    shader_collection->current->render(P, MV);
    
	MV->popMatrix();
	P->popMatrix();
	
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
	
    // add materials to the database
    define_materials();
    
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
