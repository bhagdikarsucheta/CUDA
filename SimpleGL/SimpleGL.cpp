#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include"C:/glew-2.1.0/include/GL/glew.h"
#include<GL/gl.h>
#include"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.1/include/cuda.h"
#include"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.1/include/cuda_gl_interop.h"
#include"C:/ProgramData/NVIDIA Corporation/CUDA Samples/v10.1/common/inc/helper_string.h"
#include"C:/ProgramData/NVIDIA Corporation/CUDA Samples/v10.1/common/inc/helper_cuda.h"
#include"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.1/include/device_launch_parameters.h"
#include"vmath.h"

using namespace vmath;

#define WIN_WIDTH 800
#define WIN_HEIGHT 600
	
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")

#pragma comment(lib,"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.1/lib/x64//cudart.lib")


const int gMeshWidth = 2048;
const int gMeshHeight = 2048;
#define MyArraySize gMeshWidth * gMeshHeight * 4

float pos[gMeshWidth][gMeshHeight][4];

struct cudaGraphicsResource *graphicsResource=NULL;

GLfloat gAnim = 0.0f;
cudaError_t error;
float4 *pPos;
size_t byteCount;

//Prototype of WndProc() declared gloabally
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//Global variable declarations
HWND ghwnd = NULL;
HDC ghdc = NULL;
HGLRC ghrc = NULL;

DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

bool gbActiveWindow = false;
bool gbEscapeKeyIsPressed = false;
bool gbFullScreen = false;

enum
{
	VDG_ATTRIBUTE_VERTEX = 0,
	VDG_ATTRIBUTE_COLOR,
	VDG_ATTRIBUTE_NORMAL,
	VDG_ATTRIBUTE_TEXTURE0,
};

GLuint gVertexShaderObject;
GLuint gFragmentShaderObject;
GLuint gShaderProgramObject;

GLuint gNumElements;
GLuint gNumVertices;

float sphere_vertices[1146];
float sphere_normals[1146];
float sphere_textures[764];
unsigned short sphere_elements[2280];

GLuint gVao;
GLuint gVboCpu;
GLuint gVboGpu;

GLuint gVbo_Sphere_element;

GLuint model_matrix_uniform, view_matrix_uniform, projection_matrix_uniform;

GLuint L_KeyPressed_uniform;

GLuint La_uniform;
GLuint Ld_uniform;
GLuint Ls_uniform;
GLuint light_position_uniform;

GLuint Ka_uniform;
GLuint Kd_uniform;
GLuint Ks_uniform;
GLuint material_shininess_uniform;

mat4 gPerspectiveProjectionMatrix;


GLfloat lightAmbient[] = { 0.0f,0.0f,0.0f,1.0f };
GLfloat lightDiffuse[] = { 0.0f,0.0f,0.2f,1.0f };
GLfloat lightSpecular[] = { 1.0f,1.0f,1.0f,1.0f };
GLfloat lightPosition[] = { 1.0f,1.0f,1.0f,1.0f };

GLfloat material_ambient[] = { 0.0f,0.0f,0.0f,1.0f };
GLfloat material_diffuse[] = { 1.0f,1.0f,1.0f,1.0f };
GLfloat material_specular[] = { 1.0f,1.0f,1.0f,1.0f };
GLfloat material_shininess = 50.0f;
bool gbLight;
bool bOnGpu = true;
//bool bOnCpu = true;
FILE *gpFile = NULL;

//main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	void display();
	void initialize();
	void uninitialize();
	void spin();
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szClassName[] = TEXT("RTROGL");
	bool bDone = false;

	fopen_s(&gpFile, "LogFile.txt", "w");
	if (gpFile == NULL)
	{
		printf("Error opening file\n");
		exit(0);
	}

	//initializing members of wndclass
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


	//Registering class
	RegisterClassEx(&wndclass);

	hwnd = CreateWindowEx(WS_EX_APPWINDOW,
		szClassName,
		TEXT("OpenGL Programmable PipeLine Native Windowing"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		0,
		0,
		WIN_WIDTH,
		WIN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ghwnd = hwnd;

	initialize();

	ShowWindow(hwnd, SW_SHOW);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	//Game Loop
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
			display();

			if (gbActiveWindow == true)
			{
				if (gbEscapeKeyIsPressed == true)
					bDone = true;
			}
		}
	}

	uninitialize();
	fprintf(gpFile, "Exitting Main..\n");
	return((int)msg.wParam);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	void display();
	void resize(int, int);
	void ToggleFullScreen(void);
	void uninitialize(void);

	static bool gbIsLKeyPressed = false;
	static bool gbIsGKeyPressed = false;
	static bool gbIsCKeyPressed = true;

	static bool gbIsAKeyPressed = false;
	//	static bool 

	fprintf(gpFile, "Entering WndProc..\n");

	switch (iMsg)
	{
	case WM_ACTIVATE:
		if (HIWORD(wParam) == 0)
			gbActiveWindow = true;
		else
			gbActiveWindow = false;
		break;

	case WM_SIZE:
		resize(LOWORD(lParam), HIWORD(lParam));
		break;

	/*case WM_CHAR:
	case 'G':
	case 'g':
		bOnGpu = true;
		break;

	case 'C':
	case 'c':
		bOnGpu = false;
		break;
*/
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			if (gbEscapeKeyIsPressed == false)
				gbEscapeKeyIsPressed = true;
			break;

		case 0x46:
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

		case 0x47:
			if (bOnGpu == true)
			{
				
				bOnGpu = false;
				
			}
			else 
			{
				//bOnCpu = true;
				bOnGpu = true;

			}
			break;

		case 0x43:
			/*if (bOnCpu == true)
			{
				
				bOnGpu = false;
				bOnCpu = true;
			}*/
			/*else if (bOnCpu == true)
			{
				bOnCpu = true;
				bOnGpu = false;

			}*/
			
		/*	if (bOnCpu == true)
			{
				bOnGpu = false;

			}*/

			break;
		case 0x4c:		//for l or L key (Light)
			if (gbIsLKeyPressed == false)
			{
				gbLight = true;
				gbIsLKeyPressed = true;
			}
			else
			{
				gbLight = false;
				gbIsLKeyPressed = false;
			}
			break;
		}
		break;

	case WM_LBUTTONDOWN:
		break;

	case WM_CLOSE:
		uninitialize();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return(DefWindowProc(hwnd, iMsg, wParam, lParam));
	fprintf(gpFile, "Exitting WndProc..\n");

}

void ToggleFullScreen(void)
{
	MONITORINFO mi;

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
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}
}

//__global__ void simple_vbo_kernel(float4* pos, unsigned int width, unsigned int height, float time);

void launchCUDAKernel(float4* pos, unsigned int mesh_width, unsigned int mesh_height, float time);



void launchCPUKernel(unsigned int mesh_width, unsigned int mesh_height, float time)
{
	for (int i = 0; i < mesh_width; i++)
	{
		for (int j = 0; j < mesh_height; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				float u = i / (float)mesh_width;
				float v = j / (float)mesh_height;
				//u = u * 8.0f - 4.0f;
				//v = v * 8.0f - 4.0f;

				u = u * 3.0f - 1.5f;
				v = v * 3.0f - 1.5f;

				float freq = 4.0f;
				float w = sinf(u*freq + time)*cosf(v*freq + time)  * 0.5f;

				//fill array
				if (k == 0)
					pos[i][j][k] = u;
				if (k == 1)
					pos[i][j][k] = w;
				if (k == 2)
					pos[i][j][k] = v;
				if (k == 3)
					pos[i][j][k] = 1.0;
			}

		}
	}
}

void initialize(void)
{

	void resize(int, int);
	void uninitialize();
	fprintf(gpFile, "Entering Initialize..\n");

	int LoadGLTextures(GLuint *, TCHAR[]);
	int devCount;
	//Variable 
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex;
	//GLint num;

	//code
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

	//initialize code
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

	if (wglMakeCurrent(ghdc, ghrc) == NULL)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}


	errno = cudaGetDeviceCount(&devCount);
	if (error != cudaSuccess)
	{
		fprintf(gpFile, "NVIDIA device found successfully..\n");
	}
	else if (devCount == 0)
	{
		fprintf(gpFile, "No NVIDIA CUDA device found..\n");
	}
	else
	{
		cudaSetDevice(0);
	}

	//*****VERTEX SHADER*******
	//create shader 
	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	//provide source code to shader
	const GLchar * vertexShaderShaderCode =
		"#version 430 core"	\
		"\n" \
		"in vec4 vPosition;" \
		"in vec3 vNormal;" \
		"uniform mat4 u_model_matrix;" \
		"uniform mat4 u_view_matrix;" \
		"uniform mat4 u_projection_matrix;" \
		"uniform vec4 u_light_position;" \
		"uniform int u_lighting_enabled;" \
		"out vec3 transformed_normals;" \
		"out vec3 light_direction;" \
		"out vec3 viewer_vector;" \
		"void main(void)" \
		"{" \
		"if(u_lighting_enabled == 1)" \
		"{" \
		"vec4 eye_coordinates = u_view_matrix*u_model_matrix*vPosition;" \
		"transformed_normals = mat3(u_view_matrix * u_model_matrix) * vNormal;" \
		"light_direction = vec3(u_light_position) - eye_coordinates.xyz;" \
		"viewer_vector = -eye_coordinates.xyz;" \
		"}" \
		"gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vPosition;" \
		"}";

	glShaderSource(gVertexShaderObject, 1, (const GLchar**)&vertexShaderShaderCode, NULL);

	//compile shader
	glCompileShader(gVertexShaderObject);

	GLint iInfoLength = 0;
	GLint iShaderCompiledStatus = 0;
	char * szInfoLog = NULL;

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


	//*****FRAGMENT SHADER****
	gFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

	//provide source code to shader
	const GLchar *fragmentShaderSourceCode =
		"#version 430 core" \
		"\n" \
		"in vec3 transformed_normals;" \
		"in vec3 light_direction;" \
		"in vec3 viewer_vector;" \
		"out vec4 FragColor;"\
		"uniform vec3 u_La;" \
		"uniform vec3 u_Ld;" \
		"uniform vec3 u_Ls;" \
		"uniform vec3 u_Ka;" \
		"uniform vec3 u_Kd;" \
		"uniform vec3 u_Ks;" \
		"uniform float u_material_shininess;" \
		"uniform int u_lighting_enabled;" \
		"void main(void)" \
		"{" \
		"vec3 phong_ads_color;" \
		"if(u_lighting_enabled==1)"\
		"{" \
		"vec3 normalized_transformed_normals=normalize(transformed_normals);" \
		"vec3 normalized_light_direction=normalize(light_direction);" \
		"vec3 normalized_viewer_vector=normalize(viewer_vector);" \
		"vec3 H  = normalize(normalized_light_direction + normalized_viewer_vector);"\
		"vec3 ambient = u_La * u_Ka;" \
		"float tn_dot_ld = max(dot(normalized_transformed_normals, normalized_light_direction),0.0);" \
		"vec3 diffuse = u_Ld * u_Kd * tn_dot_ld;" \
		"vec3 specular = u_Ls * u_Ks * pow(max(dot(normalized_transformed_normals,H),0.0),u_material_shininess);" \
		"phong_ads_color = ambient + diffuse + specular;" \
		"}" \
		"else" \
		"{" \
		"phong_ads_color = vec3(1.0f,1.0f,1.0f);" \
		"}" \
		"FragColor = vec4(phong_ads_color,1.0);" \
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
				fprintf(gpFile, "Fragment shader compilation Log %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}


	//*****SHADER PROGRAM******
	//create shader program
	gShaderProgramObject = glCreateProgram();

	//attach vertex shader to shader program
	glAttachShader(gShaderProgramObject, gVertexShaderObject);

	//attach fragment shader to shader program
	glAttachShader(gShaderProgramObject, gFragmentShaderObject);

	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vPosition");
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_NORMAL, "vNormal");

	//link shader
	glLinkProgram(gShaderProgramObject);

	GLint iShaderProgramLinkStatus = 0;
	glGetProgramiv(gShaderProgramObject, GL_LINK_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetProgramiv(gShaderProgramObject, GL_INFO_LOG_LENGTH, &iInfoLength);
		if (iInfoLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetProgramInfoLog(gShaderProgramObject, iInfoLength, &written, szInfoLog);
				fprintf(gpFile, "Shader Program Link Status %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	// get uniform locations

	model_matrix_uniform = glGetUniformLocation(gShaderProgramObject, "u_model_matrix");
	view_matrix_uniform = glGetUniformLocation(gShaderProgramObject, "u_view_matrix");
	projection_matrix_uniform = glGetUniformLocation(gShaderProgramObject, "u_projection_matrix");

	//L/I Key is pressed or not
	L_KeyPressed_uniform = glGetUniformLocation(gShaderProgramObject, "u_lighting_enabled");


	//ambient color intensity of LIGHT
	La_uniform = glGetUniformLocation(gShaderProgramObject, "u_La");
	//diffuse color intensity of LIGHT
	Ld_uniform = glGetUniformLocation(gShaderProgramObject, "u_Ld");
	//specular color intensity of LIGHT
	Ls_uniform = glGetUniformLocation(gShaderProgramObject, "u_Ls");
	//position of light
	light_position_uniform = glGetUniformLocation(gShaderProgramObject, "u_light_position");

	//ambient reflective color intensity of MATERIAL
	Ka_uniform = glGetUniformLocation(gShaderProgramObject, "u_Ka");
	//diffuse reflective color intensity of MATERIAL
	Kd_uniform = glGetUniformLocation(gShaderProgramObject, "u_Kd");

	//specular reflective color intensity of MATERIAL
	Ks_uniform = glGetUniformLocation(gShaderProgramObject, "u_Ks");

	//shininess of material (value is conventionally between 1 to 200)
	material_shininess_uniform = glGetUniformLocation(gShaderProgramObject, "u_material_shininess");

	//vertices colors shader attribs 


	for (int i = 0; i < gMeshWidth; i++)
	{
		for (int j = 0; j < gMeshHeight; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				pos[i][j][k] = 0.0f;
			}
		}
	}

	//vao
	glGenVertexArrays(1, &gVao);
	glBindVertexArray(gVao);

	//position vbo
	glGenBuffers(1, &gVboCpu);
	glBindBuffer(GL_ARRAY_BUFFER, gVboCpu);
	glBufferData(GL_ARRAY_BUFFER, MyArraySize*sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &gVboGpu);
	glBindBuffer(GL_ARRAY_BUFFER, gVboGpu);
	glBufferData(GL_ARRAY_BUFFER, MyArraySize * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	error = cudaGraphicsGLRegisterBuffer(&graphicsResource, gVboGpu, cudaGraphicsMapFlagsWriteDiscard);
	if (error)
	{
		fprintf(gpFile, "UnsuccessRegisterBuffer");
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);


	fprintf(gpFile, "After all vertices and normals..\n");



	glShadeModel(GL_SMOOTH);
	//DEPTH
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0, 0.0f);

	gPerspectiveProjectionMatrix = mat4::identity();
	gbLight = false;
	resize(WIN_WIDTH, WIN_HEIGHT);
	fprintf(gpFile, "Exitting Initialize..\n");

}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//start using OpenGL program object
	glUseProgram(gShaderProgramObject);


	
		//set u_lighting_enabled uniform
		glUniform1i(L_KeyPressed_uniform, 1);

		//setting lights properties
		glUniform3fv(La_uniform, 1, lightAmbient);
		glUniform3fv(Ld_uniform, 1, lightDiffuse);
		glUniform3fv(Ls_uniform, 1, lightSpecular);
		glUniform4fv(light_position_uniform, 1, lightPosition);

		//setting material's properties
		glUniform3fv(Ka_uniform, 1, material_ambient);
		glUniform3fv(Kd_uniform, 1, material_diffuse);
		glUniform3fv(Ks_uniform, 1, material_specular);
		glUniform1f(material_shininess_uniform, material_shininess);

	


	//OpenGL Drawing
	mat4 modelMatrix = mat4::identity();
	mat4 viewMatrix = mat4::identity();

	//apply z axis translation to go deep into the screen by -2.0
	//so that triangle with same fullscreen coordinated but due to above translation will look small

	modelMatrix = translate(0.0f, 0.0f, -2.0f);

	glUniformMatrix4fv(model_matrix_uniform, 1, GL_FALSE, modelMatrix);
	glUniformMatrix4fv(view_matrix_uniform, 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(projection_matrix_uniform, 1, GL_FALSE, gPerspectiveProjectionMatrix);
	glBindVertexArray(gVao);
	if (bOnGpu == true)
	{
		glBindBuffer(GL_ARRAY_BUFFER, gVboGpu);
		error = cudaGraphicsMapResources(1, &graphicsResource, 0);
		if (error)
		{
			fprintf(gpFile, "Error in mapping CUDA Resource..\n");

		}
		error = cudaGraphicsResourceGetMappedPointer((void**)&pPos, &byteCount, graphicsResource);
		if (error)
		{
			fprintf(gpFile, "Error in cudaGraphicsResourceGetMappedPointer..\n");

		}
		launchCUDAKernel(pPos, gMeshWidth, gMeshHeight, gAnim);
		//glBindBuffer(GL_ARRAY_BUFFER, gVboGpu);
		//glBufferData(GL_ARRAY_BUFFER, MyArraySize * sizeof(float), pPos, GL_DYNAMIC_DRAW);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		cudaGraphicsUnmapResources(1, &graphicsResource, 0);
	
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, gVboCpu);
		launchCPUKernel(gMeshWidth, gMeshHeight, gAnim);
		glBufferData(GL_ARRAY_BUFFER, MyArraySize * sizeof(float), pos, GL_DYNAMIC_DRAW);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);

	}
	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);
	glDrawArrays(GL_POINTS, 0, gMeshWidth*gMeshHeight);			//(each with its x,y,z ) vertices in triangleVertices array
	glBindVertexArray(0);

	/*	

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gVbo_Sphere_element);
	glDrawElements(GL_TRIANGLES, gNumElements, GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0);*/
	glUseProgram(0);
	gAnim = gAnim + 0.01f;
	SwapBuffers(ghdc);
}

void resize(int width, int height)
{
	if (height == 0)
		height = 1;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	gPerspectiveProjectionMatrix = perspective(60.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void uninitialize(void)
{

	if (gbFullScreen == true)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}
	if (gVao)
	{
		glDeleteVertexArrays(1, &gVao);
		gVao = 0;
	}

	if (gVboCpu)
	{
		glDeleteBuffers(1, &gVboCpu);
		gVboCpu = 0;
	}

	if (gVboGpu)
	{
		glDeleteBuffers(1, &gVboGpu);
		gVboGpu = 0;
	}

	if (gVbo_Sphere_element)
	{
		glDeleteBuffers(1, &gVbo_Sphere_element);
		gVbo_Sphere_element = 0;
	}

	cudaGraphicsUnregisterResource(graphicsResource);
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

	//delete shader program object
	glDeleteProgram(gShaderProgramObject);
	gShaderProgramObject = 0;

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


























