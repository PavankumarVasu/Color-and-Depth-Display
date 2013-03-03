#include <stdio.h>
#include <conio.h>
#include <Windows.h>
#include <Ole2.h>
#include <cuda_runtime.h>
#include <cuda.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <GL/glut.h>

// Preprocessor definitions for width and height of color and depth streams
#define CWIDTH 640
#define CHEIGHT 480
#define DWIDTH 640
#define DHEIGHT 480

/* Kernel to map color to the depth image space coordinates. 
   the inputs are color-space coordinates, buffer to fill up
   the mapped color */
__global__
void colordepthmap(BYTE* colorFrame, BYTE* mappedColor, LONG* colorCoordinates, int CtoDdiv)
{
  int x = blockIdx.x*blockDim.x + threadIdx.x;
	int y = blockIdx.y*blockDim.y + threadIdx.y;

	if (x >= CWIDTH || y >= CHEIGHT)
    {
        return;
    }

	int depthIndex = (y/CtoDdiv) * DWIDTH + (x/CtoDdiv);

	// extracting color coordinates mapped from the depth position (depthIndex)
	LONG colorForDepthX = colorCoordinates[depthIndex*2];
	LONG colorForDepthY = colorCoordinates[depthIndex*2 + 1];

	// check if the color coordinates lie within the range of the color map
	if(colorForDepthX >= 0 && colorForDepthX < CWIDTH && colorForDepthY >= 0 && colorForDepthY < CHEIGHT)
	{
		// calculate index in the color image
		int colorIndex = colorForDepthY * (CWIDTH*4) + (colorForDepthX*4);
		int m_ColorIndex = (y/CtoDdiv)*(DWIDTH*4)+(x/CtoDdiv)*4;

		mappedColor[m_ColorIndex] = colorFrame[colorIndex];
		mappedColor[m_ColorIndex+1] = colorFrame[colorIndex+1];
		mappedColor[m_ColorIndex+2] = colorFrame[colorIndex+2];
		mappedColor[m_ColorIndex+3] = colorFrame[colorIndex+3];
	}
}


// Function to initiate the kernel call
void mappingFunc(BYTE* h_colorFrame, BYTE* h_mappedColor, LONG* h_colorCoordinates, int CtoDiv)
{
	// Error code to check return values for CUDA calls
    cudaError_t err = cudaSuccess;

	/***** Allocating memory on the device/GPU *****/
	BYTE* d_colorFrame = NULL;
	BYTE* d_mappedColor = NULL;
	LONG* d_colorCoordinates = NULL;

	// Allocating memory for the color frame
	err = cudaMalloc(&d_colorFrame, sizeof(BYTE)* 4 * CHEIGHT * CWIDTH);

    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device memory for image (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

	// Allocating memory for mapped color
	err = cudaMalloc(&d_mappedColor, sizeof(BYTE)* 4 * DHEIGHT * DWIDTH);

	if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device memory for image (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

	// Allocating memory for color coordinates
	err = cudaMalloc(&d_colorCoordinates, sizeof(LONG)* 2 * DHEIGHT * DWIDTH);

	if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device memory for image (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }




	/***** Copying data from host to device *****/
	err = cudaMemcpy(d_colorFrame, h_colorFrame, sizeof(BYTE)* 4 * CHEIGHT * CWIDTH, cudaMemcpyHostToDevice);

    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to copy image from host to device (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

	err = cudaMemcpy(d_colorCoordinates, h_colorCoordinates, sizeof(LONG)* 2 * DHEIGHT * DWIDTH, cudaMemcpyHostToDevice);

    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to copy image from host to device (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }




	/***** Launching the Kernel code *****/
	dim3 threadsPerBlock(16, 16, 1);
    dim3 blocksPerGrid((CWIDTH+ 16-1)/16, (CHEIGHT+ 16-1)/16, 1);
	colordepthmap<<<blocksPerGrid, threadsPerBlock>>>(d_colorFrame, 
	                                                  d_mappedColor, 
													                          d_colorCoordinates, 
													                          CtoDiv);
	err = cudaGetLastError(); cudaDeviceSynchronize();



	/***** Copying data back from device to host *****/
	err = cudaMemcpy(h_mappedColor, d_mappedColor, sizeof(BYTE)* 4 * DHEIGHT * DWIDTH, cudaMemcpyDeviceToHost);

    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to copy image from device to host (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }



	/***** Freeing the memory allocated in the device *****/
	err = cudaFree(d_mappedColor);

    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to free device mapped image (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

	err = cudaFree(d_colorFrame);

    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to free device mapped image (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

	err = cudaFree(d_colorCoordinates);

    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to free device mapped image (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
}
