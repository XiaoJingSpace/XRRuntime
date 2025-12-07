#version 300 es

in vec4 vWorldPos;
in vec4 vEyeSpacePos;

uniform vec4 pointColor;

out highp vec4 outColor;

void main()
{
    // Need the eye-space sphere normal from texture (gl_PointCoord) coordinates
    vec3 WorldNorm;
    
    WorldNorm.xy = gl_PointCoord.xy * 2.0 - 1.0;
    float R2 = dot(WorldNorm.xy, WorldNorm.xy);
    if(R2 > 1.0)
        discard;

    // The Y-Component is inverted because gl_PointCoord
    // If GL_POINT_SPRITE_COORD_ORIGIN is GL_LOWER_LEFT, gl_PointCoord.t varies from 0.0 to 1.0 vertically from bottom to top. 
    // Otherwise, if GL_POINT_SPRITE_COORD_ORIGIN is GL_UPPER_LEFT then gl_PointCoord.t varies from 0.0 to 1.0 vertically from top to bottom. 
    // The default value of GL_POINT_SPRITE_COORD_ORIGIN is GL_UPPER_LEFT.
    WorldNorm.y *= -1.0;
    
    // Need Z-Component of normal
    WorldNorm.z = sqrt(1.0 - R2);

    // Simple lighting to make it look like a sphere
    float diffuse = max(0.2, dot(WorldNorm.xyz, vec3(0.0, 0.0, 1.0)));

    outColor = vec4( diffuse * pointColor.xyz, pointColor.w);

    // DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! DEBUG! 
    // outColor = vec4(WorldNorm.xyz, 1.0);
}
