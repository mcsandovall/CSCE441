#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"

using namespace std;

GLFWwindow *window; // Main application window
string RES_DIR = ""; // Where data files live
shared_ptr<Program> prog;
shared_ptr<Program> progIM; // immediate mode
shared_ptr<Shape> shape;

float rx = 0.0, ry = 1, rz = 0.0, rA = 0.5;
float z = -3.5; // this is the regular z value for better view

// make a struct for the object
class object{
public:
    glm::vec3 JTranslation; // translation in relation to the joint
    glm::vec3 MTranslation; // translation from joint to mesh
    glm::vec3 Scale;  // scaling relative to parent
    glm::vec3 Rotation = glm::vec3(0.0,0.0,0.0); // rotation to the object and its children
    
    // Hierchal Model
    object * next_object = nullptr;
    object * next_level = nullptr;
    void setNext_level(object * obj){next_level = obj;}
    void setNext_object(object * obj){next_object = obj;}
    void self_render(std::shared_ptr<MatrixStack> MV){
        // get the stack
        MV->pushMatrix();
        MV->translate(JTranslation);
        //MV->rotate(rA, Rotation); Uncomment this once we get the angles figure out
        // push the rest of the rendering
            MV->pushMatrix();
            MV->translate(MTranslation);
            MV->scale(Scale);
            glUniformMatrix4fv(prog->getUniform("MV"),1,GL_FALSE,glm::value_ptr(MV->topMatrix()));
            shape->draw(prog);
            MV->popMatrix();
        // recursively call its children
        if(next_level){
            next_level->self_render(MV);
        }
        // pop the stack
        MV->popMatrix();
        // call the same level object
        if(next_object){
            next_object->self_render(MV);
        }
    }
};

class Robot{
public:
    object * header;
    struct torso : object{
        torso(){
            JTranslation = glm::vec3(0,1.0,z); // joint in respect to world
            MTranslation = glm::vec3(0,0,0); // in relation to the joint
            Scale = glm::vec3(1.0,1.5,1.0); // scale it vertically
            Rotation = glm::vec3(rx,ry,rz);
            next_level = new head();
        }
    };
    struct head : object{
        head(){
            JTranslation = glm::vec3(0.0,1.0,0.0);
            MTranslation = glm::vec3(0,0,0);
            //Rotation = glm::vec3(0,-ry,0);
            Scale = glm::vec3(0.5,0.5,1);
            next_object = new arm(1);
        }
    };
    struct arm : object{
        arm(int sign){
            if(sign){ // right arm means positive x direction for joint translation
                JTranslation = glm::vec3(0.5,0.5,0);
                MTranslation = glm::vec3(0.5,0,0);
                Scale = glm::vec3(1.0,0.4,1.0);
                next_level = new lower_arm(1);
                next_object = new arm(0);
            }else{ // left arm
                JTranslation = glm::vec3(-0.5,0.5,0);
                MTranslation = glm::vec3(-0.5,0,0);
                Scale = glm::vec3(1.0,0.4,1.0);
                next_level = new lower_arm(0);
                next_object = new leg(1);
            }
        }
        struct lower_arm : object{
            lower_arm(int sign){ // right lower arm
                if(sign){
                    JTranslation = glm::vec3(1.0,0,0);
                    MTranslation = glm::vec3(0.4,0,0);
                    Scale = glm::vec3(0.8,0.3,1);
                }else{
                    JTranslation = glm::vec3(-1.0,0,0);
                    MTranslation = glm::vec3(-0.4,0,0);
                    Scale = glm::vec3(0.8,0.3,1);
                }
            }
        };
    };
    struct leg : object{
        leg(int sign){
            if(sign){ // right leg
                JTranslation = glm::vec3(0.25,-0.5,0);
                MTranslation = glm::vec3(0,-0.8,0);
                Scale = glm::vec3(0.45,1.2,1);
                next_level = new lower_leg();
                next_object = new leg(0);
            } // lef leg
            else{
                JTranslation = glm::vec3(-0.25,-0.5,0);
                MTranslation = glm::vec3(0,-0.8,0);
                Scale = glm::vec3(0.45,1.2,1);
                next_level = new lower_leg();
            }
        }
        struct lower_leg : object{
            lower_leg(){ // lower left, same on both
                JTranslation = glm::vec3(0,-0.6,0);
                MTranslation = glm::vec3(0,-1.3,0);
                Scale = glm::vec3(0.4,1,1);
            }
        };
    };
    
    Robot(){
        header = new torso();
    }
};

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}
static void character_callback(GLFWwindow* window, unsigned int codepoint){
    char letter = (char) codepoint;
    switch (letter) {
        case 'x':
            rx += 0.5;
            break;
        case 'X':
            rx -= 0.5;
            break;
        case 'y':
            ry += 0.5;
            break;
        case 'Y':
            ry -= 0.5;
            break;
        case 'z':
            rz += 0.5;
            break;
        case 'Z':
            rz -= 0.5;
            break;
        case '.':
            z += 0.5;
            break;
        case ',':
            z -= 0.5;
            break;
        default:
            break;
    }
}
static void init()
{
	GLSL::checkVersion();

	// Check how many texture units are supported in the vertex shader
	int tmp;
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &tmp);
	cout << "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS = " << tmp << endl;
	// Check how many uniforms are supported in the vertex shader
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &tmp);
	cout << "GL_MAX_VERTEX_UNIFORM_COMPONENTS = " << tmp << endl;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &tmp);
	cout << "GL_MAX_VERTEX_ATTRIBS = " << tmp << endl;

	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	// Initialize mesh.
	shape = make_shared<Shape>();
	shape->loadMesh(RES_DIR + "cube.obj");
	shape->init();
	
	// Initialize the GLSL programs.
	prog = make_shared<Program>();
	prog->setVerbose(true);
	prog->setShaderNames(RES_DIR + "nor_vert.glsl", RES_DIR + "nor_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->setVerbose(false);
	
	progIM = make_shared<Program>();
	progIM->setVerbose(true);
	progIM->setShaderNames(RES_DIR + "simple_vert.glsl", RES_DIR + "simple_frag.glsl");
	progIM->init();
	progIM->addUniform("P");
	progIM->addUniform("MV");
	progIM->setVerbose(false);
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

static void render()
{
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspect = width/(float)height;
	glViewport(0, 0, width, height);

	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Create matrix stacks.
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	// Apply projection.
	P->pushMatrix();
	P->multMatrix(glm::perspective((float)(45.0*M_PI/180.0), aspect, 0.01f, 100.0f));
	// Apply camera translation
    MV->pushMatrix();
    MV->translate(glm::vec3(0, 0, -3));
    
	// Begin to draw the robot
    Robot r;
	prog->bind();
    glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
    r.header->self_render(MV);
    progIM->unbind();
	
	GLSL::checkError(GET_FILE_LINE);
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RES_DIR = argv[1] + string("/");

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// https://en.wikipedia.org/wiki/OpenGL
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
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
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
    // Set the char callback
    glfwSetCharCallback(window, character_callback);
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
