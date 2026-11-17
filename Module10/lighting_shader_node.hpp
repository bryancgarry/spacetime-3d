//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	David W. Nesbitt
//	File:    lighting_shader_node.hpp
//	Purpose: Derived class to handle the lighting shader program.
//
//============================================================================

#ifndef __MODULE10_LIGHTING_SHADER_NODE_HPP__
#define __MODULE10_LIGHTING_SHADER_NODE_HPP__

#include "scene/color4.hpp"
#include "scene/shader_node.hpp"

namespace cg
{

/**
 * Simple lighting shader node.
 */
class LightingShaderNode : public ShaderNode
{
  public:
    /**
     * Gets uniform and attribute locations.
     */
    bool get_locations() override;

    /**
     * Draw method for this shader - enable the program and set up uniforms
     * and vertex attribute locations
     * @param  scene_state   Current scene state.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Set the global ambient lighting property. This sets uniforms in the
     * shader directly.
     * @param  global_ambient  Color/intensity of global ambient lighting.
     */
    void set_global_ambient(const Color4 &global_ambient);

    /**
     * Get the location of the vertex position attribute.
     * @return  Returns the vertex position attribute location.
     */
    int32_t get_position_loc() const;

    /**
     * Get the location of the vertex normal attribute.
     * @return  Returns the vertex normal attribute location.
     */
    int32_t get_normal_loc() const;

    /**
     * Get the location of the vertex texture attribute.
     * @return  Returns the vertex texture attribute location.
     */
    int32_t get_texture_loc() const;
    
    /**
     * Set the sun position for day/night texture calculations
     */
    void set_sun_position(Vector3 pos);

  protected:
    // Uniform and attribute locations:
    GLint position_loc_;       // Vertex position attribute location
    GLint vertex_normal_loc_;  // Vertex normal attribute location
    GLint texture_loc_;        // Vertex texture attribute location
    GLint pvm_matrix_loc_;     // Composite projection, view, model matrix location
    GLint model_matrix_loc_;   // Modeling composite matrix location
    GLint normal_matrix_loc_;  // Normal transformation matrix location
    GLint camera_position_loc; // Camera position uniform location

    // Material uniform locations
    GLint material_ambient_loc_;   // Material ambient location
    GLint material_diffuse_loc_;   // Material diffuse location
    GLint material_specular_loc_;  // Material specular location
    GLint material_emission_loc_;  // Material emission location
    GLint material_shininess_loc_; // Material shininess location

    // Texture uniform locations
    GLint use_texture_loc_;  // Texture use flag location
    GLint texture_unit_loc_; // Texture unit location
    GLint texture_unit_2_loc_;
    GLint texture_unit_3_loc_;
    GLint sun_position_loc_;

    // Lighting uniforms
    int32_t       light_count_;        // Number of lights
    GLint         light_count_loc_;    // Light count uniform locations
    GLint         global_ambient_loc_; // Global ambient uniform location
    LightUniforms lights_[3];          // Light source uniform locations
};

} // namespace cg

#endif
