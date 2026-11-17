//
//  saturn_ring.hpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#ifndef __MODULE10_SATURN_RING_HPP__
#define __MODULE10_SATURN_RING_HPP__

#include "scene/textured_tri_surface.hpp"
#include "geometry/geometry.hpp"

namespace cg
{

/**
 * Saturn Ring - flat disk with hole in center using TexturedTriSurface
 */
class SaturnRing : public TexturedTriSurface
{
public:
    /**
     * Constructor
     * @param inner_radius  Inner radius of ring
     * @param outer_radius  Outer radius of ring
     * @param segments  Number of radial segments (circumferential detail)
     * @param rings  Number of radial rings (radial detail)
     * @param position_loc  Vertex position attribute location
     * @param normal_loc    Vertex normal attribute location
     * @param texture_loc   Texture coordinate attribute location
     * @param double_sided  If true, creates both top and bottom faces
     */
    SaturnRing(float inner_radius,
               float outer_radius,
               uint32_t segments,
               uint32_t rings,
               int32_t position_loc,
               int32_t normal_loc,
               int32_t texture_loc,
               bool double_sided = true);

private:
    /**
     * Creates the ring geometry
     */
    void create_ring(float inner_radius,
                     float outer_radius,
                     uint32_t segments,
                     uint32_t rings,
                     int32_t position_loc,
                     int32_t normal_loc,
                     int32_t texture_loc,
                     bool double_sided);
    
    /**
     * Helper function to calculate vertex index
     */
    uint16_t calculate_index(uint32_t ring, uint32_t seg, uint32_t cols) const;
};

} // namespace cg

#endif
