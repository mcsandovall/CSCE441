#version 120

// silhoute shader

uniform mat4 P;
uniform mat4 MV;
uniform mat4 MVit;

attribute vec4 aPos; // in object space
attribute vec3 aNor; // in object space

// add the varying memebers for camera space
varying vec3 cNor; // in camera space
varying vec3 cPos; // in camera space

void main(){
    gl_Position = P * (MV * aPos);
    // perform model view transoformation to get into camera space
    cNor = vec3(normalize(MVit * vec4(aNor,0.0)));
    
    // Perform mv for the postition
    cPos = vec3(MV * aPos);
}
