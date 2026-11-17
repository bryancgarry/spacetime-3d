//
//  skybox_shader.vert
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#version 410 core

// Incoming vertex position attribute
layout (location = 0) in vec3 vtx_position;

// Outgoing texture coordinate
layout (location = 0) smooth out vec3 texture_coords;

// Uniforms for matrices
uniform mat4 projection;  // Projection matrix
uniform mat4 view;        // View matrix

// Skybox vertex shader. The skybox is rendered as a large cube centered
// at the camera position. Removed translation from the view matrix so
// the skybox appears infinitely far away and follows camera rotation only.
void main()
{
    // Pass position as texture coordinates for cubemap sampling
    texture_coords = vtx_position;
    
    // Trick to remove translation from view matrix by converting to mat3 and back to mat4
    // This keeps only the rotation component of the view matrix
    mat4 view_no_translation = mat4(mat3(view));
    
    // Transform vertex position using modified view and projection matrices
    vec4 pos = projection * view_no_translation * vec4(vtx_position, 1.0);
    
    // Set z to w so that z/w will always be 1.0 (maximum depth value)
    // This ensures the skybox is always rendered behind all other geometry
    gl_Position = pos.xyww;
}
