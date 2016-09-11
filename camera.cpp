//Ripped and converted from http://r3dux.org/2012/12/a-c-camera-class-for-simple-opengl-fps-controls/

#include "Camera.h"

const double Camera::TO_RADS = 3.141592654 / 180.0; // The value of 1 degree in radians

Camera::Camera(GLFWwindow* thewindow, float theWindowWidth, float theWindowHeight)
{
	initCamera();

	window = thewindow;

	windowWidth = theWindowWidth;
	windowHeight = theWindowHeight;

	// Calculate the middle of the window
	windowMidX = windowWidth / 2.0f;
	windowMidY = windowHeight / 2.0f;

	glfwSetCursorPos(window, windowMidX, windowMidY);
}

Camera::~Camera()
{
	// Nothing to do here - we don't need to free memory as all member variables
	// were declared on the stack.
}

void Camera::initCamera()
{
	// Set position, rotation and speed values to a default value, note rotation should be nonzero.
	position = {2,2,2 };
	rotation = {0,0,0};
	speed = {0,0,0 };

	// How fast we move (higher values mean we move and strafe faster)
	movementSpeedFactor = 1;

	pitchSensitivity = 0.05; // How sensitive mouse movements affect looking up and down
	yawSensitivity = 0.05; // How sensitive mouse movements affect looking left and right

						  // To begin with, we aren't holding down any keys
	holdingForward = false;
	holdingBackward = false;
	holdingLeftStrafe = false;
	holdingRightStrafe = false;

	altLock = true;
}

// Function to convert degrees to radians
const double Camera::toRads(const double &theAngleInDegrees) const
{
	return theAngleInDegrees * TO_RADS;
}

// Function to deal with mouse position changes
void Camera::handleMouseMove(GLFWwindow* window, int mouseX, int mouseY)
{
	// Calculate our horizontal and vertical mouse movement from middle of the window
	double horizMovement = (mouseX - windowMidX) * yawSensitivity;
	double vertMovement = (mouseY - windowMidY) * pitchSensitivity;

	//std::cout << "Mid window values: " << windowMidX << "\t" << windowMidY << std::endl;
	//std::cout << "Mouse values     : " << mouseX << "\t" << mouseY << std::endl;
	//std::cout << horizMovement << "\t" << vertMovement << std::endl << std::endl;

	// Apply the mouse movement to our rotation vector. The vertical (look up and down)
	// movement is applied on the X axis, and the horizontal (look left and right)
	// movement is applied on the Y Axis
	rotation.x += vertMovement;
	rotation.y += horizMovement;

	// Limit loking up to vertically up
	if (rotation.x < -90.0f) { rotation.x = -90.0f; }

	// Limit looking down to vertically down
	if (rotation.x >  90.0f) { rotation.x = 90.0f; }

	// If you prefer to keep the angles in the range -180 to +180 use this code
	// and comment out the 0 to 360 code below.
	//
	// Looking left and right. Keep the angles in the range -180.0f (anticlockwise turn looking behind) to 180.0f (clockwise turn looking behind)
	/*if (yRot < -180.0f)
	{
	yRot += 360.0f;
	}

	if (yRot > 180.0f)
	{
	yRot -= 360.0f;
	}*/

	// Looking left and right - keep angles in the range 0.0 to 360.0
	// 0 degrees is looking directly down the negative Z axis "North", 90 degrees is "East", 180 degrees is "South", 270 degrees is "West"
	// We can also do this so that our 360 degrees goes -180 through +180 and it works the same, but it's probably best to keep our
	// range to 0 through 360 instead of -180 through +180.
	if (rotation.y < 0.0f) { rotation.y += 360.0f; }
	if (rotation.y > 360.0f) { rotation.y -= 360.0f; }

	if (mouseX != windowMidX || mouseY != windowMidY) { glfwSetCursorPos(window, windowMidX, windowMidY); }
}

// Function to calculate which direction we need to move the camera and by what amount
void Camera::move(double deltaTime)
{
	// Vector to break up our movement into components along the X, Y and Z axis
	glm::vec3 movement = { 0,0,0 };

	//Following section has questionable use

	// Get the sine and cosine of our X and Y axis rotation
	double sinXRot = sin(toRads(rotation.x));
	double cosXRot = cos(toRads(rotation.x));

	double sinYRot = sin(toRads(rotation.y));
	double cosYRot = cos(toRads(rotation.y));

	double pitchLimitFactor = cosXRot; // This cancels out moving on the Z axis when we're looking up or down

	if (holdingForward)
	{
		movement += glm::vec3(cosYRot, -sinYRot, -sinXRot * (altLock ? 0.0 : 1.0));
	}

	if (holdingBackward)
	{
		movement -= glm::vec3(cosYRot, -sinYRot, -sinXRot * (altLock ? 0.0 : 1.0));
	}

	if (holdingLeftStrafe)
	{
		movement += glm::vec3(sinYRot, cosYRot, 0.0f);
	}

	if (holdingRightStrafe)
	{
		movement -= glm::vec3(sinYRot, cosYRot, 0.0f);
	}


	// Normalise our movement vector, but ONLY if it's non-zero! Normalising a vector of zero length
	// leads to the new vector having a length of NaN (Not a Number) because of the divide by zero.
	// Note: Using '>= 0.0f' rather than '!= 0.0f' avoids the compiler float comparison warning - we could
	// instead use '>= std::epsilon()' from the limit.h header, but I think that would be overkill.
	// Further reading: http://www.cplusplus.com/reference/limits/numeric_limits/
	if (glm::length(movement) > 0.0f)
	{
		movement = glm::normalize(movement);
	}



	// Calculate our value to keep the movement the same speed regardless of the framerate...
	double framerateIndependentFactor = movementSpeedFactor * deltaTime;

	// .. and then apply it to our movement vector.
	movement *= framerateIndependentFactor;

	// Finally, apply the movement to our position
	position += movement;
}

void Camera::handleKeypress(GLint key, GLint action)
{
	// If a key is pressed, toggle the relevant key-press flag
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		//cout << "Got keypress: " << key << "\t" << action << endl;

		switch (key)
		{
		case 87: // 'w'
				 //std::cout << "forward key down!" << std::endl;
			holdingForward = true;
			break;

		case 83: // 's'
				 //std::cout << "backward key down!" << std::endl;
			holdingBackward = true;
			break;

		case 65: // 'a'
				 //std::cout << "left strafe key down!" << std::endl;
			holdingLeftStrafe = true;
			break;

		case 68: // 'd'
				 //std::cout << "right strafe key down!" << std::endl;
			holdingRightStrafe = true;
			break;

		default:
			// Do nothing...
			break;
		}
	}
	else // If a key is released, toggle the relevant key-release flag
	{
		switch (key)
		{
		case 87: // 'w'
				 //std::cout << "forward key up!" << std::endl;
			holdingForward = false;
			break;

		case 83: // 's'
			holdingBackward = false;
			break;

		case 65: // 'a'
			holdingLeftStrafe = false;
			break;

		case 68: // 'd'
			holdingRightStrafe = false;
			break;

		default:
			// Do nothing...
			break;

		} // End of switch block

	} // End of else GLFW_RELEASE block
}

glm::vec3 Camera::getRotVec()
{
	//Manually convert xyz rotations to a unit vector for camera pointing.
	double x, y, z;

	x = cos(toRads(rotation.y))*cos(toRads(rotation.x));
	y = -sin(toRads(rotation.y))*cos(toRads(rotation.x));
	z = -sin(toRads(rotation.x));


	glm::vec3 rotVec = { x,y,z };
	glm::normalize(rotVec);
	return rotVec;
}

glm::vec3 Camera::getUpVec()
{
	return glm::vec3(0, 0, 1); //TEMPORARY, BUT THIS WORKS. DON'T MESS WITH IT UNLESS YOU WANT TO MESS WITH getRotVec.
}