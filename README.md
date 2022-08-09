# Vortek
An interactive GPU-based volume renderer written in C and using OpenGL. It has a simple Qt GUI, written in Python, that can be used to load volumetric data and edit transfer functions for color and opacity.

The following screenshot shows a rendering of a volumetric field (720 x 720 x 1116 grid cells) representing the temperature in a simulation of the Sun's atmosphere.

<img width="742" alt="vortek" src="https://user-images.githubusercontent.com/20269954/183768371-0fdabf5b-71e3-400b-9962-ff24bb4f320f.png">

## Implementation
The volume data is stored in a 3D texture on the GPU and is rendered by blending a large number of view-aligned planes on which the texture is sampled.

Before being transferred to the GPU, the volume data is subdivided into separate "bricks". This improves data locality on the GPU, and by alternating the orientation of the bricks the memory access pattern can be made more or less view independent (Weiskopf et al. (2004) "Maintaining constant frame rates in 3D texture-based volume rendering"). By storing bricks in a space-partitioning tree, they can be efficiently sorted in back to front order when drawing, and invisible bricks can be skipped (Salama and Kolb, 2005). Each brick can additionally be subdivided into even smaller boxes, so that most of the empty regions can be skipped (see Ruijters and Vilanova (2006) "Optimizing GPU Volume Rendering").

The slicing of view aligned planes through a box is implemented in a vertex shader, based on Salama and Kolb (2005) "A Vertex Program for Efficient Box-Plane Intersection". This allows for a large number of small boxes to be sliced per frame, since very little data has to be sent to the GPU for each box.
