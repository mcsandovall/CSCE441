#version 120
uniform vec3 lightPos;

varying vec3 cNor;
varying vec3 cPos;

void main()
{
    // instead write values that depend on the light and on the normal
    vec3 n = normalize(cNor);
    vec3 l = normalize(lightPos - cPos);
    
	gl_FragData[0] = vec4(n, 1.0);
	gl_FragData[1] = vec4(l, 1.0);
}
