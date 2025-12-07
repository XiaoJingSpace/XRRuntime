#version 300 es

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 texcoord0;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPosition;

out vec3 viewSpaceVertex;
out vec3 viewSpaceNormal;
out vec3 viewSpaceLightDirection;
out vec2 vTexcoord0;

void main() {
    mat4 modelView = view * model;
    viewSpaceVertex = (modelView * vec4(vertex, 1.0)).xyz;
    // work for uniform scale only
    viewSpaceNormal = (modelView * vec4(normal, 0.0)).xyz;
    vec3 viewLightPos = (view * vec4(lightPosition, 1.0)).xyz;
    viewSpaceLightDirection = viewLightPos - viewSpaceVertex;
    gl_Position = projection * modelView * vec4(vertex, 1.0);

    vTexcoord0 = texcoord0;
}
