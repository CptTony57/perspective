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

	#include "camera.h"

	const int window_width = 1024;
	const int window_height = 768;

	Camera *camera;

    //Define an error callback  
    static void error_callback(int error, const char* description)  
    {  
        fputs(description, stderr);  
        _fgetchar();  
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
				std::cout << camera->getXPos() << "\t" << camera->getYPos() <<"\t" << camera->getZPos() << "\t" << std::endl;
				std::cout << temp.x << "\t" << temp.y << "\t" << temp.z << std::endl;

			}
			if (key == GLFW_KEY_Q && action == GLFW_PRESS)
			{
				camera->altLock = !camera->altLock;
			}
			camera->handleKeypress(key, action);
		}

    } 

	void handleMouseMove(GLFWwindow *window, double mouseX, double mouseY)
	{
			camera->handleMouseMove(window, mouseX, mouseY);
	}

    bool getShaderCompileStatus(GLuint shader){
        //Get status
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if(status == GL_TRUE){
            return true;
        } else {
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
		GLfloat stepSize = 20.0f;  // The size of the separation between points

								   // Set colour to white // BROKEN
		glColor3b(127, 0, 0);

		// Draw our ground grid
		glBegin(GL_LINES);
		for (GLint loop = -extent; loop < extent; loop += stepSize)
		{
			// Draw lines along Z-Axis
			glVertex3f(loop, groundLevel, extent);
			glVertex3f(loop, groundLevel, -extent);

			// Draw lines across X-Axis
			glVertex3f(-extent, groundLevel, loop);
			glVertex3f(extent, groundLevel, loop);
		}
		glEnd();
	}

    int main( void )  
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
        //glfwWindowHint(GLFW_SAMPLES, 4); //Request 4x antialiasing  
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  
      
        //Declare a window object  
        GLFWwindow* window;  
      
        //Create a window and create its OpenGL context
        window = glfwCreateWindow(window_width, window_height, "Test Window", NULL, NULL);  
      
        //If the window couldn't be created  
        if (!window)  
        {  
            fprintf( stderr, "Failed to open GLFW window.\n" );  
            glfwTerminate();  
            exit(EXIT_FAILURE);  
        }  
      
        //This function makes the context of the specified window current on the calling thread.   
        glfwMakeContextCurrent(window);  

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
            return -1;  
        }

        //==================================
        //        Load vertex data
        //==================================

        //create Vertex array object
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Example: generate vertex buffers
        GLuint buffer;
        glGenBuffers(1, &buffer);



		//Load mesh with ASSIMP
		std::vector<GLfloat> data;
		int numberOfVertices = 0;
		loadMesh("Cube.obj", data, numberOfVertices);

		if (numberOfVertices != 0) {
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * data.size(), &data[0], GL_STATIC_DRAW);
		} else {
			std::cout << "Model Empty!!" << std::endl;
		}
		
        //==================================
        //     Compile and Link Shaders
        //==================================

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

        //==================================
        //    Link Vertex Data to Shaders
        //==================================

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

        //==================================
        //          Load Texture
        //==================================

        //Create texture buffer:
        GLuint tex;
        glGenTextures(1,&tex);
        glBindTexture(GL_TEXTURE_2D,tex);

        //Load image
        int width, height;
        unsigned char* image =
            SOIL_load_image("kitten.png", &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                      GL_UNSIGNED_BYTE, image);
        SOIL_free_image_data(image);
        
        //Set sampler parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
       



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
            frame_time = (double) (clock()-start) / double(CLOCKS_PER_SEC);
			camera->move(frame_time - prev_time);

            //==================================
            //       TODO: 3D Transforms
            //==================================
            //TODO: create model matrix based on time
            glm::mat4 model;
            float period = 10; //seconds
			//Note, this makes *everything* rotate, even the grids
			//model = glm::rotate(model, -360 * float(frame_time) / (period) , glm::vec3(0.0f, 0.0f, 1.0f)); //Tweak these last three to change the axis of rotation for the cube.
            
    		//TODO: load model matrix into shader      
            GLint uniModel = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
			glm::vec3 target = camera->getPosition() + camera->getRotVec();

			//Perpendicular to camera vector <May be wrong?>
			glm::vec3 up = camera->getUpVec();

            //TODO: create and load view matrix
			glm::mat4 view = glm::lookAt(
				/*glm::vec3(2.0f, 0.0f, 0.0f), //Camera position
				glm::vec3(0.0f, 0.0f, 0.0f), //Camera view target
				glm::vec3(0.0f, 0.0f, 1.0f)  //Camera up vector which is usually z*/
				camera->getPosition(), target, up
            );
            GLint uniView = glGetUniformLocation(shaderProgram, "view");
            glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

            //TODO: create and load projection matrix
            glm::mat4 proj = glm::perspective(45.0f,                              //VERTICAL FOV
                                              float(window_width) / float(window_height),       //aspect ratio
                                              0.01f,                                             //near plane distance (min z)
                                              1000.0f                                             //Far plane distance (max z)
            );
            GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
            glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

            //==================================
            //       TODO: Load a light
            //==================================

            glm::vec4 light_position = view * glm::rotate(model, 360 * float(frame_time) / period , glm::vec3(0.0f, 0.0f, 1.0f))* glm::vec4(1,0,1.3,1.0);
            GLint uniLightPos = glGetUniformLocation(shaderProgram, "light_position");
            glUniform4fv(uniLightPos, 1, glm::value_ptr(light_position));
            
            glm::vec4 light_colour(1,1,1,1);
            GLint uniLightCol = glGetUniformLocation(shaderProgram, "light_colour");
            glUniform4fv(uniLightCol, 1, glm::value_ptr(light_colour));


            //==================================
            //          Draw Model
            //==================================

            //Clear color buffer  
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
            
            glDrawArrays(GL_TRIANGLES, 0, numberOfVertices); 
            //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); 

			//Grids on the XZ axis, supposed to be used for gathering bearings.
			drawGround(-100.0f); // Draw lower ground grid //WRONG
			drawGround(100.0f);  // Draw upper ground grid //WRONG
            //Swap buffers  
            glfwSwapBuffers(window); 

			

            //Get and organize events, like keyboard and mouse input, window resizing, etc...  
            glfwPollEvents();  
        
        } //Check if the ESC key had been pressed or if the window had been closed  
        while (!glfwWindowShouldClose(window));  
      
        //Close OpenGL window and terminate GLFW  
        glfwDestroyWindow(window);  
        //Finalize and clean up GLFW  
        glfwTerminate();  
		delete camera;
        exit(EXIT_SUCCESS);  
    }  

