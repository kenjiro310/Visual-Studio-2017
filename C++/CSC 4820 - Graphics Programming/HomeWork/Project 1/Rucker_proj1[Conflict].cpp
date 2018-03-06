// John H Rucker 
// CSC 4820
// Project 3
// Loading, displaying, moving and lighting a 3d model
//=============================================================================

/*

a.Translations == Press ' o ' to reset position
i.	Press (uppercase) X key: move the object toward the positive X direction
ii.	Press  (lowercase) x key: move the object toward the negative X direction
iii.	Press Y key: move the object toward the positive Y direction 
iv.	Press y key: move the object toward the negative Y direction
v.	Press Z key: move the object toward the positive Z direction
vi.	Press z key: move the object toward the negative Z direction

b.	Rotations == Press ' HOME ' to reset position
i.	The object should rotate around its own center, not the origin. 
ii.	Press Left Arrow key: rotate the object clockwise around its positive Y axis. 
iii. Press Right Arrow key: rotate the object counter clockwise around its Y axis. 
iv.	Press Up Arrow key: rotate the object clockwise around its positive X axis.   
v.	Press Down Arrow key: rotate the object counter clockwise around its X axis.

EXTRA CREDIT = Roll (R)

*/

#ifdef _WIN32
#pragma comment(lib,"assimpd.lib")
#pragma comment(lib,"SOIL.lib") 
#pragma comment(lib,"glew32d.lib")

#endif

#include <math.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <SOIL.h> // Use SOIL library to load image.

#include "assimp/Importer.hpp"	// C++ importer interface
#include "assimp/PostProcess.h"
#include "assimp/Scene.h" // Output data structure

#include <GL/glew.h> // include GLEW to access OpenGL 3.3 functions
#include <GL/freeglut.h> // GLUT is the toolkit to interface with the OS

#include "textfile.h" // auxiliary C file to read the shader text files


//==================================================
// Model Info
//==================================================

// Render each assimp node
// Create object
struct MiMesh
{

	int numberFaces;
	GLuint vao, textIndex, blockIndex;

};

std::vector<struct MiMesh> MiMeshes;

// Shader uniform block
// Create object
struct MiMaterial
{

	float diff[4], ambi[4], spec[4], emiss[4], shiney;
	int textCount;

};

// Model Matrix
float matrixModelX[16];

// Push and pop matrix
std::vector<float *> stackMatrix;

// Vertex Attribute Locations
GLuint vertLoc = 0, normLoc = 1, coorLoc = 2;

// Uniform Bindings Points
GLuint uniLocMatrix = 1, uniLocMaterial = 2;

// The sampler uniform for textured models
GLuint unitText = 0;

// Uniform Buffer for Matrices (contain 3 matrices: projection, view and model)
GLuint uniBufferMatix;

#define MatricesUniBufferSize sizeof(float) * 16 * 3
#define ProjMatrixOffset 0
#define ViewMatrixOffset sizeof(float) * 16
#define ModelMatrixOffset sizeof(float) * 16 * 2
#define MatrixSize sizeof(float) * 16

// Program and Shader Identifiers
GLuint  vertexShader, fragmentShader, prog;

// Shader Names
char *vertexFileName = "Rucker_vshader.vert";
char *fragmentFileName = "Rucker_fshader.frag";

// Create an instance of the Importer class
Assimp::Importer import;

// The global Assimp scene object
const aiScene* sceneOnScreen = NULL;

// Changes size for model to fit in the window
float modelWindowSize;

// Map image filenames to textureIds
// pointer to texture Array
std::map<std::string, GLuint> textMap;

// Replace the model name by your model's filename
static const std::string modelFile = "dog.obj";

//==================================================
// Camera Info
//==================================================

// Camera Position
float cameraX = 0, cameraY = 0, cameraZ = 5;

// Camera Coordinates
float alpha = 0.0f, beta = 0.0f, r = 5.0f;

float xTrans = 0.0f, yTrans = 0.0f, zTrans = 0.0f;

static float q = 0.0f, p = 0.0f, m = 0.0f;

#define M_PI       3.14159265f

static inline float
DegToRad(float degrees)
{
	return (float)(degrees * (M_PI / 180.0f));
};

//==================================================
// OpenGL Info
//==================================================

#define printOpenGLError() define errorPrintOutGl(__FILE__, __LINE__)

int errorPrintOutGl(char *file, int line)
{

	GLenum openGlError;
	int error = 0;

	openGlError = glGetError();
	if (openGlError != GL_NO_ERROR)
	{
		printf("Error in file %s @ line %d: %s\n",
			file, line, gluErrorString(openGlError));
		error = 1;
	}
	return error;
}

//
// ===========================================
// Vector detail
// ===========================================
//

// res = a x b;
void crossXProduct(float *a, float *b, float *res)
{

	res[0] = a[1] * b[2] - b[1] * a[2];
	res[1] = a[2] * b[0] - b[2] * a[0];
	res[2] = a[0] * b[1] - b[0] * a[1];
}

// Normalize a vec3
void standardize(float *a)
{

	float manage = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);

	a[0] /= manage;
	a[1] /= manage;
	a[2] /= manage;
}

//
// =================================================
// MATRIX Info
// =================================================
//

// Push and Pop for matrixModelX
void pushMat()
{

	float *change = (float *)malloc(sizeof(float) * 16);
	memcpy(change, matrixModelX, sizeof(float) * 16);
	stackMatrix.push_back(change);
}

void popMat()
{

	float *m = stackMatrix[stackMatrix.size() - 1];
	memcpy(matrixModelX, m, sizeof(float) * 16);
	stackMatrix.pop_back();
	free(m);
}

// Sets the square matrix (mat) to the identity matrix,
// size refers to the number of rows (or columns)
void setMatrixIdentity(float *mat, int size)
{

	// fill matrix with 0s
	for (int i = 0; i < size * size; ++i)
		mat[i] = 0.0f;

	// fill diagonal with 1s
	for (int i = 0; i < size; ++i)
		mat[i + i * size] = 1.0f;
}

// a = a * b
void matMulti(float *a, float *b)
{

	float res[16];

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			res[j * 4 + i] = 0.0f;
			for (int k = 0; k < 4; ++k)
			{
				res[j * 4 + i] += a[k * 4 + i] * b[j * 4 + k];
			}
		}
	}
	memcpy(a, res, 16 * sizeof(float));

}

// Defines a transformation matrix with a translation
void setMatrixTranslation(float *mat, float x, float y, float z)
{

	setMatrixIdentity(mat, 4);
	mat[12] = x;
	mat[13] = y;
	mat[14] = z;
}

// Defines a transformation matrix mat with a scale
void setMatrixScale(float *mat, float sx, float sy, float sz)
{

	setMatrixIdentity(mat, 4);
	mat[0] = sx;
	mat[5] = sy;
	mat[10] = sz;
}

// Defines a transformation matrix (mat) with a rotation 
// angle alpha and a rotation axis (x,y,z)
void setMatrixRotation(float *mat, float angle, float x, float y, float z)
{

	float angleRad = DegToRad(angle);
	float cosine = cos(angleRad);
	float sine = sin(angleRad);

	float x2 = x*x;
	float y2 = y*y;
	float z2 = z*z;

	mat[0] = x2 + (y2 + z2) * cosine;
	mat[4] = x * y * (1 - cosine) - z * sine;
	mat[8] = x * z * (1 - cosine) + y * sine;
	mat[12] = 0.0f;

	mat[1] = x * y * (1 - cosine) + z * sine;
	mat[5] = y2 + (x2 + z2) * cosine;
	mat[9] = y * z * (1 - cosine) - x * sine;
	mat[13] = 0.0f;

	mat[2] = x * z * (1 - cosine) - y * sine;
	mat[6] = y * z * (1 - cosine) + x * sine;
	mat[10] = z2 + (x2 + y2) * cosine;
	mat[14] = 0.0f;

	mat[3] = 0.0f;
	mat[7] = 0.0f;
	mat[11] = 0.0f;
	mat[15] = 1.0f;

}

//============================================== 
// Model Matrix 
//==============================================

// Copies the matrixModelX to the uniform buffer
void setMatrixModelX()
{

	glBindBuffer(GL_UNIFORM_BUFFER, uniBufferMatix);
	glBufferSubData(GL_UNIFORM_BUFFER,
		ModelMatrixOffset, MatrixSize, matrixModelX);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

// The equivalent to glTranslate applied to the model matrix
void translateModel(float x, float y, float z)
{

	float alter1[16];

	setMatrixTranslation(alter1, x, y, z);
	matMulti(matrixModelX, alter1);
	setMatrixModelX();
}

// The equivalent to glRotate applied to the model matrix
void rotateModel(float angle,float x, float y, float z)
{

	float turn[16];

	setMatrixRotation(turn, angle, x, y, z);  
	matMulti(matrixModelX, turn);
	setMatrixModelX();
}

// The equivalent to glscaleModel applied to the model matrix
void scaleModel(float x, float y, float z)
{

	float increase[16];

	setMatrixScale(increase, x, y, z);
	matMulti(matrixModelX, increase);
	setMatrixModelX();
}

//===================================================================
// Projection Matrix 
// Calculates projection Matrix and stores it in the uniform buffer
//===================================================================

void constructProjMatrix(float fov, float ratio, float nearp, float farp)
{

	float projectXMatrix[16];
	float f = 1.0f / tan(fov * (M_PI / 360.0f));

	setMatrixIdentity(projectXMatrix, 4);

	projectXMatrix[0] = f / ratio;
	projectXMatrix[1 * 4 + 1] = f;
	projectXMatrix[2 * 4 + 2] = (farp + nearp) / (nearp - farp);
	projectXMatrix[3 * 4 + 2] = (2.0f * farp * nearp) / (nearp - farp);
	projectXMatrix[2 * 4 + 3] = -1.0f;
	projectXMatrix[3 * 4 + 3] = 0.0f;

	// Add project matrix to the Uniform Buffer Object (UBO). 
	glBindBuffer(GL_UNIFORM_BUFFER, uniBufferMatix);
	glBufferSubData(GL_UNIFORM_BUFFER, ProjMatrixOffset, MatrixSize, projectXMatrix);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

}


//===============================================================
// View Matrix
// Computes the vMatriX and stores it in the uniform buffer
//================================================================

void setViewCamera(float xPos, float yPos, float zPos,
	float positionX, float positionY, float positionZ)
{

	float direction[3], right[3], up[3];

	up[0] = 0.0f;
	up[1] = 1.0f;
	up[2] = 0.0f;

	direction[0] = (positionX - xPos);
	direction[1] = (positionY - yPos);
	direction[2] = (positionZ - zPos);
	standardize(direction);

	crossXProduct(direction, up, right);
	standardize(right);

	crossXProduct(right, direction, up);
	standardize(up);

	float vMatriX[16], alter[16];

	vMatriX[0] = right[0];
	vMatriX[4] = right[1];
	vMatriX[8] = right[2];
	vMatriX[12] = 0.0f;

	vMatriX[1] = up[0];
	vMatriX[5] = up[1];
	vMatriX[9] = up[2];
	vMatriX[13] = 0.0f;

	vMatriX[2] = -direction[0];
	vMatriX[6] = -direction[1];
	vMatriX[10] = -direction[2];
	vMatriX[14] = 0.0f;

	vMatriX[3] = 0.0f;
	vMatriX[7] = 0.0f;
	vMatriX[11] = 0.0f;
	vMatriX[15] = 1.0f;

	setMatrixTranslation(alter, -xPos, -yPos, -zPos);

	matMulti(vMatriX, alter);

	// Add view matrix to the Uniform Buffer Object (UBO). 
	glBindBuffer(GL_UNIFORM_BUFFER, uniBufferMatix);
	glBufferSubData(GL_UNIFORM_BUFFER, ViewMatrixOffset, MatrixSize, vMatriX);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

void get_container_for_node(const aiNode* nd, aiVector3D* minX, aiVector3D* maxX)
{
	aiMatrix4x4 previous;
	unsigned int n = 0, t;

	for (; n < nd->mNumMeshes; ++n)
	{
		const aiMesh* mesh = sceneOnScreen->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t)
		{

			aiVector3D temp = mesh->mVertices[t];

			minX->x = aisgl_min(minX->x, temp.x);
			minX->y = aisgl_min(minX->y, temp.y);
			minX->z = aisgl_min(minX->z, temp.z);
			maxX->x = aisgl_max(maxX->x, temp.x);
			maxX->y = aisgl_max(maxX->y, temp.y);
			maxX->z = aisgl_max(maxX->z, temp.z);

		}
	}

	for (n = 0; n < nd->mNumChildren; ++n)
	{

		get_container_for_node(nd->mChildren[n], minX, maxX);

	}
}

void get_container(aiVector3D* minX, aiVector3D* maxX)
{

	minX->x = minX->y = minX->z = 1e10f;
	maxX->x = maxX->y = maxX->z = -1e10f;
	get_container_for_node(sceneOnScreen->mRootNode, minX, maxX);
}

//===================================================================
// Importing the model with Assimp
//===================================================================
bool ImportFrom3DFile(const std::string& pFile)
{

	//check if file exists
	std::ifstream fin(pFile.c_str());
	if (!fin.fail())
	{
		fin.close();
	}
	else{
		printf("Couldn't open file: %s\n", pFile.c_str());
		printf("%s\n", import.GetErrorString());
		return false;
	}

	//The 3D model file is loaded into Assimp's data structure through the following function
	sceneOnScreen = import.ReadFile(pFile, aiProcessPreset_TargetRealtime_Quality);

	// If the import failed, report it
	if (!sceneOnScreen)
	{
		printf("%s\n", import.GetErrorString());
		return false;
	}

	// Now we can access the file's contents.
	printf("Import of scene %s succeeded.", pFile.c_str());

	aiVector3D display_min, display_max, scene_center;

	get_container(&display_min, &display_max);

	float temp;

	temp = display_max.x - display_min.x;
	temp = display_max.y - display_min.y > temp ? display_max.y - display_min.y : temp;
	temp = display_max.z - display_min.z > temp ? display_max.z - display_min.z : temp;
	modelWindowSize = 3.0f / temp; // Model zoom percentage

	// Everything will be cleaned up by the importer
	return true;
}

void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

void color4_to_float4(const aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}


// Function that loads 3d model to window
void generateVAOandUBuffer(const aiScene *fd)
{

	GLuint buffer;
	struct MiMaterial aMat;
	struct MiMesh aMesh;

	// For each mesh in the aiScene object
	for (unsigned int n = 0; n < fd->mNumMeshes; ++n)
	{
		// Get the current aiMesh object.
		const aiMesh* mesh = fd->mMeshes[n];

		// Create array with faces
		// have to convert from Assimp format to array
		unsigned int *faceArray;
		faceArray = (unsigned int *)malloc(sizeof(unsigned int)* mesh->mNumFaces * 3);
		unsigned int faceIndex = 0;

		// Copy face indices from aiMesh to faceArray. 
		for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t]; // Go through the list of aiFace

			// For each aiFace, copy its indices to faceArray.
			memcpy(&faceArray[faceIndex], face->mIndices, 3 * sizeof(unsigned int));
			faceIndex += 3;
		}
		aMesh.numberFaces = fd->mMeshes[n]->mNumFaces;

		// Generate A Vertex Array Object for mesh
		// Bind the VAO so it's the active one. 
		// This VAO contains four VBOs: index, position, normal, and texture coordinates. 
		glGenVertexArrays(1, &(aMesh.vao));
		glBindVertexArray(aMesh.vao);

		// Generate and bind an index buffer for face indices (elements)
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
		// Fill the buffer with indices
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)* mesh->mNumFaces * 3, faceArray, GL_STATIC_DRAW);

		// Generate a Vertex Buffer Object (VBO) for vertex positions
		if (mesh->HasPositions())
		{
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			// Transfer the faceArray to the vertex buffer object.
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
			// Link this VBO with a variable in the vertex shader.
			glEnableVertexAttribArray(vertLoc);
			glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, 0, 0);
		}

		// Generate a Vertex Buffer Object (VBO) for vertex normals
		if (mesh->HasNormals())
		{
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			// Transfer the vertex position array (stored in aiMesh's member variable mNormals) to the VBO.
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);

			// Link this VBO with a variable in the vertex shader.
			glEnableVertexAttribArray(normLoc);
			glVertexAttribPointer(normLoc, 3, GL_FLOAT, 0, 0, 0);
		}

		// Generate a Vertex Buffer Object (VBO) for vertex texture coordinates
		if (mesh->HasTextureCoords(0))
		{
			float *texCoords = (float *)malloc(sizeof(float) * 2 * mesh->mNumVertices);
			for (unsigned int k = 0; k < mesh->mNumVertices; ++k)
			{

				texCoords[k * 2] = mesh->mTextureCoords[0][k].x;
				texCoords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;

			}
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
			glEnableVertexAttribArray(coorLoc);
			glVertexAttribPointer(coorLoc, 2, GL_FLOAT, 0, 0, 0);
		}

		// Unbind VAO and VBOs. 
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// Create material uniform buffer
		aiMaterial *material = fd->mMaterials[mesh->mMaterialIndex];

		aiString texPath;	//contains filename of texture
		if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath))
		{

			// Retrieve texture ID from the hash map and store it in the aMesh data structure. 
			// These texture IDs will be used in renderRecur() to bind the texture. 
			unsigned int texId = textMap[texPath.data];
			aMesh.textIndex = texId;
			aMat.textCount = 1;
		}
		else
			aMat.textCount = 0;

		float c[4];
		set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
		aiColor4D diff;
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diff))
			color4_to_float4(&diff, c);
		memcpy(aMat.diff, c, sizeof(c));

		set_float4 (c, 0.2f, 0.2f, 0.2f, 1.0f); //(c, 0.8f, 0.8f, 0.8f, 1.0f);
		aiColor4D ambi;
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambi))
			color4_to_float4(&ambi, c);
		memcpy(aMat.ambi, c, sizeof(c));

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		aiColor4D spec;
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &spec))
			color4_to_float4(&spec, c);
		memcpy(aMat.spec, c, sizeof(c));

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		aiColor4D emission;
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &emission))
			color4_to_float4(&emission, c);
		memcpy(aMat.emiss, c, sizeof(c));

		float shiney = 0.0;
		unsigned int max;
		aiGetMaterialFloatArray(material, AI_MATKEY_SHININESS, &shiney, &max);
		aMat.shiney = shiney;

		// Create a Uniform Buffer Object for the uniform variable block Materials
		// in the fragment shader. 
		glGenBuffers(1, &(aMesh.blockIndex));
		glBindBuffer(GL_UNIFORM_BUFFER, aMesh.blockIndex);
		// Fills UBO with material data.
		glBufferData(GL_UNIFORM_BUFFER, sizeof(aMat), (void *)(&aMat), GL_STATIC_DRAW);

		MiMeshes.push_back(aMesh);
	}
}


//=========================================================
// Window reshape Callback Function
//=========================================================

void alterSize(int width, int height)
{

	float ratio;
	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if (height == 0)
		height = 1;

	// Set the viewport to be the entire window
	glViewport(0, 0, width, height);

	ratio = (1.0f * width) / height;
	constructProjMatrix(53.13f, ratio, 0.1f, 100.0f);
}

//=========================================================
// Render Info
//=========================================================

// Render Assimp Model
// Shows how to traverse the node tree in a aiScene object and draw the 3D meshes attached to each node.
void renderRecur(const aiScene *fd, const aiNode* nd)
{

	// Get node transformation matrix
	aiMatrix4x4 m = nd->mTransformation;
	// OpenGL matrices are column major
	m.Transpose();

	// Save model matrix and apply node transformation
	pushMat();

	float change[16];
	memcpy(change, &m, sizeof(float) * 16);
	matMulti(matrixModelX, change);
	setMatrixModelX();

	// Draws all meshes assigned to this node
	for (unsigned int n = 0; n < nd->mNumMeshes; ++n)
	{
		// bind material uniform
		glBindBufferRange(GL_UNIFORM_BUFFER, uniLocMaterial, MiMeshes[nd->mMeshes[n]].blockIndex, 0, sizeof(struct MiMaterial));

		// glActiveTexture() indicates which texture unit the texture image will be sent to. 
		glActiveTexture(GL_TEXTURE0);
		// BindTexture" means that a texture image is transferred from main memory to GPU memory.
		glBindTexture(GL_TEXTURE_2D, MiMeshes[nd->mMeshes[n]].textIndex);

		// Bind VAO, which contains the VBOs for indices, positions, normals, and texture coordinates.
		glBindVertexArray(MiMeshes[nd->mMeshes[n]].vao);
		// Used because we have an index buffer in the VAO. 
		glDrawElements(GL_TRIANGLES, MiMeshes[nd->mMeshes[n]].numberFaces * 3, GL_UNSIGNED_INT, 0);

	}

	// draw all children
	for (unsigned int n = 0; n < nd->mNumChildren; ++n)
	{

		renderRecur(fd, nd->mChildren[n]);

	}
	popMat();
}

//===========================================================
// Rendering Callback Function -handles display event
//===========================================================
void scene_Render()
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	translateModel(xTrans, yTrans, zTrans);

	// set camera matrix
	setViewCamera(cameraX, cameraY, cameraZ, xTrans, yTrans, zTrans);

	// set the model matrix to the identity Matrix
	setMatrixIdentity(matrixModelX, 4);

	// Sets the model matrix to scale the size so that the model fits in the window
	scaleModel(modelWindowSize, modelWindowSize, modelWindowSize);

	// rotating the model
	rotateModel(p, 1.0f, 0.0f, 0.0f); //about the x-axis
	rotateModel(q, 0.0f, 1.0f, 0.0f); //about the y-axis
	rotateModel(m, 0.0f, 0.0f, 1.0f); //about the z-axis

	// use shader
	glUseProgram(prog);

	glUniform1i(unitText, 0);  // 0 means Texture Unit 0. It tells fragment shader to retrieve texture from Texture Unit 0. 

	renderRecur(sceneOnScreen, sceneOnScreen->mRootNode);

	// swap buffers
	glutSwapBuffers();

	// increase the rotation angle
	/*p++;
	q++;
	m++;*/

}

// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy)
{
	double f = .50;
	switch (key) {
		case 27: glutLeaveMainLoop(); break;

		case 'h': r += 0.1f; break;

		case 'X': xTrans -= f; 
			break;
		case 'x': xTrans += f; 
			break;

		case 'Y': yTrans -= f; 
			break;
		case 'y': yTrans += f; 
			break;

		case 'Z': zTrans -= f; 
			break;
		case 'z': zTrans += f; 
			break; 

		case 'R': ++m;//++m; 
			break; // Roll
		
		case 'o':	// Default, resets the translations vies from starting view
			xTrans = yTrans = 0.0f;
			zTrans = 0.0f;
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(cameraX, cameraY, cameraZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
			break;

	} 

	// Generate a display event, which forces Freeglut to call display(). 
	glutPostRedisplay();

}

void processSpecialKeys(int key, int xx, int yy)
{

	switch (key) {

	    case GLUT_KEY_LEFT: --q; break; // rotate clockwisen y-axis
		case GLUT_KEY_RIGHT: ++q; break; // rotate counterclockwise y-axis

		case GLUT_KEY_UP: --p; break; // rotate clockwise x-axis
		case GLUT_KEY_DOWN: ++p; break; // rotate counterclockwise x-axis

		case GLUT_KEY_HOME:	// Default, resets the translations vies from starting view
			q = p = 0.0f;
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(cameraX, cameraY, cameraZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
			break;
			
		
	}

	// Generate a display event, which forces Freeglut to call display(). 
	glutPostRedisplay();
}

GLuint shaderConfig()
{

	char *vertshade = NULL, *fragshade = NULL, *fragshade2 = NULL;

	GLuint p, v, f;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vertshade = textFileRead(vertexFileName);
	fragshade = textFileRead(fragmentFileName);

	const char * vv = vertshade;
	const char * ff = fragshade;

	glShaderSource(v, 1, &vv, NULL);
	glShaderSource(f, 1, &ff, NULL);

	free(vertshade); free(fragshade);

	glCompileShader(v);
	glCompileShader(f);

	p = glCreateProgram();
	glAttachShader(p, v);
	glAttachShader(p, f);

	glBindFragDataLocation(p, 0, "Output");

	glBindAttribLocation(p, vertLoc, "Position");
	glBindAttribLocation(p, normLoc, "Normal");
	glBindAttribLocation(p, coorLoc, "Text Coordinate");

	glLinkProgram(p);
	glValidateProgram(p);

	prog = p;
	vertexShader = v;
	fragmentShader = f;

	GLuint k = glGetUniformBlockIndex(p, "Matrices");
	glUniformBlockBinding(p, k, uniLocMatrix);
	glUniformBlockBinding(p, glGetUniformBlockIndex(p, "Material"), uniLocMaterial);

	unitText = glGetUniformLocation(p, "unitText");

	return(p);
}

// ------------------------------------------------------------
//
// Model loading and OpenGL setup
// ------------------------------------------------------------


int init()
{
	if (!ImportFrom3DFile(modelFile))
		return(0);

	glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)glutGetProcAddress("glGetUniformBlockIndex");
	glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)glutGetProcAddress("glUniformBlockBinding");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glutGetProcAddress("glGenVertexArrays");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glutGetProcAddress("glBindVertexArray");
	glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)glutGetProcAddress("glBindBufferRange");
	glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)glutGetProcAddress("glDeleteVertexArrays");

	prog = shaderConfig();
	generateVAOandUBuffer(sceneOnScreen);

	glEnable(GL_DEPTH_TEST); // Enable depth test
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // Black Color
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);

	// Generate a Uniform Buffer Object (UBO) for storing model, view, and projection matrices. 
	// This UBO is then linked with the uniform Matrices block in the vertex shader. 
	// This UBO is filled with matrices in other function calls. 
	glGenBuffers(1, &uniBufferMatix);
	glBindBuffer(GL_UNIFORM_BUFFER, uniBufferMatix);
	glBufferData(GL_UNIFORM_BUFFER, MatricesUniBufferSize, NULL, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, uniLocMatrix, uniBufferMatix, 0, MatricesUniBufferSize);	//setUniforms();
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glEnable(GL_MULTISAMPLE);

	return true;
}


//==============================================================
// Main function
//==============================================================

int main(int argc, char **argv)
{

	//  GLUT initialization
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1024, 768); // Windows height and width
	glutCreateWindow("CSC 4820 Project 2 - Translation and Rotation");

	//  Callback Registration
	glutDisplayFunc(scene_Render);
	glutReshapeFunc(alterSize);
	glutIdleFunc(scene_Render);

	//Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutSpecialFunc(processSpecialKeys);

	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	// Register a debug message callback function. 
	//glDebugMessageCallback((GLDEBUGPROC)openGLDebugCallback, nullptr);

	//	Init GLEW
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "GLEW initialization failed \n");
		return -1;
	}

	//  Init the app (load model and textures) and OpenGL
	if (!init())
		printf("Model failed to load\n");

	// Return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// GLUT main loop
	glutMainLoop();

	// delete VBO
	glDeleteBuffers(1, &uniBufferMatix);

	return(0);
}





