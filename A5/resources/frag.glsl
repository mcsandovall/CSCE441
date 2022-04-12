#version 120
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;

varying vec3 cNor; // Unused
varying vec3 cPos;

uniform vec3 lightsPos[10];
uniform vec3 lightsColors[10];

void main()
{
    vec3 n = normalize(cNor);
    gl_FragColor = vec4(n,1.0);
}
