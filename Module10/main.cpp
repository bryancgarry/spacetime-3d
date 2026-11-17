//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    Module10/main.cpp
//	Purpose: OpenGL and SDL program to draw a simple 3-D scene. It starts
//           with some simple object modeling and representation, adds camera
//           and projection controls, adds lighting and shading, then adds
//           texture mapping.
//
//============================================================================

#include "filesystem_support/file_locator.hpp"
#include "geometry/geometry.hpp"
#include "scene/graphics.hpp"
#include "scene/scene.hpp"

#include "Module10/lighting_shader_node.hpp"
#include "Module10/shader_src.hpp"
#include "Module10/space_time_grid.hpp"
#include "Module10/celestial_body_transform_node.hpp"
#include "Module10/skybox.hpp"
#include "Module10/skybox_shader_node.hpp"
#include "Module10/saturn_ring.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

namespace cg
{

// Simple logging function, should be defined in the cg namespace
void logmsg(const char *message, ...)
{
    // Open file if not already opened
    static FILE *lfile = NULL;
    if(lfile == NULL) { lfile = fopen("Module10.log", "w"); }

    va_list arg;
    va_start(arg, message);
    vfprintf(lfile, message, arg);
    putc('\n', lfile);
    fflush(lfile);
    va_end(arg);
}

} // namespace cg

// SDL Objects
SDL_Window       *g_sdl_window = nullptr;
SDL_GLContext     g_gl_context;
constexpr int32_t DRAWS_PER_SECOND = 72;
constexpr int32_t DRAW_INTERVAL_MILLIS =
    static_cast<int32_t>(1000.0 / static_cast<double>(DRAWS_PER_SECOND));

// Root of the scene graph and scene state
std::shared_ptr<cg::SceneNode> g_scene_root;

// Global camera node (so we can change view)
std::shared_ptr<cg::CameraNode> g_camera;

// Camera orbit controls
bool orbit_mode = false;
int orbit_target_index = -1;  // -1 is no target, 0-8 for celestial bodies
float orbit_distance = 100.0f;
float orbit_azimuth = 0.0f;
float orbit_elevation = 30.0f;
int last_mouse_x = 0;
int last_mouse_y = 0;
bool mouse_dragging = false;

// Keep the spotlight global so we can update its poisition
std::shared_ptr<cg::LightNode> Spotlight;

cg::SceneState g_scene_state;

// While mouse button is down, the view will be updated
bool    g_animate = false;
bool    g_forward = true;
float   g_velocity = 1.0f;
int32_t g_mouse_x = 0;
int32_t g_mouse_y = 0;
int32_t g_render_width = 800;
int32_t g_render_height = 600;

// Global space time grid / planet transforms and masses
std::shared_ptr<cg::SpaceTimeGrid> g_space_time_grid;
std::vector<std::shared_ptr<cg::CelestialBodyTransformNode>> g_planet_transforms;
std::vector<float> g_planet_masses;

// Skybox
std::shared_ptr<cg::SkyboxShaderNode> g_skybox_shader;
std::shared_ptr<cg::Skybox> g_skybox;

// Pause flag to stop motion
bool pause = false;

// Sleep function to help run a reasonable timer
void sleep(int32_t milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

/**
 * Display callback. Clears the prior scene and draws a new one.
 */
void display()
{
    // Clear the framebuffer and the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Init scene state and draw the scene graph
    g_scene_state.init();
    
    // Set skybox matrices from camera before drawing
    cg::Matrix4x4 projection = g_camera->get_projection_matrix();
    cg::Matrix4x4 view = g_camera->get_view_matrix();
    
    g_skybox_shader->set_projection_matrix(projection);
    g_skybox_shader->set_view_matrix(view);
    
    g_scene_root->draw(g_scene_state);

    // Swap buffers
    SDL_GL_SwapWindow(g_sdl_window);
}

/**
 * Reshape callback. Update projection to reflect new aspect ratio.
 * @param  width  Window width
 * @param  height Window height
 */
void reshape(int32_t width, int32_t height)
{
    g_render_width = width;
    g_render_height = height;

    // Reset the viewport
    glViewport(0, 0, width, height);

    // Reset the perspective projection to reflect the change of aspect ratio
    // Make sure we cast to float so we get a fractional aspect ratio.
    g_camera->change_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
}

void update_spotlight()
{
    cg::Point3 pos = g_camera->get_position();
    Spotlight->set_position(cg::HPoint3(pos.x, pos.y, pos.z, 1.0f));
    cg::Vector3 dir = g_camera->get_view_plane_normal() * -1.0f;
    Spotlight->set_spotlight_direction(dir);
}

/**
 * Updates the view given the mouse position and whether to move
 * forward or backward.
 * @param  x        Window x position.
 * @param  y        Window y position.
 * @param  forward  Forward flag (true if moving forward).
 */
void update_view(int32_t x, int32_t y, bool forward)
{
    // Find relative dx and dy relative to center of the window
    float dx = 4.0f * ((x - (static_cast<float>(g_render_width * 0.5f))) /
                       static_cast<float>(g_render_width));
    float dy = 4.0f * (((static_cast<float>(g_render_height * 0.5f) - y)) /
                       static_cast<float>(g_render_height));
    float dz = (forward) ? g_velocity : -g_velocity;
    g_camera->move_and_turn(dx * g_velocity, dy * g_velocity, dz);
    update_spotlight();
}

// Update camera in orbit mode
void update_orbit_camera()
{
    if (orbit_target_index < 0 || orbit_target_index >= g_planet_transforms.size())
    {
        return;
    }
    
    // Get the target planet's position
    auto target_pos = g_planet_transforms[orbit_target_index]->get_world_pos_cartesian();
    
    // Convert spherical coordinates to cartesian coordinate offset
    float azimuth_rad = cg::degrees_to_radians(orbit_azimuth);
    float elevation_rad = cg::degrees_to_radians(orbit_elevation);
    
    float x_offset = orbit_distance * cos(elevation_rad) * sin(azimuth_rad);
    float y_offset = orbit_distance * cos(elevation_rad) * cos(azimuth_rad);
    float z_offset = orbit_distance * sin(elevation_rad);
    
    // Set camera position and look at target
    cg::Point3 cam_pos(target_pos.x + x_offset, target_pos.y + y_offset, target_pos.z + z_offset);
    
    // Update camera position, loot at point, and view up vector
    g_camera->set_position(cam_pos);
    g_camera->set_look_at_pt(cg::Point3(target_pos.x, target_pos.y, target_pos.z));
    g_camera->set_view_up(cg::Vector3(0.0f, 0.0f, 1.0f));
}

// Modified mouse event handler
void handle_mouse_event(const SDL_Event &event)
{
    if (orbit_mode)
    {
        // In orbit mode, left click starts/stops dragging
        if (event.button.button == 1)
        {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                mouse_dragging = true;
                last_mouse_x = event.button.x;
                last_mouse_y = event.button.y;
            }
            else
            {
                mouse_dragging = false;
            }
        }
    }
    else
    {
        // Original free-flight controls we have used before
        switch(event.button.button)
        {
            case 1:
                g_forward = true;
                if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) g_animate = true;
                else g_animate = false;
                break;
            case 3:
                g_forward = false;
                if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) g_animate = true;
                else g_animate = false;
                break;
        }
    }
}

// Modified mouse motion handler
void handle_mouse_motion_event(const SDL_Event &event)
{
    if (orbit_mode && mouse_dragging)
    {
        // Calculate mouse delta
        int dx = event.motion.x - last_mouse_x;
        int dy = event.motion.y - last_mouse_y;
        
        // Update orbit angles
        orbit_azimuth += dx * 0.3f;  // Adjust sensitivity as needed
        orbit_elevation += dy * 0.3f;
        
        // Clamp elevation to prevent flipping
        if (orbit_elevation > 89.0f) orbit_elevation = 89.0f;
        if (orbit_elevation < -89.0f) orbit_elevation = -89.0f;
        
        // Update camera
        update_orbit_camera();
        
        // Store current position
        last_mouse_x = event.motion.x;
        last_mouse_y = event.motion.y;
    }
    else
    {
        // Normal free-flight controls we have used before
        g_mouse_x = event.motion.x;
        g_mouse_y = event.motion.y;
    }
}

// Add mouse wheel handler for zooming in and out when in orbit mode
void handle_mouse_wheel_event(const SDL_Event &event)
{
    if (orbit_mode)
    {
        // Zoom in/out with mouse wheel
        if (event.wheel.y > 0)  // Scroll up
        {
            orbit_distance *= 0.9f;
            if (orbit_distance < 10.0f) orbit_distance = 10.0f;
        }
        else if (event.wheel.y < 0)  // Scroll down
        {
            orbit_distance *= 1.1f;
            if (orbit_distance > 2000.0f) orbit_distance = 2000.0f;
        }
        update_orbit_camera();
    }
}

/**
 * Window event handler.
 */
bool handle_window_event(const SDL_Event &event)
{
    bool cont_program = true;

    switch(event.type)
    {
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            reshape(event.window.data1, event.window.data2);
            break;
        default: break;
    }

    return cont_program;
}

// Modified keyboard input handling
bool handle_key_event(const SDL_Event &event)
{
    bool cont_program = true;
    bool upper_case = (event.key.mod & SDL_KMOD_SHIFT) || (event.key.mod & SDL_KMOD_CAPS);

    switch(event.key.key)
    {
        case SDLK_ESCAPE: cont_program = false; break;

        // Reset the view and exit orbit mode
        case SDLK_I:
            g_camera->set_position(cg::Point3(0.0f, -600.0f, 200.0f));
            g_camera->set_look_at_pt(cg::Point3(0.0f, 0.0f, 00.0f));
            g_camera->set_view_up(cg::Vector3(0.0f, 0.0f, 1.0f));
            orbit_mode = false;
            orbit_target_index = -1;
            break;
        
        // Number keys 0-8 for planet focus
        case SDLK_0:
            orbit_mode = true;
            orbit_target_index = 0;  // Sun
            orbit_distance = 200.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;
            
        case SDLK_1:
            orbit_mode = true;
            orbit_target_index = 1;  // Mercury
            orbit_distance = 50.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;
            
        case SDLK_2:
            orbit_mode = true;
            orbit_target_index = 2;  // Venus
            orbit_distance = 50.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;
            
        case SDLK_3:
            orbit_mode = true;
            orbit_target_index = 3;  // Earth
            orbit_distance = 50.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;
            
        case SDLK_4:
            orbit_mode = true;
            orbit_target_index = 4;  // Mars
            orbit_distance = 50.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;
            
        case SDLK_5:
            orbit_mode = true;
            orbit_target_index = 5;  // Jupiter
            orbit_distance = 80.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;
            
        case SDLK_6:
            orbit_mode = true;
            orbit_target_index = 6;  // Saturn
            orbit_distance = 80.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;
            
        case SDLK_7:
            orbit_mode = true;
            orbit_target_index = 7;  // Uranus
            orbit_distance = 60.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;
            
        case SDLK_8:
            orbit_mode = true;
            orbit_target_index = 8;  // Neptune
            orbit_distance = 60.0f;
            orbit_azimuth = 0.0f;
            orbit_elevation = 30.0f;
            update_orbit_camera();
            break;

        // Roll the camera (only works in free-flight mode)
        case SDLK_R:
            if (!orbit_mode)
            {
                if(upper_case) g_camera->roll(-5);
                else g_camera->roll(5);
                update_spotlight();
            }
            break;

        // Pause motion in the simulation
        case SDLK_P:
            if (upper_case) pause = false;
            else pause = true;
            break;

        // Change the heading (only works in free-flight mode)
        case SDLK_H:
            if (!orbit_mode)
            {
                if(upper_case) g_camera->heading(-5);
                else g_camera->heading(5);
                update_spotlight();
            }
            break;

        // Go faster/slower (only works in free-flight mode)
        case SDLK_V:
            if (!orbit_mode)
            {
                if(upper_case) g_velocity += 0.2f;
                else
                {
                    g_velocity -= 0.2f;
                    if(g_velocity < 0.2f) g_velocity = 0.1f;
                }
            }
            break;

        // Slide right/left (only works in free-flight mode)
        case SDLK_X:
            if (!orbit_mode)
            {
                if(upper_case) g_camera->slide(5.0f, 0.0f, 0.0f);
                else g_camera->slide(-5.0f, 0.0f, 0.0f);
                update_spotlight();
            }
            break;

        // Slide up/down (only works in free-flight mode)
        case SDLK_Y:
            if (!orbit_mode)
            {
                if(upper_case) g_camera->slide(0.0f, 5.0f, 0.0f);
                else g_camera->slide(0.0f, -5.0f, 0.0f);
                update_spotlight();
            }
            break;

        // Move forward/backward (only works in free-flight mode)
        case SDLK_F:
            if (!orbit_mode)
            {
                if(upper_case) g_camera->slide(0.0f, 0.0f, -5.0f);
                else g_camera->slide(0.0f, 0.0f, 5.0f);
                update_spotlight();
            }
            break;
            
        default: break;
    }

    return cont_program;
}

// Modified handle_events to include mouse wheel
bool handle_events()
{
    SDL_Event e;
    bool      cont_program = true;
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: cont_program = false; break;

            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: cont_program = handle_window_event(e); break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: handle_mouse_event(e); break;

            case SDL_EVENT_MOUSE_MOTION: handle_mouse_motion_event(e); break;
            
            case SDL_EVENT_MOUSE_WHEEL: handle_mouse_wheel_event(e); break;

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: cont_program = handle_key_event(e); break;
            default: break;
        }
    }
    return cont_program;
}

/**
 * Convenience method to add a material, then a transform, then a
 * geometry node as a child to a specified parent node.
 * @param  parent    Parent scene node.
 * @param  material  Presentation node.
 * @param  transform Transformation node.
 * @param  geometry  Geometry node.
 */
void add_sub_tree(std::shared_ptr<cg::SceneNode> parent,
                  std::shared_ptr<cg::SceneNode> material,
                  std::shared_ptr<cg::SceneNode> transform,
                  std::shared_ptr<cg::SceneNode> geometry)
{
    parent->add_child(material);
    material->add_child(transform);
    transform->add_child(geometry);
}

// Construct celestial bodies
std::shared_ptr<cg::SceneNode> construct_celestial_bodies(int32_t position_loc, int32_t normal_loc, int32_t texture_loc)
{
    // Base scene node to put all celestial bodies under
    auto celestial_body_root = std::make_shared<cg::SceneNode>();
    
    // Geometry node
    auto sphere_mesh = std::make_shared<cg::TexturedSphereSection>(-90.0f, 90.0f, 18, -180.0f, 180.0f, 36, 1.0f, position_loc, normal_loc, texture_loc);
    
    // Transformation nodes
    auto sun_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    auto mercury_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    auto venus_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    auto earth_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    auto mars_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    auto jupiter_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    auto saturn_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    auto uranus_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    auto neptune_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    
    g_planet_transforms.push_back(sun_transform);
    g_planet_transforms.push_back(mercury_transform);
    g_planet_transforms.push_back(venus_transform);
    g_planet_transforms.push_back(earth_transform);
    g_planet_transforms.push_back(mars_transform);
    g_planet_transforms.push_back(jupiter_transform);
    g_planet_transforms.push_back(saturn_transform);
    g_planet_transforms.push_back(uranus_transform);
    g_planet_transforms.push_back(neptune_transform);
    
    float scaling_factor = 5.0f;
    float sun_mass     = 25.0f * scaling_factor;
    float mercury_mass = 0.2f   * scaling_factor;
    float venus_mass   = 0.6f   * scaling_factor;
    float earth_mass   = 1.0f   * scaling_factor;
    float mars_mass    = 0.4f   * scaling_factor;
    float jupiter_mass = 5.0f  * scaling_factor;
    float saturn_mass  = 4.0f   * scaling_factor;
    float uranus_mass  = 2.0f   * scaling_factor;
    float neptune_mass = 2.0f   * scaling_factor;
    
    g_planet_transforms[0]->set_mass(sun_mass);
    g_planet_transforms[1]->set_mass(mercury_mass);
    g_planet_transforms[2]->set_mass(venus_mass);
    g_planet_transforms[3]->set_mass(earth_mass);
    g_planet_transforms[4]->set_mass(mars_mass);
    g_planet_transforms[5]->set_mass(jupiter_mass);
    g_planet_transforms[6]->set_mass(saturn_mass);
    g_planet_transforms[7]->set_mass(uranus_mass);
    g_planet_transforms[8]->set_mass(neptune_mass);
    
    // Rotation ratios (rotation speed multiplier based on Earth)
    float DAY_RATIO_SUN     = 0.04f;
    float DAY_RATIO_MERCURY = 0.41f;
    float DAY_RATIO_VENUS   = -0.004f;
    float DAY_RATIO_EARTH   = 1.0f;
    float DAY_RATIO_MARS    = 0.97f;
    float DAY_RATIO_JUPITER = 2.42f;
    float DAY_RATIO_SATURN  = 2.25f;
    float DAY_RATIO_URANUS  = -1.39f;
    float DAY_RATIO_NEPTUNE = 1.49f;

    // Orbit rate ratios (orbital speed multiplier based on Earth)
    float ORBIT_RATIO_SUN     = 0.0f;
    float ORBIT_RATIO_MERCURY = 4.15f;
    float ORBIT_RATIO_VENUS   = 1.62f;
    float ORBIT_RATIO_EARTH   = 1.0f;
    float ORBIT_RATIO_MARS    = 0.53f;
    float ORBIT_RATIO_JUPITER = 0.09f;
    float ORBIT_RATIO_SATURN  = 0.04f;
    float ORBIT_RATIO_URANUS  = 0.02f;
    float ORBIT_RATIO_NEPTUNE = 0.01f;
    
    // Earth parameters (change these as necessary)
    float EARTH_ROTATION_RATE  = 30.0f;
    float EARTH_ORBIT_SPEED    = 10.0f;
    
    // Central sun position
    auto SUN_POSITION = cg::Vector3(0.0f, 0.0f, 0.0f);
    
    // SUN
    sun_transform->set_mass(sun_mass);
    sun_transform->set_orbit_parameters(SUN_POSITION, 0.0f, 0.0f, EARTH_ORBIT_SPEED * ORBIT_RATIO_SUN);
    sun_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_SUN);
    sun_transform->scale(37.5);


    // MERCURY
    mercury_transform->set_mass(mercury_mass);
    mercury_transform->set_orbit_parameters(SUN_POSITION,
                                            70.0f,//EARTH_ORBIT_DISTANCE * ORBIT_DISTANCE_MERCURY,
                                            0.0f,
                                            EARTH_ORBIT_SPEED * ORBIT_RATIO_MERCURY);
    mercury_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_MERCURY);
    mercury_transform->scale(2.0f);


    // VENUS
    venus_transform->set_mass(venus_mass);
    venus_transform->set_orbit_parameters(SUN_POSITION,
                                          110.0f,
                                          90.0f,
                                          EARTH_ORBIT_SPEED * ORBIT_RATIO_VENUS);
    venus_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_VENUS);
    venus_transform->scale(4.75f);


    // EARTH
    earth_transform->set_mass(earth_mass);
    earth_transform->set_orbit_parameters(SUN_POSITION,
                                          150.0f,
                                          180.0f,
                                          EARTH_ORBIT_SPEED * ORBIT_RATIO_EARTH);
    earth_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_EARTH);
    earth_transform->scale(5.0f);


    // MARS
    mars_transform->set_mass(mars_mass);
    mars_transform->set_orbit_parameters(SUN_POSITION,
                                         190.0f,
                                         270.0f,
                                         EARTH_ORBIT_SPEED * ORBIT_RATIO_MARS);
    mars_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_MARS);
    mars_transform->scale(2.75f);


    // JUPITER
    jupiter_transform->set_mass(jupiter_mass);
    jupiter_transform->set_orbit_parameters(SUN_POSITION,
                                            250.0f,
                                            315.0f,
                                            EARTH_ORBIT_SPEED * ORBIT_RATIO_JUPITER);
    jupiter_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_JUPITER);
    jupiter_transform->scale(11.5f);

    
    // SATURN
    saturn_transform->set_mass(saturn_mass);
    saturn_transform->set_orbit_parameters(SUN_POSITION,
                                           310.0f,
                                           225.0f,
                                           EARTH_ORBIT_SPEED * ORBIT_RATIO_SATURN);
    saturn_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_SATURN / 3);
    saturn_transform->scale(10.0f);


    // URANUS
    uranus_transform->set_mass(uranus_mass);
    uranus_transform->set_orbit_parameters(SUN_POSITION,
                                           370.0f,
                                           45.0f,
                                           EARTH_ORBIT_SPEED * ORBIT_RATIO_URANUS);
    uranus_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_URANUS);
    uranus_transform->scale(8.2f);


    // NEPTUNE
    neptune_transform->set_mass(neptune_mass);
    neptune_transform->set_orbit_parameters(SUN_POSITION,
                                            430.0f,
                                            135.0f,
                                            EARTH_ORBIT_SPEED * ORBIT_RATIO_NEPTUNE);
    neptune_transform->set_rotation_rate(EARTH_ROTATION_RATE * DAY_RATIO_NEPTUNE);
    neptune_transform->scale(8.0f);

    
    // Presentation nodes
    auto sun_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 10.0f);
    auto mercury_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 10.0f);
    auto venus_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 10.0f);
    auto earth_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 10.0f);
    auto mars_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 10.0f);
    auto jupiter_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 10.0f);
    auto saturn_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 10.0f);
    auto uranus_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 10.0f);
    auto neptune_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                 cg::Color4(1.0f, 1.0f, 1.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 cg::Color4(0.0f, 0.0f, 0.0f),
                                                                 10.0f);
    
    // Set textures (found at: https://www.solarsystemscope.com/textures/)
    sun_material->set_texture("textures/8k_sun.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    mercury_material->set_texture("textures/8k_mercury.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    venus_material->set_texture("textures/4k_venus_atmosphere.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    earth_material->set_texture("textures/8k_earth_daymap.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    mars_material->set_texture("textures/8k_mars.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    jupiter_material->set_texture("textures/8k_jupiter.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    saturn_material->set_texture("textures/8k_saturn.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    uranus_material->set_texture("textures/2k_uranus.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    neptune_material->set_texture("textures/2k_neptune.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    
    // Extra textures for earth for multi-texturing
    // Set all three textures for Earth
    earth_material->set_texture_2("textures/8k_earth_clouds.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    earth_material->set_texture_3("textures/8k_earth_nightmap.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    earth_material->enable_multitexture();
    
    // Add extras like the moon and saturn's ring
    // Create moon material
    auto moon_material = std::make_shared<cg::PresentationNode>(cg::Color4(0.3f, 0.3f, 0.3f),
                                                                cg::Color4(1.0f, 1.0f, 1.0f),
                                                                cg::Color4(0.0f, 0.0f, 0.0f),
                                                                cg::Color4(0.0f, 0.0f, 0.0f),
                                                                10.0f);
    
    // Load moon texture
    moon_material->set_texture("textures/8k_moon.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    // Create moon transform
    auto moon_transform = std::make_shared<cg::CelestialBodyTransformNode>();
    moon_transform->set_orbit_parameters(cg::Vector3(0.0f, 0.0f, 0.0f),
                                         3.0f,
                                         0.0f,
                                         10.0f);
    moon_transform->scale(0.25f);
    
    // Create ring material
    auto ring_material = std::make_shared<cg::PresentationNode>(
        cg::Color4(0.7f, 0.7f, 0.7f),
        cg::Color4(1.0f, 1.0f, 1.0f),
        cg::Color4(0.3f, 0.3f, 0.3f),
        cg::Color4(0.0f, 0.0f, 0.0f),
        10.0f);

    // Load ring texture
    ring_material->set_texture("textures/8k_saturn_ring_alpha.png", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    // Create ring mesh
    auto ring_mesh = std::make_shared<cg::SaturnRing>(
        1.2f,           // inner_radius
        2.3f,           // outer_radius
        64,             // segments
        2,              // rings
        position_loc,
        normal_loc,
        texture_loc,
        true);          // double_sided

    // Create ring transform with slight tilt
    auto ring_transform = std::make_shared<cg::TransformNode>();
    ring_transform->rotate_x(20.0f);
    
    // Form scene graph with planet presentation nodes, transform nodes, and geometry nodes
    add_sub_tree(celestial_body_root, sun_material, sun_transform, sphere_mesh);
    add_sub_tree(celestial_body_root, mercury_material, mercury_transform, sphere_mesh);
    add_sub_tree(celestial_body_root, venus_material, venus_transform, sphere_mesh);
    add_sub_tree(celestial_body_root, earth_material, earth_transform, sphere_mesh);
    add_sub_tree(celestial_body_root, mars_material, mars_transform, sphere_mesh);
    add_sub_tree(celestial_body_root, jupiter_material, jupiter_transform, sphere_mesh);
    add_sub_tree(celestial_body_root, saturn_material, saturn_transform, sphere_mesh);
    add_sub_tree(celestial_body_root, uranus_material, uranus_transform, sphere_mesh);
    add_sub_tree(celestial_body_root, neptune_material, neptune_transform, sphere_mesh);
    
    // Add moon and saturn rings to correct places in scene graph
    add_sub_tree(earth_transform, moon_material, moon_transform, sphere_mesh);
    add_sub_tree(saturn_transform, ring_material, ring_transform, ring_mesh);

    // Return root node
    return celestial_body_root;
}

/**
 * Construct lighting for this scene. Note that it is hard coded
 * into the shader node for this exercise.
 * @param  lighting  Pointer to the lighting shader node.
 */
void construct_lighting(std::shared_ptr<cg::LightingShaderNode> lighting)
{
    // Set the global light ambient
    cg::Color4 global_ambient(0.4f, 0.4f, 0.4f, 1.0f);
    lighting->set_global_ambient(global_ambient);

    // Light 0 - point light source in back right corner
    auto light_0 = std::make_shared<cg::LightNode>(0);
    light_0->set_diffuse(cg::Color4(0.99f, 0.99f, 0.99f, 1.0f));
    light_0->set_specular(cg::Color4(0.99f, 0.99f, 0.99f, 1.0f));
    light_0->set_position(cg::HPoint3(0.0f, 0.0f, 0.0f, 1.0f));
    light_0->enable();

    // Light1 - directional light from the ceiling
    auto light_1 = std::make_shared<cg::LightNode>(1);
    light_1->set_diffuse(cg::Color4(0.7f, 0.7f, 0.7f, 1.0f));
    light_1->set_specular(cg::Color4(0.7f, 0.7f, 0.7f, 1.0f));
    light_1->set_position(cg::HPoint3(0.0f, 0.0f, -1.0f, 0.0f)); // set the light down below
    light_1->disable(); // Disabled old light from old scenes

    // Spotlight - reddish spotlight - we will place at the camera location
    // shining along -VPN
    Spotlight = std::make_shared<cg::LightNode>(2);
    Spotlight->set_diffuse(cg::Color4(0.5f, 0.1f, 0.1f, 1.0f));
    Spotlight->set_specular(cg::Color4(0.5f, 0.1f, 0.1f, 1.0f));
    cg::Point3 pos = g_camera->get_position();
    Spotlight->set_position(cg::HPoint3(pos.x, pos.y, pos.z, 1.0f));
    cg::Vector3 dir = g_camera->get_view_plane_normal() * -1.0f;
    Spotlight->set_spotlight(dir, 32.0f, 30.0f);
    Spotlight->disable(); // Disabled spotlight from old scenes

    // Lights are children of the camera node
    g_camera->add_child(light_0);
    light_0->add_child(light_1);
    light_1->add_child(Spotlight);
    
    // Set sun position for day/night calculations
    lighting->set_sun_position(cg::Vector3(0.0f, 0.0f, 0.0f));
}

// Method to construct the skybox
void construct_skybox()
{
    // Create skybox shader
    g_skybox_shader = std::make_shared<cg::SkyboxShaderNode>();
    
    // Load skybox shaders
    if(!g_skybox_shader->create("Module10/skybox_shader.vert", "Module10/skybox_shader.frag") ||
       !g_skybox_shader->get_locations())
    {
        std::cerr << "Failed to create skybox shader" << std::endl;
        exit(-1);
    }
    
    // Create skybox geometry
    g_skybox = std::make_shared<cg::Skybox>();
    
    // Load cubemap textures (made with Space-3D tool at: https://tools.wwwtyro.net/)
    std::vector<std::string> faces = {
        "textures/skybox/right.png",
        "textures/skybox/left.png",
        "textures/skybox/bottom.png",   // Had to swap top and bottom texutres
        "textures/skybox/top.png",
        "textures/skybox/front.png",
        "textures/skybox/back.png"
    };
    
    if (!g_skybox->load_cubemap(faces))
    {
        std::cerr << "Failed to load skybox textures" << std::endl;
        exit(-1);
    }
    
    g_skybox->initialize(g_skybox_shader->get_position_loc());
    
    // Add skybox to shader node
    g_skybox_shader->add_child(g_skybox);
}

/**
 * Construct the scene
 */
void construct_scene()
{
    // Shader node
    auto shader = std::make_shared<cg::LightingShaderNode>();
    if(!shader->create("Module10/pixel_lighting_tex.vert", "Module10/pixel_lighting_tex.frag") ||
       !shader->get_locations())
    {
        exit(-1);
    }

    // Get the position, texture, and normal locations to use when constructing VAOs
    int32_t position_loc = shader->get_position_loc();
    int32_t normal_loc = shader->get_normal_loc();
    int32_t texture_loc = shader->get_texture_loc();

    // Add the camera to the scene
    // Initialize the view and set a perspective projection
    g_camera = std::make_shared<cg::CameraNode>();
    
    // Initial camera settings
    g_camera->set_position(cg::Point3(0.0f, -600.0f, 200.0f));
    g_camera->set_look_at_pt(cg::Point3(0.0f, 0.0f, 00.0f));
    g_camera->set_view_up(cg::Vector3(0.0f, 0.0f, 1.0f));
    g_camera->set_perspective(50.0f, 1.0f, 1.0f, 2000.0f);

    // Construct fixed scene lighting
    construct_lighting(shader);

    // Construct the scene layout
    g_scene_root = std::make_shared<cg::SceneNode>();
    
    // Add skybox
    construct_skybox();
    g_scene_root->add_child(g_skybox_shader);
    
    // Add pixel lighting
    g_scene_root->add_child(shader);
    shader->add_child(g_camera);
    
    // Then add your lighting shader
    g_scene_root->add_child(shader);
    shader->add_child(g_camera);

    // Construct a base node for the rest of the scene, it will be a child
    // of the last light node (so entire scene is under influence of all
    // lights)
    auto myscene = std::make_shared<cg::SceneNode>();
    Spotlight->add_child(myscene);
    
    // Construct the space time grid
    float stg_scale = 1000.0f;
    
    g_space_time_grid = std::make_shared<cg::SpaceTimeGrid>(150, position_loc, normal_loc);
    g_space_time_grid->set_world_size(stg_scale);
    
    auto stg_transform = std::make_shared<cg::TransformNode>();
    stg_transform->scale(stg_scale, stg_scale, 1.0f);
    
    auto stg_presentation = std::make_shared<cg::PresentationNode>(cg::Color4(0.5f, 0.5f, 0.5f),
                                                                   cg::Color4(0.5f, 0.5f, 0.5f),
                                                                   cg::Color4(0.0f, 0.0f, 0.0f),
                                                                   cg::Color4(0.5f, 0.5f, 0.5f),
                                                                   1.0f);
    
    // Add space time grid to scene graph
    myscene->add_child(stg_presentation);
    stg_presentation->add_child(stg_transform);
    stg_transform->add_child(g_space_time_grid);

    // Construct celestial bodies and add to scene graph
    auto celestial_bodies_node = construct_celestial_bodies(position_loc, normal_loc, texture_loc);
    myscene->add_child(celestial_bodies_node);
}

/**
 * Main
 */
int main(int argc, char **argv)
{
    cg::set_root_paths(argv[0]);

    // Print the keyboard commands
    std::cout << "=== Camera Controls ===\n";
    std::cout << "i - Reset to initial view (exit orbit mode)\n";
    std::cout << "0-8 - Focus on planet (0=Sun, 1=Mercury, 2=Venus, 3=Earth, etc.)\n";
    std::cout << "\n=== Orbit Mode (when focused on planet) ===\n";
    std::cout << "Left Click + Drag - Rotate around planet\n";
    std::cout << "Mouse Wheel - Zoom in/out\n";
    std::cout << "\n=== Free Flight Mode (when not focused) ===\n";
    std::cout << "Left Mouse + Move - Move forward and turn\n";
    std::cout << "Right Mouse + Move - Move backward and turn\n";
    std::cout << "R/r - Roll camera\n";
    std::cout << "H/h - Change heading\n";
    std::cout << "X/x - Slide left/right\n";
    std::cout << "Y/y - Slide up/down\n";
    std::cout << "F/f - Move forward/backward\n";
    std::cout << "V/v - Increase/decrease speed\n";
    std::cout << "\n=== Other Controls ===\n";
    std::cout << "P/p - Pause/unpause simulation\n";
    std::cout << "ESC - Exit Program\n";
    std::cout << "========================\n\n";

    // Initialize SDL
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "Error initializing SDL: " << SDL_GetError() << '\n';
        exit(1);
    }

    // Initialize display mode and window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_PropertiesID props = SDL_CreateProperties();
    if(props == 0)
    {
        std::cout << "Error creating SDL Window Properties: " << SDL_GetError() << '\n';
        exit(1);
    }

    SDL_SetStringProperty(
        props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Final Project - Bryan Garry");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 800);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 600);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 100);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 100);

    g_sdl_window = SDL_CreateWindowWithProperties(props);
    if(g_sdl_window == nullptr)
    {
        std::cout << "Error initializing SDL Window" << SDL_GetError() << '\n';
        exit(1);
    }

    // Initialize OpenGL
    g_gl_context = SDL_GL_CreateContext(g_sdl_window);

    std::cout << "OpenGL  " << glGetString(GL_VERSION) << ", GLSL "
              << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

#if BUILD_WINDOWS
    int32_t glew_init_result = glewInit();
    if(GLEW_OK != glew_init_result)
    {
        std::cout << "GLEW Error: " << glewGetErrorString(glew_init_result) << '\n';
        exit(EXIT_FAILURE);
    }
#endif

    // Set the clear color to black. Any part of the window outside the
    // viewport should appear black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Construct scene.
    construct_scene();

    // Enable multi-sample anti-aliasing
    glEnable(GL_MULTISAMPLE);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Enable back face polygon removal
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    reshape(800, 600);

    update_view(g_mouse_x, g_mouse_y, g_forward);
    
    auto last_time = std::chrono::high_resolution_clock::now();

    // Main loop
    while(handle_events())
    {
        // Basic way to calculate delta time for animation
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - last_time;
        last_time = now;
        
        float delta_time = elapsed.count();
        g_scene_state.delta_time = delta_time;
        
        // Check mouse for free-flight mode
         if(g_animate && !orbit_mode)
             update_view(g_mouse_x, g_mouse_y, g_forward);
         
         // Update orbit camera if in orbit mode
         if(orbit_mode)
         {
             update_orbit_camera();
         }
        
        // Update planet positions
        if (!pause)
        {
            for (auto& p : g_planet_transforms)
                p->update(g_scene_state);
        }
        
        // Update space time grid
        g_space_time_grid->apply_deformation(g_planet_transforms);
        
        // Draw nodes
        display();
        
        // Sleep
        sleep(DRAW_INTERVAL_MILLIS);
    }

    // Destroy OpenGL Context, SDL Window and SDL
    SDL_GL_DestroyContext(g_gl_context);
    SDL_DestroyWindow(g_sdl_window);
    SDL_Quit();

    return 0;
}
