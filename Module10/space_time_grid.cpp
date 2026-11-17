//
//  space_time_grid.cpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#include "Module10/space_time_grid.hpp"

namespace cg
{

SpaceTimeGrid::SpaceTimeGrid() {}

SpaceTimeGrid::SpaceTimeGrid(uint32_t n, int32_t position_loc, int32_t normal_loc)
{
    // Reset vertex vector
    vertices_.clear();
    
    // Generate grid of vertices
    for (uint32_t j = 0; j <= n; ++j)
    {
        for (uint32_t i = 0; i <= n; ++i)
        {
            float x = static_cast<float>(i) / static_cast<float>(n) - 0.5f;
            float y = static_cast<float>(j) / static_cast<float>(n) - 0.5f;

            VertexAndNormal vtx;
            vtx.vertex.x = x;
            vtx.vertex.y = y;
            vtx.vertex.z = 0.0f;
            vtx.normal = {0.0f, 0.0f, 1.0f};

            vertices_.push_back(vtx);
            original_positions_.push_back(vtx.vertex);
        }
    }

    // Horizontal lines
    for (uint32_t j = 0; j <= n; ++j)
    {
        uint32_t row = j * (n + 1);
        for (uint32_t i = 0; i < n; ++i)
        {
            indices_.push_back(row + i);
            indices_.push_back(row + i + 1);
        }
    }

    // Vertical lines
    for (uint32_t i = 0; i <= n; ++i)
    {
        for (uint32_t j = 0; j < n; ++j)
        {
            uint32_t row1 = j * (n + 1);
            uint32_t row2 = (j + 1) * (n + 1);

            indices_.push_back(row1 + i);
            indices_.push_back(row2 + i);
        }
    }

    // Create vertex buffers
    create_vertex_buffers(position_loc, normal_loc);
}

SpaceTimeGrid::~SpaceTimeGrid()
{
    // Delete vertex buffer objects
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ibo_);
    glDeleteVertexArrays(1, &vao_);
}

void SpaceTimeGrid::draw(SceneState &scene_state)
{
    glBindVertexArray(vao_);
    glDrawElements(GL_LINES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void SpaceTimeGrid::create_vertex_buffers(int32_t position_loc, int32_t normal_loc)
{
    // Generate vertex buffers for the vertex list and the index list
    glGenBuffers(1, &vbo_);
//    glGenBuffers(1, &ibo_);

    // Bind the vertex list to the vertex buffer object
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices_.size() * sizeof(VertexAndNormal),
                 (void *)&vertices_[0],
                 GL_STATIC_DRAW);

    // Allocate a VAO, enable it and set the vertex attribute arrays and pointers
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // Bind the vertex buffer, set the vertex position attribute and the vertex normal attribute
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAndNormal), (void *)0);
    glVertexAttribPointer(
        normal_loc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAndNormal), (void *)(sizeof(Point3)));
    glEnableVertexAttribArray(position_loc);
    glEnableVertexAttribArray(normal_loc);
    
    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices_.size() * sizeof(GLuint),
                 indices_.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void SpaceTimeGrid::update_vertex_buffer()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_.size() * sizeof(VertexAndNormal), vertices_.data());
}

void SpaceTimeGrid::apply_deformation(const std::vector<std::shared_ptr<cg::CelestialBodyTransformNode>>& planets)
{
    if (planets.empty())
        return;

    // Constants that can be tweaked for the simulation
    const float G = 6.67430e-3f;          // Scaled gravitational constant
    const float GRAVITY_SCALE = 0.1f;     // Visual amplifier
    const float EPS = 0.0001f;            // Avoid r = 0 singularity (goes to infinity)

    for (size_t i = 0; i < vertices_.size(); ++i)
    {
        const Vector3& base = original_positions_[i];
        auto& v = vertices_[i];

        float zDeform = 0.0f;

        for (auto& planet : planets)
        {
            Vector3 pos = planet->get_world_pos_cartesian();

            // Convert planet position to grid space [-0.5 to 0.5]
            Vector3 p;
            p.x = pos.x / world_size_;
            p.y = pos.y / world_size_;
            p.z = 0;

            // Distance in grid space
            float dx = base.x - p.x;
            float dy = base.y - p.y;
            float r2 = dx*dx + dy*dy + EPS;

            float mass = planet->get_mass();

            // Gravity formula
            float g = G * mass / r2;

            // Accumulate deformation downward
            zDeform += -g * GRAVITY_SCALE;
        }

        // Set vertex position
        v.vertex.x = base.x;
        v.vertex.y = base.y;
        v.vertex.z = zDeform;
    }

    // Update vertex buffer
    update_vertex_buffer();
}

void SpaceTimeGrid::set_world_size(float scale)
{
    world_size_ = scale;
}

} //namespace cg
