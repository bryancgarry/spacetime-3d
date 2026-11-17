//
//  skybox_shader.frag
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#version 410 core

// Incoming interpolated texture coordinates
layout (location = 0) smooth in vec3 texture_coords;

// Output fragment color
layout (location = 0) out vec4 frag_color;

// Cubemap texture sampler
uniform samplerCube skybox;

// Skybox fragment shader. Samples the cubemap texture using
// the interpolated position vector as the texture coordinate.
void main()
{
    // Sample the cubemap using the texture coordinates
    frag_color = texture(skybox, texture_coords);
}
