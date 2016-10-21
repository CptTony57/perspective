//NOTE, up is +ve Z axis.

//Include GLEW  
//#define GLEW_STATIC

//Library for loading textures (Simple OpenGL Image Library)
#include <SOIL.h>

#include <GL/glew.h>  

#include<iostream> //cout
#include <fstream> //fstream
#include <vector> 
#include <ctime> 

//Include GLFW  
#include <GLFW/glfw3.h>  

//Include the standard C++ headers  
#include <stdio.h>  
#include <stdlib.h> 

//Include matrix libraries
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"

//#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

//Our includes
#include "camera.h"
#include <btBulletDynamicsCommon.h>

//Constants and globals
const int window_width = 1024;
const int window_height = 768;
int meshSelect = -99; // which mesh/object is currently selected
float grabDist;
float grabScale = 1;
Camera *camera; //For key modification
btDiscreteDynamicsWorld* dynamicsWorld; //For raytrace on keypress

enum collision_t { PLANE, BOX, SPHERE };

										//Define an error callback  
static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
	_fgetchar();
}

void printUnderCamera()
{
	glm::vec3 camPt = camera->getPosition();
	glm::vec3 endPt = camPt + camera->getRotVec()*1000.0f;
	btCollisionWorld::ClosestRayResultCallback RayCallback(
		btVector3(camPt.x, camPt.y, camPt.z),
		btVector3(endPt.x, endPt.y, endPt.z)
	);
	dynamicsWorld->rayTest(
		btVector3(camPt.x, camPt.y, camPt.z),
		btVector3(endPt.x, endPt.y, endPt.z),
		RayCallback
	);

	if (RayCallback.hasHit()) {
		if (meshSelect == (int)RayCallback.m_collisionObject->getUserIndex())
		{
			meshSelect = -99;
		}
		else
		{
			btVector3 objO = RayCallback.m_collisionObject->getWorldTransform().getOrigin();
			grabDist = sqrt((objO.x() - camPt.x)*(objO.x() - camPt.x) + (objO.y() - camPt.y)*(objO.y() - camPt.y) + (objO.z() - camPt.z)*(objO.z() - camPt.z));
			grabScale = RayCallback.m_collisionObject->getCollisionShape()->getLocalScaling().x();
			meshSelect = (int)RayCallback.m_collisionObject->getUserIndex();
		}
		std::cout << "mesh " << meshSelect << std::endl;
		std::cout << "dist " << grabDist << std::endl;
	}
	else
	{
		std::cout << "background" << std::endl;
	}
}

//Define the key input callback  
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	else
	{
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
		{
			//Debug data dump
			glm::vec3 temp = camera->getPosition() + camera->getRotVec();
			std::cout << camera->getXPos() << "\t" << camera->getYPos() << "\t" << camera->getZPos() << "\t" << std::endl;
			std::cout << temp.x << "\t" << temp.y << "\t" << temp.z << std::endl;

		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		{
			camera->altLock = !camera->altLock;
		}
		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			//Do a raytrace through the camera.
			printUnderCamera();
		}
		if (key == GLFW_KEY_F && action == GLFW_PRESS)
		{
			grabScale = grabScale * 5/4;
			grabDist = grabDist * 5/4;
		}
		if (key == GLFW_KEY_G && action == GLFW_PRESS)
		{
			grabScale = grabScale * 4/5;
			grabDist = grabDist * 4/5;
		}
		if (key == GLFW_KEY_T && action == GLFW_PRESS)
		{
			grabScale = 1;
			grabDist = 5;
		}


		camera->handleKeypress(key, action);
	}

}

void handleMouseMove(GLFWwindow *window, double mouseX, double mouseY)
{
	camera->handleMouseMove(window, mouseX, mouseY);
}

bool getShaderCompileStatus(GLuint shader) {
	//Get status
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE) {
		return true;
	}
	else {
		//Get log
		char buffer[512];
		glGetShaderInfoLog(shader, 512, NULL, buffer);
		std::cout << buffer << std::endl;
		return false;
	}
}

static void loadMesh(std::string file_name, std::vector<GLfloat>& data, int& number_of_elements) {
	Assimp::Importer importer;
	importer.ReadFile(file_name, aiProcessPreset_TargetRealtime_MaxQuality);
	const aiScene* scene = importer.GetScene();
	number_of_elements = 0;

	if (scene) {
		if (scene->HasMeshes()) {
			for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
				const struct aiMesh* mesh = scene->mMeshes[i];
				for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
					const struct aiFace* face = &mesh->mFaces[t];
					number_of_elements += face->mNumIndices;
					if (face->mNumIndices != 3) {
						std::cout << "WARNING " << __FILE__ << " : " << __LINE__ << " - faces are not triangulated" << std::endl;
					}
					for (unsigned int j = 0; j < face->mNumIndices; j++) {
						int index = face->mIndices[j];

						//Vertex positions
						data.push_back(mesh->mVertices[index].x); data.push_back(mesh->mVertices[index].z); data.push_back(mesh->mVertices[index].y);

						//Vertex normals
						if (mesh->mNormals != NULL) {
							//If we have normals, push them back next
							data.push_back(mesh->mNormals[index].x); data.push_back(mesh->mNormals[index].z); data.push_back(mesh->mNormals[index].y);
						}
						else {
							//If not, just set to zero, but warn the user as this will likely make the lighting null
							data.push_back(0); data.push_back(0); data.push_back(0);
							std::cout << "WARNING: No normals loaded for mesh " << file_name << std::endl;
						}

						//Vertex colours
						if (mesh->mColors[0] != NULL) {
							//If we have colours, append them
							data.push_back(mesh->mColors[index]->r); data.push_back(mesh->mColors[index]->g); data.push_back(mesh->mColors[index]->b);
						}
						else {
							//If no colours, push back white
							data.push_back(1); data.push_back(1); data.push_back(1);
						}

						//Texture coords
						if (mesh->mTextureCoords[0] != NULL) {
							//Push back textures
							data.push_back(mesh->mTextureCoords[0][index].x); data.push_back(1 - mesh->mTextureCoords[0][index].y);
						}
						else {
							data.push_back(0); data.push_back(0);
						}
					}
				}
			}
		}
	}
	else {
		std::cout << "No object found! - Looking for " << file_name << std::endl;
	}
}

void drawGround(float groundLevel)
{
	GLfloat extent = 600.0f; // How far on the Z-Axis and X-Axis the ground extends
	GLfloat stepSize = 10.0f;  // The size of the separation between points
	glColor3b(255, 255, 255);	//Set colour, probably broken.

								// Draw our ground grid
	glBegin(GL_LINES);
	for (GLint loop = -extent; loop < extent; loop += stepSize)
	{
		// Draw lines along Y-Axis
		glVertex3f(loop, extent, groundLevel);
		glVertex3f(loop, -extent, groundLevel);

		// Draw lines across X-Axis
		glVertex3f(-extent, loop, groundLevel);
		glVertex3f(extent, loop, groundLevel);
	}
	glEnd();
}

GLFWwindow* makeWindow()
{
	//Declare a window object  
	GLFWwindow* window;

	//Create a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Perspective", NULL, NULL);

	//If the window couldn't be created  
	if (!window)
	{
		fprintf(stderr, "Failed to open GLFW window.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	//This function makes the context of the specified window current on the calling thread.   
	glfwMakeContextCurrent(window);
	return window;
}

GLFWwindow* init()
//Initialises GLFW, GLEW and returns the window object
{
	//Set the error callback  
	glfwSetErrorCallback(error_callback);

	//Initialize GLFW  
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	//Set the GLFW window creation hints - these are optional  
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //Request a specific OpenGL version  
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2); //Request a specific OpenGL version  
	//glfwWindowHint(GLFW_SAMPLES, 16); //Request 4x antialiasing  
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  

	GLFWwindow* window = makeWindow();

	//Sets the key callback  
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, handleMouseMove);

	//Initialize GLEW  
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	//If GLEW hasn't initialized  
	if (err != GLEW_OK)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		exit(-1);
	}
	return window;
}

int loadVertex(std::string name, GLuint vao, GLuint buffer)
{
	//create Vertex array object
	glBindVertexArray(vao);

	//Load mesh with ASSIMP
	std::vector<GLfloat> data;
	int numberOfVertices = 0;
	loadMesh(name, data, numberOfVertices);

	if (numberOfVertices != 0) {
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * data.size(), &data[0], GL_STATIC_DRAW);
	}
	else {
		std::cout << "Model Empty!!" << std::endl;
	}
	return numberOfVertices;
}

GLuint makeShader(char vert[], char frag[])
{
	//Example:load shader source file
	std::ifstream in(vert);
	std::string contents((std::istreambuf_iterator<char>(in)),
		std::istreambuf_iterator<char>());
	const char* vertSource = contents.c_str();

	//Example: compile a shader source file for vertex shading
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertSource, NULL);
	glCompileShader(vertexShader);

	getShaderCompileStatus(vertexShader);

	//load and compile fragment shader shader.frag
	std::ifstream in2(frag);
	std::string contents2((std::istreambuf_iterator<char>(in2)),
		std::istreambuf_iterator<char>());
	const char* fragSource = contents2.c_str();

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragSource, NULL);
	glCompileShader(fragmentShader);

	getShaderCompileStatus(fragmentShader);

	//link shaders into a program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);
	return shaderProgram;
}

void linkVertexToShader(GLuint shaderProgram)
{
	//link vertex data to shader
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib,                //buffer identifier
		3,                        //How many data points to read
		GL_FLOAT,                 //Data type
		GL_FALSE,                 //Whether or not to clamp values between -1,1
		11 * sizeof(float),        //Stride (byte offset between consecutive values)
		0                         //pointer to first attribute (see below for better examples)
	);
	glEnableVertexAttribArray(posAttrib);

	GLint colourAttrib = glGetAttribLocation(shaderProgram, "colour");
	glVertexAttribPointer(colourAttrib, 3, GL_FLOAT, GL_TRUE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(colourAttrib);

	GLint textureAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(textureAttrib);

	GLint normalAttrib = glGetAttribLocation(shaderProgram, "normal");
	glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(normalAttrib);

}

GLuint loadTexture(char* name)
{
	//Create texture buffer:
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	//Load image
	int width, height;
	unsigned char* image =
		SOIL_load_image(name, &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
		GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);

	//Set sampler parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tex;
}

void loadTextures(GLuint texArray[], int size, char* stringList[])
{
	for (int i = 0; i < size; i++)
	{
		texArray[i] = loadTexture(stringList[i]);
	}
}

void loadVerticies(GLuint meshArray[], GLuint buffArray[], int numVArray[], int size, char* meshList[])
{
	for (int i = 0; i < size; i++)
	{
		glGenVertexArrays(1, &meshArray[i]);
		glGenBuffers(1, &buffArray[i]);
		numVArray[i] = loadVertex(meshList[i], meshArray[i], buffArray[i]);
	}


}

void linkToShader(int size, GLuint* shaderProg, GLuint meshArray[], GLuint buffArray[])
{
	for (int i = 0; i < size; i++)
	{
		glBindVertexArray(meshArray[i]);
		glBindBuffer(GL_ARRAY_BUFFER, buffArray[i]);
		linkVertexToShader(*shaderProg);
	}
}

void initPhysics()
{
	//---Bullet physics setup---
	// Build the broadphase
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();

	// Set up the collision configuration and dispatcher
	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

	// The actual physics solver
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

	// The world.
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, 0, -9.8));
}

btRigidBody* makePlane(btCollisionShape* groundShape, glm::vec3 position)
{
	btVector3 positionBT = btVector3(position.x, position.y, position.z);
	btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), positionBT));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
	btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
	return groundRigidBody;
}

btRigidBody* makeBox(btCollisionShape* boxShape, glm::vec3 position, glm::quat rotation, float mass)
{
	btVector3 positionBT = btVector3(position.x, position.y, position.z);
	btQuaternion rotationBT = btQuaternion(rotation.x, rotation.y, rotation.z);
	btVector3 fallInertia(0, 0, 0);
	boxShape -> calculateLocalInertia(mass, fallInertia);
	btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(rotationBT, positionBT));
	btRigidBody::btRigidBodyConstructionInfo rigidBodyBox1CI(mass,motionState,boxShape,fallInertia);
	btRigidBody* rigidBodyBox = new btRigidBody(rigidBodyBox1CI);
	return rigidBodyBox;
}

btRigidBody* makePhysObject(collision_t type, glm::vec3 position, glm::quat rotation, glm::vec3 size, float mass, int index)
{
	btCollisionShape* shape;
	btQuaternion rotationBT = btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w);
	btVector3 sizeBT = btVector3(size.x, size.y, size.z);
	btRigidBody* tempRB;
	switch (type)
	{
	case PLANE:
		shape = new btStaticPlaneShape(quatRotate(rotationBT,btVector3(0, 0, 1)), 1);
		tempRB = makePlane(shape, position);
		break;
	case BOX:
		shape = new btBoxShape(sizeBT);
		tempRB = makeBox(shape, position, rotation, mass);
		break;
	case SPHERE:
		shape = new btSphereShape(sizeBT.x()); //Bad, but eh
		tempRB = makeBox(shape, position, rotation, mass);
		break;
	}
	tempRB->setUserIndex(index);
	dynamicsWorld->addRigidBody(tempRB);
	return tempRB;
}

void drawObject(glm::vec3 position, float angle, glm::vec3 axis, glm::vec3 scale, GLuint tex, GLuint mesh, GLuint buff, int numV, GLint uniModel)
{
	glm::mat4 zero;
	glm::mat4 mCurrent;
	mCurrent = glm::translate(zero, position);
	mCurrent = glm::rotate(mCurrent, angle, axis);
	mCurrent = glm::scale(mCurrent, scale);
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(mCurrent));
	glBindVertexArray(mesh);
	glBindBuffer(GL_ARRAY_BUFFER, buff);
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawArrays(GL_TRIANGLES, 0, numV);

}

void drawPhysObject(btRigidBody* rigid, GLuint tex, GLuint mesh, GLuint buff, int numV, GLint uniModel)
{
	btTransform btf;
	glm::vec3 bTrans;//Translate
	glm::vec3 bAxis; //Rotate axis
	float bAngl;	 //Rotate amount
	glm::vec3 bScale;//Scaling factors
	rigid->getMotionState()->getWorldTransform(btf);
	if (rigid->getUserIndex() == meshSelect)
	{
		bTrans = camera->getPosition() + camera->getRotVec()*grabDist; //DISTANCE HERE
		
		rigid->setWorldTransform(btTransform(btf.getRotation(), btVector3(bTrans.x, bTrans.y, bTrans.z)));
		rigid->getCollisionShape()->setLocalScaling(btVector3(1,1,1)*grabScale);
		rigid->setLinearVelocity(btVector3(0, 0, 0));
		rigid->setAngularVelocity(btVector3(0, 0, 0));
		rigid->clearForces();
		rigid->activate();
	}
	else
	{
		bTrans = glm::vec3(btf.getOrigin().getX(), btf.getOrigin().getY(), btf.getOrigin().getZ());
	}
	bAxis = glm::vec3(btf.getRotation().getAxis().getX(), btf.getRotation().getAxis().getY(), btf.getRotation().getAxis().getZ());
	bAngl = btf.getRotation().getAngle()*180.0 / 3.141592654;
	bScale = glm::vec3(rigid->getCollisionShape()->getLocalScaling().x(), rigid->getCollisionShape()->getLocalScaling().y(), rigid->getCollisionShape()->getLocalScaling().z());

	drawObject(bTrans, bAngl, bAxis, bScale, tex, mesh, buff, numV, uniModel);
	
}

int main(void)
{
	GLFWwindow* window = init();

	//==================================
	//        Load vertex data
	//==================================
	const int numMeshes = 4;
	//Hardcoded list of mesh names
	char* meshList[numMeshes];
	meshList[0] = "cube.obj";
	meshList[1] = "Ball.obj";
	meshList[2] = "floor.obj";
	meshList[3] = "thingy.obj";

	GLuint meshArray[numMeshes];
	GLuint buffArray[numMeshes];
	int numVArray[numMeshes];

	loadVerticies(meshArray, buffArray, numVArray, numMeshes, meshList);

	//==================================
	//     Compile and Link Shaders
	//==================================
	GLuint shaderProgram = makeShader("shader.vert", "shader.frag");

	//==================================
	//    Link Vertex Data to Shaders
	//==================================

	linkToShader(numMeshes, &shaderProgram, meshArray, buffArray);

	//==================================
	//          Load Texture
	//==================================
	//Hardcoded list of texture names
	char* texList[2];
	texList[0] = "kitten.png";
	texList[1] = "rocks.jpg";
	//Texture array which is then fed into the binds
	GLuint texArray[2];
	loadTextures(texArray, 2, texList);

	//==================================
	//          Physics Setup
	//==================================

	initPhysics();
	//Array of Rigidbodies
	btRigidBody* rigidBodyArr[5];
	rigidBodyArr[0] = makePhysObject(PLANE, glm::vec3(0, 0, -1), glm::quat(0,0,0,1), glm::vec3(0,0,0), 0.0, 0);
	rigidBodyArr[1] = makePhysObject(BOX, glm::vec3(25,0,10), glm::quat(0,0,0,1), glm::vec3(0.5,0.5,0.5), 1.0, 1);
	rigidBodyArr[2] = makePhysObject(SPHERE, glm::vec3(24, 0, 15), glm::quat(0, 0, 0, 1), glm::vec3(1, 1, 1), 1.0, 2);
	rigidBodyArr[3] = makePhysObject(BOX, glm::vec3(1,5.2,10),glm::quat(0.5,0.15,7,1),glm::vec3(0.5,0.5,0.5), 1.0, 3);

	rigidBodyArr[4] = makePhysObject(BOX, glm::vec3(0, 0, 10), glm::quat(0, 0, 0, 1), glm::vec3(1, 1, 1), 1.0, 4);

	//--end of physics setup--

	//=============
	//Render to texture modification
	//=============
	

	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FramebufferName;
	GLuint renderedTexture;	// The texture we're going to render to
	GLuint depthrenderbuffer;

	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
	glGenTextures(1, &renderedTexture);
	glBindTexture(GL_TEXTURE_2D, renderedTexture);// "Bind" the newly created texture : all future texture functions will modify this texture
	
	// Give an empty image to OpenGL ( the last "0" means "empty" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	// Filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// The depth buffer
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

								   // Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;


	// The fullscreen quad's FBO
	static const GLfloat g_quad_vertex_buffer_data[] = 
	{
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
	};
	
	GLuint quad_mesh;
	glGenVertexArrays(1, &quad_mesh);
	glBindVertexArray(quad_mesh);

	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	// Create and compile our GLSL program from the shaders
	GLuint fake_prog = makeShader("pass.vert", "pass.frag");
	GLuint texID = glGetUniformLocation(fake_prog, "renderedTexture");
	//==================================
	//              Main Loop
	//==================================

	//Set a background color  
	glClearColor(0.0f, 0.0f, 1.0f, 0.0f);

	//TODO: turn on depth buffer
	glEnable(GL_DEPTH_TEST);
	glm::mat4 model;

	camera = new Camera(window, window_width, window_height);

	//Main Loop  
	clock_t start = std::clock();
	double prev_time;
	double frame_time = start;

	do
	{
		prev_time = frame_time;
		frame_time = (double)(clock() - start) / double(CLOCKS_PER_SEC);


		//Rigid Body Physics
		if (frame_time - prev_time > 0)
		{
			dynamicsWorld->stepSimulation(frame_time - prev_time, 1000);
		}

		camera->move(frame_time - prev_time);

		// Render to the framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewport(0, 0, window_width, window_height); // Render on the whole framebuffer, complete from the lower left corner to the upper right
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear color buffer, can happen anywhere
		glUseProgram(shaderProgram);

		float period = 10; //seconds
						   //Camera control
		glm::vec3 target = camera->getPosition() + camera->getRotVec();
		glm::vec3 up = camera->getUpVec();

		GLint uniModel = glGetUniformLocation(shaderProgram, "model");
		GLint uniView = glGetUniformLocation(shaderProgram, "view");
		GLint uniProj = glGetUniformLocation(shaderProgram, "proj");

		//Create and load view matrix
		glm::mat4 view = glm::lookAt(camera->getPosition(), target, up);
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		//Create and load projection matrix
		glm::mat4 proj = glm::perspective(
			45.0f,										//VERTICAL FOV
			float(window_width) / float(window_height), //aspect ratio
			0.01f,                                      //near plane distance (min z)
			1000.0f                                     //Far plane distance  (max z)
		);
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

		drawPhysObject(rigidBodyArr[1], texArray[1], meshArray[0], buffArray[0], numVArray[0], uniModel);
		drawPhysObject(rigidBodyArr[2], texArray[0], meshArray[1], buffArray[1], numVArray[1], uniModel);
		drawPhysObject(rigidBodyArr[3], texArray[0], meshArray[0], buffArray[0], numVArray[0], uniModel);
		drawObject(glm::vec3(0, 0, 0), 0, glm::vec3(0, 0, 1), glm::vec3(1, 1, 1), texArray[1], meshArray[2], buffArray[2], numVArray[2], uniModel);

		drawPhysObject(rigidBodyArr[4], texArray[1], meshArray[3], buffArray[3], numVArray[3], uniModel);


		glm::mat4 zero; //Thank god it defaults to the zero matrix
		glm::mat4 mCurrent;
		//Grids on the XZ axis, supposed to be used for gathering bearings.
		mCurrent = glm::translate(zero, glm::vec3(0.0, 0.0, 0.0));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(mCurrent));
		drawGround(000.0f); // Draw lower ground grid
		drawGround(100.0f);  // Draw upper ground grid

		//Do a light
		glm::vec4 light_position = view * glm::rotate(zero, 180 * float(frame_time) / period, glm::vec3(0.0f, 0.0f, 1.0f))* glm::vec4(1, 10, 2, 1.0);
		GLint uniLightPos = glGetUniformLocation(shaderProgram, "light_position");
		glUniform4fv(uniLightPos, 1, glm::value_ptr(light_position));
		glm::vec4 light_colour(5, 5, 5, 1); //Increasing may change intensity
		GLint uniLightCol = glGetUniformLocation(shaderProgram, "light_colour");
		glUniform4fv(uniLightCol, 1, glm::value_ptr(light_colour));
		
		
		//===========================
		//Render to texture to screen
		//===========================
		
		//Select the regular FB?
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// Render on the whole framebuffer, complete from the lower left corner to the upper right
		glViewport(0, 0, window_width, window_height);

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use the passthrough shader
		glUseProgram(fake_prog);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderedTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(texID, 0);
		
		// 1rst attribute buffer : vertices
		//Select the quad mesh
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);		
		glBindVertexArray(quad_mesh);
		//Enable the vertex array
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		
		// Draw the triangles
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
		//Disable the vertex array
		glDisableVertexAttribArray(0);



		//Swap buffers  (Actually render to screen)
		glfwSwapBuffers(window);

		//Get and organize events, like keyboard and mouse input, window resizing, etc...  
		glfwPollEvents();

	} //Check if the ESC key had been pressed or if the window had been closed  
	while (!glfwWindowShouldClose(window));

	//Close OpenGL window and terminate GLFW  
	glfwDestroyWindow(window);
	//Finalize and clean up GLFW  
	glfwTerminate();

	for (int i = 0; i < 4; i++)
	{
		dynamicsWorld->removeRigidBody(rigidBodyArr[i]);
		delete rigidBodyArr[i]->getMotionState();
		delete rigidBodyArr[i];
	}
	//delete groundShape;
	delete camera;
	delete dynamicsWorld;
	/*delete solver;
	delete dispatcher;
	delete collisionConfiguration;
	delete broadphase;
	*/
	exit(EXIT_SUCCESS);
}