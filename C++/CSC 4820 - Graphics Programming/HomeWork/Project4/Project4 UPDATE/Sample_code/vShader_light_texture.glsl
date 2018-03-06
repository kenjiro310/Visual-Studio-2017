#version 330

in vec3 vPos;
in vec3 vNormal;
in vec2 vTextureCoord; 

uniform mat4 mvpMatrix; // model_view_project matrix
uniform mat4 mvMatrix;	// model view matrix
uniform mat3 normalMatrix; // model matrix

out vec3 N; // The normal vector is passed over to the fragment shader
out vec3 v; // Vertex position is passed over to the fragment shader
out vec2 textureCoord; // The texture coordinates are passed over to the fragment shader

// Note that there is no out color, because the pixel color is calculated
// in the fragment shader. 

void main() 
{
    vec4 position = vec4(vPos.xyz, 1.0);
    gl_Position = mvpMatrix * position;

    vec4 eyespacePosition = mvMatrix * position;
    v = eyespacePosition.xyz;

    N = normalize(normalMatrix * vNormal);
	
	textureCoord = vTextureCoord;
}

