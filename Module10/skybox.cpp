//
//  skybox.cpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#include "Module10/skybox.hpp"
#include "stb/stb_image.h"
#include "scene/image_data.hpp"

namespace cg
{

Skybox::Skybox()
{
    vao_ = 0;
    vbo_ = 0;
    cubemap_texture_ = 0;
    position_loc_ = 0;
}

Skybox::~Skybox()
{
    if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
    if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
    if (cubemap_texture_ != 0) glDeleteTextures(1, &cubemap_texture_);
}

bool Skybox::load_cubemap(const std::vector<std::string>& faces)
{
    if (faces.size() != 6)
    {
        std::cerr << "Skybox requires exactly 6 texture faces" << std::endl;
        return false;
    }

    glGenTextures(1, &cubemap_texture_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture_);

    // Load each face of the cubemap
    for (GLuint i = 0; i < faces.size(); i++)
    {
        ImageData im_data;
        load_image_data(im_data, faces[i]);
        
        if (im_data.data != nullptr)
        {
            // Load the image data to the cubemap face
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0, GL_RGBA, im_data.w, im_data.h, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, im_data.data);
            
            std::cout << "Loaded skybox face " << i << ": " << faces[i] << std::endl;
            
            // Free the image data
            free_image_data(im_data);
        }
        else
        {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            free_image_data(im_data);
            return false;
        }
    }

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return true;
}

void Skybox::initialize(int32_t position_loc)
{
    position_loc_ = position_loc;
    create_cube_geometry();
}

void Skybox::create_cube_geometry()
{
    // Skybox cube vertices for 6 faces, 2 triangles per face (hard coded positions for simplicity)
    float skybox_vertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // Create VAO and VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    
    glBindVertexArray(vao_);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), &skybox_vertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(position_loc_);
    glVertexAttribPointer(position_loc_, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void Skybox::draw(SceneState& scene_state)
{
    // Save current depth function
    GLint prev_depth_func;
    glGetIntegerv(GL_DEPTH_FUNC, &prev_depth_func);
    
    // Change depth function so depth test passes when values are equal to depth buffer's content
    glDepthFunc(GL_LEQUAL);
    
    // Bind skybox VAO and texture
    glBindVertexArray(vao_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture_);
    
    // Draw skybox
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // Cleanup
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    // Restore original depth function
    glDepthFunc(prev_depth_func);
}

} // namespace cg
