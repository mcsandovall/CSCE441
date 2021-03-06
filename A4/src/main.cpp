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
vector<glm::vec3> colors;

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
    void draw_shape(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, bool HUD = false){
        MV->pushMatrix();
        MV->translate(Translate);
        MV->scale(Scale);
        MV->rotate(angle, Rotate);
        MV->multMatrix(Shear);
        if(!HUD){
            shader->bind(P, MV, material, false);
        }else{
            shader->bind(P, MV, material, true);
        }
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

shared_ptr<Object> bunny,teapot, Floor, sun, Frustum;

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
	
    shader = make_shared<Shader>(RESOURCE_DIR + "phong_vert.glsl",RESOURCE_DIR + "phong_frag.glsl");
    // load the object only once
    bunny = make_shared<Object>("bunny.obj");
    teapot = make_shared<Object>("teapot.obj");
    Floor = make_shared<Object>("cube.obj");
    sun = make_shared<Object>("sphere.obj");
    Frustum = make_shared<Object>("frustum.obj");
    
    floorProgram = make_shared<Program>();
    floorShape = make_shared<Shape>();
    floorShape->loadMesh(RESOURCE_DIR + "cube.obj");
    floorShape->init();
    
    floorProgram->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
    floorProgram->setVerbose(true);
    floorProgram->init();
    floorProgram->addAttribute("aPos");
    floorProgram->addAttribute("aTex");
    floorProgram->addUniform("MV");
    floorProgram->addUniform("P");
    floorProgram->addUniform("T1");
    floorProgram->addUniform("texture0");
    floorProgram->setVerbose(false);
	
    floorTexture = make_shared<Texture>();
    floorTexture->setFilename(RESOURCE_DIR + "minecraft.jpg");
    floorTexture->init();
    floorTexture->setUnit(0);
    floorTexture->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    
    //
    // Initialize geometry
    //
    // We need to fill in the position buffer, normal buffer, the texcoord
    // buffer, and the index buffer.
    // 0
    posBuf.push_back(1.0f);
    posBuf.push_back(1.0f);
    posBuf.push_back(-1.0f);
    texBuf.push_back(1.0f);
    texBuf.push_back(1.0f);
    // 1
    posBuf.push_back(1.0f);
    posBuf.push_back(-1.0f);
    posBuf.push_back(-1.0f);
    texBuf.push_back(1.0f);
    texBuf.push_back(0.0f);
    // 2
    posBuf.push_back(-1.0f);
    posBuf.push_back(-1.0f);
    posBuf.push_back(1.0f);
    texBuf.push_back(0.0f);
    texBuf.push_back(0.0f);
    // 3
    posBuf.push_back(1.0f);
    posBuf.push_back(-1.0f);
    posBuf.push_back(-1.0f);
    texBuf.push_back(1.0f);
    texBuf.push_back(0.0f);
    // 4
    posBuf.push_back(1.0f);
    posBuf.push_back(1.0f);
    posBuf.push_back(1.0f);
    texBuf.push_back(1.0f);
    texBuf.push_back(1.0f);
    
    // Index
    indBuf.push_back(0);
    indBuf.push_back(1);
    indBuf.push_back(2);
    indBuf.push_back(3);
    indBuf.push_back(2);
    indBuf.push_back(1);
    indCount = (int)indBuf.size();
    
    // Generate 3 buffer IDs and put them in the bufIDs map.
    GLuint tmp[3];
    glGenBuffers(3, tmp);
    bufIDs["bPos"] = tmp[0];
    bufIDs["bTex"] = tmp[1];
    bufIDs["bInd"] = tmp[2];
    
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bPos"]);
    glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bTex"]);
    glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs["bInd"]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size()*sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    GLSL::checkError(GET_FILE_LINE);
}

void drawHUD(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, float t){
    // draw the HUD
    float y = 0.6;
    float x = 0.8;
    glm::vec3 ka(0.0,0.0,0.0);
    glm::vec3 kd(0.7f,0.7f,0.7f);
    glm::vec3 ks(1.0f,1.0f,1.0f);
    Material m(ka,kd,ks,200.f);

    bunny->Translate = glm::vec3(x,y,-0.3f);
    bunny->Rotate = glm::vec3(0.0,1.0f,0.0);
    bunny->angle = t;
    bunny->material = m;
    bunny->Scale = glm::vec3(0.2);
    bunny->draw_shape(P, MV, true);

    x = -x;
    y += 0.1;
    teapot->Translate = glm::vec3(x,y,-0.3f);
    teapot->Rotate = glm::vec3(0.0,1.0f,0.0);
    teapot->angle = t;
    teapot->material = m;
    teapot->Scale = glm::vec3(0.2);
    teapot->draw_shape(P, MV, true);
}

void drawScene(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, float t, bool orthographic = false){
    sun->scale_obj(0.5);
    sun->Translate = glm::vec3(8.0,10.0,1.0);
    sun->material.kd = glm::vec3(0);
    sun->material.ka = glm::vec3(1.0,1.0,0.0);
    sun->material.ks = glm::vec3(0);
    sun->draw_shape(P, MV, orthographic);
    
    MV->pushMatrix();
    MV->scale(glm::vec3(12));
    MV->translate(glm::vec3(0.0,-Floor->y_min,0.0));
    floorProgram->bind();
    floorTexture->bind(floorProgram->getUniform("texture0"));
    glUniformMatrix4fv(floorProgram->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
    glUniformMatrix4fv(floorProgram->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
    glEnableVertexAttribArray(floorProgram->getAttribute("aPos"));
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bPos"]);
    glVertexAttribPointer(floorProgram->getAttribute("aPos"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(floorProgram->getAttribute("aTex"));
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bTex"]);
    glVertexAttribPointer(floorProgram->getAttribute("aTex"), 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs["bInd"]);
    floorShape->draw(floorProgram);
    floorTexture->unbind();
    floorProgram->unbind();
    MV->popMatrix();
    
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
                bunny->angle = 0.0;
                bunny->material.kd = colors[10 * (i+5) + (x+5)];
                bunny->draw_shape(P, MV, orthographic);
            }else{
                teapot->Translate = glm::vec3(x,-teapot->y_min,i);
                teapot->Rotate = glm::vec3(0.0f,1.0f,0.0f);
                teapot->angle = 0.0;
                teapot->material.kd = colors[10 * (i+5) + (x+5)];
                teapot->draw_shape(P, MV, orthographic);
            }
        }
    }
}

void draw_frustrum(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV){
    MV->pushMatrix();
    shared_ptr<MatrixStack> camera = make_shared<MatrixStack>();
    camera->pushMatrix();
    FLCamera->applyViewMatrix(camera);
    MV->multMatrix(glm::inverse(camera->topMatrix()));
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float a = (float) width / (float) height;
    float sy = tan(glm::radians(FLCamera->angle/2));
    float sx = a * sy;
    MV->scale(glm::vec3(sx,sy,1.0f));
    // make the frustum black
    Frustum->material.ka = glm::vec3(0);
    Frustum->material.kd = glm::vec3(0);
    Frustum->material.ks = glm::vec3(0);
    Frustum->draw_shape(P, MV);
    MV->popMatrix();
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
    
    // Draw the HUD
    drawHUD(P, MV, t);
    
	FLCamera->applyViewMatrix(MV);
    
    drawScene(P, MV, t);
    
    MV->popMatrix();
    P->popMatrix();
    
    if(keyToggles[(unsigned)'t']){
        double s = 0.5;
        glViewport(0, 0, s*width, s*height);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, s*width, s*height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        P->pushMatrix();
        MV->pushMatrix();
        // apply the matricies
        float aspect = width / height;
        P->multMatrix(glm::ortho(-aspect, aspect, -1.0f, 1.0f, 0.01f, 100.0f));
        MV->rotate(90, glm::vec3(1.0f,0.0,0.0));
        MV->translate(0.1,-0.6f,0.35);
        MV->scale(0.2);
        // draw scene
        drawScene(P, MV, t);
        draw_frustrum(P, MV);
        
        P->popMatrix();
        MV->popMatrix();
    }
    
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
