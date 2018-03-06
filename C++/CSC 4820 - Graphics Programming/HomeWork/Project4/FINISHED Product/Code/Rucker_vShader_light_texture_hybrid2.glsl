#version 330

#if __VERSION__ >= 130
in vec3 vPos;
in vec3 vNormal; //normal;
in vec2 vTextureCoord;
#else
attribute vec3 vPos;
attribute vec2 vTextureCoord;
#endif

uniform mat4 mvpMatrix; // model_view_project matrix
//added
uniform mat4 mvMatrix;	// model view matrix
uniform mat3 normalMatrix; // model matrix

//out vec3 N; // The normal vector is passed over to the fragment shader
//out vec3 v; // Vertex position is passed over to the fragment shader
///////////////

#if __VERSION__ >= 130
out vec2 textureCoord;
#else 
varying vec2 textureCoord;
#endif

void main() {

    // Construct a homogeneous coordinate 
    vec4 position = vec4(vPos.xyz, 1.0f);

    gl_Position = mvpMatrix * position;

    textureCoord = vTextureCoord;
  
}

