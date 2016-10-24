Perspective: A Tech Demo

To run:
Unzip the project folder somewhere
Using CMake, select source and destination to the Bullet folder, clear the cache and the regenerate and rebuild the CMake scripts
(This is because the project expects absolute file paths)
Open the solution (GLFW_Example+Bullet) and then change the Runtime Library type of all 3 Bullet projects, namely BulletCollision, BulletDynamics and LinearMath, to Multithreaded DLL (/MD).
(That is, properties, C/C++, Code Generation, Runtime Library. These are reset when the CMake scripts are. There's probably a flag to fix it, but it doesn't break once we have it set up)
Run normally.

Controls
WASD	- Move the camera
Mouse   - Rotate the camera
Q	- Toggle the Z-axis movement lock
E	- Dump camera position and facing points
R	- Grab an object, if it can be picked up.
T	- Reset a grabbed object's size and distance from the camera to 1x and 5
F	- Grow a grabbed object by a factor of 5/4 and send it away from the camera by the same factor
F	- Shrink a grabbed object by a factor of 4/5 and send it towards the camera by the same factor
V	- Cycle between shader modes: normal, no texture, ambient only, max lights
