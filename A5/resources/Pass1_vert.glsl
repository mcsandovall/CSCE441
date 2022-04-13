#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 MVit;

attribute vec4 aPos;
attribute vec3 aNor;

// pass in the corrdinates for the normal and the position
varying vec3 cNor;
varying vec3 cPos;

void main()
{
	gl_Position = P * MV * aPos;
    cNor = vec3(normalize(MVit * vec4(aNor,0.0)));
    cPos = vec3(MV * aPos);
}
