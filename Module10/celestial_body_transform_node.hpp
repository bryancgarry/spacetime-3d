//
//  celestial_body_transform_node.hpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#ifndef __MODULE10_CELESTIAL_BODY_TRANSFORM_NODE_HPP__
#define __MODULE10_CELESTIAL_BODY_TRANSFORM_NODE_HPP__

#include "scene/scene_node.hpp"
#include "scene/transform_node.hpp"
#include "geometry/geometry.hpp"

namespace cg
{

/**
 * Transform node. Applies a transformation. This class allows OpenGL style
 * transforms applied to the scene graph.
 */
class CelestialBodyTransformNode : public TransformNode
{
  public:
    
    // Updated transformation methods
    void translate(float x, float y, float z);
    void scale(float scale);
    void set_rotation_rate(float rotation);

    /**
     * Draw this transformation node and its children
     * @param  scene_state   Current scene state
     */
    void draw(SceneState &scene_state) override;

    /**
     * Update the scene node and its children
     * @param  scene_state   Current scene state
     */
    void update(SceneState &scene_state) override;
    
    // Orbit functions
    void updateOrbit(float deltaTime, const Vector3& orbitCenter);
    
    // Setters for orbit paramters
    void set_orbit_center(Vector3 center);
    void set_orbit_radius(float radius);
    void set_orbit_angle(float degrees);
    void set_orbit_speed(float speed);
    void set_orbit_parameters(Vector3 center, float radius, float degrees, float speed);
    
    // Mass methods
    void set_mass(float mass);
    float get_mass();
    
    // Coordinate conversions and functions
    Vector3 polar_to_cartesian(float radius, float degrees);
    Vector2 cartesian_to_polar(Vector3 pos);
    
    // World position methods
    void set_world_pos(Vector3 pos);
    void set_world_pos(float radius, float degrees);
    Vector3 get_world_pos_cartesian();
    Vector3 get_world_pos_polar();

  protected:
    Vector3 world_pos_;     // World coordinates
    float scale_;           // Scale
    float rotation_rate_;   // Rotation rate
    float orientiation_ = 0.0f;
    
    Vector3 orbit_center_;  // Center of orbital path
    float orbit_radius_;    // Distance from orbital center
    float orbit_angle_;     // Current position along orbit (held in DEGREES)
    float orbit_speed_;     // Orbital rate
    
    float mass_;            // Celestial Body Mass
};

} // namespace cg

#endif
