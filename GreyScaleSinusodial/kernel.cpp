#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<GL/glew.h>
#include<GL/gl.h>
#include<cuda.h>
#include<cuda_runtime.h>
#include<cuda_gl_interop.h>
#include<vector_types.h>
#include<helper_functions.h>
#include<helper_cuda.h>
#include<timer.h>
#include<vector_types.h>
//#include "cutil/inc/cutil_inline.h"
//#include "cutil/inc/cutil_gl_inline.h"
#include <cuda_gl_interop.h>
//#include "cutil/inc/rendercheck_gl.h"

////////////////////////////////////////////////////////////////////////////////
// VBO specific code

#include"vmath.h"
#include"kernel.h"

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

typedef unsigned int uint;

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
const unsigned int mesh_height = 256;
const unsigned int RestartIndex = 0xffffffff;

using namespace vmath;

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
StopWatchInterface *timer = NULL;

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



void computeFPS()
{
	frame_count++;
	fps_count++;

	if (fps_count == fps_limit)
	{
		avg_fps = 1.0f / (sdkGetAverageTimerValue(&timer) / 1000.0f);
		fps_count = 0;
		fps_limit = (int)MAX(avg_fps, 1.0f);

		sdkResetTimer(&timer);
	}

	fprintf(gpFile, "Cuda GL Interop (VBO): %3.1f fps (Max 100Hz)\n", avg_fps);
} 



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

	

	//**** vertices, colors, shader attribs, vbo, vao initializations ****//
	//glShadeModel(GL_FLAT);
	//glClearDepth(1.0f);
	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glEnable(GL_CULL_FACE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


	resize(DIMX, DIMY);

}

void display(void)
{
	
	runCuda();   //run kernel to generate vertex positions
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//start using OpenGL program Object
	//glUseProgram(gShaderProgramObject);

	//OpenL Drawing 
	//set modelview & modelviewprojection matrices to identity 
	//mat4 modelviewMatrix = mat4::identity();
	//mat4 modelviewProjectionMatrix = mat4::identity();
	//modelviewMatrix = translate(0.0f, 0.0f, -3.0f);
	//modelviewProjectionMatrix = gPerspectiveProjectionMatrix * modelviewMatrix;	//ORDER IS IMPORTANT

	//																			//pass above modelviewprojection matrix to the vertex shader in 'u_mvp_matrix' shader variable
	//																			//whose position value we already calculated in initWithFrame() by using glGetUniformLocation()

	//glUniformMatrix4fv(gMVPUniform, 1, GL_FALSE, modelviewProjectionMatrix);
	//glUniform1f(color_Uniform, color);
	////*** bind vao ****

	// ****  draw either by glDrawTriangles() or glDrawarrays() or glDrawElements()
	//render from vbo
	///	glVertexPointer(4, GL_FLOAT, 0, 0);

	//glBindVertexArray(gVao);

	//glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//glVertexPointer(4, GL_FLOAT, 0, 0);
	//glEnableClientState(GL_VERTEX_ARRAY);
	//glBindBuffer(GL_ARRAY_BUFFER, gVbo_Color);

	//glDrawArrays(GL_POINTS, 0, mesh_width*mesh_height);			//(each with its x,y,z ) vertices in triangleVertices array
	//glDisableClientState(GL_VERTEX_ARRAY);


	////**** unbind vao ****
	//glBindVertexArray(0);	//stop using OpenGL program Object

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

	//destroy vbo
	/*if (vbo)
	{
		deleteVbo(&vbo, cuda_vbo_resource);
	}

	if (gVbo_Color)
	{
		glDeleteVertexArrays(1, &gVbo_Color);
		gVbo_Color = 0;
	}*/

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



























































