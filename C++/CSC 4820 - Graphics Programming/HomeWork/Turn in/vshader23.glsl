/* John Rucker
   CSC 4820
   Homework 1   
*/

// Here is the vertex shader. The vertex shader runs for every vertex in the mesh.  
// It receives input in the form of per-vertex variables called attribute variables, and per-polygon 
// variables called uniform variables.  It must specify the coordinates of the vertex in question.

#version 150

in vec4 vPosition;
out vec4 color;

void
main()
{
    color = vec4( 0.5 + vPosition.x, 0.5 + vPosition.y,	
		  0.5 + vPosition.z, 1.0 );
    gl_Position = vPosition;
}
