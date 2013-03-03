#include <Windows.h>
#include <Ole2.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <GL/glut.h>
#include <iostream>

using namespace std; 

// functions used from ColorAndDepth-Display.cpp
extern void getKinectData(GLubyte *dest_color, GLushort *dest_depth, GLubyte *dest_mapped);

// variables from ColorAndDepth-Display.cpp
extern GLubyte colordata[];
extern GLushort depthdata[];
extern GLubyte colortodepth[];

// preprocessor directives
#define CWIDTH 640
#define CHEIGHT 480
#define DWIDTH 640
#define DHEIGHT 480

// OpenGL Variables
int screen_width; 
int screen_height;
GLuint textureId;

void doIdle()
{
  glutPostRedisplay(); // calls the display function again
}

void display()
{
  glBindTexture(GL_TEXTURE_2D, textureId);
	getKinectData(colordata,depthdata,colortodepth);
	
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,DWIDTH,DHEIGHT,GL_BGRA_EXT,GL_UNSIGNED_BYTE,(void*)colortodepth);
	
	// mapping the texture on the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);		glVertex3f(0.0,0.0,0.0);
		glTexCoord2f(1.0f, 0.0f);       glVertex3f(DWIDTH,0.0,0.0);
		glTexCoord2f(1.0f, 1.0f);       glVertex3f(DWIDTH,DHEIGHT,0.0);
		glTexCoord2f(0.0f, 1.0f);       glVertex3f(0.0,DHEIGHT,0.0);
	glEnd();

	glutSwapBuffers();
}

void myInit()
{
	glClearColor(0.0,0.0,0.0,0.0);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST); // enable depth test to avoid z-buffer fighting

	// Initialize textures
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, DWIDTH, DHEIGHT, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)colortodepth);
	glBindTexture(GL_TEXTURE_2D,0);

	// Enable the texture map
	glEnable(GL_TEXTURE_2D);

	// Camera setup
	glViewport(0.0,0.0,CWIDTH,CHEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, CWIDTH, CHEIGHT, 0, 1, -1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
