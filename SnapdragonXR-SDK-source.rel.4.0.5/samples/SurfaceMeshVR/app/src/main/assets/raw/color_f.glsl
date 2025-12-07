#version 300 es

uniform vec4 objectColor;

out highp vec4 color;

void main() {
    color = vec4(objectColor);
}
