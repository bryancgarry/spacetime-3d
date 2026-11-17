//
//  skybox.hpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#ifndef __MODULE10_SKYBOX_HPP__
#define __MODULE10_SKYBOX_HPP__

#include "scene/scene_node.hpp"

namespace cg
{

class Skybox : public SceneNode
{
public:
    Skybox();
    ~Skybox();

    // Load cubemap from 6 image files
    // Order: right, left, top, bottom, front, back (+X, -X, +Z, -Z, +Y, -Y)
    bool load_cubemap(const std::vector<std::string>& faces);
    
    // Initialize the skybox cube geometry
    void initialize(int32_t position_loc);
    
    // Draw to render skybox with depth handling
    void draw(SceneState& scene_state) override;
    
    // Get cubemap texture ID for binding in shader
    GLuint get_cubemap_texture() const { return cubemap_texture_; }

private:
    GLuint vao_;
    GLuint vbo_;
    GLuint cubemap_texture_;
    int32_t position_loc_;
    
    void create_cube_geometry();
};

} // namespace cg

#endif
