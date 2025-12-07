#version 300 es

uniform vec3 lightColor;
uniform vec4 objectColor;

in vec3 viewSpaceVertex;
in vec3 viewSpaceNormal;
in vec3 viewSpaceLightDirection;

out highp vec4 color;

void main() {
    vec3 normal = normalize(viewSpaceNormal);
    vec3 lightDir = normalize(viewSpaceLightDirection);

    vec3 ambient = vec3(0.1, 0.1, 0.1) * lightColor;
    float diffuse = clamp(dot(normal, lightDir), 0.0, 1.0);
    float specular = 0.0;
    if (diffuse > 0.0)
    {
        vec3 cameraDir = normalize(vec3(0, 0, 0) - viewSpaceVertex);
        vec3 reflectDir = reflect(-lightDir, normal);
        specular = 0.5 * pow(max(dot(cameraDir, reflectDir), 0.0), 32.0);
    }
    color = vec4((ambient + diffuse * lightColor + specular * lightColor), 1.0)* objectColor;
}
