#version 120

varying vec3 vPos;
varying vec3 vNor;

uniform vec3 ke;
uniform vec3 kd;

void main()
{
    // now there are going to be 4 fragments pos, nor, ke, kd
    gl_FragData[0].xyz = vPos;
    gl_FragData[1].xyz = vNor;
    gl_FragData[2].xyz = ke;
    gl_FragData[3].xyz = kd;
}
