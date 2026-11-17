//
//  space_time_grid.hpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#ifndef __MODULE10_SPACE_TIME_GRID_HPP__
#define __MODULE10_SPACE_TIME_GRID_HPP__

#include "scene/geometry_node.hpp"
#include "Module10/celestial_body_transform_node.hpp"

namespace cg
{

class SpaceTimeGrid : public GeometryNode
{
public:

    // Constructor
    SpaceTimeGrid(uint32_t n, int32_t position_loc, int32_t normal_loc);
    SpaceTimeGrid();
    
    // Destructor
    ~SpaceTimeGrid();
    
    // Draw
    void draw(SceneState &scene_state) override;
        
    // Handle vertex buffer creation and updating
    void create_vertex_buffers(int32_t position_loc, int32_t normal_loc);
    void update_vertex_buffer();
    
    // Handles the deformation of the extruded surface
    void apply_deformation(const std::vector<std::shared_ptr<cg::CelestialBodyTransformNode>>& planets);
    
    // Pass in a variable to keep track of the world size
    void set_world_size(float scale);
    
protected:
    // Vertex buffer support
    GLuint vao_;
    GLuint vbo_;
    GLuint ibo_;
    
    // Vertex and index list
    std::vector<VertexAndNormal> vertices_;
    std::vector<GLuint> indices_;
    
    // Store original positions of all the vertices for reference
    std::vector<Vector3> original_positions_;
    
    // Store world size
    float world_size_ = 1.0f;
};

}

#endif
