#version 300 es

uniform vec4 objectColor;
uniform sampler2D sampler;

in vec2 vTexcoord0;

out highp vec4 color;

void main() {
    color = texture(sampler, vTexcoord0) * objectColor;
}