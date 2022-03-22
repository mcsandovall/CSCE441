#version 120

uniform vec3 lightPos;
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;
uniform vec3 light0Pos;
uniform vec3 light1Pos;
uniform vec3 light0Color;
uniform vec3 light1Color;

varying vec3 cNor; // in camera space
varying vec3 cPos; // in camera space

vec3 set_color(vec3 color){
    vec3 _color = vec3(0.0);
    for(int i = 0; i < 3; ++i){
        if(color[i] < 0.25){
            _color[i] = 0.0;
        }
        if(color[i] < 0.5){
            _color[i] = color[i] * 0.25;
        }
        if(color[i] < 0.75){
            _color[i] = color[i] * 0.5;
        }else{
            _color[i] = 1.0;
        }
    }
    return _color;
}

vec3 CalculateColor(vec3 _lightPos, vec3 _lightColor, vec3 _cNor, vec3 _cPos, vec3 _ka, vec3 _kd, vec3 _ks, float _s){
    // variable definitions
    vec3 n, l, ca, cd, cs, c, h, e;
    
    // compute the normals
    n = normalize(_cNor);
    l = normalize(_lightPos - _cPos);
    e = normalize(- _cPos); // e vector
    h = normalize(l + e);
    
    //perform lighting computation
    ca = ka; // final ambient color
    cd = kd * max(0.0,dot(l,n));
    cs = ks * pow(max(0,dot(h,n)), _s);
    c = ca + cd + cs;
    
    return (_lightColor * c);
}

void main(){
    vec3 n, l, ca, cd, cs, c, h, e, color;
    float d;
    
    // compute the normals
    n = normalize(cNor);
    l = normalize(lightPos - cPos);
    e = normalize(- cPos); // e vector
    h = normalize(l + e);
    
    //perform lighting computation
    ca = ka; // final ambient color
    cd = kd * max(0.0,dot(l,n));
    cs = ks * pow(max(0,dot(h,n)), s);
    c = ca + cd + cs;
    
    color = CalculateColor(light0Pos,light0Color,cNor,cPos,ka,kd,ks,s);
    color += CalculateColor(light1Pos,light1Color,cNor,cPos,ka,kd,ks,s);
    c = set_color(color);
    
    gl_FragColor = vec4(c, 1.0);
}
