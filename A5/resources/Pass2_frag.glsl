#version 120
uniform sampler2D posTexture;
uniform sampler2D norTexture;
uniform sampler2D keTexture;
uniform sampler2D kdTexture;
uniform vec2 windowSize;
    
uniform vec3 lightsPos[10];
uniform vec3 lightsColors[10];

void main()
{
    vec2 tex;
    tex.x = gl_FragCoord.x/windowSize.x;
    tex.y = gl_FragCoord.y/windowSize.y;
    
    // Fetch shading data
   vec3 pos = texture2D(posTexture, tex).rgb;
   vec3 nor = texture2D(norTexture, tex).rgb;
   vec3 ke = texture2D(keTexture, tex).rgb;
   vec3 kd = texture2D(kdTexture, tex).rgb;
   
   gl_FragColor.rgb = pos;
}
