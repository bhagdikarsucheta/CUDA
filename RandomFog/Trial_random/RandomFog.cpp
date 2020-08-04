
#include <Windows.h>
#include<stdio.h>
#include<glew.h>
#include<gl/GL.h>

//System includes
#include<stdexcept>
#include<sstream>
#include<iomanip>
#include<math.h>

//CUDA utilities and system includes
#include"C:/ProgramData/NVIDIA Corporation/CUDA Samples/v10.1/common/inc/helper_cuda.h"
//#include"C:/ProgramData/NVIDIA Corporation/CUDA Samples/v10.1/common/inc/rendercheck_gl.h"


#include"vmath.h"
#include"rng.h"

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"curand.lib")
//#pragma comment(lib,"freegut.lib")


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


//RNG instance
RNG *g_pRng = NULL;

//CheckRender Instance
//CheckRender *g_pCheckRender = NULL;


//Simple struct which contains the position and color of a vertex
struct  SVertex
{
	GLfloat x, y, z;
	GLfloat r, g, b;
};

//Data for vertices 
SVertex *g_pVertices = NULL;
int g_nVertices;		//Size of the vertex Array
int g_nVerticesPopulated;		//Number currently populated

//Control the randomness
int nSkip1 = 0;		//number of samples to discard between x,y
int nSkip2 = 0;		//number of samples to discard between y,z
int nSkip3 = 0;		//number of samples to discard between z,x


//Control the display
enum Shape_t { Sphere };
Shape_t g_currentShape = Sphere;
bool g_bShowAxes = true;
bool g_bTexXZoom = false;
int lastShapeX = 1024;
int lastShapeY = 1024;
float xRotated = 0.0f;
float yRotated = 0.0f;

const float PI = 3.14159265359f;

GLfloat g_RotateY = 0.0f;
GLfloat g_RotateX = 0.0f;
GLfloat g_RotateZ = 0.0f;



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

GLuint gVao_Triangle;
GLuint gVbo_Position_Tri;
GLuint gVbo_Color_Tri;

GLuint gVao_Square;
GLuint gVbo_Position_Square;
GLuint gVbo;
GLuint gVboColor;

GLuint modelMatrixUniform;
GLuint viewMatrixUniform;
GLuint projectionMatrixUniform;


GLuint gl_vertex_array_object;
GLuint gl_vertex_buffer_object;


GLuint gMVPUniform;


mat4 gPerspectiveProjectionMatrix;



void createSphere(void)
{
	int startVertex = 0;

	for (int i = startVertex; i < g_nVerticesPopulated; i++)
	{
		float r;
		float rho;
		float theta;

		if (g_currentShape == Sphere)
		{
			r = g_pRng->getNextU01();
			r = powf(r, 1.0f / 3.0f);

			for (int j = 0; j < nSkip3; j++)
			{
				g_pRng->getNextU01();
			}

		}

		else
		{
			r = 1.0f;
		}

		rho = g_pRng->getNextU01()*PI *4.0f;

		for (int j = 0; j < nSkip3; j++)
		{
			g_pRng->getNextU01();
		}

		theta = (g_pRng->getNextU01()*4.0f) - 1.0f;
		theta = asin(theta);

		for (int j = 0; j < nSkip2; j++)
		{
			g_pRng->getNextU01();
		}

		g_pVertices[i].x = r * fabs(cos(theta))*cos(rho);
		g_pVertices[i].y = r * fabs(cos(theta))*sin(rho);
		g_pVertices[i].z = r * sin(theta);

		g_pVertices[i].r = 1.0f;
		g_pVertices[i].g = 1.0f;
		g_pVertices[i].b = 1.0f;



	}
}

void createCube(void)
{
	int startVertex = 0;

	for (int i = startVertex; i < g_nVerticesPopulated; i++)
	{
		g_pVertices[i].x = (g_pRng->getNextU01() - .5f) * 2;

		for (int j = 0; j < nSkip1; j++)
		{
			g_pRng->getNextU01();
		}

		g_pVertices[i].y = (g_pRng->getNextU01() - .5f) * 2;

		for (int j = 0; j < nSkip2; j++)
		{
			g_pRng->getNextU01();
		}

		g_pVertices[i].z = (g_pRng->getNextU01() - .5f) * 2;

		for (int j = 0; j < nSkip3; j++)
		{
			g_pRng->getNextU01();
		}

		g_pVertices[i].r = 1.0f;
		g_pVertices[i].g = 1.0f;
		g_pVertices[i].b = 1.0f;
	}
}

void createPlane(void)
{
	int startVertex = 0;

	for (int i = startVertex; i < g_nVerticesPopulated; i++)
	{
		g_pVertices[i].x = (g_pRng->getNextU01() - .5f) * 2;

		for (int j = 0; j < nSkip1; j++)
		{
			g_pRng->getNextU01();
		}

		g_pVertices[i].y = (g_pRng->getNextU01() - .5f) * 2;

		for (int j = 0; j < nSkip2; j++)
		{
			g_pRng->getNextU01();
		}

		g_pVertices[i].z = 0.0f;

		g_pVertices[i].r = 1.0f;
		g_pVertices[i].g = 1.0f;
		g_pVertices[i].b = 1.0f;
	}
}


void createAxes(void)
{
	//Z axis
	g_pVertices[200000].x = 0.0f;
	g_pVertices[200000].y = 0.0f;
	g_pVertices[200000].z = -1.5f;
	g_pVertices[200001].x = 0.0f;
	g_pVertices[200001].y = 0.0f;
	g_pVertices[200001].z = 1.5f;
	g_pVertices[200000].r = 1.0f;
	g_pVertices[200000].g = 0.0f;
	g_pVertices[200000].b = 0.0f;
	g_pVertices[200001].r = 0.0f;
	g_pVertices[200001].g = 1.0f;
	g_pVertices[200001].b = 1.0f;

	//Y axis
	g_pVertices[200002].x = 0.0f;
	g_pVertices[200002].y = -1.5f;
	g_pVertices[200002].z = 0.0f;
	g_pVertices[200003].x = 0.0f;
	g_pVertices[200003].y = 1.5f;
	g_pVertices[200003].z = 0.0f;
	g_pVertices[200002].r = 0.0f;
	g_pVertices[200002].g = 1.0f;
	g_pVertices[200002].b = 0.0f;
	g_pVertices[200003].r = 1.0f;
	g_pVertices[200003].g = 0.0f;
	g_pVertices[200003].b = 1.0f;


	//X axis
	g_pVertices[200004].x = -1.5f;
	g_pVertices[200004].y = 0.0f;
	g_pVertices[200004].z = 0.0f;
	g_pVertices[200005].x = 1.5f;
	g_pVertices[200005].y = 0.0f;
	g_pVertices[200005].z = 0.5f;
	g_pVertices[200004].r = 0.0f;
	g_pVertices[200004].g = 0.0f;
	g_pVertices[200004].b = 1.0f;
	g_pVertices[200005].r = 1.0f;
	g_pVertices[200005].g = 1.0f;
	g_pVertices[200005].b = 0.0f;


}

void drawPoints(void)
{
	if (g_bShowAxes)
	{
		glDrawArrays(GL_LINE_STRIP, 200000, 2);
		glDrawArrays(GL_LINE_STRIP, 200002, 2);
		glDrawArrays(GL_LINE_STRIP, 200004, 2);
	}

	glDrawArrays(GL_POINTS, 0, g_nVerticesPopulated);
}

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


	//create vertices
//	createSphere();
	//createAxes();

	//createCube();

	//intitialize()
	initialize();

	ShowWindow(hwnd, SW_SHOW);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

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

void initialize()
{
	//function prototype
	void uninitialize(void);
	void resize(int, int);

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

	//******** VERTEX SHADER ******
	//create shader
	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	//provide source code to shader
	const GLchar *vertexShaderSourceCode =
		"#version 430 core" \
		"\n" \
		"in vec4 vPosition;" \
		"uniform mat4 u_model_matrix;" \
		"uniform mat4 u_view_matrix;" \
		"uniform mat4 u_projection_matrix;" \

		"void main(void)" \
		"{" \
		"gl_Position = u_projection_matrix*u_view_matrix*u_model_matrix  * vPosition;" \
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
		"out vec4 FragColor;" \
		"void main (void)" \
		"{" \
		"FragColor = vec4(1.0,1.0,1.0,1.0);" \
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

	//glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_COLOR, "vColor");

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
	modelMatrixUniform = glGetUniformLocation(gShaderProgramObject, "u_model_matrix");
	viewMatrixUniform = glGetUniformLocation(gShaderProgramObject, "u_view_matrix");
	projectionMatrixUniform = glGetUniformLocation(gShaderProgramObject, "u_projection_matrix");

	//**** vertices, colors, shader attribs, vbo, vao initializations ****//

	const GLfloat triangleVertices[] =
	{
		0.0f, 1.0f, 0.0f,		//apex
		-1.0f, -1.0f, 0.0f,	//left bottom
		1.0f, -1.0f, 0.0f		//right bottom
	};

	const GLfloat colorVertices[] =
	{
		1.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,
		0.0f,0.0f,1.0f
	};

	const GLfloat squareVertices[] =
	{
		-1.0f,1.0f,0.0f,
		-1.0f,-1.0f,0.0f,
		1.0f,-1.0f,0.0f,
		1.0f,1.0f,0.0f
	};

	//glGenVertexArrays(1, &gVao_Triangle);
	//glBindVertexArray(gVao_Triangle);

	//glGenBuffers(1, &gVbo_Position_Tri);
	//glBindBuffer(GL_ARRAY_BUFFER, gVbo_Position_Tri);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);
	//glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	//glBindBuffer(GL_ARRAY_BUFFER, 0);			//fininshing/completion of above gVboPosition buffer state

	//											//VBO For colors

	//glGenBuffers(1, &gVbo_Color_Tri);
	//glBindBuffer(GL_ARRAY_BUFFER, gVbo_Color_Tri);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(colorVertices), colorVertices, GL_STATIC_DRAW);
	//glVertexAttribPointer(VDG_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//glEnableVertexAttribArray(VDG_ATTRIBUTE_COLOR);

	//glBindBuffer(GL_ARRAY_BUFFER, 0);	//fininshing/completion of above gVboColor buffer state

	//glBindVertexArray(0);		//fininshing vaoTriangle

	//Start of Square Vao
	glGenVertexArrays(1, &gVao_Square);
	glBindVertexArray(gVao_Square);

	//VBO of Square Pos
	glGenBuffers(1, &gVbo_Position_Square);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_Position_Square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	//	createSphere();

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);
	/////glVertexAttrib3f(VDG_ATTRIBUTE_COLOR, 0.392f, 0.584f, 0.929f);
	////
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//

		//glGenBuffers(1, &gVbo);
		//glBindBuffer(GL_ARRAY_BUFFER, gVbo);

		//glBufferData(GL_ARRAY_BUFFER, sizeof(SVertex), g_pVertices, GL_STATIC_DRAW);
		//glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		//glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

		//glEnableClientState(GL_VERTEX_ARRAY);
		//glVertexPointer(3, GL_FLOAT, sizeof(SVertex), g_pVertices);
		//glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

		//glEnableClientState(GL_COLOR_ARRAY);
		//glVertexPointer(3, GL_FLOAT, sizeof(SVertex), &g_pVertices[0].r);
		//glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);


	fprintf(gpFile, "Number of vertices:%d", g_pVertices);
	//	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);



	g_nVertices = 200000;
	g_nVerticesPopulated = 200000;
	g_pVertices = new SVertex[g_nVertices + 8];

	//Setup random number generators
	g_pRng = new RNG(12345, 1, 100000);
	//g_pCheckRender = new CheckBackBuffer(lastShapeX, lastShapeY, 4, false);

	//createSphere();
	createCube();
	//	createAxes();

		// As we do not yet use a depth buffer, we cannot fill our sphere
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	// Enable the vertex array functionality:
	glEnableClientState(GL_VERTEX_ARRAY);
	// Enable the color array functionality (so we can specify a color for each vertex)
	glEnableClientState(GL_COLOR_ARRAY);
	// Pass the vertex pointer:
	glVertexPointer(3,   //3 components per vertex (x,y,z)
		GL_FLOAT,
		sizeof(SVertex),
		g_pVertices);
	// Pass the color pointer
	glColorPointer(3,   //3 components per vertex (r,g,b)
		GL_FLOAT,
		sizeof(SVertex),
		&g_pVertices[0].r);  //Pointer to the first color



	////////////////////////////////////////////////////////////Color ///////////////////////////////////
	//glGenBuffers(1, &gVbo);
	//glBindBuffer(GL_ARRAY_BUFFER, gVbo);

	//glBufferData(GL_ARRAY_BUFFER, sizeof(SVertex), g_pVertices, GL_STATIC_DRAW);
	//glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(3, GL_FLOAT, sizeof(SVertex), g_pVertices);
	//glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	//fprintf(gpFile, "Number of vertices:%d", g_pVertices);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);



	glShadeModel(GL_FLAT);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glEnable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	gPerspectiveProjectionMatrix = mat4::identity();

	resize(WIN_WIDTH, WIN_HEIGHT);

}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//start using OpenGL program Object
	glUseProgram(gShaderProgramObject);
	glPointSize(5.4f);
	//OpenL Drawing 
	//set modelview & modelviewprojection matrices to identity 
	//mat4 modelviewMatrix = mat4::identity();
	//mat4 rotationmatrix = mat4::identity();
	//mat4 modelviewProjectionMatrix = mat4::identity();

	mat4 modelMatrix = mat4::identity();
	mat4 viewMatrix = mat4::identity();
	
	modelMatrix = translate(0.0f, 0.0f, -0.2f);
	modelMatrix = rotate(g_RotateY,1.0f, 1.0f, 1.0f);
//	modelMatrix = rotate(g_RotateY, 0.0f, 0.0f, 1.0f);

	//modelMatrix = rotate(g_RotateZ, 0.0f, 0.0f, 1.0f);
	// Point size for point mode
	glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, modelMatrix);
	glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, gPerspectiveProjectionMatrix);

	glPointSize(2.0f);

	//	glBindVertexArray(gVao_Square);
	drawPoints();
	//	glBindVertexArray(0);							//**** unbind vaoSquare *****//
		//stop using OpenGL program Object

	glPointSize(0.4f);
	drawPoints();

	glUseProgram(0);


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//glUseProgram(gShaderProgramObject);
	//glPointSize(1.0f);
	////OpenL Drawing 
	////set modelview & modelviewprojection matrices to identity 
	////mat4 modelviewMatrix = mat4::identity();
	////mat4 rotationmatrix = mat4::identity();
	////mat4 modelviewProjectionMatrix = mat4::identity();

	// modelMatrix = mat4::identity();
	// viewMatrix = mat4::identity();

	//modelMatrix = translate(0.0f, 0.0f, -0.2f);
	//modelMatrix = rotate(g_RotateX, 1.0f, 1.0f, 1.0f);
	////	modelMatrix = rotate(g_RotateY, 0.0f, 0.0f, 1.0f);

	//	//modelMatrix = rotate(g_RotateZ, 0.0f, 0.0f, 1.0f);

	//g_nVertices = 200000;
	//g_nVerticesPopulated = 200000;
	//g_pVertices = new SVertex[g_nVertices + 6];

	////Setup random number generators
	//g_pRng = new RNG(12342, 1, 100000);

	////createSphere();
	//createCube();

	//// As we do not yet use a depth buffer, we cannot fill our sphere
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//// Enable the vertex array functionality:
	//glEnableClientState(GL_VERTEX_ARRAY);
	//// Enable the color array functionality (so we can specify a color for each vertex)
	//glEnableClientState(GL_COLOR_ARRAY);
	//// Pass the vertex pointer:
	//glVertexPointer(3,   //3 components per vertex (x,y,z)
	//	GL_FLOAT,
	//	sizeof(SVertex),
	//	g_pVertices);
	//// Pass the color pointer
	//glColorPointer(3,   //3 components per vertex (r,g,b)
	//	GL_FLOAT,
	//	sizeof(SVertex),
	//	&g_pVertices[0].r);  //Pointer to the first color


	//// Point size for point mode
	//glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, modelMatrix);
	//glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, viewMatrix);
	//glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, gPerspectiveProjectionMatrix);


	////	glBindVertexArray(gVao_Square);
	//drawPoints();
	////	glBindVertexArray(0);							//**** unbind vaoSquare *****//
	//	//stop using OpenGL program Object
	//glUseProgram(0);

	SwapBuffers(ghdc);
	/*g_RotateY += 0.2f;

	if (g_RotateY >= 360.0f)
	{
		g_RotateY -= 360.0f;
	}
*/
	g_RotateX -= 0.001f;

	if (g_RotateX >= 360.0f)
	{
		g_RotateX += 360.0f;
	}
	g_RotateZ += 0.001f;

	if (g_RotateZ >= 360.0f)
	{
		g_RotateZ -= 360.0f;
	}
	g_RotateY += 0.001f;

	if (g_RotateY >= 360.0f)
	{
		g_RotateY -= 360.0f;
	}
}

void resize(int width, int height)
{
	////code
	//if (height == 0)
	//	height = 1;
	//glViewport(0, 0, (GLsizei)width, (GLsizei)height);

	//gPerspectiveProjectionMatrix = perspective(70.0f, (GLfloat)width / (GLfloat)height, 0.5f, 1000.0f);



	float xScale;
	float yScale;

	lastShapeX = width;
	lastShapeY = height;

	// Check if shape is visible
	if (width == 0 || height == 0)
	{
		return;
	}

	// Set a new projection matrix
	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();

	// Adjust fit
	if (height > width)
	{
		xScale = 1.0f;
		yScale = (float)height / width;
	}
	else
	{
		xScale = (float)width / height;
		yScale = 1.0f;
	}


	// Angle of view:40 degrees
	// Near clipping plane distance: 10.0 (default)
	// Far clipping plane distance: 10.0 (default)

		//glOrtho(-.15f * xScale, .15f * xScale, -.15f * yScale, .15f * yScale, -5.0f, 5.0f);
	gPerspectiveProjectionMatrix=	ortho(-.15f * xScale, .15f * xScale, -.15f * yScale, .15f * yScale, -5.0f, 5.0f);

	// Use the whole window for rendering
	glViewport(0, 0, width, height);
}

void uninitialize(void)
{
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
	if (gVao_Triangle)
	{
		glDeleteVertexArrays(1, &gVao_Triangle);
		gVao_Triangle = 0;
	}

	//destroy vbo
	if (gVbo_Position_Tri)
	{
		glDeleteVertexArrays(1, &gVbo_Position_Tri);
		gVbo_Position_Tri = 0;
	}

	if (gVbo_Color_Tri)
	{
		glDeleteVertexArrays(1, &gVbo_Color_Tri);
		gVbo_Color_Tri = 0;
	}

	if (gVao_Square)
	{
		glDeleteVertexArrays(1, &gVao_Square);
		gVao_Square = 0;
	}

	if (gVbo_Position_Square)
	{
		glDeleteVertexArrays(1, &gVbo_Position_Square);
	}






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

























////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////

























//
//
//
//
//
//#include <Windows.h>
//#include<stdio.h>
//#include<gl/glew.h>
//#include<gl/GL.h>
//
//#include"vmath.h"
//
//#define WIN_WIDTH 800
//#define WIN_HEIGHT 600
//
//#pragma comment(lib,"glew32.lib")
//#pragma comment(lib,"opengl32.lib")
//
//using namespace vmath;
//
//
//enum
//{
//	VDG_ATTRIBUTE_VERTEX = 0,
//	VDG_ATTRIBUTE_COLOR,
//	VDG_ATTRIBUTE_NORMAL,
//	VDG_ATTRIBUTE_TEXTURE0,
//};
//
////////Prototype of WndProc() declared Globally
//LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//
//
//////////Global variable declarations
//FILE *gpFile = NULL;
//
//HWND ghwnd = NULL;
//HDC ghdc = NULL;
//HGLRC ghrc = NULL;
//
//
//DWORD dwStyle;
//WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };
//
//bool gbActiveWindow = false;
//bool gbEscapeKeyIsPressed = false;
//bool gbFullScreen = false;
//
//GLuint gVertexShaderObject;
//GLuint gFragmentShaderObject;
//GLuint gShaderProgramObject;
//
//
//
//GLuint gVao_Triangle;
//GLuint gvao_Square;
//GLuint gVbo;
//GLuint ScaleUniform;
//GLuint gvboNormal;
//GLuint gMVPUniform;
//GLuint gmodelViewUniform;
//GLuint lightPositionUniform;
//GLuint viewMatrixUniform;
//GLuint color1Uniform;
//GLuint color2Uniform;
//GLuint gTexture_sampler_uniform;
//
//mat4 gPerspectiveProjectionMatrix;
//
//////////main()
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
//{
//	////function prototype
//	void display();
//	void initialize();
//	void uninitialize(void);
//
//	////variable declaration
//	WNDCLASSEX wndclass;
//	HWND hwnd;
//	MSG msg;
//	TCHAR szClassName[] = TEXT("OpenGLPP");
//	bool bDone = false;
//
//	if (fopen_s(&gpFile, "Log.txt", "w") != 0)
//	{
//		MessageBox(NULL, TEXT("Log File Can not be Created\Exitting.."), TEXT("ERROR"), MB_OK | MB_TOPMOST | MB_ICONSTOP);
//		exit(0);
//	}
//	else
//	{
//		fprintf(gpFile, "Log file is Successfully Openend \n");
//	}
//
//	////////code
//	////////initializing member of WNDCLASS
//	wndclass.cbSize = sizeof(WNDCLASSEX);
//	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
//	wndclass.cbClsExtra = 0;
//	wndclass.cbWndExtra = 0;
//	wndclass.hInstance = hInstance;
//	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
//	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
//	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
//	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
//	wndclass.lpfnWndProc = WndProc;
//	wndclass.lpszClassName = szClassName;
//	wndclass.lpszMenuName = NULL;
//
//	//////Registering Class
//	RegisterClassEx(&wndclass);
//
//	//////////CreateWindow
//	hwnd = CreateWindowEx(WS_EX_APPWINDOW,
//		szClassName,
//		TEXT("OpenGL Programmable PipeLine Using Native Windowing: First ortho Trianle Window Shree Ganeshaya Namaha"),
//		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
//		0,
//		0,
//		WIN_WIDTH,
//		WIN_HEIGHT,
//		NULL,
//		NULL,
//		hInstance,
//		NULL
//		);
//
//	ghwnd = hwnd;
//
//	////////intitialize()
//	initialize();
//
//	ShowWindow(hwnd, SW_SHOW);
//	SetForegroundWindow(hwnd);
//	SetFocus(hwnd);
//
////////////	Message Loop
//	while (bDone == false)
//	{
//		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//		{
//			if (msg.message == WM_QUIT)
//				bDone = true;
//			else
//			{
//				TranslateMessage(&msg);
//				DispatchMessage(&msg);
//			}
//		}
//		else
//		{
//			display();
//			if (gbActiveWindow == true)
//			{
//				display();
//
//				if (gbEscapeKeyIsPressed == true)
//					bDone = true;
//			}
//		}
//	}
//
//	uninitialize();
//	return((int)msg.wParam);
//
//}
//
//LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
//{
//		void display();
//	void resize(int, int);
//	void ToggleFullScreen(void);
//	void uninitialize(void);
//
//	switch (iMsg)
//	{
//	case WM_ACTIVATE:
//		if (HIWORD(wParam) == 0)
//			gbActiveWindow = true;
//		else
//			gbActiveWindow = false;
//		break;
//	case WM_PAINT:
//				display();
//		break;
//
//	case WM_SIZE:
//		resize(LOWORD(lParam), HIWORD(lParam));
//		break;
//
//	case WM_KEYDOWN:
//		switch (wParam)
//		{
//		case VK_ESCAPE:
//			gbEscapeKeyIsPressed = true;
//			break;
//
//		case 0x46:			//for 'f' or 'F'
//			if (gbFullScreen == false)
//			{
//				ToggleFullScreen();
//				gbFullScreen = true;
//			}
//			else
//			{
//				ToggleFullScreen();
//				gbFullScreen = false;
//			}
//			break;
//		default:
//			break;
//		}
//		break;
//
//
//	case WM_LBUTTONDOWN:
//		break;
//
//	case WM_DESTROY:
//		PostQuitMessage(0);
//		break;
//
//	default:
//		break;
//	}
//	return(DefWindowProc(hwnd, iMsg, wParam, lParam));
//
//}
//
//
//void ToggleFullScreen(void)
//{
//		MONITORINFO mi;
//
//	if (gbFullScreen == false)
//	{
//		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
//		if (dwStyle & WS_OVERLAPPEDWINDOW)
//		{
//			mi = { sizeof(MONITORINFO) };
//			if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
//			{
//				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
//				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left,
//					mi.rcMonitor.top,
//					mi.rcMonitor.right - mi.rcMonitor.left,
//					mi.rcMonitor.bottom - mi.rcMonitor.top,
//					SWP_NOZORDER | SWP_FRAMECHANGED);
//			}
//
//		}
//		ShowCursor(FALSE);
//	}
//	else
//	{
//		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
//		SetWindowPlacement(ghwnd, &wpPrev);
//		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
//		ShowCursor(TRUE);
//	}
//}
//
//void initialize()
//{
//	void uninitialize(void);
//	void resize(int, int);
//
//	PIXELFORMATDESCRIPTOR pfd;
//	int iPixelFormatIndex;
//
//	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
//
//	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
//	pfd.nVersion = 1;
//	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
//	pfd.iPixelType = PFD_TYPE_RGBA;
//	pfd.cColorBits = 8;
//	pfd.cRedBits = 8;
//	pfd.cGreenBits = 8;
//	pfd.cBlueBits = 8;
//	pfd.cAlphaBits = 8;
//
//	ghdc = GetDC(ghwnd);
//
//	iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);
//	if (iPixelFormatIndex == 0)
//	{
//		ReleaseDC(ghwnd, ghdc);
//		ghdc = NULL;
//	}
//	if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == FALSE)
//	{
//		ReleaseDC(ghwnd, ghdc);
//		ghdc = NULL;
//	}
//
//	ghrc = wglCreateContext(ghdc);
//	if (ghrc == NULL)
//	{
//		ReleaseDC(ghwnd, ghdc);
//		ghdc = NULL;
//	}
//
//	if (wglMakeCurrent(ghdc, ghrc) == FALSE)
//	{
//		wglDeleteContext(ghrc);
//		ghrc = NULL;
//		ReleaseDC(ghwnd, ghdc);
//		ghdc = NULL;
//	}
//
//	GLenum glew_error = glewInit();
//	if (glew_error != GLEW_OK)
//	{
//		wglDeleteContext(ghrc);
//		ghrc = NULL;
//		ReleaseDC(ghwnd, ghdc);
//		ghdc = NULL;
//	}
//
//	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
//
//	const GLchar *vertexShaderSourceCode =
//		"#version 430 core" \
//		"\n" \
//		"in vec4 vPosition;" \
//		"in vec3 vNormal;" \
//
//		"uniform mat4 u_model_view_matrix;" \
//		"uniform mat4 u_mvp_matrix;" \
//		"uniform vec3 u_light_position;" \
//
//		"uniform vec3 LightPos;" \
//		"uniform float scale;" \
//		"out float LightIntensity;" \
//		"out vec3 MCposition;" \
//
//		"void main(void)" \
//		"{" \
//		"vec3 eye_coordinates= vec3(u_model_view_matrix *vPosition);" \
//		"MCposition = vec3(vPosition) * scale;" \
//		"vec3 tnorm = normalize(vec3(u_model_view_matrix) * vNormal);" \
//
//	//	"vec3 light_direction = normalize(vec3(u_light_position));"\
//
//		"LightIntensity = dot(normalize(u_light_position - eye_coordinates.xyz),tnorm);" \
//		"LightIntensity *= 1.5;" \
//		"gl_Position = u_mvp_matrix * vPosition;" \
//		"}";
//
//	glShaderSource(gVertexShaderObject, 1, (const GLchar **)&vertexShaderSourceCode, NULL);
//
//	glCompileShader(gVertexShaderObject);
//	GLint iInfoLength = 0;
//	GLint iShaderCompiledStatus = 0;
//	char *szInfoLog = NULL;
//
//	glGetShaderiv(gVertexShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
//	if (iShaderCompiledStatus == GL_FALSE)
//	{
//		glGetShaderiv(gVertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLength);
//		if (iInfoLength > 0)
//		{
//			szInfoLog = (char *)malloc(iInfoLength);
//			if (szInfoLog != NULL)
//			{
//				GLsizei written;
//				glGetShaderInfoLog(gVertexShaderObject, iInfoLength, &written, szInfoLog);
//				fprintf(gpFile, "Vertex Shader Compilation Log:%s\n", szInfoLog);
//				free(szInfoLog);
//				uninitialize();
//				exit(0);
//			}
//		}
//	}
//	gFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
//
//	const GLchar *fragmentShaderSourceCode =
//		"#version 430 core" \
//		"\n" \
//		"in float LightIntensity;" \
//		"in vec3 MCposition;" \
//
//		"uniform sampler3D Noise;" \
//		"uniform vec3 Color1;" \
//		"uniform vec3 Color2;" \
//		"uniform float NoiseScale;" \
//
//		"out vec4 FragColor;" \
//		"void main()" \
//		"{" \
//		//	"vec4 noisevec = texture3D(Noise,MCposition *NoiseScale );" \
//		//	"float intensity = abs(noisevec[0] - 0.25) + abs(noisevec[1] - 0.125) + abs(noisevec[2] - 0.0625) + abs(noisevec[3] -0.03125);" \
//		//	"intensity = clamp(intensity * 6.0,0.0,1.0);"\
//		//	"vec3 color = mix(Color1,Color2,intensity)*LightIntensity;" \
//
//		"vec4 noisevec = texture(Noise,NoiseScale * MCposition);" \
//		"float intensity = min(1.0,noisevec[3] * 18.0);" \
//		"vec3 color = vec3(intensity * LightIntensity)"
//		"FragColor = vec4(color, 1.0);" \
//		"}";
//
//	glShaderSource(gFragmentShaderObject, 1, (const GLchar**)&fragmentShaderSourceCode, NULL);
//
//	glCompileShader(gFragmentShaderObject);
//	glGetShaderiv(gFragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
//	if (iShaderCompiledStatus == GL_FALSE)
//	{
//		glGetShaderiv(gFragmentShaderObject, GL_INFO_LOG_LENGTH, &iShaderCompiledStatus);
//		if (iInfoLength > 0)
//		{
//			szInfoLog = (char *)malloc(iInfoLength);
//			if (szInfoLog != NULL)
//			{
//				GLsizei written;
//				glGetShaderInfoLog(gFragmentShaderObject, iInfoLength, &written, szInfoLog);
//				fprintf(gpFile, "Fragment Shader Compilation Log : %s\n", szInfoLog);
//				free(szInfoLog);
//				uninitialize();
//				exit(0);
//			}
//		}
//	}
//
//
//	gShaderProgramObject = glCreateProgram();
//
//	glAttachShader(gShaderProgramObject, gVertexShaderObject);
//
//	glAttachShader(gShaderProgramObject, gFragmentShaderObject);
//
//	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vPosition");
//	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vNormal");
//
//
//	glLinkProgram(gShaderProgramObject);
//	GLint iShaderProgramLinkStatus = 0;
//	glGetProgramiv(gShaderProgramObject, GL_LINK_STATUS, &iShaderProgramLinkStatus);
//	if (iShaderProgramLinkStatus == GL_FALSE)
//	{
//		glGetProgramiv(gShaderProgramObject, GL_INFO_LOG_LENGTH, &iInfoLength);
//		if (iInfoLength > 0)
//		{
//			szInfoLog = (char *)malloc(iInfoLength);
//			if (szInfoLog != NULL)
//			{
//				GLsizei written;
//				glGetProgramInfoLog(gShaderProgramObject, iInfoLength, &written, szInfoLog);
//				fprintf(gpFile, "Shader Program LinK Log %s\n");
//				free(szInfoLog);
//				uninitialize();
//				exit(0);
//			}
//		}
//	}
//
//
//	gMVPUniform = glGetUniformLocation(gShaderProgramObject, "u_mvp_matrix");
//	gmodelViewUniform = glGetUniformLocation(gShaderProgramObject, "u_model_view_matrix");
//	lightPositionUniform = glGetUniformLocation(gShaderProgramObject, "u_light_position");
//	ScaleUniform = glGetUniformLocation(gShaderProgramObject, "NoiseScale");
//	color1Uniform = glGetUniformLocation(gShaderProgramObject, "Color1");
//	color2Uniform = glGetUniformLocation(gShaderProgramObject, "Color2");
//	gTexture_sampler_uniform = glGetUniformLocation(gShaderProgramObject, "Noise");
//
//	const GLfloat normal[] =
//	{ /*0.0f, 1.0f, 0.0f,
//		0.0f, 1.0f, 0.0f,
//		0.0f, 1.0f, 0.0f,
//		0.0f, 1.0f, 0.0f,
//
//		0.0f,-1.0f, 0.0f,
//		0.0f,-1.0f, 0.0f,
//		0.0f,-1.0f, 0.0f,
//		0.0f,-1.0f, 0.0f,
//
//		0.0f, 0.0f, 1.0f,
//		0.0f, 0.0f, 1.0f,
//		0.0f, 0.0f, 1.0f,
//		0.0f, 0.0f, 1.0f,
//
//
//		0.0f, 0.0f, -1.0f,
//		0.0f, 0.0f, -1.0f,
//		0.0f, 0.0f, -1.0f,
//		0.0f, 0.0f, -1.0f,*/
//
//		1.0f, 1.0f, 1.0f,
//		1.0f, 1.0f, 1.0f,
//		1.0f, 1.0f, 1.0f,
//		1.0f, 1.0f, 1.0f,
//
//		/*-1.0f, 0.0f, 0.0f,
//		-1.0f, 0.0f, 0.0f,
//		-1.0f, 0.0f, 0.0f,
//		-1.0f, 0.0f, 0.0f,*/
//
//
//	};
//
//	const GLfloat squareVertices[] = 
//	{
//		-1.0f,1.0f,0.0f,
//		-1.0f,-1.0f,0.0f,
//		 1.0f,-1.0f,0.0f,
//		 1.0f,1.0f,0.0f
//	};
//
//	
//	glGenVertexArrays(1, &gvao_Square);
//	glBindVertexArray(gvao_Square);
//
//	glGenBuffers(1, &gVbo);
//	glBindBuffer(GL_ARRAY_BUFFER, gVbo);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);
//	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
//	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);
//
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//	glGenBuffers(1, &gvboNormal);
//	glBindBuffer(GL_ARRAY_BUFFER, gvboNormal);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
//	glVertexAttribPointer(VDG_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
//	glEnableVertexAttribArray(VDG_ATTRIBUTE_NORMAL);
//
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindVertexArray(0);					//finishing completion of above gVaoSquare
//
//
//	glShadeModel(GL_FLAT);
//	glClearDepth(1.0f);
//	glEnable(GL_DEPTH_TEST);
//	glDepthFunc(GL_LEQUAL);
//	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
//	glEnable(GL_CULL_FACE);
//
//	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//
//	gPerspectiveProjectionMatrix = mat4::identity();
//
//	resize(WIN_WIDTH, WIN_HEIGHT);
//
//}
//
//void display(void)
//{
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//	glUseProgram(gShaderProgramObject);
//
//	mat4 modelviewMatrix = mat4::identity();
//	mat4 modelviewProjectionMatrix = mat4::identity();
//	mat4 viewMatrix = mat4::identity();
//	modelviewMatrix = translate(0.0f, 0.0f, -3.0f);
//
//	//multipley the modelview and orthographic matrix to get modelviewprojection matrix
//	modelviewProjectionMatrix = gPerspectiveProjectionMatrix * modelviewMatrix;	//ORDER IS IMPORTANT
//
////	glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, viewMatrix);
//	glUniformMatrix4fv(gMVPUniform, 1, GL_FALSE, modelviewProjectionMatrix);
//
//	glUniformMatrix4fv(gmodelViewUniform, 1, GL_FALSE, modelviewMatrix);
//
//
//	GLfloat NoiseScale = 1.2f;
//
//	glUniform1f(ScaleUniform, NoiseScale);
//
//	float color1[] = { 0.8,0.7,0.8};
//	glUniform3fv(color1Uniform, 1, (GLfloat*)color1);
//
//	float color2[] = { 0.6,0.1,0.8 };
//	glUniform3fv(color2Uniform, 1, (GLfloat*)color2);
//
//	glUniform1i(gTexture_sampler_uniform, 0);
//
//	float lightPosition[] = { 5.0,0.0,0.0 };
//	glUniform3fv(lightPositionUniform, 1, (GLfloat*) lightPosition);
//
//	glBindVertexArray(gvao_Square);
//	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
//	glBindVertexArray(0);
//
//	glUseProgram(0);
//
//	SwapBuffers(ghdc);
//
//}
//
//void resize(int width, int height)
//{
//	if (height == 0)
//		height = 1;
//	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
//
//	gPerspectiveProjectionMatrix = perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
//}
//
//void uninitialize(void)
//{
//	if (gbFullScreen == true)
//	{
//		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
//		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
//		SetWindowPlacement(ghwnd, &wpPrev);
//		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
//		ShowCursor(TRUE);
//	}
//
//	if (gVao_Triangle)
//	{
//		glDeleteVertexArrays(1, &gVao_Triangle);
//		gVao_Triangle = 0;
//	}
//
//	if (gvao_Square)
//	{
//		glDeleteVertexArrays(1, &gvao_Square);
//		gvao_Square = 0;
//	}
//
//	if (gVbo)
//	{
//		glDeleteBuffers(1, &gVbo);
//		gVbo = 0;
//	}
//
//
//	glDetachShader(gShaderProgramObject, gVertexShaderObject);
//
//	glDetachShader(gShaderProgramObject, gFragmentShaderObject);
//
//	glDeleteShader(gVertexShaderObject);
//	gVertexShaderObject = 0;
//
//	glDeleteShader(gFragmentShaderObject);
//	gFragmentShaderObject = 0;
//
//	glUseProgram(0);
//
//	wglMakeCurrent(NULL, NULL);
//
//	wglDeleteContext(ghrc);
//	ghrc = NULL;
//
//	ReleaseDC(ghwnd, ghdc);
//	ghdc = NULL;
//
//	DestroyWindow(ghwnd);
//	ghwnd = NULL;
//
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
