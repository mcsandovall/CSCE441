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
    program->addUniform("ka");
    program->addUniform("kd");
    program->addUniform("ks");
    program->addUniform("s");
    program->addUniform("lightsPos");
    program->addUniform("lightsColors");
    program->setVerbose(false);
}

void Shader::bind(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, Material M, vector<glm::vec3> lightsPos, vector<glm::vec3> lightsColors){
    program->bind();
    glUniformMatrix4fv(program->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
    glm::mat4 MVit = glm::inverse(glm::transpose(MV->topMatrix()));
    glUniformMatrix4fv(program->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
    glUniformMatrix4fv(program->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(MVit));
    glUniform3fv(program->getUniform("ka"), 1, glm::value_ptr(M.ka));
    glUniform3fv(program->getUniform("kd"), 1, glm::value_ptr(M.kd));
    glUniform3fv(program->getUniform("ks"), 1 , glm::value_ptr(M.ks));
    glUniform1f(program->getUniform("s"),  M.s);
    glUniform3fv(program->getUniform("lightsPos"), 10, glm::value_ptr(lightsPos[0]));
    glUniform3fv(program->getUniform("lightsColors"), 10, glm::value_ptr(lightsColors[0]));
}

void Shader::program_unbind(){
    program->unbind();
}

DShader::DShader(const string &vert, const string &frag){
    prog = make_shared<Program>();
    prog->setShaderNames(vert,frag);
    prog->setVerbose(true);
    prog->init();
    prog->addAttribute("aPos");
    prog->addAttribute("aNor");
    prog->addUniform("MV");
    prog->addUniform("P");
    prog->addUniform("MVit"); // add the uniform
    prog->addUniform("ke");
    prog->addUniform("kd");
    prog->setVerbose(false);
}

void DShader::bind(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, Material M){
    prog->bind();
    glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
    glm::mat4 MVit = glm::inverse(glm::transpose(MV->topMatrix()));
    glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
    glUniformMatrix4fv(prog->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(MVit));
    glUniform3fv(prog->getUniform("ke"), 1, glm::value_ptr(M.ka));
    glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(M.kd));
}

void DShader::unbind(){
    prog->unbind();
}
