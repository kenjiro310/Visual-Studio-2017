/*
	This program demonstrates the basic steps for loading a 3D model using Assimp, transform it once, 
	light the object, add texture mapping, and display it. 
	Different light shaders can be used. 

	This program needs the following libraries to run:
		Freeglut
		Glew
		Assimp
		glm
		SOIL (Simple OpenGL Image Library)

	Shaders are in external files. Please modify the shader file path for your computer. 
	Also modify the path of the 3D object file for your computer. 

	Ying Zhu
	Department of Computer Science
	Georgia State University

	2014

*/

#include <fstream>
#include <iostream>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "assimp/Importer.hpp"
#include "assimp/PostProcess.h"
#include "assimp/Scene.h"

#include <glm/glm.hpp> 

#include <glm/gtx/projection.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <SOIL.h>

// This header file contains utility functions that print out the content of 
// aiScene data structure. 
#include "check_error.hpp"

using namespace std;
using namespace glm;

#define BUFFER_OFFSET( offset ) ((GLvoid*) offset)

//-------------------------
// Shader related variables

const char* vShaderFilename = "D:\\Work\\Teaching\\Courses\\2014\\CSC4820-6820\\Code\\Shaders\\vShader_lighting_texture.glsl";
const char* fShaderFilename = "D:\\Work\\Teaching\\Courses\\2014\\CSC4820-6820\\Code\\Shaders\\fShader_light_texture.glsl";

// Index of the shader program
GLuint program;

//---------------------------
// 3D object related variable

// Make sure you modify the path of the 3D object file for your computer. 
const char * objectFileName = "D:\\Work\\Teaching\\Courses\\2014\\CSC4820-6820\\Models\\square_textured.obj";

// This is the Assimp importer object that loads the 3D file. 
Assimp::Importer importer;

// Global Assimp scene object
const aiScene* scene = NULL;

// This array stores the VAO indices for each corresponding mesh in the aiScene object. 
// For example, vaoArray[0] stores the VAO index for the mesh scene->mMeshes[0], and so on. 
unsigned int *vaoArray = NULL;

// This is the 1D array that stores the face indices of a mesh. 
unsigned int *faceArray = NULL;

//---------------------------------
// Transformation related variables
GLint vPos; // Index of the in variable vPos in the vertex shader
GLint vNormal; // Index of the in variable vNormal in the vertex shader
GLint vTextureCoord; // Index of the in variable vTextureCoord in the vertex shader

GLuint mvpMatrixID; // uniform attribute: model-view-projection matrix

mat4 projMatrix; // projection matrix
mat4 viewMatrix; // view matrix

//-----------------
// Camera related variables
vec3 eyePosition = vec3(0.0f, 0.0f, 2.0f);

//---------------------------
// Lighting related variables
GLint normalID; // vertex attribute: normal
GLint mvMatrixID; // uniform variable: model-view matrix
GLint normalMatrixID; // uniform variable: normal matrix for transforming normal vectors

GLuint materialBlockIndex; 
GLuint lightBlockIndex;

// surface material properties
struct SurfaceMaterialProp {
	float Kambient[4]; //ambient component
	float Kdiffuse[4]; //diffuse component
	float Kspecular[4]; // Surface material property: specular component
	float shininess; 
};

SurfaceMaterialProp surfaceMaterial1 = {
	{1.0f, 1.0f, 1.0f, 1.0f},  // Kambient: ambient coefficient
	{1.0f, 0.8f, 0.72f, 1.0f},  // Kdiffuse: diffuse coefficient
	{1.0f, 1.0f, 1.0f, 1.0f},  // Kspecular: specular coefficient
	5.0f // Shininess
};

struct LightSourceProp {
	float lightSourcePosition[4]; 
	float diffuseLightIntensity[4];
	float specularLightIntensity[4];
	float ambientLightIntensity[4];
	float constantAttenuation; 
	float linearAttenuation;
	float quadraticAttenuation;
	float spotlightDirection[3];
	float spotlightCutoffAngle; 
};

LightSourceProp lightSource1 = {
	{1.0f, 1.0f, 1.0f, 1.0f},  // light source position
	{1.0f, 1.0f, 1.0f, 1.0f},  // diffuse light intensity
	{1.0f, 1.0f, 1.0f, 1.0f},  // specular light intensity
	{0.2f, 0.2f, 0.2f, 1.0f}, // ambient light intensity
	1.0f, 0.5, 0.1f,   // constant, linear, and quadratic attenuation factors
	{0.0f, 0.0f, -1.0f},  // spotlight direction
	{0.8f} // spotlight cutoff angle (in radian)
	};

// Texture mapping related variables. 
float* textureCoordArray = 0;
unsigned int* textureObjectIDArray = 0;
unsigned int textureUnit;

//----------
// Functions

//---------------
// Load a 3D file
bool load3DFile( const char *filename) {

	ifstream fileIn(filename);

	// Check if the file exists. 
	if (fileIn.good()) {
		fileIn.close();  // The file exists. 
	} else {
		fileIn.close();
		cout << "Unable to open the 3D file." << endl;
		return false;
	}

	cout << "Loading 3D file " << filename << endl;

	// Load the 3D file using Assimp. The content of the 3D file is stored in an aiScene object. 
	scene = importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Quality);

	// Check if the file is loaded successfully. 
	if(!scene)
	{
		// Fail to load the file
		cout << importer.GetErrorString() << endl;
		return false;
	} else {
		cout << "3D file " << filename << " loaded." << endl;
	}

	// Print the content of the aiScene object, if needed. 
	// This function is defined in the check_error.hpp file. 
	 printAiSceneInfo(scene, PRINT_AISCENE_SUMMARY);

	return true;
}

//-------------------
// Read a shader file
const char *readShaderFile(const char * filename) {
	ifstream shaderFile (filename);

	if (!shaderFile.is_open()) {
		cout << "Cannot open the shader file " << filename << endl;
		return NULL;
	}

	string line;
	// Must created a new string, otherwise the returned pointer 
	// will be invalid
	string *shaderSourceStr = new string();

	while (getline(shaderFile, line)) {
			*shaderSourceStr += line + '\n';
	}

	const char *shaderSource = shaderSourceStr->c_str();

	shaderFile.close();

	return shaderSource;
}

//-------------------------------
//Prepare the shaders and 3D data
void init()
{
	// **************************
	// Load and build the shaders. 

	GLuint vShaderID, fShaderID;

	// Create empty shader objects
	vShaderID = glCreateShader(GL_VERTEX_SHADER);
	if (vShaderID == 0) {
		cout << "There is an error creating the vertex shader." << endl;
	}

	fShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	if (fShaderID == 0) {
		cout << "There is an error creating the fragment shader." << endl;
	}

	const char* vShader = readShaderFile(vShaderFilename);

	// OpenGL fragment shader source code
	const char* fShader = readShaderFile(fShaderFilename);

	// Attach shader source code the shader objects
	glShaderSource(vShaderID, 1, &vShader, NULL);
	glShaderSource(fShaderID, 1, &fShader, NULL);

	// Compile the vertex shader object
	glCompileShader(vShaderID);
	printShaderInfoLog(vShaderID); // Print error messages, if any. 

	// Compile the fragment shader object
	glCompileShader(fShaderID);
	printShaderInfoLog(fShaderID); // Print error messages, if any. 

	// Create an empty shader program object
	program = glCreateProgram();
	if (program == 0) {
		cout << "There is an error creating the shader program." << endl;
	}

	// Attach vertex and fragment shaders to the shader program
	glAttachShader(program, vShaderID);
	glAttachShader(program, fShaderID);

	// Link the shader program
	glLinkProgram(program);
	// Check if the shader program can run in the current OpenGL state, just for testing purposes. 
	glValidateProgram(program);
	printShaderProgramInfoLog(program); // Print error messages, if any. 

	// glGetAttribLocation() should not be called before glLinkProgram()
	// because the shader variables haven't been bound before the program is linked.
	// Get the ID of the vPos variable in the vertex shader.
	vPos = glGetAttribLocation( program, "vPos" );
	if (vPos == -1) {
		cout << "There is an error getting the handle of GLSL variable vPos." << endl; 
	}

	vNormal = glGetAttribLocation( program, "vNormal" );
	if (vNormal == -1) {
		cout << "There is an error getting the handle of GLSL variable vNormal." << endl; 
	}

	vTextureCoord = glGetAttribLocation( program, "vTextureCoord" );
	if (vTextureCoord == -1) {
		cout << "There is an error getting the handle of GLSL variable vTextureCoord." << endl; 
	}

	// Get the ID of the uniform matrix variable in the vertex shader. 
	mvpMatrixID = glGetUniformLocation(program, "mvpMatrix");
	if (mvpMatrixID == -1) {
		cout << "There is an error getting the handle of GLSL uniform variable mvp_matrix." << endl;
	}

	mvMatrixID = glGetUniformLocation(program, "mvMatrix");
	if (mvMatrixID == -1) {
		cout << "There is an error getting the handle of GLSL uniform variable mvMatrix." << endl;
	}

	normalMatrixID = glGetUniformLocation(program, "normalMatrix");
	if (mvpMatrixID == -1) {
		cout << "There is an error getting the handle of GLSL uniform variable normalMatrix." << endl;
	}

	// Get the ID of the material uniform block in the shader program
	materialBlockIndex = glGetUniformBlockIndex(program, "materialProp");
	if (materialBlockIndex == -1) {
		cout << "There is an error getting the handle of GLSL uniform block materialProp" << endl;
	}

	// Get the ID of the light source uniform block in the shader program
	lightBlockIndex = glGetUniformBlockIndex(program, "LightSourceProp");
	if (lightBlockIndex == -1) {
		cout << "There is an error getting the handle of GLSL uniform block LightSourceProp" << endl;
	}

	textureUnit = glGetUniformLocation(program,"texUnit");
	if (textureUnit == -1) {
		cout << "There is an error getting the handle of texUnit" << endl;
	}

	// ****************
	// Load the 3D file
	
	// This variable temporarily stores the VBO index. 
	GLuint buffer;

	// Load the 3D file using Assimp.
	bool fileLoaded = load3DFile(objectFileName);
	if (!fileLoaded) {
		return;
	}

	//********************************************************************************
	// Retrieve vertex arrays from the aiScene object and bind them with VBOs and VAOs.

	// Create an array to store the VAO indices for each mesh. 
	vaoArray = (unsigned int*) malloc(sizeof(unsigned int) * scene->mNumMeshes);

	// Go through each mesh stored in the aiScene object, bind it with a VAO, 
	// and save the VAO index in the vaoArray. 
	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* currentMesh = scene->mMeshes[i];

		// Create an empty Vertex Array Object (VAO). VAO is only available from OpenGL 3.0 or higher. 
		// Note that the vaoArray[] index is in sync with the mMeshes[] array index. 
		// That is, for mesh #0, the corresponding VAO index is stored in vaoArray[0], and so on. 
		glGenVertexArrays(1, &vaoArray[i]);
		glBindVertexArray(vaoArray[i]);

		if (currentMesh->HasPositions()) {
			// Create an empty Vertex Buffer Object (VBO)
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);

			// Bind (transfer) the vertex position array (stored in aiMesh's member variable mVertices) 
			// to the VBO.
			// Note that the vertex positions are stored in a continuous 1D array (i.e. mVertices) in the aiScene object. 
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * currentMesh->mNumVertices, 
				currentMesh->mVertices, GL_STATIC_DRAW);

			// Associate this VBO with an the vPos variable in the vertex shader. 
			// The vertex data and the vertex shader must be connected. 
			glEnableVertexAttribArray( vPos );
			glVertexAttribPointer( vPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
		} 

		if (currentMesh->HasFaces()) {
			// Create an array to store the face indices (elements) of this mesh. 
			// This is necessary becaue face indices are NOT stored in a continuous 1D array inside aiScene. 
			// Instead, there is an array of aiFace objects. Each aiFace object stores a number of (usually 3) face indices.
			// We need to copy the face indices into a continuous 1D array. 
			faceArray = (unsigned int*)malloc(sizeof(unsigned int) * currentMesh->mNumFaces * currentMesh->mFaces[0].mNumIndices);

			// copy the face indices from aiScene into a continuous 1D array faceArray.  
			int faceArrayIndex = 0;
			for (unsigned int j = 0; j < currentMesh->mNumFaces; j++) {
					for (unsigned int k = 0; k < currentMesh->mFaces[j].mNumIndices; k++) {
						faceArray[faceArrayIndex] = currentMesh->mFaces[j].mIndices[k]; 
						faceArrayIndex++;
					}
			}

			// Create an empty VBO
			glGenBuffers(1, &buffer);

			// This VBO is an GL_ELEMENT_ARRAY_BUFFER, not a GL_ARRAY_BUFFER. 
			// GL_ELEMENT_ARRAY_BUFFER stores the face indices (elements), while 
			// GL_ARRAY_BUFFER stores vertex positions. 
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
				sizeof(unsigned int) * currentMesh->mNumFaces * currentMesh->mFaces[0].mNumIndices, 
				faceArray, GL_STATIC_DRAW);
		}

		if (currentMesh->HasNormals()) {
			// Create an empty Vertex Buffer Object (VBO)
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);

			// Bind (transfer) the vertex normal array (stored in aiMesh's member variable mNormals) 
			// to the VBO.
			// Note that the vertex normals are stored in a continuous 1D array (i.e. mVertices) 
			// in the aiScene object. 
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * currentMesh->mNumVertices, 
				currentMesh->mNormals, GL_STATIC_DRAW);

			// Associate this VBO with an the vPos variable in the vertex shader. 
			// The vertex data and the vertex shader must be connected. 
			glEnableVertexAttribArray( vNormal );
			glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
		} 

		//**************************************
		// Set up texture mapping data

		// Each mesh may have multiple UV(texture) channels (multi-texture). Here we only use 
		// the first channel. Call currentMesh->GetNumUVChannels() to get the number of UV channels
		// for this mesh. 
		if (currentMesh->HasTextureCoords(0)) {
			// Create an empty Vertex Buffer Object (VBO)
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);

			// mTextureCoords is different from mVertices or mNormals. It is a 2D array, not a 1D array. 
			// So we need to copy it to a 1D texture coordinate array.
			// The first dimension of this array is the texture channel for this mesh.
			// The second dimension is the vertex index number. 
			// The number of texture coordinates is always the same as the number of vertices.
			textureCoordArray = (float *) malloc(sizeof(float) * 2 * currentMesh->mNumVertices);
			unsigned int k = 0;
			for (unsigned int j = 0; j < currentMesh->mNumVertices; j++) {
				textureCoordArray[k] = currentMesh->mTextureCoords[0][j].x;
				k++;
				textureCoordArray[k] = currentMesh->mTextureCoords[0][j].y;
				k++;
			}

			// Bind (transfer) the texture coordinate array to the VBO.
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * currentMesh->mNumVertices, 
				textureCoordArray, GL_STATIC_DRAW);

			// Associate this VBO with the vTextureCoord variable in the vertex shader. 
			// The vertex data and the vertex shader must be connected. 
			glVertexAttribPointer( vTextureCoord, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
			glEnableVertexAttribArray( vTextureCoord );
		}

		textureObjectIDArray = (unsigned int*) malloc(sizeof(unsigned int) * scene->mNumMeshes);

		aiMaterial* currentMaterial = scene->mMaterials[currentMesh->mMaterialIndex];

		if (currentMaterial != NULL) {
			int texIndex = 0; // To make it simple, we only retrieve the first texture for each material.
			aiString path;	// filename

			// Get the diffuse texture file path for this material. 
			aiReturn texFound = currentMaterial->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);

			if (texFound == AI_SUCCESS) {
				string filename = path.data;  // get filename	

				// Use SOIL to load texture image. SOIL will create a texture object for this texture
				// image and return the texture object ID. 
				textureObjectIDArray[i] = SOIL_load_OGL_texture(filename.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

				// YZ: If the returned texture ID > 0, it means the imaged is loaded successfully. 
				if (textureObjectIDArray[i] <= 0) {
					cout << "Couldn't load texture image: " << filename.c_str() << endl;
				} // end if

			} // end if 
			else {
				textureObjectIDArray[i] = 0; 
				cout << "There is no texture for mesh #" << i << endl; 
			} // end else

		} // end if 

		//Close the VAOs and VBOs for later use. 
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	}

	//****************************
	// Set up lighting related parameters

	GLuint materialBuffer, lightBuffer;

	// Create a uniform buffer to store the surface material properties
	glGenBuffers(1, &materialBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, materialBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(surfaceMaterial1), 
		(void *)(&surfaceMaterial1), GL_STATIC_DRAW);

	// Link the uniform buffer for material with 
	// the material uniform block in the shader program
	glUniformBlockBinding(program, materialBlockIndex, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, materialBuffer);

	// Create a uniform buffer to store the light source properties
	glGenBuffers(1, &lightBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, lightBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(lightSource1), 
		(void *)(&lightSource1), GL_STATIC_DRAW);
	
	// Link the uniform buffer of light source properties with
	// the lighting uniform block in the shader program
	glUniformBlockBinding(program, lightBlockIndex, 1);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, lightBuffer);

	//****************************
	// Set up other OpenGL states. 

	// Turn on visibility test. 
	glEnable(GL_DEPTH_TEST);

	// Draw the object in wire frame mode. 
	// You can comment out this line to draw the object in 
	// shaded mode. But without lighting, the object will look very dark. 
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// This is the background color. 
	glClearColor( 1.0, 1.0, 1.0, 1.0 );

	// Uncomment this line for debugging purposes.
	// Check error
	checkOpenGLError("init()");
}

//--------------------------------------------------------------------------------------------
// Traverse the node tree in the aiScene object and draw the meshes associated with each node. 
// This function is called recursively to perform a depth-first tree traversal. 
void nodeTreeTraversal(const aiNode* node) {
	if (!node) {
		cout << "nodeTreeTraversal(): Null node" << endl;
		return; 
	}

	// Draw all the meshes associated with the current node.
	// Certain node may have no mesh associated with it. 
	// Certain node may have multiple meshes associated with it. 
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		// This is the index of the mesh associated with this node.
		int meshIndex = node->mMeshes[i];   

	    // "BindTexture" means that a texture image is transferred from main memory to GPU memory. 
		// glActiveTexture() indicates which texture unit the texture image will be sent to. 
		// A texture unit is used to hold an active texture image that is about to be accessed by shaders. 
		// There are multiple texture units on a GPU, allowing multi-texturing. 
		// Before "binding" a texture, you must indicate which texture unit to use. 
		// You also need to link the texture sampler variable in the fragment shader to the correct texture unit. 

		// In this case, we are using texture unit 0. So in renderScene() function, we call glUniform1i(texUnit, 0) to 
		// link texUnit (a handle of the "texUnit" variable in the fragment shader) to 0. 
		// glActiveTexture() function and glUniform1i(textUnit, ...) function calls must be consistent. 
		// If you change texture unit number in one function all, you must change the texture unit in the other function call. 
		if (textureObjectIDArray[meshIndex] > 0) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, textureObjectIDArray[meshIndex]);
		}
		
		// YZ: Set texture filter parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// This mesh should have already been associated with a VAO in a previous function. 
		// Note that mMeshes[] array and the vaoArray[] array are in sync. 
		// That is, for mesh #0, the corresponding VAO index is stored in vaoArray[0], and so on. 
		// Bind the corresponding VAO for this mesh. 
		glBindVertexArray(vaoArray[meshIndex]); 
 
		const aiMesh* currentMesh = scene->mMeshes[meshIndex];
		// How many faces are in this mesh?
		unsigned int numFaces = currentMesh->mNumFaces; 
		// How many indices are for each face?
		unsigned int numIndicesPerFace = currentMesh->mFaces[0].mNumIndices;

		// The second parameter is crucial. This is the number of face indices, not the number of faces.
		// (numFaces * numIndicesPerFace) is the total number of elements(face indices) of this mesh.
		// Now draw all the faces. We know these faces are triangle because in 
		// importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Quality);
		// "aiProcessPreset_TargetRealtime_Quality" indicates that the 3D object will be triangulated. 
		glDrawElements(GL_TRIANGLES, (numFaces * numIndicesPerFace), GL_UNSIGNED_INT, 0);

		// We are done with the current VAO. Move on to the next VAO, if any. 
		glBindVertexArray(0); 
	}

	// Uncomment this line for debugging purposes. 
	// checkOpenGLError();

	// Recursively visit and draw all the child nodes. This is a depth-first traversal. 
	for (unsigned int j = 0; j < node->mNumChildren; j++) {
		nodeTreeTraversal(node->mChildren[j]); 
	}
}

//--------------------------
// Display callback function
void display() {
	// Clear the background color and the depth buffer. 
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Activate the shader program. 
	glUseProgram( program );

	//***************************
	// Construct the model matrix
	// Create individual translation/rotation/scaling matrices. You transform a 3D object to a 
	// specific location by a sequence of translations, rotations, and scalings. Each transformation
	// is encoded in an elementary transformation matrix. 
	mat4 scaleMatrix = glm::scale(mat4(1.0f), vec3(0.5f));

	mat4 translateMatrix = translate(mat4(1.0f), vec3(0.0f, 0.3f, 0.0f));

	mat4 rotationMatrixY = rotate(mat4(1.0f), -60.0f, vec3(1.0f, 0.0f, 0.0f));
	// rotationMatrixY = rotate(rotationMatrixY, 60.0f, vec3(1.0f, 0.0f, 0.0f));

	// The overall transformation matrix is created by multiplying these elementary matrices. 
	// This is the model matrix. 
	// 
	// It's important to know how to interpret the sequence of transformation. 
	// To read the sequence of transformations, always start from the vertex position, and read from right to left. 
	// Note that in the vertex shader, the vertex position is multiplied from the right side of the matrix. 
	// In this case, each vertex is first scaled, then rotated, and then translated. 
	mat4 modelMatrix = translateMatrix * rotationMatrixY * scaleMatrix;

	//**********************************************************************************
	// Combine the model, view, and project matrix into one model-view-projection matrix.

	// In the following line, each vertex is first rotated, scaled, and then translated. 
	// mat4 modelMatrix = translateMatrix * scaleMatrix * rotationMatrixY;

	// Model matrix is then multiplied with view matrix and projection matrix to create a combined
	// model_view_projection matrix. 
	// The view and project matrix are created in reshape() callback function. However, if you need
	// to move the camera or change focal length (Fielf of View) during run time, then you need to create
	// projection and view matrix in the display() callback function. Why? Because display() function
	// is called more frequently than reshape() (which is only called when the window is created or resized). 
	// Therefore any change you make to the view and projection matrix can be instantly displayed. 
	//
	// The sequence of multiplication is important here. Model matrix, view matrix, and projection matrix 
	// must be multiplied from right to left, because the vertex position is on the right hand side. 
	mat4 mvpMatrix =  projMatrix * viewMatrix * modelMatrix;

	mat4 mvMatrix = viewMatrix * modelMatrix;

	mat3 normalMatrix = mat3(1.0);
	normalMatrix = column(normalMatrix, 0, vec3(mvMatrix[0][0], mvMatrix[0][1], mvMatrix[0][2]));
	normalMatrix = column(normalMatrix, 1, vec3(mvMatrix[1][0], mvMatrix[1][1], mvMatrix[1][2]));
	normalMatrix = column(normalMatrix, 2, vec3(mvMatrix[2][0], mvMatrix[2][1], mvMatrix[2][2]));

	normalMatrix = inverseTranspose(normalMatrix);

	// The model_view_projection matrix is transferred to the graphics memory to be used in the vertex shader. 
	// Here we send the combined model_view_projection matrix to the shader so that we don't have to do the same 
	// multiplication in the vertex shader repeatedly. Note that the vertex shader is executed for each vertex. 
	// Sometimes you need to send individual model, view, or projection matrices to the shader.
	// The vertex shader may need these matrices for other calculations. 
	// 
	// It's important to understand how the parameters in the shader and the parameters in this (OpenGL) program are
	// connected. In this case, how the matrices are connect. If you makes changes in the shader, make sure you update
	// the corresponding parameters in the OpenGL program, and vice versa. 
	glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

	glUniformMatrix4fv(mvMatrixID, 1, GL_FALSE, glm::value_ptr(mvMatrix));
	glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	// We only use texture unit 0. Here 0 means Texture Unit 0. 
	// This tells fragment shader to retrieve texture from Texture Unit 0. 
	glUniform1i(textureUnit, 1);  

	//*************
	// Render scene
	// Start the node tree traversal and process each node. 
	if (scene) {
		nodeTreeTraversal(scene->mRootNode);
	}

	// Swap front and back buffers. The rendered image is now displayed. 
	glutSwapBuffers();
}

//----------------------------------------------------------------
// This function is called when the size of the window is changed. 
void reshape( int width, int height )
{
	// Specify the width and height of the picture within the window
	// Creates a viewport matrix and insert it into the graphics pipeline. 
	// This operation is not done in shader, but taken care of by the hardware. 
	glViewport(0, 0, width, height);

	// Creates a projection matrix. 
	// The first parameter is camera field of view; the second is the window aspect ratio; 
	// the third is the distance of the near clipping plane; 
	// the fourth is the far clipping plane. 
	projMatrix = perspective(60.0f, (float)width /(float)height, 0.1f, 1000.0f); 

	// Create a view matrix
	// You specify where the camera location and orientation and GLM will create a view matrix. 
	// The first parameter is the location of the camera; 
	// the second is where the camera is pointing at; the third is the up vector for camera.
	// If you need to move or animate your camera during run time, then you need to construct the 
	// view matrix in display() function. 
	viewMatrix = lookAt(eyePosition, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}

// This function is called when a key is pressed. 
void keyboard( unsigned char key, int x, int y )
{
	switch( key ) {
		case 033: // Escape Key
			exit( EXIT_SUCCESS );
			break;
	}

	// Generate a display event, which forces Freeglut to call display(). 
	glutPostRedisplay();
}

int main( int argc, char* argv[] )
{
	glutInit( &argc, argv );

	// Initialize double buffer and depth buffer. 
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	// If you don't specify an OpenGL context version,
	// a default OpenGL context and profile will be created. 
	// Must explicity specify OpenGL context 4.3 for the debug callback function to work. 
	// glutInitContextVersion( 4, 3 );

	// Initialize OpenGL debug context, which is available from OpenGL 4.3+. 
	// glutInitContextFlags(GLUT_DEBUG);

	glutCreateWindow( argv[0] );

	// Initialize Glew. This must be called before any OpenGL function call. 
	glewInit();

	// These cannot be called before glewInit().
	cout << "OpenGL version " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Shading Language version " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl << endl;

	// Uncomment these lines for debugging purposes. 
	// Usually you want to enable GL_DEBUG_OUTPUT_SYNCHRONOUS so that error messages are immediately reported. 
	// glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	// Register a debug message callback function. 
	// glDebugMessageCallback((GLDEBUGPROC)openGLDebugCallback, nullptr);

	// Uncomment this line for debugging purposes. 
	checkOpenGLError("main()");

	init();

	glutDisplayFunc( display );

	glutReshapeFunc( reshape );

	glutKeyboardFunc( keyboard );

	glutMainLoop();

	//Release the dynamically allocated memory blocks. 
	free(vaoArray);
	free(faceArray);
	free(textureCoordArray);
	free(textureObjectIDArray);
}