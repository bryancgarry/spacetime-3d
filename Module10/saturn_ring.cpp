//
//  saturn_ring.cpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#include "Module10/saturn_ring.hpp"

namespace cg
{

SaturnRing::SaturnRing(float inner_radius,
                       float outer_radius,
                       uint32_t segments,
                       uint32_t rings,
                       int32_t position_loc,
                       int32_t normal_loc,
                       int32_t texture_loc,
                       bool double_sided)
{
    create_ring(inner_radius, outer_radius, segments, rings, position_loc, normal_loc, texture_loc, double_sided);
}

void SaturnRing::create_ring(float inner_radius,
                             float outer_radius,
                             uint32_t segments,
                             uint32_t rings,
                             int32_t position_loc,
                             int32_t normal_loc,
                             int32_t texture_loc,
                             bool double_sided)
{
    std::vector<PNTVertex> vertex_list;
    std::vector<uint16_t> face_list;
    
    float angle_step = 2.0f * M_PI / segments;
    float radius_step = (outer_radius - inner_radius) / rings;
    
    // Generate vertices for top side
    for (uint32_t ring = 0; ring <= rings; ring++)
    {
        float radius = inner_radius + ring * radius_step;
        float u = (float)ring / (float)rings;  // 0.0 at inner, 1.0 at outer
        
        for (uint32_t seg = 0; seg <= segments; seg++)
        {
            float angle = seg * angle_step;
            float cos_a = std::cos(angle);
            float sin_a = std::sin(angle);
            float v = (float)seg / (float)segments;  // 0.0 to 1.0 around circle
            
            PNTVertex vertex;
            
            // Position
            vertex.vertex.x = radius * cos_a;
            vertex.vertex.y = radius * sin_a;
            vertex.vertex.z = 0.0f;
            
            // Normal (pointing up for top face)
            vertex.normal.x = 0.0f;
            vertex.normal.y = 0.0f;
            vertex.normal.z = 1.0f;
            
            // Texture coordinates
            vertex.texture.x = u;  // Radial direction
            vertex.texture.y = v;  // Circumferential direction
            
            vertex_list.push_back(vertex);
        }
    }
    
    // Generate face indices for top side
    uint32_t top_cols = segments + 1;
    
    for (uint32_t ring = 0; ring < rings; ring++)
    {
        for (uint32_t seg = 0; seg < segments; seg++)
        {
            uint16_t v0 = calculate_index(ring, seg, top_cols);
            uint16_t v1 = calculate_index(ring + 1, seg, top_cols);
            uint16_t v2 = calculate_index(ring, seg + 1, top_cols);
            uint16_t v3 = calculate_index(ring + 1, seg + 1, top_cols);
            
            // First triangle (counter-clockwise from top)
            face_list.push_back(v0);
            face_list.push_back(v1);
            face_list.push_back(v2);
            
            // Second triangle
            face_list.push_back(v1);
            face_list.push_back(v3);
            face_list.push_back(v2);
        }
    }
    
    // If double-sided, add bottom face
    if (double_sided)
    {
        uint32_t bottom_vertex_offset = static_cast<uint32_t>(vertex_list.size());
        
        // Generate vertices for bottom side (same positions, opposite normals)
        for (uint32_t ring = 0; ring <= rings; ring++)
        {
            float radius = inner_radius + ring * radius_step;
            float u = (float)ring / (float)rings;
            
            for (uint32_t seg = 0; seg <= segments; seg++)
            {
                float angle = seg * angle_step;
                float cos_a = std::cos(angle);
                float sin_a = std::sin(angle);
                float v = (float)seg / (float)segments;
                
                PNTVertex vertex;
                
                // Position (same as top)
                vertex.vertex.x = radius * cos_a;
                vertex.vertex.y = radius * sin_a;
                vertex.vertex.z = 0.0f;
                
                // Normal (pointing down for bottom face)
                vertex.normal.x = 0.0f;
                vertex.normal.y = 0.0f;
                vertex.normal.z = -1.0f;
                
                // Texture coordinates
                vertex.texture.x = u;
                vertex.texture.y = v;
                
                vertex_list.push_back(vertex);
            }
        }
        
        // Generate face indices for bottom side (reversed winding)
        for (uint32_t ring = 0; ring < rings; ring++)
        {
            for (uint32_t seg = 0; seg < segments; seg++)
            {
                uint16_t v0 = bottom_vertex_offset + calculate_index(ring, seg, top_cols);
                uint16_t v1 = bottom_vertex_offset + calculate_index(ring + 1, seg, top_cols);
                uint16_t v2 = bottom_vertex_offset + calculate_index(ring, seg + 1, top_cols);
                uint16_t v3 = bottom_vertex_offset + calculate_index(ring + 1, seg + 1, top_cols);
                
                // First triangle (clockwise from bottom, which is CCW when viewed from below)
                face_list.push_back(v0);
                face_list.push_back(v2);
                face_list.push_back(v1);
                
                // Second triangle
                face_list.push_back(v1);
                face_list.push_back(v2);
                face_list.push_back(v3);
            }
        }
    }
    
    // Construct the surface
    construct(vertex_list, face_list, position_loc, normal_loc, texture_loc);
}

uint16_t SaturnRing::calculate_index(uint32_t ring, uint32_t seg, uint32_t cols) const
{
    return static_cast<uint16_t>(ring * cols + seg);
}

} // namespace cg
