//
//  celestial_body_transform_node.cpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#include "Module10/celestial_body_transform_node.hpp"

namespace cg
{

void CelestialBodyTransformNode::translate(float x, float y, float z)
{
    world_pos_ = {x, y, z};
    model_matrix_.translate(x, y, z);
}

void CelestialBodyTransformNode::scale(float scale)
{
    scale_ = scale;
    model_matrix_.scale(scale, scale, scale);
}
void CelestialBodyTransformNode::set_rotation_rate(float rotation)
{
    rotation_rate_ = rotation;
}

void CelestialBodyTransformNode::draw(SceneState &scene_state)
{
    // Copy current transforms onto stack
    scene_state.push_transforms();

    // Apply to the current modeling matrix.
    scene_state.model_matrix *= model_matrix_;
    glUniformMatrix4fv(scene_state.model_matrix_loc, 1, GL_FALSE, scene_state.model_matrix.get());

    // Set the normal transform matrix
    Matrix4x4 normal_matrix = scene_state.model_matrix.get_inverse().transpose();
    glUniformMatrix4fv(scene_state.normal_matrix_loc, 1, GL_FALSE, normal_matrix.get());

    // Set the composite projection, view, modeling matrix
    Matrix4x4 pvm = scene_state.pv * scene_state.model_matrix;
    glUniformMatrix4fv(scene_state.pvm_matrix_loc, 1, GL_FALSE, pvm.get());

    // Draw all children
    SceneNode::draw(scene_state);

    // Pop matrix stack to revert to prior matrices
    scene_state.pop_transforms();
}

void CelestialBodyTransformNode::update(SceneState &scene_state)
{
    // Advance orbital angle
    orbit_angle_ += orbit_speed_ * scene_state.delta_time;
    float rad = degrees_to_radians(orbit_angle_);
    
    orientiation_ += rotation_rate_ * scene_state.delta_time;
    if (orientiation_ > 360.0f) orientiation_ -= 360.0f;
    if (orientiation_ < 0.0f) orientiation_ += 360.0f;

    // Compute new orbital position
    float x = orbit_center_.x + orbit_radius_ * std::cos(rad);
    float y = orbit_center_.y + orbit_radius_ * std::sin(rad);
    float z = orbit_center_.z;

    // Set identity matrix
    model_matrix_.set_identity();
    
    // Apply TRS
    model_matrix_.translate(x, y, z);
    model_matrix_.rotate_z(orientiation_);
    model_matrix_.scale(scale_, scale_, scale_);

    // Update world position
    world_pos_ = {x, y, z};
}

void CelestialBodyTransformNode::set_orbit_center(Vector3 center)
{
    orbit_center_ = center;
}

void CelestialBodyTransformNode::set_orbit_radius(float radius)
{
    orbit_radius_ = radius;
}

void CelestialBodyTransformNode::set_orbit_angle(float degrees)
{
    orbit_angle_ = degrees;
}

void CelestialBodyTransformNode::set_orbit_speed(float speed)
{
    orbit_speed_ = speed;
}

void CelestialBodyTransformNode::set_orbit_parameters(Vector3 center, float radius, float degrees, float speed)
{
    orbit_center_ = center;
    orbit_radius_ = radius;
    orbit_angle_ = degrees;
    orbit_speed_ = speed;
    
    // Convert angle to radians
    float rad = degrees_to_radians(degrees);

    // Compute world position on orbit
    float x = center.x + radius * std::cos(rad);
    float y = center.y + radius * std::sin(rad);
    float z = center.z;

    // Set position
    set_world_pos(Vector3(x, y, z));
}

void CelestialBodyTransformNode::set_mass(float mass)
{
    mass_ = mass;
}

float CelestialBodyTransformNode::get_mass()
{
    return mass_;
}

Vector3 CelestialBodyTransformNode::polar_to_cartesian(float radius, float degrees)
{
    // Convert degrees to radians
    float rad = degrees_to_radians(degrees);
    
    // Calculate sin and cos
    float c = std::cos(rad);
    float s = std::sin(rad);

    Vector3 pos;
    pos.x = radius * c;
    pos.y = radius * s;
    pos.z = 0.0f; // Default z to 0 so it lies in x-y plane
    
    return pos;
}

Vector2 CelestialBodyTransformNode::cartesian_to_polar(Vector3 pos)
{
    float radius = sqrt(pos.x * pos.x + pos.y * pos.y);
    float degrees = radians_to_degrees(atan2(pos.y, pos.x));
    return Vector2(radius, degrees);
}


void CelestialBodyTransformNode::set_world_pos(Vector3 pos)
{
    translate(pos.x, pos.y, pos.z);
}

void CelestialBodyTransformNode::set_world_pos(float radius, float degrees)
{
    Vector3 pos = polar_to_cartesian(radius, degrees);
    translate(pos.x, pos.y, pos.z);
}

Vector3 CelestialBodyTransformNode::get_world_pos_cartesian()
{
    return world_pos_;
}

Vector3 CelestialBodyTransformNode::get_world_pos_polar()
{
    return Vector3();
}

} // namespace cg
