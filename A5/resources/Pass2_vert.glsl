#version 120

uniform mat4 P;
uniform mat4 MV;

attribute vec4 aPos;
attribute vec2 aTex;

varying vec2 vTex;

void main()
{
	gl_Position = P * MV * aPos;
	vTex = aTex;
}
