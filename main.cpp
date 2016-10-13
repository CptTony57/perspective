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
Camera *camera; //For key modification
btDiscreteDynamicsWorld* dynamicsWorld; //For raytrace on keypress

										//Define an error callback  
static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
	_fgetchar();
}

void printUnderCamera()
{
	glm::vec3 camPt = camera->getPosition() + camera->getRotVec();
	glm::vec3 endPt = camPt + camera->getRotVec()*999.0f;
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
		if (meshSelect == (int)RayCallback.m_collisionObject->getUserIndex()) { meshSelect = -99;}
		else { meshSelect = (int)RayCallback.m_collisionObject->getUserIndex(); }
		std::cout << "mesh " << meshSelect << std::endl;
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
						data.push_back(mesh->mVertices[index].x); data.push_back(mesh->mVertices[index].y); data.push_back(mesh->mVertices[index].z);

						//Vertex normals
						if (mesh->mNormals != NULL) {
							//If we have normals, push them back next
							data.push_back(mesh->mNormals[index].x); data.push_back(mesh->mNormals[index].y); data.push_back(mesh->mNormals[index].z);
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
	window = glfwCreateWindow(window_width, window_height, "Test Window", NULL, NULL);

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

GLuint makeShader()
{
	//Example:load shader source file
	std::ifstream in("shader.vert");
	std::string contents((std::istreambuf_iterator<char>(in)),
		std::istreambuf_iterator<char>());
	const char* vertSource = contents.c_str();

	//Example: compile a shader source file for vertex shading
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertSource, NULL);
	glCompileShader(vertexShader);

	getShaderCompileStatus(vertexShader);

	//load and compile fragment shader shader.frag
	std::ifstream in2("shader.frag");
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

int main(void)
{
	GLFWwindow* window = init();

	//==================================
	//        Load vertex data
	//==================================
	//Hardcoded list of mesh names
	char* meshList[3];
	meshList[0] = "cube.obj";
	meshList[1] = "Ball.obj";
	meshList[2] = "floor.obj";

	GLuint meshArray[3];
	GLuint buffArray[3];
	int numVArray[3];

	loadVerticies(meshArray, buffArray, numVArray, 3, meshList);

	//==================================
	//     Compile and Link Shaders
	//==================================
	GLuint shaderProgram = makeShader();

	//==================================
	//    Link Vertex Data to Shaders
	//==================================

	linkToShader(3, &shaderProgram, meshArray, buffArray);

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

	btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 0, 1), 1);

	btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, -1)));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
	btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
	dynamicsWorld->addRigidBody(groundRigidBody);

	btCollisionShape* boxCollisionShape = new btBoxShape(btVector3(0.5, 0.5, 0.5));
	btCollisionShape* ballCShape = new btSphereShape(1);
	btScalar mass = 1;
	btVector3 fallInertia(0, 0, 0);
	boxCollisionShape->calculateLocalInertia(mass, fallInertia);
	ballCShape->calculateLocalInertia(mass, fallInertia);

	btDefaultMotionState* motionstate1 = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(25, 0, 10)));

	btRigidBody::btRigidBodyConstructionInfo rigidBodyBox1CI(
		mass,                  // mass, in kg. 0 -> Static object, will never move.
		motionstate1,
		boxCollisionShape,  // collision shape of body
		fallInertia    // local inertia
	);
	btRigidBody *rigidBodyBox1 = new btRigidBody(rigidBodyBox1CI);
	rigidBodyBox1->setUserIndex(0);
	dynamicsWorld->addRigidBody(rigidBodyBox1);

	btDefaultMotionState* motionstate2 = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(24, 0, 15)));

	btRigidBody::btRigidBodyConstructionInfo rigidBodyBox2CI(
		mass,                  // mass, in kg. 0 -> Static object, will never move.
		motionstate2,
		ballCShape,  // collision shape of body
		fallInertia    // local inertia
	);
	btRigidBody *rigidBodyBox2 = new btRigidBody(rigidBodyBox2CI);
	rigidBodyBox2->setUserIndex(1);
	dynamicsWorld->addRigidBody(rigidBodyBox2);

	btDefaultMotionState* motionstate3 = new btDefaultMotionState(btTransform(btQuaternion(0.5, 0, 0, 1), btVector3(1, 10.75, 10)));

	btRigidBody::btRigidBodyConstructionInfo rigidBodyBox3CI(
		mass,                  // mass, in kg. 0 -> Static object, will never move.
		motionstate3,
		boxCollisionShape,  // collision shape of body
		fallInertia    // local inertia
	);
	btRigidBody *rigidBodyBox3 = new btRigidBody(rigidBodyBox3CI);
	rigidBodyBox3->setUserIndex(2);
	dynamicsWorld->addRigidBody(rigidBodyBox3);
	//--end of physics setup--

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
		dynamicsWorld->stepSimulation(frame_time / 100, 100);

		btTransform trans1, trans2, trans3;
		glm::vec3 B1, B2, B3;
		rigidBodyBox1->getMotionState()->getWorldTransform(trans1);
		rigidBodyBox2->getMotionState()->getWorldTransform(trans2);
		rigidBodyBox3->getMotionState()->getWorldTransform(trans3);
		if (rigidBodyBox1->getUserIndex() == meshSelect) {
			B1 = camera->getPosition() + camera->getRotVec()*5.0f;
			btVector3 B1btv3 = btVector3(B1.x,B1.y,B1.z);
			btTransform transN1 = btTransform(trans1.getRotation(), B1btv3);
			rigidBodyBox1->setCenterOfMassTransform(transN1);
			rigidBodyBox1->clearForces(); rigidBodyBox1->setLinearVelocity(btVector3(0, 0, 0)); rigidBodyBox1->setAngularVelocity(btVector3(0, 0, 0));
		}
		else {B1 = glm::vec3(trans1.getOrigin().getX(), trans1.getOrigin().getY(), trans1.getOrigin().getZ());}	

		if (rigidBodyBox2->getUserIndex() == meshSelect) {
			B2 = camera->getPosition() + camera->getRotVec()*5.0f;
			btVector3 B2btv3 = btVector3(B2.x, B2.y, B2.z);
			btTransform transN2 = btTransform(trans2.getRotation(), B2btv3);
			rigidBodyBox2->setCenterOfMassTransform(transN2);
			rigidBodyBox2->clearForces(); rigidBodyBox2->setLinearVelocity(btVector3(0, 0, 0)); rigidBodyBox2->setAngularVelocity(btVector3(0, 0, 0));
		}
		else {B2 = glm::vec3(trans2.getOrigin().getX(), trans2.getOrigin().getY(), trans2.getOrigin().getZ());}

		if (rigidBodyBox3->getUserIndex() == meshSelect) {
			B3 = camera->getPosition() + camera->getRotVec()*5.0f;
			btVector3 B3btv3 = btVector3(B3.x, B3.y, B3.z);
			btTransform transN3 = btTransform(trans3.getRotation(), B3btv3);
			rigidBodyBox3->setCenterOfMassTransform(transN3);
			rigidBodyBox3->clearForces(); rigidBodyBox3->setLinearVelocity(btVector3(0, 0, 0)); rigidBodyBox3->setAngularVelocity(btVector3(0, 0, 0));
		}
		else {B3 = glm::vec3(trans3.getOrigin().getX(), trans3.getOrigin().getY(), trans3.getOrigin().getZ());}

		glm::vec3 B3ax = glm::vec3(trans3.getRotation().getAxis().getX(), trans3.getRotation().getAxis().getY(), trans3.getRotation().getAxis().getZ());
		float B3an = trans3.getRotation().getAngle()*180.0 / 3.14159;
		//

		camera->move(frame_time - prev_time);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear color buffer, can happen anywhere

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
		glm::mat4 zero; //Thank god it defaults to the zero matrix
		glm::mat4 mCurrent;

		//Render first cube
		mCurrent = glm::translate(zero, B1);
		mCurrent = glm::rotate(mCurrent, -360 * float(frame_time) / (period), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(mCurrent));

		glBindVertexArray(meshArray[0]);
		glBindBuffer(GL_ARRAY_BUFFER, buffArray[0]);
		glBindTexture(GL_TEXTURE_2D, texArray[1]);
		glDrawArrays(GL_TRIANGLES, 0, numVArray[0]);

		//Render second cube
		mCurrent = glm::translate(zero, B2);
		mCurrent = glm::rotate(mCurrent, -360 * float(frame_time) / (period), glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(mCurrent));

		glBindVertexArray(meshArray[1]);
		glBindBuffer(GL_ARRAY_BUFFER, buffArray[1]);
		glBindTexture(GL_TEXTURE_2D, texArray[0]);
		glDrawArrays(GL_TRIANGLES, 0, numVArray[1]);

		//Render third cube
		mCurrent = glm::translate(zero, B3);
		mCurrent = glm::rotate(mCurrent, B3an, B3ax);
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(mCurrent));

		glBindVertexArray(meshArray[0]);
		glBindBuffer(GL_ARRAY_BUFFER, buffArray[0]);
		glBindTexture(GL_TEXTURE_2D, texArray[0]);
		glDrawArrays(GL_TRIANGLES, 0, numVArray[0]);

		//Render not-physics'd floor
		mCurrent = glm::translate(zero, glm::vec3(0, 0, 0));
		mCurrent = glm::rotate(mCurrent, 90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(mCurrent));

		glBindVertexArray(meshArray[2]);
		glBindBuffer(GL_ARRAY_BUFFER, buffArray[2]);
		glBindTexture(GL_TEXTURE_2D, texArray[1]);
		glDrawArrays(GL_TRIANGLES, 0, numVArray[2]);

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

	dynamicsWorld->removeRigidBody(rigidBodyBox1);
	delete rigidBodyBox1->getMotionState();
	delete rigidBodyBox1;
	dynamicsWorld->removeRigidBody(rigidBodyBox2);
	delete rigidBodyBox2->getMotionState();
	delete rigidBodyBox2;
	dynamicsWorld->removeRigidBody(rigidBodyBox3);
	delete rigidBodyBox3->getMotionState();
	delete rigidBodyBox3;

	dynamicsWorld->removeRigidBody(groundRigidBody);
	delete groundRigidBody->getMotionState();
	delete groundRigidBody;
	delete groundShape;
	delete boxCollisionShape;
	delete ballCShape;
	delete camera;
	delete dynamicsWorld;
	delete solver;
	delete dispatcher;
	delete collisionConfiguration;
	delete broadphase;
	exit(EXIT_SUCCESS);
}
