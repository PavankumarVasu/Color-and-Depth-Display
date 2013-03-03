#include <Windows.h>
#include <Ole2.h>
#include <iostream>

#include <gl/GL.h>
#include <gl/GLU.h>
#include <GL/glut.h>

#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSensor.h>

using namespace std;

// input to KINECT SDK calls to change the resolution 
// please change the preprocessor definitions here
#define CWIDTH 640
#define CHEIGHT 480
#define DWIDTH 640
#define DHEIGHT 480

// include the functions defined in GLutils.cpp
extern void display();
extern void myInit();
extern void doIdle();
extern void mappingFunc(BYTE* h_colorFrame, BYTE* h_mappedColor, LONG* h_colorCoordinates, int CtoDiv);

// OpenGL Variables
GLubyte colordata[CWIDTH*CHEIGHT*4]; // BGRA array that contains the texture data
GLushort depthdata[DWIDTH*DHEIGHT];  // Depth values stored in this array
GLubyte colortodepth[DWIDTH*DHEIGHT*4]; // BGRA array that contains the mapped color data

//Kinect variables
int CtoDdiv = CWIDTH/DWIDTH;
HANDLE rgbStream;  		  // Identifier of Kinect's Color Camera
HANDLE depthStream;           // Identifier of Kinect's Depth Camera
HANDLE NextDepthFrameEvent;   // Event handler for arrival of next depth frame
HANDLE NextColorFrameEvent;   // Event handler for arrival of next color frame
NUI_IMAGE_RESOLUTION colorResolution = NUI_IMAGE_RESOLUTION_640x480;
NUI_IMAGE_RESOLUTION depthResolution = NUI_IMAGE_RESOLUTION_640x480;
INuiSensor *sensor;           // Kinect Sensor


// Kinect Initialization, opening both the depth and color streams
HRESULT initKinect()
{
	// Getting a working Kinect sesor
	int numSensors;
	HRESULT hr = NuiGetSensorCount(&numSensors);
	if(FAILED(hr))		return hr;

	// identifying the sensor (We are assuming that there is only one sensor)
	hr = NuiCreateSensorByIndex(0,&sensor);
	if(FAILED(hr))		return hr;

	// Initialize sensor
	hr = sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR);
	if(FAILED(hr))		return hr;

	// Create an event that will be signaled when depth data is available
    NextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Initialize sensor to open up depth stream
	hr = sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_DEPTH,
		depthResolution,
		NUI_IMAGE_DEPTH_MAXIMUM,
		2,
		NextDepthFrameEvent,
		&depthStream);
	if(FAILED(hr))		return hr;

	// Create an event that will be signaled when color data is available
    NextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Initialize sensor to open up color stream
	hr = sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,
		colorResolution,
		0,
		2,
		NextColorFrameEvent,
		&rgbStream);
	if(FAILED(hr))		return hr;

	return S_OK;
}

// function to get the depth data from kinect: Please note that the depth 
// obtained from this function is in packed format has to be unpacked later
bool getDepth(GLushort *dest)
{
	NUI_IMAGE_FRAME imageFrame; // structure containing all the metadata about the frame
	NUI_LOCKED_RECT LockedRect; // contains the pointer to the actual data

	if(sensor->NuiImageStreamGetNextFrame(depthStream,0,&imageFrame) < 0)
		return false;

	INuiFrameTexture *texture = imageFrame.pFrameTexture;
	texture->LockRect(0,&LockedRect,NULL,0);

	// Now copy the data to our own memory location
	if(LockedRect.Pitch != 0)
	{
		const GLushort* curr = (const GLushort*) LockedRect.pBits;

		// copy the texture contents from current to destination
		memcpy( dest, curr, sizeof(GLushort)*(DWIDTH*DHEIGHT) );
	}

	texture->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(depthStream, &imageFrame);

	return true;
}

// function to get the color data from kinect
bool getColor(GLubyte *dest)
{
	NUI_IMAGE_FRAME imageFrame; // structure containing all the metadata about the frame
	NUI_LOCKED_RECT LockedRect; // contains the pointer to the actual data

	if(sensor->NuiImageStreamGetNextFrame(rgbStream,0,&imageFrame) < 0)
		return false;

	INuiFrameTexture *texture = imageFrame.pFrameTexture;
	texture->LockRect(0,&LockedRect,NULL,0);

	// Now copy the data to our own memory location
	if(LockedRect.Pitch != 0)
	{
		const BYTE* curr = (const BYTE*) LockedRect.pBits;

		// copy the texture contents from current to destination
		memcpy( dest, curr, sizeof(BYTE)*(CWIDTH*CHEIGHT*4) );
	}

	texture->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(rgbStream, &imageFrame);

	return true;
}

// function to map color pixel to depth. 
void MapColorToDepth(BYTE* colorFrame, USHORT* depthFrame, BYTE* mappedColor)
{
	// Initialize a vector for storing all the coordinate locations of the color image
	LONG* colorCoordinates = (LONG*)malloc(sizeof(LONG)*DWIDTH*DHEIGHT*2);

	// Find the location in the color image corresponding to the depth image
	sensor->NuiImageGetColorPixelCoordinateFrameFromDepthPixelFrameAtResolution(
		colorResolution,
		depthResolution,
		DWIDTH*DHEIGHT,
		depthFrame,
		(DWIDTH*DHEIGHT)*2,
		colorCoordinates);

	mappingFunc(colorFrame, mappedColor, colorCoordinates, CtoDdiv);

	free(colorCoordinates);
}

// Getting RGB and Depth data, note the additional procedure  
// used to synchronize the arrival of depth and the color data

void getKinectData(GLubyte *dest_color, GLushort *dest_depth, GLubyte *dest_mapped)
{
	bool needToMapColorToDepth = false;
	bool depthReceived = false;
	bool colorReceived = false;

	if( WAIT_OBJECT_0 == WaitForSingleObject(NextDepthFrameEvent, 0) )
	{
		// if we have received any valid new depth data we proceed to mapping
        if ( getDepth(dest_depth) )
        {
			depthReceived = true;
			if( WAIT_OBJECT_0 == WaitForSingleObject(NextColorFrameEvent, 0) )
			{
				// if we have received any valid new color data we proceed to mapping
				if ( getColor(dest_color) )
				{
					colorReceived = true;
					needToMapColorToDepth = true;
				}
			}
    }
	}
	
	if(needToMapColorToDepth)
	{
		MapColorToDepth((BYTE*)dest_color, (USHORT*)dest_depth, (BYTE*)dest_mapped);
	}
}

int main(int argc, char* argv[])
{
	// Check for Kinect
    HRESULT hr;
    if ((hr = initKinect()) != S_OK) return 1;

	glutInit(&argc,(char**)argv);
	glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB );
	glutInitWindowSize(CWIDTH,CHEIGHT);
	glutInitWindowPosition(100,100);
	glutCreateWindow("Skeleton-OpenGL");

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);

	/* Idle function where display is called again */
	glutIdleFunc(doIdle);

	myInit();

	glutMainLoop();

	return 0;
}
