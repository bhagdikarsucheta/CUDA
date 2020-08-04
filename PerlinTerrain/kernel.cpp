#include<windows.h>


#include<GL/glew.h>
#include<GL/gl.h>
#include<cuda.h>

#include<cuda_runtime.h>
#include<cuda_gl_interop.h>
#include<cutil_inline.h>
#include<cutil_math.h>
#include<cutil_gl_inline.h>
#include<device_launch_parameters.h>
#include"glm/glm/glm.hpp"
#include"glm/glm/vec3.hpp"
#include"glm/glm/mat4x4.hpp"
#include"glm/glm/gtc/type_ptr.hpp"
#include"glm/glm/gtc/matrix_transform.hpp"


#include <cuda_gl_interop.h>

#ifndef __CUDACC__
#define __CUDACC__
#endif
#include<device_functions.h>

////////////////////////////////////////////////////////////////////////////////
// VBO specific code
#include"vmath.h"
#include"kernel.h"
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float cameraSpeed = 0.3f;

#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define MAX_EPSILON_ERROR 10.0f
#define THRESHHOLD 0.30f
#define REFRESH_DELAY 10
#define DIMX 1920
#define DIMY 1080
unsigned char * dev_ptr;
GLuint vbo;
cudaGraphicsResource *vbo_res = NULL;
#define Z_PLANE 50.0f

__constant__ unsigned char c_perm[256];
__shared__ unsigned char s_perm[256];	///shared memory copy of permutation array
unsigned char * d_perm = NULL;	///global memory copy of permutation array

typedef unsigned int uint;

float gain = 0.75f, xStart = 2.0f, yStart = 1.0f;
float zOffset = 0.0f, octaves = 2.0f, lacunarity = 2.0f;

typedef struct {
	GLuint vbo;
	GLuint typeSize;
	struct cudaGraphicsResource * cudaResource;
}mappedBuffer_t;

//vbo variables
mappedBuffer_t vertexVBO = { NULL,sizeof(float4),NULL };
mappedBuffer_t colorVBO = { NULL,sizeof(uchar4),NULL };
GLuint * qIndices = NULL;
int qIndexSize = 0;

 float animTime;

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")

const unsigned int mesh_width = 256;
const unsigned int mesh_height = 500;
const unsigned int RestartIndex = 0xffffffff;
void launch_kernel(float4* pos, uchar4* posColor,
	unsigned int mesh_width, unsigned int mesh_height, float time);

using namespace vmath;
const static unsigned char h_perm[] = { 151,160,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152,2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184,84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};
enum
{
	VDG_ATTRIBUTE_VERTEX = 0,
	VDG_ATTRIBUTE_COLOR,
	VDG_ATTRIBUTE_NORMAL,
	VDG_ATTRIBUTE_TEXTURE0,
};

//Prototype of WndProc() declared Globally
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


//Global variable declarations
FILE *gpFile = NULL;

HWND ghwnd = NULL;
HDC ghdc = NULL;
HGLRC ghrc = NULL;


DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

bool gbActiveWindow = false;
bool gbEscapeKeyIsPressed = false;
bool gbFullScreen = false;

GLuint gVertexShaderObject;
GLuint gFragmentShaderObject;
GLuint gShaderProgramObject;

GLuint gVao;
//GLuint vbo;
struct cudaGraphicsResource *cuda_vbo_resource;
GLuint gVbo_Color;
GLfloat g_fAnime = 0.0f;

GLuint gMVPUniform;

//mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;
float translate_z = -3.0f;
//StopWatchInterface *timer = NULL;

GLfloat color = 1.0f;
GLuint color_Uniform;

void createVbo(mappedBuffer_t * mbuf);

void runCuda();
//Auto-verifcation code
int fps_count = 0;
int fps_limit = 1;
int g_Index = 0;
float avg_fps = 0.0f;
unsigned int frame_count = 0;
unsigned int g_TotalErrors = 0;
bool g_ReadBack = false;

#define MAX(a,b) ((a > b) ? a : b)

mat4 gPerspectiveProjectionMatrix;
GLuint model_matrix_uniform, view_matrix_uniform, projection_matrix_uniform;

//main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	//function prototype
	void display();
	void initialize();
	void uninitialize(void);
	void initCuda();

	//variable declaration
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szClassName[] = TEXT("OpenGLPP");
	bool bDone = false;

	if (fopen_s(&gpFile, "Log.txt", "w") != 0)
	{
		MessageBox(NULL, TEXT("Log File Can not be Created\Exitting.."), TEXT("ERROR"), MB_OK | MB_TOPMOST | MB_ICONSTOP);
		exit(0);
	}
	else
	{
		fprintf(gpFile, "Log file is Successfully Openend \n");
	}

	//code
	//initializing member of WNDCLASS
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszClassName = szClassName;
	wndclass.lpszMenuName = NULL;

	//Registering Class
	RegisterClassEx(&wndclass);

	//CreateWindow
	hwnd = CreateWindowEx(WS_EX_APPWINDOW,
		szClassName,
		TEXT("OpenGL Programmable PipeLine Using Native Windowing: First ortho Trianle Window Shree Ganeshaya Namaha"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		0,
		0,
		WIN_WIDTH,
		WIN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	ghwnd = hwnd;

	//intitialize()
	//createVbo(&vbo, &cuda_vbo_resource, cudaGraphicsMapFlagsWriteDiscard);

	initialize();
	//createVbo(&vbo, &cuda_vbo_resource, cudaGraphicsMapFlagsWriteDiscard);



	//runCuda(&cuda_vbo_resource);


	//create vbo

	ShowWindow(hwnd, SW_SHOW);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);
	initCuda();
	//Message Loop
	while (bDone == false)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				bDone = true;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			//display();
			if (gbActiveWindow == true)
			{
				runCuda();

				display();

				if (gbEscapeKeyIsPressed == true)
					bDone = true;
			}
		}
	}

	uninitialize();
	return((int)msg.wParam);

}

//WndProc()
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	//function prototypes
	//	void display();
	void resize(int, int);
	void ToggleFullScreen(void);
	void uninitialize(void);

	//code
	switch (iMsg)
	{
	case WM_ACTIVATE:
		if (HIWORD(wParam) == 0)
			gbActiveWindow = true;
		else
			gbActiveWindow = false;
		break;
	case WM_PAINT:
		//		display();
		break;

	case WM_SIZE:
		resize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			gbEscapeKeyIsPressed = true;
			break;

		case 0x46:			//for 'f' or 'F'
			if (gbFullScreen == false)
			{
				ToggleFullScreen();
				gbFullScreen = true;
			}
			else
			{
				ToggleFullScreen();
				gbFullScreen = false;
			}
			break;
		case 'W':
		case 'w':
			cameraPos += cameraSpeed * cameraFront;

			break;
		case 'S':
		case 's':
			cameraPos -= cameraSpeed * cameraFront;
			break;

		case 'A':
		case 'a':
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp))*cameraSpeed;
			break;

		case 'D':
		case 'd':
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp))*cameraSpeed;

			break;
		default:
			break;
		}
		break;


	case WM_LBUTTONDOWN:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		break;
	}
	return(DefWindowProc(hwnd, iMsg, wParam, lParam));

}


void ToggleFullScreen(void)
{
	//variable declarations
	MONITORINFO mi;

	//code
	if (gbFullScreen == false)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			mi = { sizeof(MONITORINFO) };
			if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left,
					mi.rcMonitor.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_NOZORDER | SWP_FRAMECHANGED);
			}

		}
		ShowCursor(FALSE);
	}
	else
	{
		//code
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}
}




__device__ inline int perm(int i)
{
	return(s_perm[i & 0xff]);
}

__device__ inline float fade(float t)
{
	return t * t*t*(t*(t*6.0f - 15.0f) + 10.0f);
}

__device__ inline float lerpP(float t, float a, float b)
{
	return a + t * (b - a);
}

__device__ inline float grad(int hash, float x, float y, float z)
{
	int h = hash & 15;			//convert LO 4 bits of Hash code 
	float u = h < 8 ? x : y,	//into 12 gradient directions
		v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}



__device__ float inoise(float x, float y, float z)
{
	int X = ((int)floorf(x)) & 255,	//Find unit cube 
		Y = ((int)floorf(y)) & 255,	//contains Point
		Z = ((int)floorf(z)) & 255;


	x -= floorf(x);			//Find relative X,Y,Z
	y -= floorf(y);			//of that point in cube
	z -= floorf(z);

	float u = fade(x),			//compute fade curves
		v = fade(y),
		w = fade(z);

	int A = perm(X) + Y, AA = perm(A) + Z, AB = perm(A + 1) + Z,					//HASH coordinates of 
		B = perm(X + 1) + Y, BA = perm(B) + Z, BB = perm(B + 1) + Z;				//the 8 cube corners

	return lerpP(w, lerpP(v, lerpP(u, grad(perm(AA), x, y, z),
		grad(perm(BA), x - 1.0f, y, z)),
		lerpP(u, grad(perm(AB), x, y - 1.0, z),
			grad(perm(BB), x - 1.0, y - 1.0, z))),
		lerpP(v, lerpP(u, grad(perm(AA + 1), x, y, z - 1.0f),
			grad(perm(BA + 1), x - 1.0f, y, z - 1.0f)),
			lerpP(u, grad(perm(AB + 1), x, y - 1.0f, z - 1.0f),
				grad(perm(BB + 1), x - 1.0, y - 1.0, z - 1.0))));

	return(perm(X));

}



__device__ float fBm(float x, float y, int octaves, float lacunarity = 2.0f, float gain = 0.5f)
{
	float freq = 1.0f, amp = 0.5f;
	float sum = 0.0f;
	for (int i = 0; i < octaves; i++)
	{
		sum += inoise(x*freq, y*freq, Z_PLANE)*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}



__device__ inline uchar4 colorElevation(float texHeight)
{
	uchar4 pos;
	texHeight = 8.0f;
	////color texel (r,g,b,a)
	//if (texHeight < -1.000f) pos = make_uchar4(000, 000, 128, 255); //deeps
	//else if (texHeight < -.2500f) pos = make_uchar4(255, 255, 255, 255); //shallow
	//else if (texHeight < 0.0000f) pos = make_uchar4(255, 255, 255, 255); //shore
	//else if (texHeight < 0.0125f) pos = make_uchar4(255, 255, 255, 255); //sand
	//else if (texHeight < 0.1250f) pos = make_uchar4(255, 255, 255, 255); //grass
	//else if (texHeight < 0.3750f) pos = make_uchar4(255, 255, 255, 255); //dirt
	//else if (texHeight < 0.7500f) pos = make_uchar4(255, 255, 255, 255); //rock
	//else                          pos = make_uchar4(255, 255, 255, 255); //snow


	if(texHeight >= 8.0) pos = make_uchar4(255, 255, 255, 255);
	return(pos);


}

void checkCUDAError(const char *msg)
{
	cudaError_t err = cudaGetLastError();
	if (cudaSuccess != err)
	{
		fprintf(stderr, "Cuda error : %s:%s", msg, cudaGetErrorString(err));
		exit(EXIT_FAILURE);
	}
}





///Simple Kernel fills an array with perlin noise
__global__ void k_perlin(float4 *pos, uchar4 *colorPos,
	unsigned int width, unsigned int height,
	float2 start, float2 delta, float gain,
	float zOffset, unsigned char* d_perm,
	float ocataves, float lacunarity)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	float xCur = start.x + ((float)(idx%width)) * delta.x;
	float yCur = start.y + ((float)(idx / width)) * delta.y;

	if (threadIdx.x < 256)
		//optimization:this causes bank conflicts
		s_perm[threadIdx.x] = d_perm[threadIdx.x];
	//this synchronization can be imp.if there are more than 256 threads
	__syncthreads();

	//Each thread creates one pixel location in the texture (textel)
	if (idx < width*height)
	{
		float w = fBm(xCur, yCur, ocataves, lacunarity, gain) + zOffset;

		colorPos[idx] = colorElevation(w);
		float u = ((float)(idx%width)) / (float)width;
		float v = ((float)(idx / width)) / (float)height;
		u = u * 4.0f - 2.0f;
		v = v * 10.0f - 5.0f;
		w = (w > 0.0f) ? w : 0.0f;		//dont show region underwater
		pos[idx] = make_float4(u, w, v, 1.0f);

	}
}



uchar4 *eColor = NULL;
//Wrapper for __global__ call that setups the kernel call
 void launch_kernel(float4 *pos, uchar4 *posColor,
	unsigned int image_width, unsigned int image_height, float time)
{
	int nThreads = 556;	//must be equal or larger than 256!
	int totalThreads = image_height * image_width;
	int nBlocks = totalThreads / nThreads;
	nBlocks += ((totalThreads%nThreads) > 0) ? 1 : 0;

	float xExtent = 10.0f;
	float yExtent = 110.0f;
	float xDelta = xExtent / (float)image_width;
	float yDelta = yExtent / (float)image_height;


	if (!d_perm)
	{
		//for convenience allocate and copy d_perm here
		cudaMalloc((void**)&d_perm, sizeof(h_perm));
		cudaMemcpy(d_perm, h_perm, sizeof(h_perm), cudaMemcpyHostToDevice);
		checkCUDAError("d_perm malloc or copy failed!!");
	}

	k_perlin << <nBlocks, nThreads >> > (pos, posColor, image_width, image_height,
		make_float2(xStart, yStart),
		make_float2(xDelta, yDelta),
		gain, zOffset, d_perm,
		octaves, lacunarity);

	//make certain the kernel has completed
	cudaThreadSynchronize();
	checkCUDAError("kernel failed!!");

}






//////////////////////////////////////////////////////////////////////////

void runCuda()
{
	float4 *dptr;
	uchar4 *cptr;
	uint *iptr;
	size_t start;

	cudaGraphicsMapResources(1, &vertexVBO.cudaResource, NULL);
	cudaGraphicsResourceGetMappedPointer((void**)&dptr, &start, vertexVBO.cudaResource);
	cudaGraphicsMapResources(1, &colorVBO.cudaResource, NULL);
	cudaGraphicsResourceGetMappedPointer((void**)&cptr, &start, colorVBO.cudaResource);

	// execute the kernel
	launch_kernel(dptr, cptr, mesh_width, mesh_height, animTime);

	//unmap buffer object
	cudaGraphicsUnmapResources(1, &vertexVBO.cudaResource, NULL);
	cudaGraphicsUnmapResources(1, &colorVBO.cudaResource, NULL);
	
}

///////////////////////////////////////////////////////////////////////////////
//create VBO
/////////////////////////////////////////////////////////////////////////////1
void createVbo(mappedBuffer_t * mbuf)
{	

	glGenBuffers(1, &(mbuf->vbo));
	glBindBuffer(GL_ARRAY_BUFFER, mbuf->vbo);

	//initialize buffer object
	unsigned int size = mesh_width * mesh_height * mbuf->typeSize;
	glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	cudaGraphicsGLRegisterBuffer(&(mbuf->cudaResource), mbuf->vbo, cudaGraphicsMapFlagsNone);

}

void deleteVbo(mappedBuffer_t * mbuf)
{
	glBindBuffer(1, mbuf->vbo);
	glDeleteBuffers(1, &(mbuf->vbo));

	cudaGraphicsUnregisterResource(mbuf->cudaResource);
	mbuf->cudaResource = NULL;
	mbuf->vbo = NULL;
}

void cleanupCuda()
{
	if (qIndices) free(qIndices);
	deleteVbo(&vertexVBO);
	deleteVbo(&colorVBO);
}

//void initCuda

void initCuda()
{

	void cleanupCuda();
	createVbo(&vertexVBO);
	createVbo(&colorVBO);

	//allocate and assign triangle fan indices
	qIndexSize = 5 * (mesh_height - 1)*(mesh_width - 1);
	qIndices = (GLuint*)malloc(qIndexSize * sizeof(GLint));
	int index = 0;
	for (int i = 1; i < mesh_height; i++) {
		for (int j = 1; j < mesh_width; j++) {
			qIndices[index++] = (i)*mesh_width + j;
			qIndices[index++] = (i)*mesh_width + j - 1;
			qIndices[index++] = (i - 1)*mesh_width + j - 1;
			qIndices[index++] = (i - 1)*mesh_width + j;
			qIndices[index++] = RestartIndex;
		}
	}

	atexit(cleanupCuda);

	runCuda();


}

void initialize()
{
	//function prototype
	void uninitialize(void);
	void resize(int, int);
	//void createVbo(GLuint, struct cudsGraphicsResource**, unsigned int);

	

	//variable declaration
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex;

	//code
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

	//Initialization code
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 8;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cAlphaBits = 8;

	ghdc = GetDC(ghwnd);

	iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);
	if (iPixelFormatIndex == 0)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}
	if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == FALSE)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	ghrc = wglCreateContext(ghdc);
	if (ghrc == NULL)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	if (wglMakeCurrent(ghdc, ghrc) == FALSE)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	//provide source code to shader
	const GLchar *vertexShaderSourceCode =
		"#version 430 core" \
		"\n" \
		"in vec4 vPosition;" \
		"in vec3 vColor;"	\
		"uniform mat4 u_projection_matrix;" \
		"uniform mat4 u_view_matrix;" \
		"uniform mat4 u_model_matrix;" \
		"out vec3 out_Color;" \
		"void main(void)" \
		"{" \
		"gl_Position = u_view_matrix * u_model_matrix * vPosition;" \
		"out_Color = vColor;" \
		"}";

	glShaderSource(gVertexShaderObject, 1, (const GLchar **)&vertexShaderSourceCode, NULL);

	//compile shader
	glCompileShader(gVertexShaderObject);
	GLint iInfoLength = 0;
	GLint iShaderCompiledStatus = 0;
	char *szInfoLog = NULL;

	glGetShaderiv(gVertexShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(gVertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLength);
		if (iInfoLength > 0)
		{
			szInfoLog = (char *)malloc(iInfoLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gVertexShaderObject, iInfoLength, &written, szInfoLog);
				fprintf(gpFile, "Vertex Shader Compilation Log:%s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	//**********FRAGMENT SHADER*********
	//create shader
	gFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

	//provide source code to shader
	const GLchar *fragmentShaderSourceCode =
		"#version 430 core" \
		"\n" \
		"in vec3 out_Color;" \
		"uniform float color;"\
		"out vec4 FragColor;" \
		"void main (void)" \
		"{" \
		"FragColor = vec4(1,1,1,1.0);" \
		"}";

	glShaderSource(gFragmentShaderObject, 1, (const GLchar**)&fragmentShaderSourceCode, NULL);

	//compile shader
	glCompileShader(gFragmentShaderObject);
	glGetShaderiv(gFragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(gFragmentShaderObject, GL_INFO_LOG_LENGTH, &iShaderCompiledStatus);
		if (iInfoLength > 0)
		{
			szInfoLog = (char *)malloc(iInfoLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gFragmentShaderObject, iInfoLength, &written, szInfoLog);
				fprintf(gpFile, "Fragment Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}



	//********** SHADER PROGRAM *********
	//create
	gShaderProgramObject = glCreateProgram();

	//attach vertex shader to shader program
	glAttachShader(gShaderProgramObject, gVertexShaderObject);

	//attach fragment shader to shader program
	glAttachShader(gShaderProgramObject, gFragmentShaderObject);

	//pre-link binding of shader program object with vertex position attribute
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vPosition");

	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_COLOR, "vColor");

	//Link Shader
	glLinkProgram(gShaderProgramObject);
	GLint iShaderProgramLinkStatus = 0;
	glGetProgramiv(gShaderProgramObject, GL_LINK_STATUS, &iShaderProgramLinkStatus);
	if (iShaderProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(gShaderProgramObject, GL_INFO_LOG_LENGTH, &iInfoLength);
		if (iInfoLength > 0)
		{
			szInfoLog = (char *)malloc(iInfoLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetProgramInfoLog(gShaderProgramObject, iInfoLength, &written, szInfoLog);
				fprintf(gpFile, "Shader Program LinK Log %s\n");
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}


	//get MVP uniform locaion
	model_matrix_uniform = glGetUniformLocation(gShaderProgramObject, "u_model_matrix");
	view_matrix_uniform = glGetUniformLocation(gShaderProgramObject, "u_view_matrix");
	projection_matrix_uniform = glGetUniformLocation(gShaderProgramObject, "u_projection_matrix");
	color_Uniform = glGetUniformLocation(gShaderProgramObject, "color");

	glClearColor(0.0, 0.0, 0.0, 0.0f);


	resize(DIMX, DIMY);

}

void display(void)
{
	
	runCuda();   //run kernel to generate vertex positions
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(gShaderProgramObject);

	//OpenL Drawing 
	//set modelview & modelviewprojection matrices to identity 
	mat4 modelMatrix = mat4::identity();
	glm::mat4 viewMatrix;
	//////mat4 viewMatrix = mat4::identity();

	////////////////////////////////////////////Camera movement 
	mat4 projectionMatrix = mat4::identity();
	mat4 rotationmatrix = mat4::identity();
	mat4 scaleMatrix = mat4::identity();
	modelMatrix = translate(0.0f, -0.5f, -2.5f);
	scaleMatrix = scale(2.75f,  2.55f, 2.75f);
	//rotationmatrix = rotate(-30.0f, 1.0f, 0.0f, 0.0f);

	viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	modelMatrix = modelMatrix * scaleMatrix;
	modelMatrix = modelMatrix * rotationmatrix;

	//multipley the modelview and orthographic matrix to get modelviewprojection matrix

	projectionMatrix = gPerspectiveProjectionMatrix * modelMatrix;	//ORDER IS I
	glUniformMatrix4fv(model_matrix_uniform, 1, GL_FALSE, modelMatrix);
	glUniformMatrix4fv(view_matrix_uniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	//glUniformMatrix4fv(view_matrix_uniform, 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(projection_matrix_uniform, 1, GL_FALSE, projectionMatrix);
	glUniform1f(color_Uniform, color);

	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO.vbo);
	glVertexPointer(4, GL_FLOAT, 0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, colorVBO.vbo);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);
	glEnableClientState(GL_COLOR_ARRAY);

	glPrimitiveRestartIndexNV(RestartIndex);
	glEnableClientState(GL_PRIMITIVE_RESTART_NV);
	glDrawElements(GL_TRIANGLE_FAN, qIndexSize, GL_UNSIGNED_INT, qIndices);
	glDisableClientState(GL_PRIMITIVE_RESTART_NV);
	
	
	//glDrawArrays(GL_POINTS, 0, mesh_width * mesh_height);


	/*for (int i = 0; i < mesh_width*mesh_height; i += mesh_width)
		glDrawArrays(GL_LINE_STRIP, i, mesh_width);*/
	glUseProgram(0);

	SwapBuffers(ghdc);

	animTime += 0.01f;
}

void resize(int width, int height)
{
	//code
	if (height == 0)
		height = 1;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);

	gPerspectiveProjectionMatrix = perspective(60.0f, (GLfloat)width / (GLfloat)height, 0.10f, 10.0f);
}

void uninitialize(void)
{
	//void  deleteVbo(mappedBuffer_t*);
	//UNINITIALIZATION CODE
	if (gbFullScreen == true)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}

	//destroy vao
	if (gVao)
	{
		glDeleteVertexArrays(1, &gVao);
		gVao = 0;
	}
	//deleteVbo(&mbuf);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);



	//detach vertex shader from shader program object
	glDetachShader(gShaderProgramObject, gVertexShaderObject);

	//detach fragment shader from shader program object
	glDetachShader(gShaderProgramObject, gFragmentShaderObject);

	//delete vertex shader object 
	glDeleteShader(gVertexShaderObject);
	gVertexShaderObject = 0;

	//delete fragment shader object
	glDeleteShader(gFragmentShaderObject);
	gFragmentShaderObject = 0;

	//unlink shader program
	glUseProgram(0);

	wglMakeCurrent(NULL, NULL);

	wglDeleteContext(ghrc);
	ghrc = NULL;

	ReleaseDC(ghwnd, ghdc);
	ghdc = NULL;

	DestroyWindow(ghwnd);
	ghwnd = NULL;

}




























































