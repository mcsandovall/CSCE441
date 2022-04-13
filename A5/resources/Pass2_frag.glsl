#version 120
uniform sampler2D textureA;
uniform sampler2D textureB;

uniform float t;

varying vec2 vTex;
    
void main()
{
    vec4 texture;
    
   float n = mod(t,10.0f);
   float f = 5.0f;
   if(n < f){
       texture = texture2D(textureA, vTex);
   }else{
       texture = texture2D(textureB, vTex);
   }
    
    gl_FragColor.rgb = texture.rgb;
}
