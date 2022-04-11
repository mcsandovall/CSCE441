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

// vectors to compute the colors
uniform vec3 lightsPos[10];
uniform vec3 lightsColors[10];

void main()
{
    // change the fragment color
    vec3 fragColor = ka;
    
    float Az = 1.0f, Ao = 0.0429, At = 0.9857;
    for(int i = 0; i < 10; ++i){
        vec3 n = normalize(cNor);
        vec3 e = normalize(-cPos); // e vector
        vec3 l = normalize(lightsPos[i] - cPos);
        vec3 h = normalize(l + e);
        float diffuse = max(0.0,dot(l,n));
        float specular = pow(max(0,dot(h,n)), s);
        vec3 color = lightsColors[i] * (kd * diffuse + ks * specular);
        float r = distance(lightsPos[i], cPos);
        float attenuation = 1.0 / (Az + (Ao * r) + (At * pow(r,2)));
        fragColor += color * attenuation;
    }
    
    gl_FragColor = vec4(fragColor, 1.0);
}
