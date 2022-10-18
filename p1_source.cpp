// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <sstream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

// Include AntTweakBar
#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

// ATTN 1A is the general place in the program where you have to change the code base to satisfy a Task of Project 1A.
// ATTN 1B for Project 1B. ATTN 1C for Project 1C. Focus on the ones relevant for the assignment you're working on.

typedef struct Vertex {
	float Position[4];
	float Color[4];
	void SetCoords(float *coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = coords[3];
	}
	void SetColor(float *color) {
		Color[0] = color[0];
		Color[1] = color[1];
		Color[2] = color[2];
		Color[3] = color[3];
	}
};

// ATTN: use POINT structs for cleaner code (POINT is a part of a vertex)
// allows for (1-t)*P_1+t*P_2  avoiding repeat for each coordinate (x,y,z)
typedef struct point {
	float x, y, z;
	point(const float x = 0, const float y = 0, const float z = 0) : x(x), y(y), z(z){};
	point(float *coords) : x(coords[0]), y(coords[1]), z(coords[2]){};
	point operator -(const point& a) const {
		return point(x - a.x, y - a.y, z - a.z);
	}
	point operator +(const point& a) const {
		return point(x + a.x, y + a.y, z + a.z);
	}
	point operator *(const float& a) const {
		return point(x * a, y * a, z * a);
	}
	point operator /(const float& a) const {
		return point(x / a, y / a, z / a);
	}
	
	float* toArray() {
		float array[] = { x, y, z, 1.0f };
		return array;
	}
};

// Function prototypes
int initWindow(void);
void initOpenGL(void);
void createVAOs(Vertex[], GLushort[], int);
void createObjects(void);
void pickVertex(void);
void moveVertex(void);
void draw_B_Spline(int);
void create_B_spline_objects(void);
void draw_Bezier_Curves(int);
void create_Bezier_curve_objects(void);
void draw_Catmull_Rom_Curves(int);
void show_second_view(int);
void create_catmull_rom_objects(void);
void create_second_view_objects(void);
void set_color(void);
void renderScene(void);
void cleanup(void);
static void mouseCallback(GLFWwindow*, int, int, int);
static void keyCallback(GLFWwindow*, int, int, int, int);

// GLOBAL VARIABLES
GLFWwindow* window;
const GLuint window_width = 1024, window_height = 768;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

// Program IDs
GLuint programID;
GLuint pickingProgramID;

// Uniform IDs
GLuint MatrixID;
GLuint ViewMatrixID;
GLuint ModelMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorArrayID;
GLuint pickingColorID;

GLuint gPickedIndex;
std::string gMessage;

// ATTN: INCREASE THIS NUMBER AS YOU CREATE NEW OBJECTS
const GLuint NumObjects = 10; // Number of objects types in the scene

// Keeps track of IDs associated with each object
GLuint VertexArrayId[NumObjects];
GLuint VertexBufferId[NumObjects];
GLuint IndexBufferId[NumObjects];

size_t VertexBufferSize[NumObjects];
size_t IndexBufferSize[NumObjects];
size_t NumVerts[NumObjects];	// Useful for glDrawArrays command
size_t NumIdcs[NumObjects];	// Useful for glDrawElements command

// Initialize ---  global objects -- not elegant but ok for this project
const size_t IndexCount = 3000; //Not sure about this but I changed 4 into 8 on 9/12/2022
Vertex Vertices[IndexCount];
Vertex CatmullVertices[40];
GLushort Indices[IndexCount];

// ATTN: DON'T FORGET TO INCREASE THE ARRAY SIZE IN THE PICKING VERTEX SHADER WHEN YOU ADD MORE PICKING COLORS
float pickingColor[IndexCount];
float OriginalColorR, OriginalColorG, OriginalColorB;
bool isClick = false;
bool drawCRLine = false;
bool doubleView = false;
int posi = 1000;
int shift = 0;
int index = 1000;
int counter = 0;

int initWindow(void) {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // FOR MAC

	// ATTN: Project 1A, Task 0 == Change the name of the window
	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Jiangan,Chen(3676-5451)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Initialize the GUI display
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(window_width, window_height);
	TwBar * GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &gMessage, NULL);

	// Set up inputs
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetMouseButtonCallback(window, mouseCallback);
	glfwSetKeyCallback(window, keyCallback);


	return 0;
}

void initOpenGL(void) {
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	//glm::mat4 ProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for Project 1, use an ortho camera :
	gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	gViewMatrix = glm::lookAt(
		glm::vec3(0, 0, -5), // Camera is at (0,0,-5) below the origin, in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is looking up at the origin (set to 0,-1,0 to look upside-down)
	);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("p1_StandardShading.vertexshader", "p1_StandardShading.fragmentshader");
	pickingProgramID = LoadShaders("p1_Picking.vertexshader", "p1_Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	
	// Get a handle for our "pickingColorID" uniform
	pickingColorArrayID = glGetUniformLocation(pickingProgramID, "PickingColorArray");
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");

	// Define pickingColor array for picking program
	// use a for-loop here------------------------------used on 9/12/2022
	for (int iterator = 0; iterator < IndexCount; iterator++) {
		pickingColor[iterator] = iterator / 255.0f;
	}
	// Define objects
	createObjects();

	// ATTN: create VAOs for each of the newly created objects here:
	// for several objects of the same type use a for-loop
	int obj = 0;  // initially there is only one type of object 
	VertexBufferSize[obj] = sizeof(Vertices);
	IndexBufferSize[obj] = sizeof(Indices);
	NumIdcs[obj] = IndexCount;

	createVAOs(Vertices, Indices, obj);
}

// this actually creates the VAO (structure) and the VBO (vertex data buffer)
void createVAOs(Vertex Vertices[], GLushort Indices[], int ObjectId) {
	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].Position);

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);
	glBindVertexArray(VertexArrayId[ObjectId]);

	// Create buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Vertices, GL_STATIC_DRAW);

	// Create buffer for indices
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[ObjectId], Indices, GL_STATIC_DRAW);
	}

	// Assign vertex attributes
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)RgbOffset);

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color

	// Disable our Vertex Buffer Object 
	glBindVertexArray(0);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
		);
	}
}

void create_B_spline_objects(void) {
	//k = 1
	for (int j = 0; j <= 3; j++) {
		Vertices[10].Position[j] = (Vertices[9].Position[j] + Vertices[0].Position[j]) / 2;
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[11].Position[j] = (Vertices[9].Position[j] + 6 * Vertices[0].Position[j] + Vertices[1].Position[j]) / 8;
	}
	for (int i = 12; i <= 28; i++) {
		if (i % 2 == 0) {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 10) / 2].Position[j] +
					Vertices[(i - 10) / 2 - 1].Position[j]) / 2;
			}
		}
		else {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 11) / 2 - 1].Position[j] +
					6 * Vertices[(i - 11) / 2].Position[j] + Vertices[(i - 11) / 2 + 1].Position[j]) / 8;
			}
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[29].Position[j] = (Vertices[8].Position[j] + 6 * Vertices[9].Position[j] + Vertices[0].Position[j]) / 8;
	}


	//k = 2
	for (int j = 0; j <= 3; j++) {
		Vertices[30].Position[j] = (Vertices[29].Position[j] + Vertices[10].Position[j]) / 2;
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[31].Position[j] = (Vertices[29].Position[j] + 6 * Vertices[10].Position[j] + Vertices[11].Position[j]) / 8;
	}
	for (int i = 32; i <= 68; i++) {
		if (i % 2 == 0) {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 30) / 2 + 10].Position[j] +
					Vertices[(i - 30) / 2 + 9].Position[j]) / 2;
			}
		}
		else {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 31) / 2 + 9].Position[j] +
					6 * Vertices[(i - 31) / 2 + 10].Position[j] + Vertices[(i - 31) / 2 + 11].Position[j]) / 8;
			}
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[69].Position[j] = (Vertices[28].Position[j] + 6 * Vertices[29].Position[j] + Vertices[10].Position[j]) / 8;
	}

	//k = 3
	for (int j = 0; j <= 3; j++) {
		Vertices[70].Position[j] = (Vertices[69].Position[j] + Vertices[30].Position[j]) / 2;
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[71].Position[j] = (Vertices[69].Position[j] + 6 * Vertices[30].Position[j] + Vertices[31].Position[j]) / 8;
	}
	for (int i = 72; i <= 148; i++) {
		if (i % 2 == 0) {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 70) / 2 + 30].Position[j] +
					Vertices[(i - 70) / 2 + 29].Position[j]) / 2;
			}
		}
		else {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 71) / 2 + 29].Position[j] +
					6 * Vertices[(i - 71) / 2 + 30].Position[j] + Vertices[(i - 71) / 2 + 31].Position[j]) / 8;
			}
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[149].Position[j] = (Vertices[68].Position[j] + 6 * Vertices[69].Position[j] + Vertices[30].Position[j]) / 8;
	}

	//k = 4
	for (int j = 0; j <= 3; j++) {
		Vertices[150].Position[j] = (Vertices[149].Position[j] + Vertices[70].Position[j]) / 2;
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[151].Position[j] = (Vertices[149].Position[j] + 6 * Vertices[70].Position[j] + Vertices[71].Position[j]) / 8;
	}
	for (int i = 152; i <= 318; i++) {
		if (i % 2 == 0) {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 150) / 2 + 70].Position[j] +
					Vertices[(i - 150) / 2 + 69].Position[j]) / 2;
			}
		}
		else {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 151) / 2 + 69].Position[j] +
					6 * Vertices[(i - 151) / 2 + 70].Position[j] + Vertices[(i - 151) / 2 + 71].Position[j]) / 8;
			}
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[319].Position[j] = (Vertices[148].Position[j] + 6 * Vertices[149].Position[j] + Vertices[70].Position[j]) / 8;
	}

	//k = 5
	for (int j = 0; j <= 3; j++) {
		Vertices[310].Position[j] = (Vertices[309].Position[j] + Vertices[150].Position[j]) / 2;
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[311].Position[j] = (Vertices[309].Position[j] + 6 * Vertices[150].Position[j] + Vertices[151].Position[j]) / 8;
	}
	for (int i = 312; i <= 628; i++) {
		if (i % 2 == 0) {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 310) / 2 + 150].Position[j] +
					Vertices[(i - 310) / 2 + 149].Position[j]) / 2;
			}
		}
		else {
			for (int j = 0; j <= 3; j++) {
				Vertices[i].Position[j] = (Vertices[(i - 311) / 2 + 149].Position[j] +
					6 * Vertices[(i - 311) / 2 + 150].Position[j] + Vertices[(i - 311) / 2 + 151].Position[j]) / 8;
			}
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[629].Position[j] = (Vertices[308].Position[j] + 6 * Vertices[309].Position[j] + Vertices[150].Position[j]) / 8;
	}
}

void create_Bezier_curve_objects(void) {
	for (int i = 0; i <= 8; i++) {
		for (int j = 0; j <= 3; j++) {
			Vertices[630 + i].Position[j] = (2 * Vertices[i].Position[j] + Vertices[i + 1].Position[j]) / 3;
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[639].Position[j] = (2 * Vertices[9].Position[j] + Vertices[0].Position[j]) / 3;
	}
	for (int i = 0; i <= 8; i++) {
		for (int j = 0; j <= 3; j++) {
			Vertices[640 + i].Position[j] = (Vertices[i].Position[j] + 2 * Vertices[i + 1].Position[j]) / 3;
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[649].Position[j] = (Vertices[9].Position[j] + 2 * Vertices[0].Position[j]) / 3;
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[650].Position[j] = (Vertices[631].Position[j] + Vertices[640].Position[j]) / 2;
		Vertices[651].Position[j] = (Vertices[632].Position[j] + Vertices[641].Position[j]) / 2;
		Vertices[652].Position[j] = (Vertices[633].Position[j] + Vertices[642].Position[j]) / 2;
		Vertices[653].Position[j] = (Vertices[634].Position[j] + Vertices[643].Position[j]) / 2;
		Vertices[654].Position[j] = (Vertices[635].Position[j] + Vertices[644].Position[j]) / 2;
		Vertices[655].Position[j] = (Vertices[636].Position[j] + Vertices[645].Position[j]) / 2;
		Vertices[656].Position[j] = (Vertices[637].Position[j] + Vertices[646].Position[j]) / 2;
		Vertices[657].Position[j] = (Vertices[638].Position[j] + Vertices[647].Position[j]) / 2;
		Vertices[658].Position[j] = (Vertices[639].Position[j] + Vertices[648].Position[j]) / 2;
	}
}

void create_catmull_rom_objects(void) {
	for (int i = 700; i <= 707; i++) {
		for (int j = 0; j <= 3; j++) {
			Vertices[i].Position[j] = Vertices[i - 700 + 1].Position[j] -
				(Vertices[i - 700 + 2].Position[j] - Vertices[i - 700].Position[j]) / 6;
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[708].Position[j] = Vertices[9].Position[j] -
			(Vertices[0].Position[j] - Vertices[8].Position[j]) / 6;
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[709].Position[j] = Vertices[0].Position[j] -
			(Vertices[1].Position[j] - Vertices[9].Position[j]) / 6;
	}
	for (int i = 710; i <= 717; i++) {
		for (int j = 0; j <= 3; j++) {
			Vertices[i].Position[j] = Vertices[i - 710 + 1].Position[j] +
				(Vertices[i - 710 + 2].Position[j] - Vertices[i - 710].Position[j]) / 6;
		}
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[718].Position[j] = Vertices[9].Position[j] +
			(Vertices[0].Position[j] - Vertices[8].Position[j]) / 6;
	}
	for (int j = 0; j <= 3; j++) {
		Vertices[719].Position[j] = Vertices[0].Position[j] +
			(Vertices[1].Position[j] - Vertices[9].Position[j]) / 6;
	}

	//draw green points
	CatmullVertices[0] = Vertices[0];
	CatmullVertices[1] = Vertices[719];
	CatmullVertices[2] = Vertices[700];
	CatmullVertices[3] = Vertices[1];
	CatmullVertices[4] = Vertices[710];
	CatmullVertices[5] = Vertices[701];
	CatmullVertices[6] = Vertices[2];
	CatmullVertices[7] = Vertices[711];
	CatmullVertices[8] = Vertices[702];
	CatmullVertices[9] = Vertices[3];
	CatmullVertices[10] = Vertices[712];
	CatmullVertices[11] = Vertices[703];
	CatmullVertices[12] = Vertices[4];
	CatmullVertices[13] = Vertices[713];
	CatmullVertices[14] = Vertices[704];
	CatmullVertices[15] = Vertices[5];
	CatmullVertices[16] = Vertices[714];
	CatmullVertices[17] = Vertices[705];
	CatmullVertices[18] = Vertices[6];
	CatmullVertices[19] = Vertices[715];
	CatmullVertices[20] = Vertices[706];
	CatmullVertices[21] = Vertices[7];
	CatmullVertices[22] = Vertices[716];
	CatmullVertices[23] = Vertices[707];
	CatmullVertices[24] = Vertices[8];
	CatmullVertices[25] = Vertices[717];
	CatmullVertices[26] = Vertices[708];
	CatmullVertices[27] = Vertices[9];
	CatmullVertices[28] = Vertices[718];
	CatmullVertices[29] = Vertices[709];
	CatmullVertices[30] = Vertices[0];

	Vertices[1500] = Vertices[719];
	Vertices[1501] = Vertices[700];
	Vertices[1502] = Vertices[710];
	Vertices[1503] = Vertices[701];
	Vertices[1504] = Vertices[711];
	Vertices[1505] = Vertices[702];
	Vertices[1506] = Vertices[712];
	Vertices[1507] = Vertices[703];
	Vertices[1508] = Vertices[713];
	Vertices[1509] = Vertices[704];
	Vertices[1510] = Vertices[714];
	Vertices[1511] = Vertices[705];
	Vertices[1512] = Vertices[715];
	Vertices[1513] = Vertices[706];
	Vertices[1514] = Vertices[716];
	Vertices[1515] = Vertices[707];
	Vertices[1516] = Vertices[717];
	Vertices[1517] = Vertices[708];
	Vertices[1518] = Vertices[718];
	Vertices[1519] = Vertices[709];

	float t;
	posi = 1000;

	for (int i = 0; i <= 27; i++) {
		if (i % 3 == 0) {
			point p0 = { CatmullVertices[i].Position[0], CatmullVertices[i].Position[1], 0.0f };
			point p1 = { CatmullVertices[i + 1].Position[0], CatmullVertices[i + 1].Position[1], 0.0f };
			point p2 = { CatmullVertices[i + 2].Position[0], CatmullVertices[i + 2].Position[1], 0.0f };
			point p3 = { CatmullVertices[i + 3].Position[0], CatmullVertices[i + 3].Position[1], 0.0f };

			for (int k = 0; k <= 16; k++) {
				t = k * 1.0 / 16.0;
				point p = p0 * pow(1 - t, 3) +
					p1 * (3 * (pow(t, 3) - 2 * pow(t, 2) + t)) +
					p2 * (3 * (pow(t, 2) - pow(t, 3))) +
					p3 * pow(t, 3);
				Vertices[posi] = { {p.x, p.y, p.z, 1.0f}, {0.0, 1.0f, 0.0f, 1.0f} };
				posi++;
			}
		}
	}

	for (int i = 1000; i < posi; i++) {
		Vertices[i].Position[2] = 0.0f;
		Vertices[i].Position[3] = 1.0f;
	}

	for (int i = 0; i <= 30; i++) {
		CatmullVertices[i].Color[0] = 1.0f;
		CatmullVertices[i].Color[1] = 0.0f;
		CatmullVertices[i].Color[2] = 0.0f;
		CatmullVertices[i].Color[3] = 1.0f;
	}
}

void set_color(void) {
	for (int i = 10; i <= 629; i++) {
		Vertices[i].Color[0] = 0.0f;
		Vertices[i].Color[1] = 1.0f;
		Vertices[i].Color[2] = 1.0f;
		Vertices[i].Color[3] = 1.0f;
	}

	for (int i = 630; i <= 658; i++) {
		Vertices[i].Color[0] = 1.0f;
		Vertices[i].Color[1] = 1.0f;
		Vertices[i].Color[2] = 0.0f;
		Vertices[i].Color[3] = 1.0f;
	}

	for (int i = 700; i <= 719; i++) {
		Vertices[i].Color[0] = 1.0f;
		Vertices[i].Color[1] = 0.0f;
		Vertices[i].Color[2] = 0.0f;
		Vertices[i].Color[3] = 1.0f;
	}

	for (int i = 1000; i <= posi; i++) {
		Vertices[i].Color[0] = 0.0f;
		Vertices[i].Color[1] = 1.0f;
		Vertices[i].Color[2] = 0.0f;
		Vertices[i].Color[3] = 1.0f;
	}
}

void create_second_view_objects(void) { 
	for (int i = 2000; i <= 2009; i++) {
		Vertices[i].Position[0] = Vertices[i - 2000].Position[2] - 2.0f;
		Vertices[i].Position[1] = Vertices[i - 2000].Position[1];
		Vertices[i].Position[2] = Vertices[i - 2000].Position[0];
		Vertices[i].Position[3] = Vertices[i - 2000].Position[3];
		for (int j = 0; j <= 3; j++) {
			Vertices[i].Color[j] = Vertices[i - 2000].Color[j];
		}
	}
}

void createObjects(void) {
	// ATTN: DERIVE YOUR NEW OBJECTS HERE:  each object has
	// an array of vertices {pos;color} and
	// an array of indices (no picking needed here) (no need for indices)
	// ATTN: Project 1A, Task 1 == Add the points in your scene

	Vertices[0] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[1] = { { 0.809f, 0.5878f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[2] = { { 0.5f, 1.538f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[3] = { { -0.5f, 1.538f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[4] = { { -0.809f, 0.5878f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[5] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[6] = { { 0.809f, -0.5878f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[7] = { { 0.5f, -1.538f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[8] = { { -0.5f, -1.538f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	Vertices[9] = { { -0.809f, -0.5878f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };

	create_second_view_objects();

	//B-spline Curves
	create_B_spline_objects();

	//Bezier Curves
	create_Bezier_curve_objects();

	//Catmull-Rom Curves
	create_catmull_rom_objects();

	//set color
	set_color();

	Indices[0] = 0;
	Indices[1] = 1;
	Indices[2] = 2;
	Indices[3] = 3;
	Indices[4] = 4;
	Indices[5] = 5;
	Indices[6] = 6;
	Indices[7] = 7;
	Indices[8] = 8;
	Indices[9] = 9;

	// ATTN: Project 1B, Task 1 == create line segments to connect the control points

	// ATTN: Project 1B, Task 2 == create the vertices associated to the smoother curve generated by subdivision

	// ATTN: Project 1B, Task 4 == create the BB control points and apply De Casteljau's for their corresponding for each piece

	// ATTN: Project 1C, Task 3 == set coordinates of yellow point based on BB curve and perform calculations to find
	// the tangent, normal, and binormal
}

void pickVertex(void) {
	// Clear the screen in white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pickingProgramID);
	{
		glm::mat4 ModelMatrix = glm::mat4(1.0); // initialization
		// ModelMatrix == TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;
		// MVP should really be PVM...
		// Send the MVP to the shader (that is currently bound)
		// as data type uniform (shared by all shader instances)
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);

		// pass in the picking color array to the shader
		glUniform1fv(pickingColorArrayID, IndexCount, pickingColor);

		// --- enter vertices into VBO and draw
		glEnable(GL_PROGRAM_POINT_SIZE);
		glBindVertexArray(VertexArrayId[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, VertexBufferSize[0], Vertices);	// update buffer data
		glDrawElements(GL_POINTS, NumIdcs[0], GL_UNSIGNED_SHORT, (void*)0);
		glBindVertexArray(0);
	}
	glUseProgram(0);
	glFlush();
	// --- Wait until all the pending drawing commands are really done.
	// Ultra-mega-over slow ! 
	// There are usually a long time between glDrawElements() and
	// all the fragments completely rasterized.
	glFinish();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// --- Read the pixel at the center of the screen.
	// You can also use glfwGetMousePos().
	// Ultra-mega-over slow too, even for 1 pixel, 
	// because the framebuffer is on the GPU.
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];  // 2x2 pixel region
	glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
    // window_height - ypos;  
	// OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);
	

	// ATTN: Project 1A, Task 2
	// Find a way to change color of selected vertex and
	// store original color
	OriginalColorR = Vertices[gPickedIndex].Color[0];
	OriginalColorG = Vertices[gPickedIndex].Color[1];
	OriginalColorB = Vertices[gPickedIndex].Color[2];

	// glBindBuffer(GL_ARRAY_BUFFER, VertexArrayId[0]);   // This is the wrong position to put BindBuffer
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	isClick = true;

	Vertices[gPickedIndex].Color[0] = 0.8f;
	Vertices[gPickedIndex].Color[1] = 0.8f;
	Vertices[gPickedIndex].Color[2] = 0.8f;


	// Uncomment these lines if you wan to see the picking shader in effect
	// glfwSwapBuffers(window);
	// continue; // skips the visible rendering
}

// ATTN: Project 1A, Task 3 == Retrieve your cursor position, get corresponding world coordinate, and move the point accordingly

// ATTN: Project 1C, Task 1 == Keep track of z coordinate for selected point and adjust its value accordingly based on if certain
// buttons are being pressed
void moveVertex(void) {
	glm::mat4 ModelMatrix = glm::mat4(1.0);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glm::vec4 vp = glm::vec4(viewport[0], viewport[1], viewport[2], viewport[3]);

	if (gPickedIndex >= IndexCount) { 
		// Any number > vertices-indices is background!
		gMessage = "background";
	}
	else {
		if (shift == 1) {
			std::ostringstream oss;
			oss << "point " << gPickedIndex;
			gMessage = oss.str();
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			vec3 mousePos = glm::unProject(glm::vec3(xpos, ypos, 0.0), ModelMatrix, gProjectionMatrix, vec4(viewport[0], viewport[1], viewport[2], viewport[3]));

			float distance = Vertices[gPickedIndex].Position[1] + mousePos.y;
			Vertices[gPickedIndex].Position[2] = 0.0f + distance;

			/*glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[0], Indices, GL_STATIC_DRAW);*/

			create_second_view_objects();
			create_B_spline_objects();
			create_Bezier_curve_objects();
			create_catmull_rom_objects();
			set_color();

			createVAOs(Vertices, Indices, 0);
		}
		else {
			std::ostringstream oss;
			oss << "point " << gPickedIndex;
			gMessage = oss.str();
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			vec3 mousePos = glm::unProject(glm::vec3(xpos, ypos, 0.0), ModelMatrix, gProjectionMatrix, vec4(viewport[0], viewport[1], viewport[2], viewport[3]));

			Vertices[gPickedIndex].Position[0] = -mousePos.x;
			Vertices[gPickedIndex].Position[1] = -mousePos.y;

			/*glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[0], Indices, GL_STATIC_DRAW);*/

			create_second_view_objects();
			create_B_spline_objects();
			create_Bezier_curve_objects();
			create_catmull_rom_objects();
			set_color();

			createVAOs(Vertices, Indices, 0);
		}
	}
}

void draw_B_Spline(int k) {
	
	if (k == 1) {
		for (int i = 10; i <= 29; i++) {
			Indices[i] = i;
		}
	}
	else if (k == 2) {
		for (int i = 10; i <= 29; i++) {
			Indices[i] = NULL;
		}
		for (int i = 30; i <= 69; i++) {
			Indices[i] = i;
		}
	}
	else if (k == 3) {
		for (int i = 30; i <= 69; i++) {
			Indices[i] = NULL;
		}
		for (int i = 70; i <= 149; i++) {
			Indices[i] = i;
		}
	}
	else if (k == 4) {
		for (int i = 70; i <= 149; i++) {
			Indices[i] = NULL;
		}
		for (int i = 150; i <= 309; i++) {
			Indices[i] = i;
		}
	}
	else if (k == 5) {
		for (int i = 150; i <= 309; i++) {
			Indices[i] = NULL;
		}
		for (int i = 310; i <= 629; i++) {
			Indices[i] = i;
		}
	}
	else if (k == 6) {
		for (int i = 10; i <= 629; i++) {
			Indices[i] = NULL;
		}
	}
	createVAOs(Vertices, Indices, 0);
}

void draw_Bezier_Curves(int flg) {
	if (flg == 1) {
		for (int i = 630; i <= 658; i++) {
			Indices[i] = i;
		}
	}
	else if (flg == 2) {
		for (int i = 630; i <= 658; i++) {
			Indices[i] = NULL;
		}
	}
	createVAOs(Vertices, Indices, 0);
}

void draw_Catmull_Rom_Curves(int jorg) {
	if (jorg == 1) {
		for (int i = 700; i <= 719; i++) {
			Indices[i] = i;
		}
	}
	else if (jorg == 2) {
		for (int i = 700; i <= 719; i++) {
			Indices[i] = NULL;
		}
	}
	createVAOs(Vertices, Indices, 0);
}

void show_second_view(int peters) {
	if (peters == 1) {
		for (int i = 0; i <= 9; i++) {
			Vertices[i].Position[0] += 2;
		}
		for (int i = 0; i <= 9; i++) {
			Indices[i] = i;
		}
		for (int i = 2000; i <= 2009; i++) {
			Indices[i] = i;
		}
	}
	else if (peters == 2) {
		for (int i = 0; i <= 9; i++) {
			Vertices[i].Position[0] -= 2;
		}
		for (int i = 0; i <= 9; i++) {
			Indices[i] = i;
		}
		for (int i = 2000; i <= 2009; i++) {
			Indices[i] = NULL;
		}
	}
	createVAOs(Vertices, Indices, 0);
}


void renderScene(void) {    
	// Dark blue background

	glUseProgram(programID);
	{
		// see comments in pick
		glm::mat4 ModelMatrix = glm::mat4(1.0); 
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		
		glEnable(GL_PROGRAM_POINT_SIZE);

		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
		glBindVertexArray(VertexArrayId[0]);	// Draw Vertices
		glBufferSubData(GL_ARRAY_BUFFER, 0, VertexBufferSize[0], Vertices);		// Update buffer data
		glDrawElements(GL_POINTS, NumIdcs[0], GL_UNSIGNED_SHORT, (void*)0);

		if (drawCRLine) {
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
			std::vector<GLushort> indices;
			for (int i = 1500; i <= 1519; i++) {
				indices.push_back(i);
			}
			indices.push_back(1500);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
			glDrawElements(GL_LINE_STRIP, indices.size(), GL_UNSIGNED_SHORT, (void*)0);

			std::vector<GLushort> indices2;
			for (int i = 1000; i < posi; i++) {
				indices2.push_back(i);
			}
			indices2.push_back(1000);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(GLushort), indices2.data(), GL_STATIC_DRAW);
			glDrawElements(GL_LINE_STRIP, indices2.size(), GL_UNSIGNED_SHORT, (void*)0);

			std::vector<GLushort> indices3;
			for (int i = 0; i < 10; i++) {
				indices3.push_back(i);
			}
			indices3.push_back(0);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices3.size() * sizeof(GLushort), indices3.data(), GL_STATIC_DRAW);
			glDrawElements(GL_POINTS, indices3.size(), GL_UNSIGNED_SHORT, (void*)0);
		}

		if (doubleView) {
			std::vector<GLushort> indices2;
			for (int i = 0; i < 10; i++) {
				indices2.push_back(i);
			}
			indices2.push_back(0);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(GLushort), indices2.data(), GL_STATIC_DRAW);
			glDrawElements(GL_LINE_STRIP, indices2.size(), GL_UNSIGNED_SHORT, (void*)0);

			std::vector<GLushort> indices3;
			for (int i = 2000; i < 2010; i++) {
				indices3.push_back(i);
			}
			indices3.push_back(2000);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices3.size() * sizeof(GLushort), indices3.data(), GL_STATIC_DRAW);
			glDrawElements(GL_LINE_STRIP, indices3.size(), GL_UNSIGNED_SHORT, (void*)0);
		}

		if (counter) {
			{
				std::vector<GLushort> indices2; 
				indices2.push_back(2500);
				indices2.push_back(2501);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(GLushort), indices2.data(), GL_STATIC_DRAW);
				glDrawElements(GL_LINE_STRIP, indices2.size(), GL_UNSIGNED_SHORT, (void*)0);
			}

			{
				std::vector<GLushort> indices2;
				indices2.push_back(2500);
				indices2.push_back(2502);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(GLushort), indices2.data(), GL_STATIC_DRAW);
				glDrawElements(GL_LINE_STRIP, indices2.size(), GL_UNSIGNED_SHORT, (void*)0);
			}

			{
				std::vector<GLushort> indices2; 
				indices2.push_back(2500);
				indices2.push_back(2503);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(GLushort), indices2.data(), GL_STATIC_DRAW);
				glDrawElements(GL_LINE_STRIP, indices2.size(), GL_UNSIGNED_SHORT, (void*)0);
			}
		}

		// // If don't use indices
		// glDrawArrays(GL_POINTS, 0, NumVerts[0]);	

		// ATTN: OTHER BINDING AND DRAWING COMMANDS GO HERE
		// one set per object:
		// glBindVertexArray(VertexArrayId[<x>]); etc etc

		// ATTN: Project 1C, Task 2 == Refer to https://learnopengl.com/Getting-started/Transformations and
		// https://learnopengl.com/Getting-started/Coordinate-Systems - draw all the objects associated with the
		// curve twice in the displayed fashion using the appropriate transformations

		glBindVertexArray(0);
	}
	glUseProgram(0);
	// Draw GUI
	TwDraw();

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void cleanup(void) {
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

// Alternative way of triggering functions on mouse click and keyboard events
static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		pickVertex();
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		Vertices[gPickedIndex].Color[0] = OriginalColorR;
		Vertices[gPickedIndex].Color[1] = OriginalColorG;
		Vertices[gPickedIndex].Color[2] = OriginalColorB;
	}
}

bool isKeyPressed = false;
int k = 0;
int flg = 0;
int jorg = 0;
int peters = 0;
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {

		if (!isKeyPressed) {
			draw_B_Spline(++k);
			if (k == 6) {
				k = 0;
			}
			isKeyPressed = true;
		}
	}
	else if (key == GLFW_KEY_2 && action == GLFW_PRESS) {

		if (!isKeyPressed) {
			draw_Bezier_Curves(++flg);
			if (flg == 2) {
				flg = 0;
			}
			isKeyPressed = true;
		}
	}
	else if (key == GLFW_KEY_3 && action == GLFW_PRESS) {

		if (!isKeyPressed) {
			drawCRLine = true;
			draw_Catmull_Rom_Curves(++jorg);
			if (jorg == 2) {
				jorg = 0;
				drawCRLine = false;
			}
			isKeyPressed = true;
		}
	}
	else if (key == GLFW_KEY_4 && action == GLFW_PRESS) {

		if (!isKeyPressed) {
			doubleView = true;
			show_second_view(++peters);
			if (peters == 2) {
				peters = 0;
				doubleView = false;
			}
			isKeyPressed = true;
		}
	}
	else if (key == GLFW_KEY_5 && action == GLFW_PRESS) {

		if (!isKeyPressed) {
			++counter;
			if (counter == 2) {
				counter = 0;
			}
			isKeyPressed = true;
		}
	}
	else if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
		if (!isKeyPressed) {
			shift++;
			if (shift == 2) {
				shift = 0;
			}
			isKeyPressed = true;
		}
	}
	else {
		isKeyPressed = false;
	}
}

int main(void) {
	// ATTN: REFER TO https://learnopengl.com/Getting-started/Creating-a-window
	// AND https://learnopengl.com/Getting-started/Hello-Window to familiarize yourself with the initialization of a window in OpenGL

	// Initialize window
	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;

	// ATTN: REFER TO https://learnopengl.com/Getting-started/Hello-Triangle to familiarize yourself with the graphics pipeline
	// from setting up your vertex data in vertex shaders to rendering the data on screen (everything that follows)

	// Initialize OpenGL pipeline
	initOpenGL();

	double lastTime = glfwGetTime();
	int nbFrames = 0;
	createObjects();	// re-evaluate curves in case vertices have been moved
	do {
		// Timing 
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0){ // If last prinf() was more than 1sec ago
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}
		
		// DRAGGING: move current (picked) vertex with cursor
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
			moveVertex();
		}

		glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
		// Re-clear the screen for visible rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (counter == 1) {
			Vertices[index].Color[0] = 1.0f;
			Vertices[index].Color[1] = 1.0f;
			Vertices[index].Color[2] = 0.0f;
			Vertices[index].Color[3] = 0.0f;
			Indices[index - 1] = NULL;
			Indices[index] = index;
			index++;
			if (index == posi) {
				index = 1000;
			}
			createVAOs(Vertices, Indices, 0);

			auto current = Vertices[index].Position;
			auto next = Vertices[index + 1].Position;
			float tmp[3];
			tmp[0] = next[0] - current[0];
			tmp[1] = next[1] - current[1];
			tmp[2] = next[2] - current[2];
			auto ans = glm::vec3(tmp[0], tmp[1], tmp[2]);
			glm::vec3 T = normalize(ans);
			tmp[0] = next[0] + current[0];
			tmp[1] = next[1] + current[1];
			tmp[2] = next[2] + current[2];
			auto ans2 = glm::vec3(tmp[0], tmp[1], tmp[2]);
			glm::vec3 B = normalize(cross(T, ans2));
			glm::vec3 N = normalize(cross(B, T));

			Vertices[2500] = { {current[0], current[1], current[3], 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}};
			Vertices[2501] = { {N.x + current[0], N.y + current[1], N.z + current[2], 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};
			Vertices[2502] = { {B.x + current[0], B.y + current[1], B.z + current[2], 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}};
			Vertices[2503] = { {T.x + current[0], T.y + current[1], T.z + current[2], 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f} };
		}

		// ATTN: Project 1B, Task 2 and 4 == account for key presses to activate subdivision and hiding/showing functionality
		// for respective tasks

		// DRAWING the SCENE
		renderScene();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}
