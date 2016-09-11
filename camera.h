#ifndef CAMERA_H
#define CAMERA_H

#include <iostream>
#include <math.h>         // Used only for sin() and cos() functions

#include <GLFW/glfw3.h>      // Include OpenGL Framework library for the GLFW_PRESS constant only!

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"

class Camera
{
protected:
	// Camera position
	glm::vec3 position;

	// Camera rotation
	glm::vec3 rotation;

	// Camera movement speed. When we call the move() function on a camera, it moves using these speeds
	glm::vec3 speed;

	double movementSpeedFactor; // Controls how fast the camera moves
	double pitchSensitivity;    // Controls how sensitive mouse movements affect looking up and down
	double yawSensitivity;      // Controls how sensitive mouse movements affect looking left and right

								// Window size in pixels and where the midpoint of it falls
	int windowWidth;
	int windowHeight;
	int windowMidX;
	int windowMidY;
	
	GLFWwindow* window;

	// Method to set some reasonable default values. For internal use by the class only.
	void initCamera();

public:
	static const double TO_RADS; // The value of 1 degree in radians

								 // Holding any keys down?
	bool holdingForward;
	bool holdingBackward;
	bool holdingLeftStrafe;
	bool holdingRightStrafe;
	
	// Constructor
	Camera(GLFWwindow* window, float windowWidth, float windowHeight);

	// Destructor
	~Camera();

	// Mouse movement handler to look around
	void handleMouseMove(GLFWwindow* window, int mouseX, int mouseY);

	// Method to convert an angle in degress to radians
	const double toRads(const double &angleInDegrees) const;

	// Method to move the camera based on the current direction
	void move(double deltaTime);

	void handleKeypress(GLint key, GLint action);

	glm::vec3 getRotVec();
	glm::vec3 getUpVec();
	// --------------------------------- Inline methods ----------------------------------------------

	// Setters to allow for change of vertical (pitch) and horizontal (yaw) mouse movement sensitivity
	double getPitchSensitivity() { return pitchSensitivity; }
	void  setPitchSensitivity(float value) { pitchSensitivity = value; }
	double getYawSensitivity() { return yawSensitivity; }
	void  setYawSensitivity(float value) { yawSensitivity = value; }

	// Position getters
	glm::vec3 getPosition() const { return position; }
	double getXPos()           const { return position.x; }
	double getYPos()           const { return position.y; }
	double getZPos()           const { return position.z; }

	// Rotation getters
	glm::vec3 getRotation() const { return rotation; }
	double getXRot()           const { return rotation.x; }
	double getYRot()           const { return rotation.y; }
	double getZRot()           const { return rotation.z; }

	
};

#endif // CAMERA_H