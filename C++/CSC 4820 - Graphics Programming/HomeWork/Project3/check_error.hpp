/* This is a utility program that helps OpenGL programmers check their program for errors. 
The following functions are provided.

// Print the content of an aiScene object. 
// Call this function after Assimp::Importer.ReadFile(). 
void printAiSceneInfo(const aiScene* scene, AiScenePrintOption option); 

// Call this function after glCompileShader().
void printShaderInfoLog(GLuint shaderID)

// Call this function after glLinkProgram() or glValidateProgram().
void printShaderProgramInfoLog(GLuint shaderProgID)

// Print OpenGL error message, if any. 
// Call this function at the end of a function that contains OpenGL APIs. 
void checkOpenGLError()

Written by Ying Zhu
Department of Computer Science
Georgia State University

2014

*/

#include <iostream>
#include <fstream>

enum AiScenePrintOption {PRINT_AISCENE_SUMMARY, PRINT_AISCENE_DETAIL};

using namespace std;

void printNodeTree(const aiNode* node, unsigned int layer) {
	if (!node) {
		cout << "printNodeTree(): null pointer" << endl;
		return;
	}

	for (unsigned int m = 0; m < layer; m++) {
		cout << "    ";
	}

	cout << "Node " << node->mName.C_Str();
	if (node->mNumMeshes > 0) {
		cout << " is linked with " << node->mNumMeshes << " meshes ("; 
	
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			cout << node->mMeshes[i] << " ";
		}
		cout << ")" << endl;
	} else {
		cout << endl;
	}

	for (unsigned int j = 0; j < node->mNumChildren; j++) {
		printNodeTree(node->mChildren[j], layer + 1); 
	}
}

void printAiSceneInfo(const aiScene* scene, AiScenePrintOption option) {
	
	if (!scene) {
		cout << "printAiSceneInfo(): null pointer" << endl;
		return;
	}

	cout << "Total number of meshes: " << scene->mNumMeshes << endl;
	if (scene->HasMaterials()) {
		cout << "Total number of materials: " << scene->mNumMaterials << endl;
	}
	if (scene->HasTextures()) {
		cout << "Total number of textures: " << scene->mNumTextures << endl;
	}
	if (scene->HasLights()) {
		cout << "Total number of lights: " << scene->mNumLights << endl;
	}

	printNodeTree(scene->mRootNode, 0);

	cout << "Print all the meshes: " << endl; 
	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* currentMesh = scene->mMeshes[i];
		cout << "\tMesh #" << i << " " << currentMesh->mName.C_Str() << endl;
		cout << "This mesh has " << currentMesh->GetNumUVChannels() << " UV(Texture) channels." << endl;
		cout << "This mesh is linked with material #" << currentMesh->mMaterialIndex << endl;

		if (currentMesh->HasPositions()) {
			cout << "Number of vertex positions:" << currentMesh->mNumVertices << endl;

			if (option == PRINT_AISCENE_DETAIL) {
				for (unsigned int j = 0; j < currentMesh->mNumVertices; j++) {
				
					cout << "\tvertex (" << currentMesh->mVertices[j].x << ", " 
										 << currentMesh->mVertices[j].y << ", "
										 << currentMesh->mVertices[j].z << ")" << endl;
				}
			}
		} else {
			cout << "There is no vertex position in mesh # " << i << endl;
		}

		if (currentMesh->HasFaces()) {
			cout << "Number of vertex faces(elements):" << currentMesh->mNumFaces << endl;

			if (option == PRINT_AISCENE_DETAIL) {
				for (unsigned int j = 0; j < currentMesh->mNumFaces; j++) {
					cout << "\tface #" << j << ": ";

					for (unsigned int k = 0; k < currentMesh->mFaces[j].mNumIndices; k++) {
						cout << currentMesh->mFaces[j].mIndices[k] << ", "; 
					}
					cout << endl;
				}
			}
		} else {
			cout << "There is no face (element) in mesh # " << i << endl;
		}

		if (currentMesh->HasNormals()) {
			cout << "Number of normals:" << currentMesh->mNumVertices << endl;

			if (option == PRINT_AISCENE_DETAIL) {
				for (unsigned int j = 0; j < currentMesh->mNumVertices; j++) {
					cout << "\tnormal (" << currentMesh->mNormals[j].x << ", "
						<< currentMesh->mNormals[j].y << ", " << currentMesh->mNormals[j].z << ")" << endl;
				}
			}
		} else {
			cout << "There is no normal vectors in mesh # " << i << endl;
		}

		// Each mesh may have multiple UV(texture) channels (multi-texture). Here we only print out 
		// the first channel. Call currentMesh->GetNumUVChannels() to get the number of UV channels
		// for this mesh. 
		if (currentMesh->HasTextureCoords(0)) {
			cout << "Number of texture coordinates for UV(texture) channel 0:" << currentMesh->mNumVertices << endl;

			if (option == PRINT_AISCENE_DETAIL) {
				// mTextureCoords is different from mVertices or mNormals in that it is a 2D array, not a 1D array. 
				// The first dimension of this array is the texture channel for this mesh.
				// The second dimension is the vertex index number. 
				for (unsigned int j = 0; j < currentMesh->mNumVertices; j++) {
					cout << "\ttexture coordinates (" << currentMesh->mTextureCoords[0][j].x << ", "
						 << currentMesh->mTextureCoords[0][j].y << ")" << endl;
				}
			}
		} else {
			cout << "There is no texture coordinate in mesh # " << i << endl;
		}
	}
}

// Print out the output of the shader compiler. 
// Call this function after glCompileShader(). 
// If there is no error, OpenGL doesn't generate any message. 
void printShaderInfoLog(GLuint shaderID)
{
	unsigned int maxLength = 1024;
    char infoLog[1024];
	 int infologLength = 0;
 
	if (glIsShader(shaderID)) {
		glGetShaderInfoLog(shaderID, maxLength, &infologLength, infoLog);
	} else {
		cout << "printShaderInfoLog(): Invalid shader ID" << shaderID << endl;
	}
 
	if (infologLength > 0) {
		cout << "Shader: " << infoLog << endl;
	}
}

//--------------------------------------------
// Print out the output of the shader program.
// Call this function after glLinkProgram() or glValidateProgram().
// If there is no error, OpenGL doesn't generate any message. 
void printShaderProgramInfoLog(GLuint shaderProgID)
{
	unsigned int maxLength = 1024;
    char infoLog[1024];
	 int infologLength = 0;
 
	if (glIsProgram(shaderProgID)) {
		glGetProgramInfoLog(shaderProgID, maxLength, &infologLength, infoLog);
	} else {
		cout << "printShaderProgramInfoLog(): Invalid shader program ID" << shaderProgID << endl;
	}
 
	if (infologLength > 0) {
		cout << "Shader program: " << infoLog;
	}
}

//------------------------------------
// Convert an error type to a printable character string. 
const char* genErrorString( GLenum error )
{
    const char*  msg = NULL;

	// A clever technique to convert an OpenGL type to a character string.
	// The following are all the error types returned by glGetError(). 
    switch( error ) {
#define Case( Token )  case Token: msg = #Token; break;
	Case( GL_NO_ERROR );
	Case( GL_INVALID_VALUE );
	Case( GL_INVALID_ENUM );
	Case( GL_INVALID_OPERATION );
	Case( GL_STACK_OVERFLOW );
	Case( GL_STACK_UNDERFLOW );
	Case( GL_OUT_OF_MEMORY );
	Case( GL_INVALID_FRAMEBUFFER_OPERATION);
#undef Case	
    }

    return msg;
}

//----------------------------------------------------------------------------
// Print OpenGL error message, if any. 
void checkOpenGLError(string name = "")
{
    GLenum  error = glGetError();

	//If more than one flag has recorded an error, glGetError returns 
	// and clears an arbitrary error flag value. Thus, glGetError should always 
	// be called in a loop, until it returns GL_NO_ERROR.
    while(error != GL_NO_ERROR) {
		const char* msg = genErrorString(error);
		if (msg != NULL) {
			cout << "OpenGL error";
			
			if (name.length() > 0) {
				cout << " in " << name;  
			}

			cout << ": " << msg << endl;
		}

		// Continue to get error messsage until there is no error
		error = glGetError();
    } 
}

// This callback function displayes the debug message sent by OpenGL.
// Register this function by calling
// glDebugMessageCallback(openGLDebugCallback, nullptr);
void APIENTRY openGLDebugCallback(GLenum source, GLenum type, GLuint id, 
			  GLenum severity, GLsizei length, 
			  const GLchar* message, void* userParam) {
    cout << "---------------------opengl-callback-start------------" << endl;
    cout << "message: "<< message << endl;
    cout << "type: ";
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        cout << "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        cout << "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        cout << "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        cout << "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        cout << "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        cout << "OTHER";
        break;
    }
    cout << endl;
 
    cout << "id: " << id << endl;
	
	cout << "severity: ";
    switch (severity){
    case GL_DEBUG_SEVERITY_LOW:
        cout << "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        cout << "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        cout << "HIGH";
        break;
    }
    cout << endl;
    cout << "---------------------opengl-callback-end--------------" << endl;
}
