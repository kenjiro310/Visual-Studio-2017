// John H Rucker 
// CSC 4820
// Project 1
// Loading and displaying a Textured Model

#ifdef _WIN32
#pragma comment(lib,"assimpd.lib")
#pragma comment(lib,"SOIL.lib") 
#pragma comment(lib,"glew32d.lib")

#endif

#include <SOIL.h> // Use SOIL library to load image.

#include <GL/glew.h> // include GLEW to access OpenGL 3.3 functions
#include <GL/freeglut.h> // GLUT is the toolkit to interface with the OS

#include "textfile.h" // auxiliary C file to read the shader text files

#include "assimp/Importer.hpp"	// C++ importer interface
#include "assimp/PostProcess.h"
#include "assimp/Scene.h" // Output data structure

#include <math.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// Information to render each assimp node
struct MiMesh{

	int numberFaces;
	GLuint vao, textIndex, blockIndex;

};

std::vector<struct MiMesh> MiMeshes;

// This is for a shader uniform block
struct MyMaterial{

	float diff[4], ambi[4], spec[4], emiss[4], shiney;
	int textCount;

};

// Model Matrix
float modelMatrix[24];

// For push and pop matrix
std::vector<float *> matrixStack;

// Vertex Attribute Locations
GLuint vertLoc = 0, normLoc = 1, coorLoc = 2;

// Uniform Bindings Points
GLuint matricesUniLoc = 1, materialUniLoc = 2;

// The sampler uniform for textured models
// we are assuming a single texture so this will
// always be texture unit 0
GLuint unitText = 0;

// Uniform Buffer for Matrices
// this buffer will contain 3 matrices: projection, view and model
// each matrix is a float array with 16 components
GLuint uniBufferMatix;

#define MatricesUniBufferSize sizeof(float) * 16 * 3
#define ProjMatrixOffset 0
#define ViewMatrixOffset sizeof(float) * 16
#define ModelMatrixOffset sizeof(float) * 16 * 2
#define MatrixSize sizeof(float) * 16

// Program and Shader Identifiers
GLuint prog, vertexShader, fragmentShader;

// Shader Names
char *vertexFileName = "dirlightdiffambpix.vert";
char *fragmentFileName = "dirlightdiffambpix.frag";

// Create an instance of the Importer class
Assimp::Importer import;

// The global Assimp scene object
const aiScene* sceneOnScreen = NULL;

// Scale factor for the model to fit in the window
float scaleFactor;

// Map image filenames to textureIds
// pointer to texture Array
std::map<std::string, GLuint> textureIdMap;

// Replace the model name by your model's filename
static const std::string modelname = "bench.obj";

// Camera Position
float cameraX = 0, cameraY = 0, cameraZ = 5;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 0.0f, beta = 0.0f, r = 5.0f;

#define M_PI       3.14159265358979323846f

static inline float
DegToRad(float degrees)
{
	return (float)(degrees * (M_PI / 180.0f));
};

// Frame counting and FPS computation
long time, timebase = 0, frame = 0;
char s[32];

//-----------------------------------------------------------------
// Print for OpenGL errors
//
// Returns 1 if an OpenGL error occurred, 0 otherwise.
//

#define printOpenGLError() printOglError(__FILE__, __LINE__)

int printOglError(char *file, int line)
{

	GLenum glErr;
	int retCode = 0;

	glErr = glGetError();
	if (glErr != GL_NO_ERROR)
	{
		printf("glError in file %s @ line %d: %s\n",
			file, line, gluErrorString(glErr));
		retCode = 1;
	}
	return retCode;
}

// ----------------------------------------------------
// VECTOR STUFF
//


// res = a cross b;
void crossProduct(float *a, float *b, float *res) {

	res[0] = a[1] * b[2] - b[1] * a[2];
	res[1] = a[2] * b[0] - b[2] * a[0];
	res[2] = a[0] * b[1] - b[0] * a[1];
}


// Normalize a vec3
void normalize(float *a) {

	float mag = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);

	a[0] /= mag;
	a[1] /= mag;
	a[2] /= mag;
}


// ----------------------------------------------------
// MATRIX STUFF
//

// Push and Pop for modelMatrix
void pushMatrix() {

	float *aux = (float *)malloc(sizeof(float) * 16);
	memcpy(aux, modelMatrix, sizeof(float) * 16);
	matrixStack.push_back(aux);
}

void popMatrix() {

	float *m = matrixStack[matrixStack.size() - 1];
	memcpy(modelMatrix, m, sizeof(float) * 16);
	matrixStack.pop_back();
	free(m);
}

// Sets the square matrix mat to the identity matrix,
// size refers to the number of rows (or columns)
void setIdentityMatrix(float *mat, int size) {

	// fill matrix with 0s
	for (int i = 0; i < size * size; ++i)
		mat[i] = 0.0f;

	// fill diagonal with 1s
	for (int i = 0; i < size; ++i)
		mat[i + i * size] = 1.0f;
}

// a = a * b;
void multMatrix(float *a, float *b) {

	float res[16];

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			res[j * 4 + i] = 0.0f;
			for (int k = 0; k < 4; ++k) {
				res[j * 4 + i] += a[k * 4 + i] * b[j * 4 + k];
			}
		}
	}
	memcpy(a, res, 16 * sizeof(float));

}

// Defines a transformation matrix mat with a translation
void setTranslationMatrix(float *mat, float x, float y, float z) {

	setIdentityMatrix(mat, 4);
	mat[12] = x;
	mat[13] = y;
	mat[14] = z;
}

// Defines a transformation matrix mat with a scale
void setScaleMatrix(float *mat, float sx, float sy, float sz) {

	setIdentityMatrix(mat, 4);
	mat[0] = sx;
	mat[5] = sy;
	mat[10] = sz;
}

// Defines a transformation matrix mat with a rotation 
// angle alpha and a rotation axis (x,y,z)
void setRotationMatrix(float *mat, float angle, float x, float y, float z) {

	float radAngle = DegToRad(angle);
	float co = cos(radAngle);
	float si = sin(radAngle);
	float x2 = x*x;
	float y2 = y*y;
	float z2 = z*z;

	mat[0] = x2 + (y2 + z2) * co;
	mat[4] = x * y * (1 - co) - z * si;
	mat[8] = x * z * (1 - co) + y * si;
	mat[12] = 0.0f;

	mat[1] = x * y * (1 - co) + z * si;
	mat[5] = y2 + (x2 + z2) * co;
	mat[9] = y * z * (1 - co) - x * si;
	mat[13] = 0.0f;

	mat[2] = x * z * (1 - co) - y * si;
	mat[6] = y * z * (1 - co) + x * si;
	mat[10] = z2 + (x2 + y2) * co;
	mat[14] = 0.0f;

	mat[3] = 0.0f;
	mat[7] = 0.0f;
	mat[11] = 0.0f;
	mat[15] = 1.0f;

}

// ----------------------------------------------------
// Model Matrix 
//
// Copies the modelMatrix to the uniform buffer


void setModelMatrix() {

	glBindBuffer(GL_UNIFORM_BUFFER, uniBufferMatix);
	glBufferSubData(GL_UNIFORM_BUFFER,
		ModelMatrixOffset, MatrixSize, modelMatrix);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

// The equivalent to glTranslate applied to the model matrix
void translate(float x, float y, float z) {

	float aux[16];

	setTranslationMatrix(aux, x, y, z);
	multMatrix(modelMatrix, aux);
	setModelMatrix();
}

// The equivalent to glRotate applied to the model matrix
void rotate(float angle, float x, float y, float z) {

	float aux[16];

	setRotationMatrix(aux, angle, x, y, z);
	multMatrix(modelMatrix, aux);
	setModelMatrix();
}

// The equivalent to glScale applied to the model matrix
void scale(float x, float y, float z) {

	float aux[16];

	setScaleMatrix(aux, x, y, z);
	multMatrix(modelMatrix, aux);
	setModelMatrix();
}

// ----------------------------------------------------
// Projection Matrix 
// Computes the projection Matrix and stores it in the uniform buffer

void buildProjectionMatrix(float fov, float ratio, float nearp, float farp) {

	float projMatrix[16];

	float f = 1.0f / tan(fov * (M_PI / 360.0f));

	setIdentityMatrix(projMatrix, 4);

	projMatrix[0] = f / ratio;
	projMatrix[1 * 4 + 1] = f;
	projMatrix[2 * 4 + 2] = (farp + nearp) / (nearp - farp);
	projMatrix[3 * 4 + 2] = (2.0f * farp * nearp) / (nearp - farp);
	projMatrix[2 * 4 + 3] = -1.0f;
	projMatrix[3 * 4 + 3] = 0.0f;

	// Add project matrix to the Uniform Buffer Object (UBO). 
	glBindBuffer(GL_UNIFORM_BUFFER, uniBufferMatix);
	glBufferSubData(GL_UNIFORM_BUFFER, ProjMatrixOffset, MatrixSize, projMatrix);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

// ----------------------------------------------------
// View Matrix
// Computes the viewMatrix and stores it in the uniform buffer
void setCamera(float posX, float posY, float posZ,
	float lookAtX, float lookAtY, float lookAtZ) {

	float dir[3], right[3], up[3];

	up[0] = 0.0f;	up[1] = 1.0f;	up[2] = 0.0f;

	dir[0] = (lookAtX - posX);
	dir[1] = (lookAtY - posY);
	dir[2] = (lookAtZ - posZ);
	normalize(dir);

	crossProduct(dir, up, right);
	normalize(right);

	crossProduct(right, dir, up);
	normalize(up);

	float viewMatrix[16], aux[16];

	viewMatrix[0] = right[0];
	viewMatrix[4] = right[1];
	viewMatrix[8] = right[2];
	viewMatrix[12] = 0.0f;

	viewMatrix[1] = up[0];
	viewMatrix[5] = up[1];
	viewMatrix[9] = up[2];
	viewMatrix[13] = 0.0f;

	viewMatrix[2] = -dir[0];
	viewMatrix[6] = -dir[1];
	viewMatrix[10] = -dir[2];
	viewMatrix[14] = 0.0f;

	viewMatrix[3] = 0.0f;
	viewMatrix[7] = 0.0f;
	viewMatrix[11] = 0.0f;
	viewMatrix[15] = 1.0f;

	setTranslationMatrix(aux, -posX, -posY, -posZ);

	multMatrix(viewMatrix, aux);

	// Add view matrix to the Uniform Buffer Object (UBO). 
	glBindBuffer(GL_UNIFORM_BUFFER, uniBufferMatix);
	glBufferSubData(GL_UNIFORM_BUFFER, ViewMatrixOffset, MatrixSize, viewMatrix);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// ----------------------------------------------------------------------------

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

void get_bounding_box_for_node(const aiNode* nd,
	aiVector3D* min,
	aiVector3D* max)

{
	aiMatrix4x4 prev;
	unsigned int n = 0, t;

	for (; n < nd->mNumMeshes; ++n) {
		const aiMesh* mesh = sceneOnScreen->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			aiVector3D temp = mesh->mVertices[t];

			min->x = aisgl_min(min->x, temp.x);
			min->y = aisgl_min(min->y, temp.y);
			min->z = aisgl_min(min->z, temp.z);

			max->x = aisgl_max(max->x, temp.x);
			max->y = aisgl_max(max->y, temp.y);
			max->z = aisgl_max(max->z, temp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(nd->mChildren[n], min, max);
	}
}

void get_bounding_box(aiVector3D* min, aiVector3D* max)
{

	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(sceneOnScreen->mRootNode, min, max);
}
//------------------------------------------------------------
// Importing the model with Assimp
//------------------------------------------------------------
bool ImportFrom3DFile(const std::string& pFile)
{

	//check if file exists
	std::ifstream fin(pFile.c_str());
	if (!fin.fail()) {
		fin.close();
	}
	else{
		printf("Couldn't open file: %s\n", pFile.c_str());
		printf("%s\n", import.GetErrorString());
		return false;
	}

	sceneOnScreen = import.ReadFile(pFile, aiProcessPreset_TargetRealtime_Quality);

	// If the import failed, report it
	if (!sceneOnScreen)
	{
		printf("%s\n", import.GetErrorString());
		return false;
	}

	// Now we can access the file's contents.
	printf("Import of scene %s succeeded.", pFile.c_str());

	aiVector3D scene_min, scene_max, scene_center;
	get_bounding_box(&scene_min, &scene_max);
	float temp;
	temp = scene_max.x - scene_min.x;
	temp = scene_max.y - scene_min.y > temp ? scene_max.y - scene_min.y : temp;
	temp = scene_max.z - scene_min.z > temp ? scene_max.z - scene_min.z : temp;
	scaleFactor = 4.f / temp; // Model zoom percentage

	// Everything will be cleaned up by the importer destructor
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
void genVAOsAndUniformBuffer(const aiScene *sc) {

	struct MiMesh aMesh;
	struct MyMaterial aMat;
	GLuint buffer;

	// For each mesh in the aiScene object
	for (unsigned int n = 0; n < sc->mNumMeshes; ++n)
	{
		// Get the current aiMesh object.
		const aiMesh* mesh = sc->mMeshes[n];

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
		aMesh.numberFaces = sc->mMeshes[n]->mNumFaces;

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
		if (mesh->HasPositions()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			// Transfer the faceArray to the vertex buffer object.
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
			// Link this VBO with a variable in the vertex shader.
			glEnableVertexAttribArray(vertLoc);
			glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, 0, 0);
		}

		// Generate a Vertex Buffer Object (VBO) for vertex normals
		if (mesh->HasNormals()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			// Transfer the vertex position array (stored in aiMesh's member variable mNormals) to the VBO.
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);

			// Link this VBO with a variable in the vertex shader.
			glEnableVertexAttribArray(normLoc);
			glVertexAttribPointer(normLoc, 3, GL_FLOAT, 0, 0, 0);
		}

		// Generate a Vertex Buffer Object (VBO) for vertex texture coordinates
		if (mesh->HasTextureCoords(0)) {
			float *texCoords = (float *)malloc(sizeof(float) * 2 * mesh->mNumVertices);
			for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {

				texCoords[k * 2] = mesh->mTextureCoords[0][k].x;
				texCoords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;

			}
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
			glEnableVertexAttribArray(coorLoc);
			glVertexAttribPointer(coorLoc, 2, GL_FLOAT, 0, 0, 0);
		}

		// Unbind VAO and VBOs. We don't need them for now. 
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// Create material uniform buffer
		aiMaterial *mtl = sc->mMaterials[mesh->mMaterialIndex];

		aiString texPath;	//contains filename of texture
		if (AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)){

			// Retrieve texture ID from the hash map and store it in the aMesh data structure. 
			// These texture IDs will be used in recursive_render() to bind the texture. 
			unsigned int texId = textureIdMap[texPath.data];
			aMesh.textIndex = texId;
			aMat.textCount = 1;
		}
		else
			aMat.textCount = 0;

		float c[4];
		set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
		aiColor4D diff;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diff))
			color4_to_float4(&diff, c);
		memcpy(aMat.diff, c, sizeof(c));

		set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
		aiColor4D ambi;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambi))
			color4_to_float4(&ambi, c);
		memcpy(aMat.ambi, c, sizeof(c));

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		aiColor4D spec;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &spec))
			color4_to_float4(&spec, c);
		memcpy(aMat.spec, c, sizeof(c));

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		aiColor4D emission;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
			color4_to_float4(&emission, c);
		memcpy(aMat.emiss, c, sizeof(c));

		float shiney = 0.0;
		unsigned int max;
		aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shiney, &max);
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


// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h) {

	float ratio;
	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if (h == 0)
		h = 1;

	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);

	ratio = (1.0f * w) / h;
	buildProjectionMatrix(53.13f, ratio, 0.1f, 100.0f);
}


// ------------------------------------------------------------
//
// Render stuff
//

// Render Assimp Model

void recursive_render(const aiScene *sc, const aiNode* nd)
{

	// Get node transformation matrix
	aiMatrix4x4 m = nd->mTransformation;
	// OpenGL matrices are column major
	m.Transpose();

	// Save model matrix and apply node transformation
	pushMatrix();

	float aux[16];
	memcpy(aux, &m, sizeof(float) * 16);
	multMatrix(modelMatrix, aux);
	setModelMatrix();

	// Draws all meshes assigned to this node
	for (unsigned int n = 0; n < nd->mNumMeshes; ++n){
		// bind material uniform
		glBindBufferRange(GL_UNIFORM_BUFFER, materialUniLoc, MiMeshes[nd->mMeshes[n]].blockIndex, 0, sizeof(struct MyMaterial));

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
	for (unsigned int n = 0; n < nd->mNumChildren; ++n){
		recursive_render(sc, nd->mChildren[n]);
	}
	popMatrix();
}

// Rendering Callback Function

void renderScene(void) {

	static float step = 0.0f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set camera matrix
	setCamera(cameraX, cameraY, cameraZ, 0, 0, 0);

	// set the model matrix to the identity Matrix
	setIdentityMatrix(modelMatrix, 4);

	// sets the model matrix to a scale matrix so that the model fits in the window
	scale(scaleFactor, scaleFactor, scaleFactor);

	// keep rotating the model
	rotate(step, 0.0f, 1.0f, 0.0f);

	// use our shader
	glUseProgram(prog);

	// we are only going to use texture unit 0
	// unfortunately samplers can't reside in uniform blocks
	// so we have set this uniform separately
	glUniform1i(unitText, 0);  // 0 means Texture Unit 0. It tells fragment shader to retrieve texture from Texture Unit 0. 

	recursive_render(sceneOnScreen, sceneOnScreen->mRootNode);

	// FPS computation and display
	frame++;
	time = glutGet(GLUT_ELAPSED_TIME);
	if (time - timebase > 1000) {
		sprintf(s, "FPS:%4.2f",
			frame*1000.0 / (time - timebase));
		timebase = time;
		frame = 0;
		glutSetWindowTitle(s);
	}

	// swap buffers
	glutSwapBuffers();

	// increase the rotation angle
	//step++;
}


// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy)
{
	switch (key) {

	case 27:

		glutLeaveMainLoop();
		break;

	case 'x': r -= 0.1f; break;
	case 'X': r += 0.1f; break;
	}
	cameraX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	cameraZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	cameraY = r * sin(beta * 3.14f / 180.0f);
}


// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN)  {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
	}

	//stop tracking the mouse
	else if (state == GLUT_UP) {
		if (tracking == 1) {
			alpha += (startX - xx);
			beta += (yy - startY);
		}
		else if (tracking == 2) {
			r += (yy - startY) * 0.01f;
		}
		tracking = 0;
	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{

	int deltaX, deltaY;
	float alphaAux, betaAux;
	float rAux;

	deltaX = startX - xx;
	deltaY = yy - startY;

	// left mouse button: move camera
	if (tracking == 1) {


		alphaAux = alpha + deltaX;
		betaAux = beta + deltaY;

		if (betaAux > 85.0f)
			betaAux = 85.0f;
		else if (betaAux < -85.0f)
			betaAux = -85.0f;

		rAux = r;

		cameraX = rAux * cos(betaAux * 3.14f / 180.0f) * sin(alphaAux * 3.14f / 180.0f);
		cameraZ = rAux * cos(betaAux * 3.14f / 180.0f) * cos(alphaAux * 3.14f / 180.0f);
		cameraY = rAux * sin(betaAux * 3.14f / 180.0f);
	}

	// right mouse button: zoom
	else if (tracking == 2) {

		alphaAux = alpha;
		betaAux = beta;
		rAux = r + (deltaY * 0.01f);

		cameraX = rAux * cos(betaAux * 3.14f / 180.0f) * sin(alphaAux * 3.14f / 180.0f);
		cameraZ = rAux * cos(betaAux * 3.14f / 180.0f) * cos(alphaAux * 3.14f / 180.0f);
		cameraY = rAux * sin(betaAux * 3.14f / 180.0f);
	}

}


void mouseWheel(int wheel, int direction, int x, int y) {

	r += direction * 0.1f;
	cameraX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	cameraZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	cameraY = r * sin(beta * 3.14f / 180.0f);
}

// --------------------------------------------------------
//
// Shader Stuff
//

void printProgramInfoLog(GLuint obj)
{
	int infologLength = 0;
	int charsWritten = 0;
	char *infoLog;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n", infoLog);
		free(infoLog);
	}
}

GLuint setupShaders() {

	char *vs = NULL, *fs = NULL, *fs2 = NULL;

	GLuint p, v, f;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead(vertexFileName);
	fs = textFileRead(fragmentFileName);

	const char * vv = vs;
	const char * ff = fs;

	glShaderSource(v, 1, &vv, NULL);
	glShaderSource(f, 1, &ff, NULL);

	free(vs); free(fs);

	glCompileShader(v);
	glCompileShader(f);

	p = glCreateProgram();
	glAttachShader(p, v);
	glAttachShader(p, f);

	glBindFragDataLocation(p, 0, "output");

	glBindAttribLocation(p, vertLoc, "Position");
	glBindAttribLocation(p, normLoc, "Normal");
	glBindAttribLocation(p, coorLoc, "Text Coordinate");

	glLinkProgram(p);
	glValidateProgram(p);
	printProgramInfoLog(p);

	prog = p;
	vertexShader = v;
	fragmentShader = f;

	GLuint k = glGetUniformBlockIndex(p, "Matrices");
	glUniformBlockBinding(p, k, matricesUniLoc);
	glUniformBlockBinding(p, glGetUniformBlockIndex(p, "Material"), materialUniLoc);

	unitText = glGetUniformLocation(p, "unitText");

	return(p);
}

// ------------------------------------------------------------
//
// Model loading and OpenGL setup
//


int init()
{
	if (!ImportFrom3DFile(modelname))
		return(0);

	glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)glutGetProcAddress("glGetUniformBlockIndex");
	glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)glutGetProcAddress("glUniformBlockBinding");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glutGetProcAddress("glGenVertexArrays");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glutGetProcAddress("glBindVertexArray");
	glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)glutGetProcAddress("glBindBufferRange");
	glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)glutGetProcAddress("glDeleteVertexArrays");

	prog = setupShaders();
	genVAOsAndUniformBuffer(sceneOnScreen);

	glEnable(GL_DEPTH_TEST); // Enable depth test
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue color
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_LIGHTING);

	// Generate a Uniform Buffer Object (UBO) for storing model, view, and projection matrices. 
	// This UBO is then linked with the uniform Matrices block in the vertex shader. 
	// This UBO is filled with matrices in other function calls. 
	glGenBuffers(1, &uniBufferMatix);
	glBindBuffer(GL_UNIFORM_BUFFER, uniBufferMatix);
	glBufferData(GL_UNIFORM_BUFFER, MatricesUniBufferSize, NULL, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, matricesUniLoc, uniBufferMatix, 0, MatricesUniBufferSize);	//setUniforms();
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return true;
}

// ------------------------------------------------------------
//
// Main function
//

int main(int argc, char **argv) {

	//  GLUT initialization
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1024, 768); // Windows height and width
	glutCreateWindow("CSC 4820 Project 1 - Assimp Demo");

	//  Callback Registration
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutIdleFunc(renderScene);

	//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc(mouseWheel);

	//	Init GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "GLEW failed to initialize \n");
		return -1;
	}

	//  Init the app (load model and textures) and OpenGL
	if (!init())
		printf("Model could not be loaded\n");

	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version: %s\n", glGetString(GL_VERSION));
	printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// GLUT main loop
	glutMainLoop();

	// Cleaning up
	textureIdMap.clear();

	// clear MiMeshes stuff
	for (unsigned int i = 0; i < MiMeshes.size(); ++i) {

		glDeleteVertexArrays(1, &(MiMeshes[i].vao));
		glDeleteTextures(1, &(MiMeshes[i].textIndex));
		glDeleteBuffers(1, &(MiMeshes[i].blockIndex));
	}

	// delete VBO
	glDeleteBuffers(1, &uniBufferMatix);

	return(0);
}





