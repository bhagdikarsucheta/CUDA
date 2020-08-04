#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
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
#include<device_launch_parameters.h>
#include"vmath.h"

#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define MAX_EPSILON_ERROR 10.0f
#define THRESHHOLD 0.30f
#define REFRESH_DELAY 10
#define DIMX 1920
#define DIMY 1080
//#define DIM 1000
#define DIM 1024
#define PI acos(-1.)

GLuint vbo;
cudaGraphicsResource *vbo_res = NULL;
//dim3 grid(DIMX, DIMY);

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")

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

dim3 blocks(DIMX / 16, DIMY / 16);
dim3 threads(16, 16);


void createVbo();

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


struct CPUAnimBitmap {
	unsigned char *pixels;
	int           width, height;
	void          *dataBlock;

	CPUAnimBitmap(int w, int h, void *d = NULL) {
		width = w;
		height = h;
		pixels = new unsigned char[DIMX * DIMY * 4];
		dataBlock = d;
	}
};

struct DataBlock {
	unsigned char *dev_bitmap;
	CPUAnimBitmap *bitmap;
};

//DataBlock * dev_ptr;
unsigned char * dev_ptr;

//main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	//function prototype
	void display();
	void initialize();
	void uninitialize(void);
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
	createVbo();

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

///////////////////////////////////////////////////////////////////////////////////////////
////////////////////Simple Kernel to modify vertex positions in sine wave Pattern ////////
///////////////////////////////////////////////////////////////////////////////////////////
//__global__ void simple_vbo_kernel(float4 *pos, unsigned int width, unsigned int height, float time)
//{
//	unsigned int x = blockIdx.x*blockDim.x + threadIdx.x;
//	unsigned int y = blockIdx.y*blockDim.y + threadIdx.y;
//
//	// calculate uv coordinates
//	float u = x / (float)width;
//	float v = y / (float)height;
//	//u = u * 8.0f - 4.0f;
//	//v = v * 8.0f - 4.0f;
//
//	u = u * 3.0f - 1.5f;
//	v = v * 3.0f - 1.5f;
//
//	// calculate simple sine wave pattern
//	//float freq = 4.0f;
//	//float w = sinf(u*freq) * cosf(v*freq) * 0.5f;
//	////float w = 0.0f;
//	//// write output vertex
//	//pos[y*width + x] = make_float4(u, w, v, 1.0f);
//	float freq = 4.0f;
//	float w = sinf(u*freq + time)*cosf(v*freq + time)  * 0.5f;
//
//	// write output vertex
//	pos[y*width + x] = make_float4(u, w, v, 1.0f);
//
//}

__global__ void kernel(unsigned char *ptr, int ticks)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x;
	int y = threadIdx.y + blockIdx.y * blockDim.y;
	int offset = x + y * blockDim.x * gridDim.x;


	float fx = x - DIMX / 2;
	float fy = y - DIMY / 2;

	float d = sqrtf(fx * fx + fy * fy);

	unsigned char grey = (unsigned char)(120.0f + 119.0f *cos(d / 10.0f - ticks / 7.0f) / (d / 10.0f + 1.0f));

	ptr[offset * 4 + 0] = grey;
	ptr[offset * 4 + 1] = grey;
	ptr[offset * 4 + 2] = grey;
	ptr[offset * 4 + 3] = 255;
}

void generate_frame(DataBlock *d, int ticks)
{
	//dim3 blocks(DIM / 16, DIM / 16);
	//dim3 threads(16, 16);
	//kernel << <blocks, threads >> > (d->dev_bitmap, ticks);

		//HANDLE_ERROR(cudaMemcpy(d->bitmap->get_ptr(),
		//	d->dev_bitmap,
		//	d->bitmap->image_size(),
		//	cudaMemcpyHostToDevice));
}



void launch_kernel(unsigned char *d, int ticks)
{
	//execute the kernel
 // execute the kernel
	//dim3 block(8, 8, 1);	//8 * 8 *1 threads
	//dim3 grid(mesh_width / block.x, mesh_height / block.y, 1);	//1024 / 8 = 128	blocks , 1024 /8 = 128 blocks means(128*128 = 16384)blocks
	kernel << <blocks, threads >> > (d, ticks);

	//simple_vbo_kernel << < grid, block >> > (pos, mesh_width, mesh_height, time);
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
	checkCudaErrors(cudaGraphicsMapResources(1, &vbo_res, 0));
	size_t num_bytes,int ticks = GetTickCount();
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void**)&dev_ptr, &num_bytes, vbo_res));

	launch_kernel(dev_ptr,ticks);																	

	checkCudaErrors(cudaGraphicsUnmapResources(1, &vbo_res, 0));
}


void createVbo()
{	

	//assert(vbo);f
	glGenBuffers(1, &vbo);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, vbo);
	////initialize buffer object
	unsigned int  size = DIMX *DIMY*4;
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, size, NULL, GL_DYNAMIC_DRAW_ARB);
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);			//fininshing/completion of above gVboPosition buffer state
	////register this buffer object with CUDA
	//checkCudaErrors(cudaGraphicsGLRegisterBuffer(&vbo_res, *vbo, cudaGraphicsMapFlagsNone));
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&vbo_res, vbo, cudaGraphicsMapFlagsNone));

	//SDK_CHECK_ERROR_GL();
}

void deleteVbo()
{
	checkCudaErrors(cudaGraphicsUnregisterResource(vbo_res));
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB,0);
	glDeleteBuffers(1, &vbo);
	vbo = 0;
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
	
	DataBlock data;
	CPUAnimBitmap bitmap(DIMX, DIMY, &data);
	data.bitmap = &bitmap;

	glDrawPixels(DIMX, DIMY, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	//stop using OpenGL program Object
	SwapBuffers(ghdc);

	
}

void resize(int width, int height)
{
	//code
	if (height == 0)
		height = 1;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);

	gPerspectiveProjectionMatrix = perspective(60.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void uninitialize(void)
{
	void  deleteVbo();
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
	deleteVbo();
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



























































