#version 120
// phong fragment shader

uniform vec3 lightPos;
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;

//varying vec3 color; // passed from the vertex shader
varying vec3 cNor;
varying vec3 cPos;

void main()
{
    // variable definitions
    vec3 n, l, ca, cd, cs, c, h, e, r;
    
    // compute the normals
    n = normalize(cNor);
    l = normalize(lightPos - cPos);
    e = normalize(-cPos); // e vector
    h = normalize(l + e);
    
    //perform lighting computation
    ca = ka; // final ambient color
    cd = kd * max(0.0,dot(l,n));
    cs = ks * pow(max(0,dot(h,n)), s);
    c = ca + cd + cs;
    gl_FragColor = vec4(c, 1.0);
}
