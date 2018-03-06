/* John Rucker
   CSC 4820
   Homework 1   
*/

/* sierpinski gasket using vertex buffer objects 
   This example is from the book "Interactive Computer Graphics" 6th edition
   by Ed Angel and Dave Shreiner
*/

#include "Angel.h"

const int NumPoints = 10000;

//----------------------------------------------------------------------------

void
init( void )
{
    vec3 points[NumPoints];

    // Specifiy the vertices for a tetrahedron
    vec3 vertices[] = {
	vec3( -0.5, -0.5, -0.5 ),
	vec3(  0.5, -0.5, -0.5 ),
	vec3(  0.0,  0.5,  0.0 ),
	vec3(  0.0, -0.5,  0.5 )
    };

    // Select an arbitrary initial point inside of the triangle
    points[0] = vec3( 0.0, 0.0, 0.0 );

    // Compute and store N-1 new points
    for ( int i = 1; i < NumPoints; ++i ) {
        int j = rand() % 3;   // pick a vertex at random

        // Compute the point halfway between the selected vertex
        // and the previous point
        points[i] = ( points[i - 1] + vertices[j] ) / 2.0;
    }

	// Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

	// Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW );

	// This tells the InitShader to make use of the following files
	// vshader23.glsl and fshader23.glsl
	GLuint program = InitShader("vshader23.glsl", "fshader23.glsl" );
    glUseProgram( program );

	// Initialize the vertex position attribute from the vertex shader
    GLuint loc = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( loc );
    glVertexAttribPointer( loc, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(0) );

	// Creates white background
    glClearColor( 1.0, 1.0, 1.0, 1.0 ); 
}

void
display( void )
{
	// Clear frame buffer
    glClear( GL_COLOR_BUFFER_BIT ); 

	// Draws the points
    glDrawArrays( GL_POINTS, 0, NumPoints );    
    glFlush(); // Ensures that the drawing commands are actually executed, 
			  // rather than stored in a buffer awaiting additional OpenGL commands
}

void
keyboard( unsigned char key, int x, int y ) // Captures messages from the keyboard
{
    switch ( key ) {
    case 033: // ESCAPE key
        exit( EXIT_SUCCESS );
        break;
    }
}

//----------------------------------------------------------------------------

int
main( int argc, char **argv )
{
    glutInit( &argc, argv );  // Initialize the GLUT library and negotiate a session with the window system.
    glutInitDisplayMode( GLUT_RGBA ); // Sets the initial display mode.
    glutInitWindowSize( 512, 512 ); // Sets the Window display mode.
    glutInitContextVersion( 3, 2 ); // Set a compatibility context with OpenGL 3.2
    glutInitContextProfile( GLUT_CORE_PROFILE ); // Get a core OpenGL 4.1 context with Debug
    glutCreateWindow( "Simple GLSL example" ); // Create the window

	glewExperimental = GL_TRUE; // Mpe added to avoid access violation error at first call to glGenVertexArrays
    glewInit();

    init(); // set OpenGL state and initialize shaders

    glutDisplayFunc( display ); // Set display callback fn: display will be called when 
								// the window needs redrawing
    glutKeyboardFunc( keyboard ); // Used to tell the windows system to process the keyboard function

    glutMainLoop(); // Enter event loop
    return 0;
}
