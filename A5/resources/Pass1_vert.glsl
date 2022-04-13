#version 120
// vertex shader for the bling phong

uniform mat4 P;
uniform mat4 MV;
uniform mat4 MVit;

attribute vec4 aPos;
attribute vec3 aNor;

// pass in the corrdinates for the normal and the position
varying vec3 vNor;
varying vec3 vPos;

void main()
{
	gl_Position = P * MV * aPos;
    vNor = vec3(normalize(MVit * vec4(aNor,0.0)));
    vPos = vec3(MV * aPos);
}
