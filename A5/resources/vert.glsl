#version 120

uniform mat4 P;
uniform mat4 MV;
uniform float t;
attribute vec4 aPos; // In object space
attribute vec3 aNor; // In object space


varying vec3 vNor; // In camera space

void main()
{
    float x = aPos.x;
    float theta = aPos.y;
    float f = cos(x + t) + 2.0;
    float y = f * cos(theta);
    float z = f * sin(theta);
    vec4 point = vec4(x,y,z,1.0f);
    
    vec3 dpdx = vec3(1, -sin(x + t) * cos(theta), -sin(x + t) * sin(theta));
    vec3 dpdt = vec3(0, -f * sin(theta), f * cos(theta));
    vec3 n = aNor; // use it to not have to discart it
    n = normalize(cross(dpdx, dpdt));

    gl_Position = P * (MV * point);
    vNor = vec3(MV * vec4(n, 0.0)); // Assuming MV contains only translations and rotations

}
