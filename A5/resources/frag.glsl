#version 120

uniform sampler2D texture0;

varying vec3 vNor; // Unused

void main()
{
    vec3 n = normalize(vNor);
    gl_FragColor = vec4(n,1.0);
}
