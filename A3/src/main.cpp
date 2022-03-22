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
shared_ptr<Shape> shape;
shared_ptr<Shape> tea_shape;

Shader::Shader(string vertex_file, string frag_file, SHADER_TYPE _type){
    next = nullptr;
    prev = nullptr;
    program = make_shared<Program>();
    program->setShaderNames(RESOURCE_DIR + vertex_file, RESOURCE_DIR + frag_file);
    program->setVerbose(true);
    program->init();
    program->addAttribute("aPos");
    program->addAttribute("aNor");
    program->addUniform("MV");
    program->addUniform("P");
    if(_type == PHONG || _type == CELL){
        program->addUniform("MVit"); // add the uniform
        program->addUniform("lightPos");
        program->addUniform("ka");
        program->addUniform("kd");
        program->addUniform("ks");
        program->addUniform("s");
        program->addUniform("light0Pos");
        program->addUniform("light1Pos");
        program->addUniform("light0Color");
        program->addUniform("light1Color");
    }
    if(_type == SILHOUETTE){
        program->addUniform("MVit");
    }
    type = _type;
    program->setVerbose(false);
}
void Shader::bind(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV){
        program->bind();
        glUniformMatrix4fv(program->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(program->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        if(type == PHONG || type == CELL){
            // make the MVit
            glm::mat4 MVit = glm::inverse(glm::transpose(MV->topMatrix()));
            glUniformMatrix4fv(program->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(MVit));
            glUniform3f(program->getUniform("lightPos"), 1.0f, 1.0f, 1.0f);
            glUniform3f(program->getUniform("ka"), materials[m_index].ka.x,materials[m_index].ka.y,materials[m_index].ka.z);
            glUniform3f(program->getUniform("kd"), materials[m_index].kd.x,materials[m_index].kd.y,materials[m_index].kd.z);
            glUniform3f(program->getUniform("ks"), materials[m_index].ks.x,materials[m_index].ks.y,materials[m_index].ks.z);
            glUniform1f(program->getUniform("s"),  materials[m_index].s);
            glUniform3f(program->getUniform("light0Pos"), lights[0].lightPos.x,lights[0].lightPos.y,lights[0].lightPos.z);
            glUniform3f(program->getUniform("light1Pos"), lights[1].lightPos.x,lights[1].lightPos.y,lights[1].lightPos.z);
            glUniform3f(program->getUniform("light0Color"), lights[0].lightColor.x,lights[0].lightColor.y,lights[0].lightColor.z);
            glUniform3f(program->getUniform("light1Color"), lights[1].lightColor.x,lights[1].lightColor.y,lights[1].lightColor.z);
        }
        if(type == SILHOUETTE){
            glm::mat4 MVit = glm::inverse(glm::transpose(MV->topMatrix()));
            glUniformMatrix4fv(program->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(MVit));
        }
    }
void Shader::program_unbind(){
    program->unbind();
}
    void Shader::next_material(){
        if(materials.size() == 0) return;
        m_index = ++m_index % materials.size();
    }
    
    void Shader::prev_material(){
        if(materials.size() == 0) return;
        if(m_index == 0) m_index = materials.size()-1;
        --m_index;
    }
    void Shader::next_ligth(){
        if(lights.size() == 0)return;
        l_index = ++l_index % lights.size();
    }
void Shader::prev_light(){
    if(lights.size() == 0)return;
    if(l_index == 0) l_index = lights.size()-1;
    --l_index;
}
Light * Shader::current_light(){
    if(lights.size() == 0) return nullptr;
    return &lights[l_index];
}
void Shader_Collection::push_back(string vert_file, string frag_file, SHADER_TYPE _type){
    shared_ptr<Shader> newShader = make_shared<Shader>(vert_file,frag_file, _type);
    if(!header){ // make the first shader
        header = newShader;
        tail = newShader;
        current = newShader;
        return;
    }
    tail->next = newShader;
    newShader->prev = tail;
    tail = tail->next;
}
void Shader_Collection::move_forward(){
    if(!current->next)return;
    current = current->next;
}
void Shader_Collection::move_backward(){
    if(!current->prev)return;
    current = current->prev;
}

// make a pointer to a shared collection
shared_ptr<Shader_Collection> shader_collection;

static void define_materials(vector<Material> &materials){
    // in here i will define the materials for the objects
    glm::vec3 _ka, _kd, _ks;
    float s;

    // first material
    _ka = glm::vec3(0.2,0.2,0.2);
    _kd = glm::vec3(0.8,0.7,0.7);
    _ks = glm::vec3(1.0,0.9,0.8);
    s = 200;
    materials.push_back(Material(_ka,_kd,_ks,s));

    // second material blue
    _ka = glm::vec3(0.1,0.1,0.1);
    _kd = glm::vec3(0.1,0.1,1.0);
    _ks = glm::vec3(0.1,1.0,0.1);
    s = 100;
    materials.push_back(Material(_ka,_kd,_ks,s));

    // third material grey
    _ka = glm::vec3(0.1,0.1,0.1);
    _kd = glm::vec3(0.5,0.5,0.7);
    _ks = glm::vec3(0.1,0.1,0.1);
    s = 200;
    materials.push_back(Material(_ka,_kd,_ks,s));
}

void define_lights(vector<Light> &lights){
    // here i will define the lights to be passed into the vecotr of the shader
    glm::vec3 _lp, _lc;
    
    // light 1
    _lp = glm::vec3(1.0,1.0,1.0);
    _lc = glm::vec3(0.8, 0.8, 0.8);
    lights.push_back(Light(_lp,_lc));
    
    // light 2
    _lp = glm::vec3(-1.0, 1.0, 1.0);
    _lc = glm::vec3(0.2, 0.2, 0.0);
    lights.push_back(Light(_lp,_lc));
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
    if(keyToggles[(unsigned) 's']){ // move the shader
        shader_collection->move_forward();
    }
    if(keyToggles[(unsigned) 'S']){ // move the shader
        shader_collection->move_backward();
    }
    if(keyToggles[(unsigned) 'm']){ // move materials
        // forward material
        shader_collection->current->next_material();
    }
    if(keyToggles[(unsigned) 'M']){ // move the materials
        // backwards material
        shader_collection->current->prev_material();
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
    shader_collection->push_back("phong_vert.glsl", "phong_frag.glsl", PHONG);
    // set the materials for the phong shader
    define_materials(shader_collection->tail->materials);
    // set the lights for the phong shaders
    define_lights(shader_collection->tail->lights);
    
    // add the silhouette shader
    shader_collection->push_back("silh_vert.glsl", "silh_frag.glsl", SILHOUETTE);
	
    // add the cell shader
    shader_collection->push_back("cell_vert.glsl", "cell_frag.glsl", CELL);
    //set the materials for this shader
    define_materials(shader_collection->tail->materials);
    //set the lights
    define_lights(shader_collection->tail->lights);
    
	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation
	
	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();
    
    tea_shape = make_shared<Shape>();
    tea_shape->loadMesh(RESOURCE_DIR + "teapot.obj");
    tea_shape->init();
	
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
    if(keyToggles[(unsigned) 'l']){
        shader_collection->current->next_ligth();
    }
    if(keyToggles[(unsigned) 'L']){
        shader_collection->current->prev_light();
    }
    if(keyToggles[(unsigned) 'x']){// toggle the light in x dir
        shader_collection->current->current_light()->lightPos.x += 0.2;
    }
    if(keyToggles[(unsigned) 'X']){// toggle the light in x dir
        shader_collection->current->current_light()->lightPos.x -= 0.2;
    }
    if(keyToggles[(unsigned) 'y']){// toggle the light in x dir
        shader_collection->current->current_light()->lightPos.y += 0.2;
    }
    if(keyToggles[(unsigned) 'Y']){// toggle the light in x dir
        shader_collection->current->current_light()->lightPos.y += 0.2;
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
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
    // Matrix stacks for the teapot
    auto _MV = make_shared<MatrixStack>();
    
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
    
    _MV->pushMatrix();
    camera->applyViewMatrix(_MV);
    
    // Task 1 transform the bunny
    MV->scale(0.5);
    MV->translate(-1.0f,-1.0f,0.0f);
    // make the bunny rotate
    MV->rotate(t, 0.0f,1.0f,0.0f);
    
    // Transform the teapot
    _MV->scale(0.5);
    _MV->translate(1.0,0.0,0.0);
    _MV->rotate(-3.10,0.0,1.0,0.0);
    
    //shear rotate
    glm::mat4 S(1.0f);
    S[0][1] = -0.5f * cos(t);
    _MV->multMatrix(S);
    
    // draw the bunny
    shader_collection->current->bind(P, MV);
    shape->draw(shader_collection->current->program);
    // draw the teapot
    shader_collection->current->bind(P, _MV);
    tea_shape->draw(shader_collection->current->program);
    
	MV->popMatrix();
    _MV->popMatrix();
	P->popMatrix();
    shader_collection->current->program_unbind();
	
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
