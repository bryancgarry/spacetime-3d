EN.605.667 - Computer Graphics
Bryan Garry
Final Project

===================================================

Files Modified:
- Module10/main.cpp
- scene/scene_state.hpp
- scene/presentation_node.hpp/.cpp
- Module10/lighting_shader_node.hpp/.cpp
- Module10/pixel_lighting_tex.frag
- scene/camera_node.hpp/.cpp

Files Added:
- Module10/celestial_body_transform_node.hpp/.cpp
- Module10/space_time_grid.hpp/.cpp
- Module10/skybox.hpp/.cpp
- Module10/skybox_shader_node.hpp/.cpp
- Module10/skybox_shader.vert/.frag
- Module10/saturn_ring.hpp/.cpp

===================================================

My final project was made from the Module 11 lab
that was given to us and runs from the Module 10
project in Xcode.

I started the final project working on an extended
implementation of the transform node. I added some
extra variables and methods to handle all the
important calculations needed for the planets in the
scene. This also handles the movement along a curved
path feature that I implemented. I accomplished this 
by moving the planets with polar coordinates as that
seemed like the simplest implementation to achieve
this.

I then moved on to implementing the space time grid
of the scene that is meant to be the extruded surface
advanced feature. I took inspiration from the 
TriSurface that we have used before and extended my own
GeometryNode to create this. A list of vertices is
created and connected in a grid like pattern, and then
drawn on screen with lines instead of faces. The extruded
surface updates the sub buffer to alter the positions of
the vertices based on the passed in mass values of the 
planets. The deformation is continually updated and it
tries to capture the g = GM/r^2 equation of the real
world. There is a limit to the amount of vertices possible
in the grid before performance starts to degrade.

I modified the existing lighting, texture, and shader
setup that we have to achieve multi-texturing in the
scene. The goal was to modify the earth planet to have
a day and night texture as well as a cloud texture
overlaid on top. I started by making some additions to
the scene_state to hold these additional texture
locations. I also added a sun position as well. I made
some slight changes to the presentation node to allow
for the loading of multiple textures. Likewise I modified
the existing lightning shader node to handle these
additional locations as well as a multi-texturing flag.
I finally updated the existing fragment shader used for
our Phong shading and I added in so additional uniform
sampler2Ds as well as an additional vec3 for the sun's
position. I then filter out which color to use based
on the direction of the sun and combine the result.

For the skybox I added a separate skybox geometry node,
skybox shader node, and skybox shader. The geometry node
is essentially 6 flat surfaces that make up a cube, each
surface of the cube is comprised of two triangles. The
skybox shader node acts as a replacement for the lighting
shader node and it also helps pass in the view and
projection matrices needed to achieve the skybox effect.
The skybox shaders are simple and just render a cubemap
texture on the interior of the cube. The shaders also
establish the skybox to have the maximum depth value
so that it is rendered behind all other geometry.

Lastly, I added a few minor things such as a small
geometry node that handles Saturn's rings for some
extra detail in the scene.

The main.cpp file was changed to handle the building of
the scene as well as a new way to use the camera to
navigate the scene. There is also a way to focus the
scene on specific planets and pause the simulation
as well.


(Using MacOS)