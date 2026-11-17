//
//  skybox_shader_node.cpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#include "Module10/skybox_shader_node.hpp"

namespace cg
{

SkyboxShaderNode::SkyboxShaderNode()
    : projection_loc_(-1)
    , view_loc_(-1)
    , skybox_loc_(-1)
    , position_loc_(-1)
{
}

bool SkyboxShaderNode::get_locations()
{
    // Get vertex position attribute location
    position_loc_ = glGetAttribLocation(shader_program_.get_program(), "vtx_position");
    if(position_loc_ < 0)
    {
        std::cout << "SkyboxShaderNode: Error getting vtx_position location\n";
        return false;
    }
    
    // Get projection matrix uniform location
    projection_loc_ = glGetUniformLocation(shader_program_.get_program(), "projection");
    if(projection_loc_ < 0)
    {
        std::cout << "SkyboxShaderNode: Error getting projection location\n";
        return false;
    }
    
    // Get view matrix uniform location
    view_loc_ = glGetUniformLocation(shader_program_.get_program(), "view");
    if(view_loc_ < 0)
    {
        std::cout << "SkyboxShaderNode: Error getting view location\n";
        return false;
    }
    
    // Get skybox cubemap sampler uniform location
    skybox_loc_ = glGetUniformLocation(shader_program_.get_program(), "skybox");
    if(skybox_loc_ < 0)
    {
        std::cout << "SkyboxShaderNode: Error getting skybox texture location\n";
        return false;
    }
    
    return true;
}

void SkyboxShaderNode::draw(SceneState& scene_state)
{
    // Enable this shader program
    shader_program_.use();
    
    // Set skybox cubemap sampler
    glUniform1i(skybox_loc_, 0);
    
    // Set position location in scene state for skybox geometry
    scene_state.position_loc = position_loc_;
    
    // Draw all children
    SceneNode::draw(scene_state);
}

void SkyboxShaderNode::set_projection_matrix(const Matrix4x4& projection)
{
    shader_program_.use();
    glUniformMatrix4fv(projection_loc_, 1, GL_FALSE, projection.get());
}

void SkyboxShaderNode::set_view_matrix(const Matrix4x4& view)
{
    shader_program_.use();
    glUniformMatrix4fv(view_loc_, 1, GL_FALSE, view.get());
}

int SkyboxShaderNode::get_position_loc() const
{
    return position_loc_;
}

} // namespace cg
