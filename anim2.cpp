////////////////////////////////////////////////////
// anim.cpp version 4.1
// Template code for drawing an articulated figure.
// CS 174A 
////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#include "GL/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#ifdef _WIN32
#include "GL/freeglut.h"
#else
#include <GLUT/glut.h>
#endif

#include "Ball.h"
#include "FrameSaver.h"
#include "Timer.h"
#include "Shapes.h"
#include "tga.h"

#include "Angel/Angel.h"

#ifdef __APPLE__
#define glutInitContextVersion(a,b)
#define glutInitContextProfile(a)
#define glewExperimental int glewExperimentalAPPLE
#define glewInit()
#endif

/////////////////////////////////////////////////////
// These are global variables
//
//
// Add additional ones if you need,
// especially for animation
//////////////////////////////////////////////////////

//**ADDITIONALS**//
float seg_length = 0.6;
int sway_amt = 4;
float sway_rate = 0.5;
float wasp_speed = 20;

FrameSaver FrSaver ;
Timer TM ;

BallData *Arcball = NULL ;
int Width = 800;
int Height = 800 ;
int Button = -1 ;
float Zoom = 1 ;
int PrevY = 0 ;

int Animate = 0 ;
int Recording = 0 ;

void resetArcball() ;
void save_image();
void instructions();
void set_colour(float r, float g, float b) ;

const int STRLEN = 100;
typedef char STR[STRLEN];

#define SQRT2 1.41421356237
#define PI    3.1415926535897
#define X 0
#define Y 1
#define Z 2

//texture
GLuint texture_cube;
GLuint texture_earth;

// Structs that hold the Vertex Array Object index and number of vertices of each shape.
ShapeData cubeData;
ShapeData sphereData;
ShapeData coneData;
ShapeData cylData;

// Matrix stack that can be used to push and pop the modelview matrix.
class MatrixStack {
    int    _index;
    int    _size;
    mat4*  _matrices;

   public:
    MatrixStack( int numMatrices = 32 ):_index(0), _size(numMatrices)
        { _matrices = new mat4[numMatrices]; }

    ~MatrixStack()
	{ delete[]_matrices; }

    void push( const mat4& m ) {
        assert( _index + 1 < _size );
        _matrices[_index++] = m;
    }

    mat4& pop( void ) {
        assert( _index - 1 >= 0 );
        _index--;
        return _matrices[_index];
    }
};

MatrixStack  mvstack;
mat4         model_view;
GLint        uModelView, uProjection, uView;
GLint        uAmbient, uDiffuse, uSpecular, uLightPos, uShininess;
GLint        uTex, uEnableTex;

// The eye point and look-at point.
// Currently unused. Use to control a camera with LookAt().
Angel::vec4 eye(0, 0.0, 50.0,1.0);
Angel::vec4 ref(0.0, 0.0, 0.0,1.0);
Angel::vec4 up(0.0,1.0,0.0,0.0);

double TIME = 0.0 ;


/////////////////////////////////////////////////////
//    PROC: drawCylinder()
//    DOES: this function 
//          render a solid cylinder  oriented along the Z axis. Both bases are of radius 1. 
//          The bases of the cylinder are placed at Z = 0, and at Z = 1.
//
//          
// Don't change.
//////////////////////////////////////////////////////
void drawCylinder(void)
{
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( cylData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cylData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawCone()
//    DOES: this function 
//          render a solid cone oriented along the Z axis with base radius 1. 
//          The base of the cone is placed at Z = 0, and the top at Z = 1. 
//         
// Don't change.
//////////////////////////////////////////////////////
void drawCone(void)
{
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( coneData.vao );
    glDrawArrays( GL_TRIANGLES, 0, coneData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawCube()
//    DOES: this function draws a cube with dimensions 1,1,1
//          centered around the origin.
// 
// Don't change.
//////////////////////////////////////////////////////
void drawCube(void)
{
    glBindTexture( GL_TEXTURE_2D, texture_cube );
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( cubeData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cubeData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawSphere()
//    DOES: this function draws a sphere with radius 1
//          centered around the origin.
// 
// Don't change.
//////////////////////////////////////////////////////
void drawSphere(void)
{
    glBindTexture( GL_TEXTURE_2D, texture_earth);
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( sphereData.vao );
    glDrawArrays( GL_TRIANGLES, 0, sphereData.numVertices );
}


void resetArcball()
{
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}


/*********************************************************
 PROC: set_colour();
 DOES: sets all material properties to the given colour
 -- don't change
 **********************************************************/

void set_colour(float r, float g, float b)
{
    float ambient  = 0.2f;
    float diffuse  = 0.6f;
    float specular = 0.2f;
    glUniform4f(uAmbient,  ambient*r,  ambient*g,  ambient*b,  1.0f);
    glUniform4f(uDiffuse,  diffuse*r,  diffuse*g,  diffuse*b,  1.0f);
    glUniform4f(uSpecular, specular*r, specular*g, specular*b, 1.0f);
}


/*********************************************************
 PROC: instructions()
 DOES: display instruction in the console window.
 -- No need to change

 **********************************************************/
void instructions()
{
    printf("Press:\n");
    printf("  s to save the image\n");
    printf("  r to restore the original view.\n") ;
    printf("  0 to set it to the zero state.\n") ;
    printf("  a to toggle the animation.\n") ;
    printf("  m to toggle frame dumping.\n") ;
    printf("  q to quit.\n");
}


/*********************************************************
 PROC: myinit()
 DOES: performs most of the OpenGL intialization
 -- change these with care, if you must.
 
 **********************************************************/
void myinit(void)
{
    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram(program);
    
    // Generate vertex arrays for geometric shapes
    generateCube(program, &cubeData);
    generateSphere(program, &sphereData);
    generateCone(program, &coneData);
    generateCylinder(program, &cylData);
    
    uModelView  = glGetUniformLocation( program, "ModelView"  );
    uProjection = glGetUniformLocation( program, "Projection" );
    uView       = glGetUniformLocation( program, "View"       );
    
    glClearColor( 0.7, 1.0, 1.0, 1.0 ); // dark blue background
    
    uAmbient   = glGetUniformLocation( program, "AmbientProduct"  );
    uDiffuse   = glGetUniformLocation( program, "DiffuseProduct"  );
    uSpecular  = glGetUniformLocation( program, "SpecularProduct" );
    uLightPos  = glGetUniformLocation( program, "LightPosition"   );
    uShininess = glGetUniformLocation( program, "Shininess"       );
    uTex       = glGetUniformLocation( program, "Tex"             );
    uEnableTex = glGetUniformLocation( program, "EnableTex"       );
    
    glUniform4f(uAmbient,    0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uDiffuse,    0.6f,  0.6f,  0.6f, 1.0f);
    glUniform4f(uSpecular,   0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uLightPos,  15.0f, 15.0f, 30.0f, 0.0f);
    glUniform1f(uShininess, 100.0f);
    
    glEnable(GL_DEPTH_TEST);
    
    Arcball = new BallData;
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}


//////////////////////////////////////////////////////
//    PROC: myKey()
//    DOES: this function gets caled for any keypresses
// 
//////////////////////////////////////////////////////
void myKey(unsigned char key, int x, int y)
{
    float time ;
    switch (key) {
        case 'q':
        case 27:
            exit(0); 
        case 's':
            FrSaver.DumpPPM(Width,Height) ;
            break;
        case 'r':
            resetArcball() ;
            glutPostRedisplay() ;
            break ;
        case 'a': // togle animation
            Animate = 1 - Animate ;
            // reset the timer to point to the current time		
            time = TM.GetElapsedTime() ;
            TM.Reset() ;
            // printf("Elapsed time %f\n", time) ;
            break ;
        case '0':
            //reset your object
            break ;
        case 'm':
            if( Recording == 1 )
            {
                printf("Frame recording disabled.\n") ;
                Recording = 0 ;
            }
            else
            {
                printf("Frame recording enabled.\n") ;
                Recording = 1  ;
            }
            FrSaver.Toggle(Width);
            break ;
        case 'h':
        case '?':
            instructions();
            break;
    }
    glutPostRedisplay() ;

}


/**********************************************
 PROC: myMouseCB()
 DOES: handles the mouse button interaction
 
 -- don't change
 **********************************************************/
void myMouseCB(int button, int state, int x, int y)
{
    Button = button ;
    if( Button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
        HVect arcball_coords;
        arcball_coords.x = 2.0*(float)x/(float)Width-1.0;
        arcball_coords.y = -2.0*(float)y/(float)Height+1.0;
        Ball_Mouse(Arcball, arcball_coords) ;
        Ball_Update(Arcball);
        Ball_BeginDrag(Arcball);

    }
    if( Button == GLUT_LEFT_BUTTON && state == GLUT_UP )
    {
        Ball_EndDrag(Arcball);
        Button = -1 ;
    }
    if( Button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN )
    {
        PrevY = y ;
    }


    // Tell the system to redraw the window
    glutPostRedisplay() ;
}


/**********************************************
 PROC: myMotionCB()
 DOES: handles the mouse motion interaction
 
 -- don't change
 **********************************************************/
void myMotionCB(int x, int y)
{
    if( Button == GLUT_LEFT_BUTTON )
    {
        HVect arcball_coords;
        arcball_coords.x = 2.0*(float)x/(float)Width - 1.0 ;
        arcball_coords.y = -2.0*(float)y/(float)Height + 1.0 ;
        Ball_Mouse(Arcball,arcball_coords);
        Ball_Update(Arcball);
        glutPostRedisplay() ;
    }
    else if( Button == GLUT_RIGHT_BUTTON )
    {
        if( y - PrevY > 0 )
            Zoom  = Zoom * 1.03 ;
        else 
            Zoom  = Zoom * 0.97 ;
        PrevY = y ;
        glutPostRedisplay() ;
    }
}


/**********************************************
 PROC: myReshape()
 DOES: handles the window being resized
 
 -- don't change
 **********************************************************/
void myReshape(int w, int h)
{
    Width = w;
    Height = h;
    
    glViewport(0, 0, w, h);
    
    mat4 projection = Perspective(50.0f, (float)w/(float)h, 1.0f, 1000.0f);
    glUniformMatrix4fv( uProjection, 1, GL_TRUE, projection );
}


/*********************************************************
 **********************************************************
 **********************************************************
 
 PROC: display()
 DOES: this gets called by the event handler to draw the scene
       so this is where you need to build your BEE
 
 MAKE YOUR CHANGES AND ADDITIONS HERE
 
 ** Add other procedures, such as drawLegs
 *** Use a hierarchical approach
 
 **********************************************************
 **********************************************************
 **********************************************************/

void display(void)
{
    // Clear the screen with the background colour (set in myinit)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    mat4 model_trans(1.0f);
    mat4 view_trans(1.0f);

	//mine

	mat4 flower_trans(1.0f);
	mat4 bee_trans(1.0f);

	///mine

	vec4 eye_pnt(0,8,8,1.0);
	vec4 ref_pnt(0,2,0,1.0);
	vec4 up_vec(0,1,0,0);

//	eye_pnt.x -= 2*TIME;
//	eye_pnt.y -= 
	if (TIME < 8.5)
	{
		eye_pnt.x = 40 - TIME*40/8.5;
		eye_pnt.y = 40 - ((40-8)/72.25)*TIME*TIME;
		eye_pnt.z = 40 - TIME*(40-8)/8.5;
	}
	// 72.25
	if (TIME >= 8.5 && TIME < 10.5)
	{
		eye_pnt.x -= 8*sin((TIME-8.5)*PI);
		eye_pnt.z -= 8*(1-cos((TIME-8.5)*PI));
	}
	if (TIME >= 10.5)
	{
		eye_pnt.x += (TIME - 10.5);
		eye_pnt.z += (TIME - 10.5);
	}
    view_trans *= LookAt(eye_pnt, ref_pnt, up_vec);
//  view_trans *= Translate(0.0f, 0.0f, -15.0f); //the same effect as zoom out
    
    // below deals with zoom in/out by mouse
    HMatrix r;
    Ball_Value(Arcball,r);
    mat4 mat_arcball_rot(
                         r[0][0], r[0][1], r[0][2], r[0][3],
                         r[1][0], r[1][1], r[1][2], r[1][3],
                         r[2][0], r[2][1], r[2][2], r[2][3],
                         r[3][0], r[3][1], r[3][2], r[3][3]);
    view_trans *= mat_arcball_rot;
    view_trans *= Scale(Zoom);
    
    glUniformMatrix4fv( uView, 1, GL_TRUE, model_view );
    
    mvstack.push(model_trans);//push, now identity is on the stack
    
   
/**************************************************************
   Your drawing/modeling starts here
***************************************************************/

	double ollie_start = 3;
	bool shuvit = true;
	bool kickflip = false;
	bool spin360 = true;

	if (TIME < 5)	 // POPPPED AN OLLIE I'M SWEATIN'
	{
		// set vars
		ollie_start = 3;
		shuvit = false;
		kickflip = false;
		spin360 = false;
	}
	else if (TIME < 8)		// SHUVIT
	{
		ollie_start = 6;
		shuvit = true;
		kickflip = false;
		spin360 = false;
	}
	else if (TIME < 11)		// KICKFLIP
	{
		ollie_start = 9;
		shuvit = false;
		kickflip = true;
		spin360 = false;
	}

	else if (TIME < 14)		// 360
	{
		ollie_start = 12;
		shuvit = true;
		kickflip = false;
		spin360 = true;
	}
	else	// TREFLIP
	{
		ollie_start = 15;
		shuvit = true;
		kickflip = true;
		spin360 = false;
	}

//////////////////////////////

	mat4 ollie_pop(1.0f);
	mat4 ollie_rotate(1.0f);
	mat4 ollie_translate(1.0f);
	
	if (TIME >= ollie_start && TIME < ollie_start + 0.25)
	{
		ollie_rotate *= Translate(-1,-0.4,0);
		ollie_rotate *= RotateZ(45*(4*(TIME - ollie_start)));
		ollie_rotate *= Translate(1,0.4,0);
	}
	if (TIME >= ollie_start + 0.25 && TIME < ollie_start + 0.75)
	{
		ollie_rotate *= Translate(1,-0.4,0);
		ollie_rotate *= RotateZ(45*2*((ollie_start + 0.75)- TIME));
		ollie_rotate *= Translate(-1,0.4,0);
		if (shuvit == true)
			ollie_rotate *= RotateY(-180*2*(TIME - (ollie_start + 0.25)));
		if (kickflip == true)
			ollie_rotate *= RotateX(-180*2*(TIME - (ollie_start + 0.25)));
	}
	if (TIME >= ollie_start + 0.75 && TIME < ollie_start + 1.25)
	{
		if (shuvit == true)
			ollie_rotate *= RotateY(-180*2*(TIME - (ollie_start + 0.75))-180);
		if (kickflip == true)
			ollie_rotate *= RotateX(-180*2*(TIME - (ollie_start + 0.75))-180);
	}

	double height = 0;
	if (TIME >= ollie_start + 0.25 && TIME < ollie_start + 1.25)
	{
		height = -(6)*(TIME - (ollie_start))*(TIME - (ollie_start + 1.25));
		if (TIME < ollie_start + 0.75)
			ollie_translate *= Translate((SQRT2 - 2)*2*(ollie_start + 0.75 - TIME), 0, 0);
		ollie_translate *= Translate(0,height,0);
	}
	ollie_pop *= ollie_translate;
	ollie_pop *= ollie_rotate;
	
	mat4 body_torso(1.0f);
	if (TIME >= ollie_start + 0.20 && TIME < ollie_start + 1.25)
	{
		if (spin360 == true && TIME >= ollie_start + 0.25)
			body_torso *= RotateY(-360*(TIME - (ollie_start + 0.25)));
		body_torso *= Translate(0,-(8)*(TIME - (ollie_start + 0.20))*(TIME - (ollie_start + 1.25)),0);
	}

	mat4 bu_rot(1.0f);
	if (TIME >= ollie_start && TIME < ollie_start + 0.25)
		bu_rot *= RotateX(-(ollie_start - TIME)*100);
	if (TIME >= ollie_start + 0.25 && TIME < ollie_start + 0.75)
		bu_rot *= RotateX(25-(TIME-(ollie_start + 0.25))*100);
	if (TIME >= ollie_start + 0.75 && TIME < ollie_start + 1)
		bu_rot *= RotateX(-(25-(TIME-(ollie_start + 0.75))*100));

	mat4 bl_rot(1.0f);
	if (TIME >= ollie_start && TIME < ollie_start + 0.25)
		bl_rot *= RotateX(2*(ollie_start - TIME)*100);
	if (TIME >= ollie_start + 0.25 && TIME < ollie_start + 0.75)
		bl_rot *= RotateX(-(2)*(25-(TIME-(ollie_start + 0.25))*100));
	if (TIME >= ollie_start + 0.75 && TIME < ollie_start + 1)
		bl_rot *= RotateX((2)*(25-(TIME-(ollie_start + 0.75))*100));

	mat4 fu_rot(1.0f);
	if (TIME >= ollie_start && TIME < ollie_start + 0.5)
		fu_rot *= RotateX(-(TIME - ollie_start)*100);
	if (TIME >= ollie_start + 0.5 && TIME < ollie_start + 1)
		fu_rot *= RotateX(-(50-(TIME - (ollie_start + 0.5))*100));

	mat4 fl_rot(1.0f);
	if (TIME >= ollie_start && TIME < ollie_start + 0.5)
		fl_rot *= RotateX(2*(TIME - ollie_start)*100);
	if (TIME >= ollie_start + 0.5 && TIME < ollie_start + 1)
		fl_rot *= RotateX(2*(50-(TIME - (ollie_start + 0.5))*100));

	mat4 ground_motion(1.0f);
	if (TIME < 2)
		ground_motion *= Translate(-4*TIME*TIME,0,0);
	else if (TIME < 18)
		ground_motion *= Translate(-16*(TIME-1),0,0);
	else
		ground_motion *= Translate(-16*(17),0,0);

// 0. Coordinates
	// Z
	model_trans *= Scale(0.01,0.01,5);
	model_view = view_trans * model_trans;
	set_colour(0,0,1);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Y
	model_trans *= RotateX(90);
	model_trans *= Scale(0.01,0.01,5);
	model_view = view_trans * model_trans;
	set_colour(0,1,0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// X
	model_trans *= RotateY(90);
	model_trans *= Scale(0.01,0.01,5);
	model_view = view_trans * model_trans;
	set_colour(1,0,0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
// 0. Terrain
// 0a. Ground
	// Sidewalk 1
	model_trans *= ground_motion;
	model_trans *= Translate(-70,-0.8,-28);
	model_trans *= Scale(210,0.5,60);
	model_view = view_trans * model_trans;
	set_colour(0.8,0.8,0.8);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Sidewalk 2
	model_trans *= ground_motion;
	model_trans *= Translate(65,-0.8,-28);
	model_trans *= Scale(35,0.5,60);
	model_view = view_trans * model_trans;
	set_colour(0.8,0.8,0.8);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Sidewalk 3
	model_trans *= ground_motion;
	model_trans *= Translate(115,-0.8,-28);
	model_trans *= Scale(35,0.5,60);
	model_view = view_trans * model_trans;
	set_colour(0.8,0.8,0.8);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Sidewalk 4
	model_trans *= ground_motion;
	model_trans *= Translate(162,-0.8,-28);
	model_trans *= Scale(35,0.5,60);
	model_view = view_trans * model_trans;
	set_colour(0.8,0.8,0.8);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Sidewalk 5
	model_trans *= ground_motion;
	model_trans *= Translate(210,-0.8,-28);
	model_trans *= Scale(35,0.5,60);
	model_view = view_trans * model_trans;
	set_colour(0.8,0.8,0.8);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Sidewalk 6
	model_trans *= ground_motion;
	model_trans *= Translate(260,-0.8,-28);
	model_trans *= Scale(35,0.5,60);
	model_view = view_trans * model_trans;
	set_colour(0.8,0.8,0.8);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Street
	model_trans *= ground_motion;
	model_trans *= Translate(50,-1.3,8);
	model_trans *= Scale(500,0.5,200);
	model_view = view_trans * model_trans;
	set_colour(0.2,0.2,0.2);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// 0b. Houses
	// Building 1
		// Body
	model_trans *= ground_motion;
	model_trans *= Translate(-20,9,-27);
	model_trans *= Scale(30,20,40);
	model_view = view_trans * model_trans;
	set_colour(184.0/255,134.0/255,11.0/255);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
		// Roof
	model_trans *= ground_motion;
	model_trans *= Translate(-20,9,-27);
	model_trans *= RotateZ(30);
	model_trans *= Scale(20,0.5,40);
	model_view = view_trans * model_trans;
	set_colour(184.0/255,134.0/255,11.0/255);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
		// Door
	
	// Building 2
	model_trans *= ground_motion;
	model_trans *= Translate(15,9,-27);
	model_trans *= Scale(30,20,40);
	model_view = view_trans * model_trans;
	set_colour(47.0/255,79.0/255,79.0/255);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Building 3
	model_trans *= ground_motion;
	model_trans *= Translate(65,9,-27);
	model_trans *= Scale(30,20,40);
	model_view = view_trans * model_trans;
	set_colour(255.0/255,228.0/255,196.0/255);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Building 4
	model_trans *= ground_motion;
	model_trans *= Translate(115,9,-27);
	model_trans *= Scale(30,20,40);
	model_view = view_trans * model_trans;
	set_colour(176.0/255,224.0/255,230.0/255);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Building 5
	model_trans *= ground_motion;
	model_trans *= Translate(162,9,-27);
	model_trans *= Scale(30,20,40);
	model_view = view_trans * model_trans;
	set_colour(143.0/255,188.0/255,143.0/255);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Building 6
	model_trans *= ground_motion;
	model_trans *= Translate(210,9,-27);
	model_trans *= Scale(30,20,40);
	model_view = view_trans * model_trans;
	set_colour(189.0/255,183.0/255,107.0/255);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Building 7
	model_trans *= ground_motion;
	model_trans *= Translate(260,9,-27);
	model_trans *= Scale(30,20,40);
	model_view = view_trans * model_trans;
	set_colour(205.0/255,133.0/255,63.0/255);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);

// 0b. House 1
// A. Skateboard
// A1. Deck
	//Middle
	model_trans *= ollie_pop;
	model_trans *= Scale(2.5,0.1,1);
	model_view = view_trans * model_trans;
	set_colour(0.5,0,0);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Tail
	model_trans *= ollie_pop;
	model_trans *= Translate(-1.25,0,0);
	model_trans *= RotateXYZ(90,0,-10);
	model_trans *= Scale(0.5,0.5,0.05);
	model_view = view_trans * model_trans;
	set_colour(0.5,0,0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Nose
	model_trans *= ollie_pop;
	model_trans *= Translate(1.25,0,0);
	model_trans *= RotateXYZ(90,0,10);
	model_trans *= Scale(0.5,0.5,0.05);
	model_view = view_trans * model_trans;
	set_colour(0.5,0,0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
// A2. Trucks
	// Back
	model_trans *= ollie_pop;
	model_trans *= Translate(-1,-0.1,0);
	model_trans *= Scale(0.3,0.2,0.3);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.5, 0.5);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Lower piece
	model_trans *= ollie_pop;
	model_trans *= Translate(-1,-0.3,0);
	model_trans *= Scale(0.2,0.2,0.2);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.5, 0.5);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Axle
	model_trans *= ollie_pop;
	model_trans *= Translate(-1,-0.4,0);
	model_trans *= Scale(0.03,0.03,0.5);
	model_view = view_trans * model_trans;
	set_colour(0.7, 0.7, 0.7);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Front
	model_trans *= ollie_pop;
	model_trans *= Translate(1,-0.1,0);
	model_trans *= Scale(0.3,0.2,0.3);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.5, 0.5);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Lower piece
	model_trans *= ollie_pop;
	model_trans *= Translate(1,-0.3,0);
	model_trans *= Scale(0.2,0.2,0.2);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.5, 0.5);
	drawCube();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Axle
	model_trans *= ollie_pop;
	model_trans *= Translate(1,-0.4,0);
	model_trans *= Scale(0.03,0.03,0.5);
	model_view = view_trans * model_trans;
	set_colour(0.7, 0.7, 0.7);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
// A3. Wheels
	// Front left
	model_trans *= ollie_pop;
	model_trans *= Translate(1,-0.4,0.4);
	model_trans *= Scale(0.15,0.15,0.075);
	model_view = view_trans * model_trans;
	set_colour(0.9, 0.9, 1);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Front right
	model_trans *= ollie_pop;
	model_trans *= Translate(1,-0.4,-0.4);
	model_trans *= Scale(0.15,0.15,0.075);
	model_view = view_trans * model_trans;
	set_colour(0.9, 0.9, 1);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Back left
	model_trans *= ollie_pop;
	model_trans *= Translate(-1,-0.4,0.4);
	model_trans *= Scale(0.15,0.15,0.075);
	model_view = view_trans * model_trans;
	set_colour(0.9, 0.9, 1);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Back right
	model_trans *= ollie_pop;
	model_trans *= Translate(-1,-0.4,-0.4);
	model_trans *= Scale(0.15,0.15,0.075);
	model_view = view_trans * model_trans;
	set_colour(0.9, 0.9, 1);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
// B. Rider
// B1. Body
// Upper part
	model_trans *= body_torso;
	model_trans *= Translate(-0.5,4,-0.3);//
	model_trans *= Scale(1,1.5,0.75);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.25, 0.0);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
// Lower part
	model_trans *= body_torso;
	model_trans *= Translate(-0.5,3.5,-0.3);//
	model_trans *= Scale(1,1.5,0.75);
	model_view = view_trans * model_trans;
	set_colour(0.4,0.7,1.0);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
// B2. Back leg
	// Upper
	model_trans *= body_torso;
	model_trans *= Translate(-1,2,-0.3);//

	model_trans *= Translate(0,0.75,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(-30,0,-20);
	model_trans *= Translate(0,-0.75,0);
	model_trans *= RotateX(90);
	model_trans *= Scale(0.25,0.25,0.75);
	model_view = view_trans * model_trans;
	set_colour(0.4,0.7,1.0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
//	Lower
	model_trans *= body_torso;
	model_trans *= Translate(-1,0.5,-0.3);//

	model_trans *= Translate(0,2.25,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(-30,0,-20);
	model_trans *= Translate(0,-2.25,0);

	model_trans *= Translate(0,0.75,0);
	model_trans *= bl_rot;
	model_trans *= RotateXYZ(60,0,20);
	model_trans *= Translate(0,-0.75,0);

	model_trans *= RotateX(90);
	model_trans *= Scale(0.25,0.25,0.75);
	model_view = view_trans * model_trans;
	set_colour(0.4,0.7,1.0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Knee
	model_trans *= body_torso;
	model_trans *= Translate(-1,1.275,-0.3);//

	model_trans *= Translate(0,1.5,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(-30,0,-20);
	model_trans *= Translate(0,-1.5,0);
	model_trans *= Scale(0.25,0.25,0.25);
	model_view = view_trans * model_trans;
	set_colour(0.4,0.7,1.0);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
// B3. Front leg
	// Upper
	model_trans *= body_torso;
	model_trans *= Translate(0,2,-0.3);//

	model_trans *= Translate(0,0.75,0);
	model_trans *= fu_rot;
	model_trans *= RotateXYZ(-30,0,20);
	model_trans *= Translate(0,-0.75,0);
	model_trans *= RotateX(90);
	model_trans *= Scale(0.25,0.25,0.75);
	model_view = view_trans * model_trans;
	set_colour(0.4,0.7,1.0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	//	Lower
	model_trans *= body_torso;
	model_trans *= Translate(0.0,0.5,-0.3);//

	model_trans *= Translate(0,2.25,0);
	model_trans *= fu_rot;
	model_trans *= RotateXYZ(-30,0,20);
	model_trans *= Translate(0,-2.25,0);

	model_trans *= Translate(0,0.75,0);
	model_trans *= fl_rot;
	model_trans *= RotateXYZ(60,0,-20);
	model_trans *= Translate(0,-0.75,0);

	model_trans *= RotateX(90);
	model_trans *= Scale(0.25,0.25,0.75);
	model_view = view_trans * model_trans;
	set_colour(0.4,0.7,1.0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Knee
	model_trans *= body_torso;
	model_trans *= Translate(0.9,1.275,-0.3);//

	model_trans *= Translate(0,1.5,0);
	model_trans *= fu_rot;
	model_trans *= RotateXYZ(-30,0,-20);
	model_trans *= Translate(0,-1.5,0);
	model_trans *= Scale(0.25,0.25,0.25);
	model_view = view_trans * model_trans;
	set_colour(0.4,0.7,1.0);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);

// B4. Head								MAP LIL B FACE!!!
	model_trans *= body_torso;
	model_trans *= Translate(-0.5,6,-0.3);//
	model_trans *= Scale(0.5,0.5,0.5);
	model_view = view_trans * model_trans;
	set_colour(1.0, 0.8, 0.6);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);

// B5. Feet
	// Front
	model_trans *= body_torso;
	model_trans *= Translate(0,0.5,0);

	model_trans *= Translate(0,2.25,0);
	model_trans *= fu_rot;
	model_trans *= RotateXYZ(-30,0,20);
	model_trans *= Translate(0,-2.25,0);

	model_trans *= Translate(0,0.75,0);
	model_trans *= fl_rot;
	model_trans *= RotateXYZ(60,0,-20);
	model_trans *= Translate(0,-0.75,0);

	model_trans *= Translate(0,-0.85,0);
	model_trans *= RotateX(-30);
	model_trans *= Scale(0.25,0.15,0.5);
	model_view = view_trans * model_trans;
	set_colour(0.25, 0.25, 0.25);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Back

	model_trans *= body_torso;
	model_trans *= Translate(-1,0.5,0);//

	model_trans *= Translate(0,2.25,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(-30,0,-20);
	model_trans *= Translate(0,-2.25,0);

	model_trans *= Translate(0,0.75,0);
	model_trans *= bl_rot;
	model_trans *= RotateXYZ(60,0,20);
	model_trans *= Translate(0,-0.75,0);

	model_trans *= Translate(0,-0.85,0);
	model_trans *= RotateX(-30);
	model_trans *= Scale(0.25,0.15,0.5);
	model_view = view_trans * model_trans;
	set_colour(0.25, 0.25, 0.25);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);

// B6. Arms
	// Back
		// Upper
	model_trans *= body_torso;
	model_trans *= Translate(-1,2+2,-0.3);//

	model_trans *= Translate(0,0.75,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(40,0,-40);
	model_trans *= Translate(0,-0.75,0);
	model_trans *= RotateX(90);
	model_trans *= Scale(0.25,0.25,0.75);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.25, 0.0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
//	Lower
	model_trans *= body_torso;
	model_trans *= Translate(-1,0.5+2,-0.3);//

	model_trans *= Translate(0,2.25,0);
	model_trans *= bu_rot; 
	model_trans *= RotateXYZ(40,0,-40);
	model_trans *= Translate(0,-2.25,0);

	model_trans *= Translate(0,0.75,0);
	model_trans *= bl_rot;
	model_trans *= RotateXYZ(-80,0,40);
	model_trans *= Translate(0,-0.75,0);

	model_trans *= RotateX(90);
	model_trans *= Scale(0.25,0.25,0.75);
	model_view = view_trans * model_trans;
	set_colour(1.0, 0.8, 0.6);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
		// Hand
	model_trans *= body_torso;
	model_trans *= Translate(-1,0.5+2,-0.3);

	model_trans *= Translate(0,2.25,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(40,0,-40);
	model_trans *= Translate(0,-2.25,0);

	model_trans *= Translate(0,0.75,0);
	model_trans *= bl_rot;
	model_trans *= RotateXYZ(-80,0,40);
	model_trans *= Translate(0,-0.75,0);

	model_trans *= Translate(0,-0.85,0);
	model_trans *= Scale(0.3,0.3,0.3);
	model_view = view_trans * model_trans;
	set_colour(1.0, 0.8, 0.6);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);


	// Front
		// Upper
	model_trans *= body_torso;
	model_trans *= Translate(0,2+2,-0.3);//

	model_trans *= Translate(0,0.75,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(40,0,40);
	model_trans *= Translate(0,-0.75,0);
	model_trans *= RotateX(90);
	model_trans *= Scale(0.25,0.25,0.75);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.25, 0.0);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
//	Lower
	model_trans *= body_torso;
	model_trans *= Translate(0,0.5+2,-0.3);//

	model_trans *= Translate(0,2.25,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(40,0,40);
	model_trans *= Translate(0,-2.25,0);

	model_trans *= Translate(0,0.75,0);
	model_trans *= bl_rot;
	model_trans *= RotateXYZ(-80,0,-40);
	model_trans *= Translate(0,-0.75,0);

	model_trans *= RotateX(90);
	model_trans *= Scale(0.25,0.25,0.75);
	model_view = view_trans * model_trans;
	set_colour(1.0, 0.8, 0.6);
	drawCylinder();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	// Hand
	model_trans *= body_torso;
	model_trans *= Translate(0,0.5+2,-0.3);

	model_trans *= Translate(0,2.25,0);
	model_trans *= bu_rot;
	model_trans *= RotateXYZ(40,0,40);
	model_trans *= Translate(0,-2.25,0);

	model_trans *= Translate(0,0.75,0);
	model_trans *= bl_rot;
	model_trans *= RotateXYZ(-80,0,-40);
	model_trans *= Translate(0,-0.75,0);

	model_trans *= Translate(0,-0.85,0);
	model_trans *= Scale(0.3,0.3,0.3);
	model_view = view_trans * model_trans;
	set_colour(1.0, 0.8, 0.6);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);




	/*
	model_trans *= Translate(-1.5,0.2,0);
	model_trans *= Scale(0.25,0.15,0.5);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.5, 0.5);
	drawSphere();
	model_trans = mvstack.pop();
	mvstack.push(model_trans);
	*/
// Done!
	mvstack.pop();

/**************************************************************
     Your drawing/modeling ends here
 ***************************************************************/
    
    glutSwapBuffers();
    if(Recording == 1)
        FrSaver.DumpPPM(Width, Height);
}


/*********************************************************
 **********************************************************
 **********************************************************
 
 PROC: idle()
 DOES: this gets called when nothing happens. 
       That's not true. 
       A lot of things happen can here.
       This is where you do your animation.
 
 MAKE YOUR CHANGES AND ADDITIONS HERE
 
 **********************************************************
 **********************************************************
 **********************************************************/
void idle(void)
{
    if( Animate == 1 )
    {
        // TM.Reset() ; // commenting out this will make the time run from 0
        // leaving 'Time' counts the time interval between successive calls to idleCB
        if( Recording == 0 )
            TIME = TM.GetElapsedTime() ;
        else
            TIME += 0.033 ; // save at 30 frames per second.
        
        //Your code starts here
        
        
        //Your code ends here
        
        printf("TIME %f\n", TIME) ;
        glutPostRedisplay() ;
    }
}

/*********************************************************
     PROC: main()
     DOES: calls initialization, then hands over control
           to the event handler, which calls 
           display() whenever the screen needs to be redrawn
**********************************************************/

int main(int argc, char** argv) 
{
    glutInit(&argc, argv);
    // If your code fails to run, uncommenting these lines may help.
    //glutInitContextVersion(3, 2);
    //glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition (0, 0);
    glutInitWindowSize(Width,Height);
    glutCreateWindow(argv[0]);
    printf("GL version %s\n", glGetString(GL_VERSION));
    glewExperimental = GL_TRUE;
    glewInit();
    
    instructions();
    myinit(); //performs most of the OpenGL intialization
    
    
    glutKeyboardFunc( myKey );   //keyboard event handler
    glutMouseFunc(myMouseCB) ;   //mouse button event handler
    glutMotionFunc(myMotionCB) ; //mouse motion event handler
    
    glutReshapeFunc (myReshape); //reshape event handler
    glutDisplayFunc(display);    //draw a new scene
    glutIdleFunc(idle) ;         //when nothing happens, do animaiton

    
    glutMainLoop();

    TM.Reset() ;
    return 0;         // never reached
}




