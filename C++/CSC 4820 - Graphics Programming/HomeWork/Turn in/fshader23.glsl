/* John Rucker
   CSC 4820
   Homework 1   
*/

// Here the fragment shader is passed values from the vertex shader.  
// The fragment shader is run for every pixel on the screen where the polygons of the mesh appear.  
// The fragment shader is responsible for setting the final color of that little piece of the mesh.

#version 150

in  vec4  color;
out vec4  fColor;

void
main()
{
    fColor = color;
}
