//
//  skybox_shader_node.hpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#ifndef _MODULE10_SKYBOX_SHADER_NODE_HPP
#define _MODULE10_SKYBOX_SHADER_NODE_HPP

#include "scene/shader_node.hpp"
#include <iostream>

namespace cg
{

class SkyboxShaderNode : public ShaderNode
{
public:
    SkyboxShaderNode();
    ~SkyboxShaderNode() = default;
    
    // Get shader uniform/attribute locations
    bool get_locations() override;
    
    // Draw
    void draw(SceneState& scene_state) override;
    
    // Set projection and view matrices
    void set_projection_matrix(const Matrix4x4& projection);
    void set_view_matrix(const Matrix4x4& view);
    
    // Get position location for skybox geometry
    int get_position_loc() const;

private:
    int projection_loc_;
    int view_loc_;
    int skybox_loc_;
    int position_loc_;
};

} // namespace cg

#endif
