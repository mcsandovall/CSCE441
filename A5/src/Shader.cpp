#include "Shader.h"
#include "Program.h"
#include <memory>
#include "MatrixStack.h"

using namespace std;

// Shader class function implementation
Shader::Shader(string vertex_file, string frag_file){
    // move this to another func for the uniforms
    program = make_shared<Program>();
    program->setShaderNames(vertex_file,frag_file);
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
    // make the light position
    glm::vec3 lightPos(8.0,10.0,1.0);
    lightPos = glm::vec3(MV->topMatrix() * glm::vec4(lightPos,1.0));
    glUniform3f(program->getUniform("lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glm::mat4 MVit = glm::inverse(glm::transpose(MV->topMatrix()));
    glUniformMatrix4fv(program->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
    glUniformMatrix4fv(program->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(MVit));
    glUniform3f(program->getUniform("ka"), M.ka.x,M.ka.y,M.ka.z);
    glUniform3f(program->getUniform("kd"), M.kd.x,M.kd.y,M.kd.z);
    glUniform3f(program->getUniform("ks"), M.ks.x,M.ks.y,M.ks.z);
    glUniform1f(program->getUniform("s"),  M.s);
}

void Shader::program_unbind(){
    program->unbind();
}
