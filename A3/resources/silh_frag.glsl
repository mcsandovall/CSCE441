varying vec3 cNor; // in camera space
varying vec3 cPos; // in camera space

void main(){
    // Take the dot product of the normal and eye vectors
    vec3 n, e, color;
    float d;
    n = normalize(cNor);
    e = normalize(-cPos);
    d = dot(n,e);
    if(d < 0.3) color = vec3(0.0);
    else color = vec3(1.0);
    gl_FragColor = vec4(color, 1.0);
}
