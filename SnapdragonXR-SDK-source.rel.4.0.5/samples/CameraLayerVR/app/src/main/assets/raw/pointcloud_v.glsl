#version 300 es

in vec3 position;

uniform mat4 projMtx;
uniform mat4 viewMtx;
uniform mat4 mdlMtx;

// X: Screen Width
// Y: Screen Height
// Z: 1.0 / Screen Width
// W: 1.0 / Screen Height
uniform vec4 screenSize;

// X: Sphere Width
// Y: Sphere Height
// Z: Not Used
// W: Not Used
uniform vec4 sphereSize;

out vec4 vWorldPos;
out vec4 vEyeSpacePos;

void main()
{
    // Get the world position...
    vWorldPos = mdlMtx * vec4(position.xyz, 1.0);

    // ... and the eye position
    vEyeSpacePos = viewMtx * vWorldPos; 

    // Project the sprite using the desired sprite size
    vec4 ProjSprite = projMtx * vec4(0.5 * sphereSize.x, 0.5 * sphereSize.y, vEyeSpacePos.z, vEyeSpacePos.w);

    // Can calculate sprite size now
    gl_PointSize = screenSize.x * ProjSprite.x / ProjSprite.w;

    // Finish the MVP calculation
    gl_Position = projMtx * vEyeSpacePos;

    // DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! 
    // gl_Position = projMtx * (viewMtx * (mdlMtx * vec4(position, 1.0))); 
    // gl_PointSize = 64.0;
}

