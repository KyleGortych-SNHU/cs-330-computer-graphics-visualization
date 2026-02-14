#version 330 core

// Input attributes (matches ShapeMeshes memory layout: pos, normal, UV)
layout (location = 0) in vec3 inVertexPosition;
layout (location = 1) in vec3 inVertexNormal;
layout (location = 2) in vec2 inTextureCoordinate;

// Outputs to fragment shader
out vec3 fragmentPosition;       // world-space position
out vec3 fragmentVertexNormal;   // world-space normal
out vec2 fragmentTextureCoordinate;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // World-space fragment position
    fragmentPosition = vec3(model * vec4(inVertexPosition, 1.0));

    // Transform normal to world space (use normal matrix for non-uniform scale)
    fragmentVertexNormal = normalize(mat3(transpose(inverse(model))) * inVertexNormal);

    // Pass through texture coordinates
    fragmentTextureCoordinate = inTextureCoordinate;

    // Final clip-space position
    gl_Position = projection * view * vec4(fragmentPosition, 1.0);
}
