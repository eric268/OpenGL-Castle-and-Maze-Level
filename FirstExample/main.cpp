
///////////////////////////////////////////////////////////////////////
//
// 01_JustAmbient.cpp
//
///////////////////////////////////////////////////////////////////////

using namespace std;

#include <cstdlib>
#include <ctime>
#include "vgl.h"
#include "LoadShaders.h"
#include "Light.h"
#include "Shape.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <iostream>
#include "OrientationEnum.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FPS 60
#define MOVESPEED 0.2f
#define TURNSPEED 0.05f
#define X_AXIS glm::vec3(1,0,0)
#define Y_AXIS glm::vec3(0,1,0)
#define Z_AXIS glm::vec3(0,0,1)
#define XY_AXIS glm::vec3(1,1,0)
#define YZ_AXIS glm::vec3(0,1,1)
#define XZ_AXIS glm::vec3(1,0,1)


enum keyMasks {
	KEY_FORWARD =  0b00000001,		// 0x01 or 1 or 01
	KEY_BACKWARD = 0b00000010,		// 0x02 or 2 or 02
	KEY_LEFT = 0b00000100,		
	KEY_RIGHT = 0b00001000,
	KEY_UP = 0b00010000,
	KEY_DOWN = 0b00100000,
	KEY_MOUSECLICKED = 0b01000000
	// Any other keys you want to add.
};

// IDs.
GLuint vao, ibo, points_vbo, colors_vbo, uv_vbo, normals_vbo, modelID, viewID, projID;
GLuint program;

// Matrices.
glm::mat4 View, Projection;

// Our bitflags. 1 byte for up to 8 keys.
unsigned char keys = 0; // Initialized to 0 or 0b00000000.

// Camera and transform variables.
float scale = 1.0f, angle = 0.0f;
glm::vec3 position, frontVec, worldUp, upVec, rightVec; // Set by function
GLfloat pitch, yaw;
int lastX, lastY;
bool crystalPickedUp = false, crystalOnAlter = false, gameCompleted = false;
float bobCrystal = 0;
float speed = 0.005;
int counter = 0;

// Texture variables.
GLuint gridTx, hedgeTx, turretTx, turretRoofTx, turretTrimTx, turretArchTx, doorTx, stepTx, parapitTx, alterTx, crystalTx;
GLint width, height, bitDepth;

// Light variables.
AmbientLight aLight(glm::vec3(1.0f, 1.0f, 1.0f),	// Ambient colour.
	0.1f);							// Ambient strength.

DirectionalLight dLight(glm::vec3(-1.0f, 0.0f, -0.5f), // Direction.
	glm::vec3(1.0f, 1.0f, 0.25f),  // Diffuse colour.
	0.1f);						  // Diffuse strength.

PointLight pLights[2] = { { glm::vec3(8.3f, 1.2f, -11.9f), 10.0f, glm::vec3(1.0f, 0.0f, 0.0f), 1.0f },
						  { glm::vec3(20.7f, 2.0f, -17.9f), 10.0f, glm::vec3(1.0f, 0.0f, 0.0f), 1.0f } };

SpotLight sLight(glm::vec3(25.0f, 75.75f, -25.0f),	// Position.
	glm::vec3(1.0f, 1.0f, 1.0f),	// Diffuse colour.w
	1.0f,							// Diffuse strength.
	glm::vec3(0.0f, -1.0f, 0.0f),  // Direction.
	35.0f);

void timer(int);
void resetView()
{
	position = glm::vec3(22.0f, 2.0f, -1.5f);
	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	pitch = 0.0f;
	yaw = -90.0f;
	// View will now get set only in transformObject
}

// Shapes. Recommend putting in a map
Cube g_mainHedge(1), g_mainWall(2), g_parapit(1), g_steps(1), g_courtyard(1), g_alter(1);
Prism g_mainTurretPrism(24,100,100), g_upperTurretPrism(24,200,30), g_turretArchPrism(8, 16,8);
//Plane g_plane;
Grid g_ground(50,10); // New UV scale parameter. Works with texture now.
Cone g_turretTop(24,100,100), g_crystal(6,25,25);
Plane g_castleDoor;




void init(void)
{
	srand((unsigned)time(NULL));
	//Specifying the name of vertex and fragment shaders.
	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	//Loading and compiling shaders
	program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up

	modelID = glGetUniformLocation(program, "model");
	projID = glGetUniformLocation(program, "projection");
	viewID = glGetUniformLocation(program, "view");

	// Projection matrix : 45∞ Field of View, aspect ratio, display range : 0.1 unit <-> 100 units
	Projection = glm::perspective(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	// Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	resetView();

	// Image loading.
	stbi_set_flip_vertically_on_load(true);

	unsigned char* gridImage = stbi_load("Dirt.jpg", &width, &height, &bitDepth, 0);
	if (!gridImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &gridTx);
	glBindTexture(GL_TEXTURE_2D, gridTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, gridImage);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(gridImage);

	// Second texture. Blank one.
	
	unsigned char* hedgeImage = stbi_load("FoliageTile.jpg", &width, &height, &bitDepth, 0);
	if (!hedgeImage) cout << "Unable to load file!" << endl;
	
	glGenTextures(1, &hedgeTx);
	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, hedgeImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(hedgeImage);

	unsigned char* turretImage = stbi_load("TurretTexture1.jpg", &width, &height, &bitDepth, 0);
	if (!turretImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &turretTx);
	glBindTexture(GL_TEXTURE_2D, turretTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, turretImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(turretImage);

	unsigned char* turretRoofImage = stbi_load("TurretRoofTexture.jpg", &width, &height, &bitDepth, 0);
	if (!turretRoofImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &turretRoofTx);
	glBindTexture(GL_TEXTURE_2D, turretRoofTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, turretRoofImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(turretRoofImage);

	unsigned char* turretTrimImage = stbi_load("WoodTrim.jpg", &width, &height, &bitDepth, 0);
	if (!turretTrimImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &turretTrimTx);
	glBindTexture(GL_TEXTURE_2D, turretTrimTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, turretTrimImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(turretTrimImage);

	unsigned char* turretArchImage = stbi_load("TurretArchEditNoBG.png", &width, &height, &bitDepth, 0);
	if (!turretTrimImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &turretArchTx);
	glBindTexture(GL_TEXTURE_2D, turretArchTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, turretArchImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(turretArchImage);

	unsigned char* castleDoorImage = stbi_load("CastleDoorTexture2.png", &width, &height, &bitDepth, 0);
	if (!castleDoorImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &doorTx);
	glBindTexture(GL_TEXTURE_2D, doorTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, castleDoorImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(castleDoorImage);

	unsigned char* stepImage = stbi_load("StepTexture.jpg", &width, &height, &bitDepth, 0);
	if (!stepImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &stepTx);
	glBindTexture(GL_TEXTURE_2D, stepTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, stepImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(stepImage);

	unsigned char* parapitImage = stbi_load("ParapitTexture2.png", &width, &height, &bitDepth, 0);
	if (!parapitImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &parapitTx);
	glBindTexture(GL_TEXTURE_2D, parapitTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, parapitImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(parapitImage);

	unsigned char* alterImage = stbi_load("MarbleTexture.jpg", &width, &height, &bitDepth, 0);
	if (!alterImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &alterTx);
	glBindTexture(GL_TEXTURE_2D, alterTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, alterImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(alterImage);

	unsigned char* crystalImage = stbi_load("CrystalTx.jpg", &width, &height, &bitDepth, 0);
	if (!crystalImage) cout << "Unable to load file!" << endl;

	glGenTextures(1, &crystalTx);
	glBindTexture(GL_TEXTURE_2D, crystalTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, crystalImage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(crystalImage);

	glUniform1i(glGetUniformLocation(program, "texture0"), 0);

	// Setting ambient Light.
	glUniform3f(glGetUniformLocation(program, "aLight.ambientColour"), aLight.ambientColour.x, aLight.ambientColour.y, aLight.ambientColour.z);
	glUniform1f(glGetUniformLocation(program, "aLight.ambientStrength"), aLight.ambientStrength);

	// Setting directional light.
	glUniform3f(glGetUniformLocation(program, "dLight.base.diffuseColour"), dLight.diffuseColour.x, dLight.diffuseColour.y, dLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "dLight.base.diffuseStrength"), dLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "dLight.direction"), dLight.direction.x, dLight.direction.y, dLight.direction.z);

	// Setting point lights.
	glUniform3f(glGetUniformLocation(program, "pLights[0].base.diffuseColour"), pLights[0].diffuseColour.x, pLights[0].diffuseColour.y, pLights[0].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[0].base.diffuseStrength"), pLights[0].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[0].position"), pLights[0].position.x, pLights[0].position.y, pLights[0].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[0].constant"), pLights[0].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[0].linear"), pLights[0].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[0].exponent"), pLights[0].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[1].base.diffuseColour"), pLights[1].diffuseColour.x, pLights[1].diffuseColour.y, pLights[1].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[1].base.diffuseStrength"), pLights[1].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[1].position"), pLights[1].position.x, pLights[1].position.y, pLights[1].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[1].constant"), pLights[1].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[1].linear"), pLights[1].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[1].exponent"), pLights[1].exponent);

	// Setting spot light.
	glUniform3f(glGetUniformLocation(program, "sLight.base.diffuseColour"), sLight.diffuseColour.x, sLight.diffuseColour.y, sLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "sLight.base.diffuseStrength"), sLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "sLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);

	glUniform3f(glGetUniformLocation(program, "sLight.direction"), sLight.direction.x, sLight.direction.y, sLight.direction.z);
	glUniform1f(glGetUniformLocation(program, "sLight.edge"), sLight.edgeRad);

	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

		ibo = 0;
		glGenBuffers(1, &ibo);
	
		points_vbo = 0;
		glGenBuffers(1, &points_vbo);

		colors_vbo = 0;
		glGenBuffers(1, &colors_vbo);

		uv_vbo = 0;
		glGenBuffers(1, &uv_vbo);

		normals_vbo = 0;
		glGenBuffers(1, &normals_vbo);

	glBindVertexArray(0); // Can optionally unbind the vertex array to avoid modification.

	// Change shape data.
	g_mainTurretPrism.SetMat(0.1, 16);
	g_upperTurretPrism.SetMat(0.1, 16);
	g_turretArchPrism.SetMat(0.1, 16);
	g_mainWall.SetMat(0.1, 16);
	g_ground.SetMat(0.0, 16);
	g_turretTop.SetMat(0.1, 16);
	g_parapit.SetMat(0.1, 16);
	g_crystal.SetMat(0.4, 16);

	// Enable depth test and blend.
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	// Enable face culling.
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	timer(0); 
}

//---------------------------------------------------------------------
//
// calculateView
//
void calculateView()
{
	frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec.y = sin(glm::radians(pitch));
	frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec = glm::normalize(frontVec);
	rightVec = glm::normalize(glm::cross(frontVec, worldUp));
	upVec = glm::normalize(glm::cross(rightVec, frontVec));

	View = glm::lookAt(
		position, // Camera position
		position + frontVec, // Look target
		upVec); // Up vector
	glUniform3f(glGetUniformLocation(program, "eyePosition"), position.x, position.y, position.z);
}

//---------------------------------------------------------------------
//
// transformModel
//
void transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation) {
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, scale);
	
	calculateView();
	glUniformMatrix4fv(modelID, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(viewID, 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(projID, 1, GL_FALSE, &Projection[0][0]);
}
float calcDistance(glm::vec3 pos)
{
	float distance = 0.0f;

	float x1 = pos.x;
	float y1 = pos.y;
	float z1 = pos.z;

	float x2 = position.x;
	float y2 = position.y;
	float z2 = position.z;

	float x = x2 - x1;
	float y = y2 - y1;
	float z = z2 - z1;

	distance = sqrt(((x * x) + (y * y) + z * z));

	return distance;
}

void bobCrystalY()
{
	if (bobCrystal <= 0)
	{
		speed = 0.005;
	}
	if (bobCrystal >= 0.3)
	{
		speed = -0.005;
	}
	bobCrystal += speed;
}
//---------------------------------------------------------------------
//
// display
//


void drawHedge(glm::vec3 pos, float rotation)
{
	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	g_mainHedge.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 0.5f), Y_AXIS, rotation, glm::vec3(pos.x + 0.0f, pos.y + 0.0f, pos.z -0.0f));
	glDrawElements(GL_TRIANGLES, g_mainHedge.NumIndices(), GL_UNSIGNED_SHORT, 0);
	transformObject(glm::vec3(1.0f, 1.0f, 0.5f), Y_AXIS, rotation, glm::vec3(pos.x + 0.0f, pos.y + 1.0f, pos.z - 0.0f));
	glDrawElements(GL_TRIANGLES, g_mainHedge.NumIndices(), GL_UNSIGNED_SHORT, 0);
	transformObject(glm::vec3(1.0f, 1.0f, 0.5f), Y_AXIS, rotation, glm::vec3(pos.x + 0.0f, pos.y + 2.0f, pos.z - 0.0f));
	glDrawElements(GL_TRIANGLES, g_mainHedge.NumIndices(), GL_UNSIGNED_SHORT, 0);
}
void drawHedge(glm::vec3 pos, float rotation, float scale)
{
	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	g_mainHedge.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(scale, 1.0f, 0.5f), Y_AXIS, rotation, glm::vec3(pos.x + 0.0f, pos.y + 0.0f, pos.z - 0.0f));
	glDrawElements(GL_TRIANGLES, g_mainHedge.NumIndices(), GL_UNSIGNED_SHORT, 0);
	transformObject(glm::vec3(scale, 1.0f, 0.5f), Y_AXIS, rotation, glm::vec3(pos.x + 0.0f, pos.y + 1.0f, pos.z - 0.0f));
	glDrawElements(GL_TRIANGLES, g_mainHedge.NumIndices(), GL_UNSIGNED_SHORT, 0);
	transformObject(glm::vec3(scale, 1.0f, 0.5f), Y_AXIS, rotation, glm::vec3(pos.x + 0.0f, pos.y + 2.0f, pos.z - 0.0f));
	glDrawElements(GL_TRIANGLES, g_mainHedge.NumIndices(), GL_UNSIGNED_SHORT, 0);
}
void drawDeadEndRight( glm::vec3 pos) //1x3
{
	drawHedge(glm::vec3(pos.x + 0.0f, pos.y + 0.0f, pos.z + 0.0f), 0.0f, 0.5f);
	drawHedge(glm::vec3(pos.x + 0.0f, pos.y + 0.0f, pos.z + 1.5f), 0.0f, 0.5f);
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 0.0f),  0.0f);
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 1.5f),  0.0f);
	drawHedge(glm::vec3(pos.x + 1.5f, pos.y + 0.0f, pos.z + 1.0f),  90.0f);
	drawHedge(glm::vec3(pos.x + 1.5f, pos.y + 0.0f, pos.z+ 2.0f), 90.0f);
}
void drawDeadEndLeft(glm::vec3 pos) //1x3
{
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 0.0f), 0.0f);
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 1.5f), 0.0f);
	drawHedge(glm::vec3(pos.x + 1.5f, pos.y + 0.0f, pos.z + 0.0f), 0.0f, 0.5f);
	drawHedge(glm::vec3(pos.x + 1.5f, pos.y + 0.0f, pos.z + 1.5f), 0.0f, 0.5f);
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 0.0f), -90.0f);
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z +1.0f), -90.0f);

}
void drawDeadEndUp(glm::vec3 pos) //3x1
{
	drawHedge(glm::vec3(pos.x + 0.0f, pos.y + 0.0f, pos.z + 0.0f), 0.0f);
	drawHedge(glm::vec3(pos.x + 1.0f, pos.y + 0.0f, pos.z + 0.0f), 0.0f);
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 0.5f), -90.0f);
	drawHedge(glm::vec3(pos.x + 2.0f, pos.y + 0.0f, pos.z + 0.5f), -90.0f);
	
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 1.5f), -90.0f, 0.5);
	drawHedge(glm::vec3(pos.x + 2.0f, pos.y + 0.0f, pos.z + 1.5f), -90.0f, 0.5);
}
void drawDeadEndDown(glm::vec3 pos) // 3x1
{
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 0.0f), -90.0f, 0.5f);
	drawHedge(glm::vec3(pos.x + 2.0f, pos.y + 0.0f, pos.z + 0.0f), -90.0f, 0.5f);
	drawHedge(glm::vec3(pos.x + 0.5f, pos.y + 0.0f, pos.z + 0.5f), -90.0f);
	drawHedge(glm::vec3(pos.x + 2.0f, pos.y + 0.0f, pos.z + 0.5f), -90.0f);
	drawHedge(glm::vec3(pos.x + 0.0f, pos.y + 0.0f, pos.z + 1.5f), 0.0f);
	drawHedge(glm::vec3(pos.x + 1.0f, pos.y + 0.0f, pos.z + 1.5f), 0.0f);
	
}
void drawDeadEnd(glm::vec3 pos, Orientation orientation)
{
	if (orientation == RIGHT)
	{
		drawDeadEndRight(pos);
	}
	else if (orientation == LEFT)
	{
		drawDeadEndLeft(pos);
	}
	else if (orientation == UP)
	{
		drawDeadEndUp(pos);
	}
	else if (orientation == DOWN)
	{
		drawDeadEndDown(pos);
	}
	else
		{
			std::cout << "Wrong orientation entered!\n";
		}
}
void drawHedgeLine(int numHedges, Orientation orientation, glm::vec3 pos, float rotation)
{
	for (int i = 0; i < numHedges; i++)
	{
		//Horizontal
		if (orientation == HORIZONTAL)
		{
			drawHedge(glm::vec3(pos.x + i + 0.0f, pos.y + 0.0f, pos.z + 0.0f), rotation);
		}
		else if(orientation == VERTICAL)
		{
			drawHedge(glm::vec3(pos.x + 0.0f, pos.y + 0.0f, pos.z - i + 0.0f), 90.0f + rotation);
		}
		else
		{
			std::cout << "Wrong orientation entered!\n";
		}
	}
}


void drawTurret(glm::vec3 pos)
{
	glBindTexture(GL_TEXTURE_2D, turretTx);
	g_mainTurretPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 8.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(pos.x + 3.0f, pos.y + 0.0f, pos.z -2.0f));
	glDrawElements(GL_TRIANGLES, g_mainTurretPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, turretTrimTx);
	g_upperTurretPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 1.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(pos.x + 2.0f, pos.y + 8.0f, pos.z - 3.0f));
	glDrawElements(GL_TRIANGLES, g_upperTurretPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);
	transformObject(glm::vec3(8.0f, 1.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(pos.x + 2.0f, pos.y + 13.0f, pos.z - 3.0f));
	glDrawElements(GL_TRIANGLES, g_upperTurretPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, turretArchTx);
	g_turretArchPrism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 4.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(pos.x + 2.0f, pos.y + 9.0f, pos.z - 3.0f));
	glDrawElements(GL_TRIANGLES, g_turretArchPrism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, turretRoofTx);
	g_turretTop.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(9.0f, 5.0f, 9.0f), X_AXIS, 0.0f, glm::vec3(pos.x + 1.5f, pos.y + 14.0f, pos.z -3.5f));
	glDrawElements(GL_TRIANGLES, g_turretTop.NumIndices(), GL_UNSIGNED_SHORT, 0);
}
void drawParapit(glm::vec3 pos, Orientation orientation)
{
	glBindTexture(GL_TEXTURE_2D, parapitTx);
	g_parapit.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	if (orientation == VERTICAL)
	{
		transformObject(glm::vec3(0.5f, 1.0f, 1.0f), Y_AXIS, 0.0f, glm::vec3(pos.x, pos.y, pos.z));
	}
	else if (orientation == HORIZONTAL)
	{
		transformObject(glm::vec3(1.0f, 1.0f, 0.5f), Y_AXIS, 0.0f, glm::vec3(pos.x, pos.y, pos.z));
	}
	glDrawElements(GL_TRIANGLES, g_parapit.NumIndices(), GL_UNSIGNED_SHORT, 0);
}
void drawWallSegment(glm::vec3 pos, glm::vec3 scale, Orientation orientation)
{
	glBindTexture(GL_TEXTURE_2D, turretTx);
	g_mainWall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(scale.x*1.0f, scale.y*1.0f, scale.z* 1.0f), X_AXIS, 0.0f, glm::vec3(pos.x, pos.y, pos.z));
	glDrawElements(GL_TRIANGLES, g_mainWall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	if (orientation == VERTICAL)
	{
		drawParapit(glm::vec3(pos.x, pos.y + scale.y, pos.z - 1.0f + scale.z), VERTICAL);
		drawParapit(glm::vec3(pos.x, pos.y + scale.y, pos.z - 2.0f + scale.z), VERTICAL);
		drawParapit(glm::vec3(pos.x, pos.y + 1.0f + scale.y, pos.z - 2.0f + scale.z), VERTICAL);

		drawParapit(glm::vec3(pos.x + 1.5, pos.y + scale.y, pos.z - 1.0f + scale.z), VERTICAL);
		drawParapit(glm::vec3(pos.x + 1.5, pos.y + scale.y, pos.z - 2.0f + scale.z), VERTICAL);
		drawParapit(glm::vec3(pos.x + 1.5, pos.y + 1.0f + scale.y, pos.z - 2.0f + scale.z), VERTICAL);
	}
	else if (orientation == HORIZONTAL)
	{
		drawParapit(glm::vec3(pos.x -1.0f + scale.z, pos.y + scale.y, pos.z + 1.5f ), HORIZONTAL);
		drawParapit(glm::vec3(pos.x - 0.0f, pos.y + scale.y, pos.z + 1.5f ), HORIZONTAL);
		drawParapit(glm::vec3(pos.x-0.0f, pos.y + 1.0f + scale.y, pos.z + 1.5f ), HORIZONTAL);

		drawParapit(glm::vec3(pos.x - 1.0, pos.y + scale.y, pos.z - 2.0f + scale.z), HORIZONTAL);
		drawParapit(glm::vec3(pos.x -0.0f , pos.y + scale.y, pos.z - 2.0f + scale.z), HORIZONTAL);
		drawParapit(glm::vec3(pos.x -0.0f, pos.y + 1.0f + scale.y, pos.z - 2.0f + scale.z), HORIZONTAL);
	}
	else
	{
		std::cout << "Incorrect orientation in drawWallSegement() Function!\n";
	}
}



void drawWallSide(int numSegments, Orientation orientation, glm::vec3 pos, glm::vec3 scale)
{
	for (int i = 0; i < numSegments * 2; i += 2)
	{
		if (orientation == HORIZONTAL)
		{
			drawWallSegment(glm::vec3(pos.x + i, pos.y, pos.z), glm::vec3(scale.x + 2.0f, scale.y + 6.0f, scale.z + 2.0f), HORIZONTAL);
		}
		else if (orientation == VERTICAL)
		{
			drawWallSegment(glm::vec3(pos.x, pos.y, pos.z - i), glm::vec3(scale.x + 2.0f, scale.y + 6.0f, scale.z + 2.0f),VERTICAL);
		}
	}
}
void drawGateHouseDoor()
{
	glBindTexture(GL_TEXTURE_2D, doorTx);
	g_castleDoor.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 4.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(17.0f, 1.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_castleDoor.NumIndices(), GL_UNSIGNED_SHORT, 0);

	transformObject(glm::vec3(7.0f, 4.0f, 2.0f), Y_AXIS, 180.0f, glm::vec3(24.0f, 1.0f, -39.5f));
	glDrawElements(GL_TRIANGLES, g_castleDoor.NumIndices(), GL_UNSIGNED_SHORT, 0);
}
void drawSteps()
{
	for (int i = 0; i < 8; i++)
	{
		glBindTexture(GL_TEXTURE_2D, stepTx);
		g_steps.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 0.25f, 2.0f), X_AXIS, 0.0f, glm::vec3(17.0f + i, 0.75f, -39.0f));
		glDrawElements(GL_TRIANGLES, g_steps.NumIndices(), GL_UNSIGNED_SHORT, 0);
		transformObject(glm::vec3(1.0f, 0.25f, 6.0f), X_AXIS, 0.0f, glm::vec3(17.0f + i, 0.5f, -42.0f));
		glDrawElements(GL_TRIANGLES, g_steps.NumIndices(), GL_UNSIGNED_SHORT, 0);
		transformObject(glm::vec3(1.0f, 0.25f, 6.0f), X_AXIS, 0.0f, glm::vec3(17.0f + i, 0.25f, -41.0f));
		glDrawElements(GL_TRIANGLES, g_steps.NumIndices(), GL_UNSIGNED_SHORT, 0);
		transformObject(glm::vec3(1.0f, 0.25f, 6.0f), X_AXIS, 0.0f, glm::vec3(17.0f + i, 0.0f, -40.0f));
		glDrawElements(GL_TRIANGLES, g_steps.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 25.0f, glm::vec3(24.0f, 0.0f, -41.0f));
	glDrawElements(GL_TRIANGLES, g_steps.NumIndices(), GL_UNSIGNED_SHORT, 0);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 25.0f, glm::vec3(16.0f, 0.0f, -41.0f));
	glDrawElements(GL_TRIANGLES, g_steps.NumIndices(), GL_UNSIGNED_SHORT, 0);
}
void drawCourtYard()
{
	glBindTexture(GL_TEXTURE_2D, stepTx);
	g_courtyard.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 0.25f, 9.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -25.0f));
	glDrawElements(GL_TRIANGLES, g_courtyard.NumIndices(), GL_UNSIGNED_SHORT, 0);
}
void drawCastle(int segments, glm::vec3 pos)
{

	drawWallSide(segments, VERTICAL, glm::vec3(pos.x,pos.y,pos.z), glm::vec3(0.0f));
	drawWallSide(segments, VERTICAL, glm::vec3(pos.x+ segments*2, pos.y, pos.z), glm::vec3(0.0f));
	drawWallSide(segments, HORIZONTAL, glm::vec3(pos.x, pos.y, pos.z), glm::vec3(0.0f));


	drawTurret(glm::vec3(-5.0f,pos.y,pos.z));
	drawTurret(glm::vec3(-5.0f+segments * 2, pos.y, pos.z));
	drawTurret(glm::vec3(-5.0f, pos.y, pos.z -segments * 2));
	drawTurret(glm::vec3(-5.0f + segments * 2, pos.y, pos.z - segments * 2)); 

	drawTurret(glm::vec3( 2+ segments / 3, pos.y, pos.z - segments * 2));
	drawTurret(glm::vec3(-9.0f + segments * 2 - segments / 2, pos.y, pos.z - segments * 2));
	drawWallSide(segments / 3, HORIZONTAL, glm::vec3(pos.x, pos.y, pos.z - segments * 2), glm::vec3(0.0f));
	drawWallSide(segments / 3, HORIZONTAL, glm::vec3(pos.x + segments * 2 - segments / 2, pos.y, pos.z - segments * 2), glm::vec3(0.0f));

	drawWallSide(4, HORIZONTAL, glm::vec3(pos.x + segments - 4.0f, pos.y + 5, pos.z - segments * 2), glm::vec3(0.0f, -5.0f, 0.0f));

	drawGateHouseDoor();
	drawSteps();
	drawCourtYard();
}
void drawGround()
{
	glBindTexture(GL_TEXTURE_2D, gridTx);
	g_ground.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f), X_AXIS, -90.0f, glm::vec3(-5.0f, 0.0f, 5.0f));
	glDrawElements(GL_TRIANGLES, g_ground.NumIndices(), GL_UNSIGNED_SHORT, 0);
}
void drawCourtYardPerimeter()
{
	drawHedgeLine(9, VERTICAL, glm::vec3(18.0f, 0.0f, -17.0f), 180.0f);
	drawHedgeLine(11, HORIZONTAL, glm::vec3(13.5f, 0.0f, -16.0f), 0);
	drawHedgeLine(3, VERTICAL, glm::vec3(24.0f, 0.0f, -16.0f), 0.0f);
	drawHedgeLine(5, VERTICAL, glm::vec3(24.0f, 0.0f, -20.0f), 0.0f);
	drawHedgeLine(7, HORIZONTAL, glm::vec3(13.5f, 0.0f, -25.5f), 0);
	drawHedgeLine(3, HORIZONTAL, glm::vec3(21.5f, 0.0f, -25.5f), 0);

	drawHedgeLine(3, VERTICAL, glm::vec3(15.5f, 0.0f, -34.5f), 0);
	drawHedgeLine(3, VERTICAL, glm::vec3(25.0f, 0.0f, -34.5f), 0);

	drawHedgeLine(5, VERTICAL, glm::vec3(17.5f, 0.0f, 0.5f), 0);
	drawHedgeLine(5, VERTICAL, glm::vec3(26.0f, 0.0f, 0.5f), 0);
}

void drawMazePerimeter()
{
	drawHedgeLine(13, HORIZONTAL, glm::vec3(4.0f, 0.0f, -34.5f), 0);
	drawHedgeLine(30, VERTICAL, glm::vec3(4.0f, 0.0f, -4.0f), 0.0f);
	drawHedgeLine(13, HORIZONTAL, glm::vec3(25.0f, 0.0f, -34.5f), 0);
	drawHedgeLine(30, VERTICAL, glm::vec3(37.5f, 0.0f, -4.0f), 0.0f);
	drawHedgeLine(17, HORIZONTAL, glm::vec3(4.5f, 0.0f, -4.5f), 0);
	drawHedgeLine(15, HORIZONTAL, glm::vec3(22.5f, 0.0f, -4.5f), 0);
}
void drawPathOutCourtYardSouth()
{
	drawHedgeLine(3, HORIZONTAL, glm::vec3(24.5f, 0.0f, -19.0f), 0);
	drawHedgeLine(3, HORIZONTAL, glm::vec3(24.5f, 0.0f, -20.5f), 0);

	drawHedgeLine(4, VERTICAL, glm::vec3(27.5f, 0.0f, -20.0f), 0);
	drawHedgeLine(6, VERTICAL, glm::vec3(29.0f, 0.0f, -20.0f), 0);
	
	drawHedgeLine(3, VERTICAL, glm::vec3(27.5f, 0.0f, -16.0f), 0);
	drawHedgeLine(4, VERTICAL, glm::vec3(29.0f, 0.0f, -15.0f), 0);

	drawHedgeLine(5, HORIZONTAL, glm::vec3(24.0f, 0.0f, -25.5f), 0);
	drawHedgeLine(2, HORIZONTAL, glm::vec3(26.0f, 0.0f, -24.0f), 0);
	drawHedgeLine(2, VERTICAL, glm::vec3(26.0f, 0.0f, -21.5f), 0);

	drawHedgeLine(5, HORIZONTAL, glm::vec3(29.5f, 0.0f, -20.5f), 0);
	drawHedgeLine(5, HORIZONTAL, glm::vec3(29.0f, 0.0f, -19.0f), 0);
	drawHedgeLine(24, VERTICAL, glm::vec3(36.0f, 0.0f, -9.0f), 0.0f);
	drawHedgeLine(3, HORIZONTAL, glm::vec3(33.5f, 0.0f, -9.0f), 0.0f);
	drawHedgeLine(1, VERTICAL, glm::vec3(33.5f, 0.0f, -7.5f), 0.0f);
	drawHedgeLine(6, HORIZONTAL, glm::vec3(28.0f, 0.0f, -7.5f), 0.0f);
	drawHedgeLine(2, VERTICAL, glm::vec3(27.5f, 0.0f, -7.0f), 0.0f);
	drawHedgeLine(7, HORIZONTAL, glm::vec3(25.5f, 0.0f, -9.5f), 0.0f);
	drawHedgeLine(8, VERTICAL, glm::vec3(25.5f, 0.0f, -9.5f), 0.0f);
	drawHedgeLine(8, HORIZONTAL, glm::vec3(27.0f, 0.0f, -15.0f), 0.0f);
	drawHedgeLine(8, VERTICAL, glm::vec3(25.5f, 0.0f, -9.5f), 0.0f);
	drawHedgeLine(4, VERTICAL, glm::vec3(27.0f, 0.0f, -10.5f), 0.0f);
	drawHedgeLine(7, HORIZONTAL, glm::vec3(27.5f, 0.0f, -11.0f), 0.0f);
	drawHedgeLine(3, VERTICAL, glm::vec3(34.5f, 0.0f, -10.5f), 0.0f);
	drawHedgeLine(6, HORIZONTAL, glm::vec3(28.5f, 0.0f, -13.5f), 0.0f);
	drawHedgeLine(1, VERTICAL, glm::vec3(32.5f, 0.0f, -12.0f), 0.0f);
	drawHedgeLine(3, VERTICAL, glm::vec3(34.5f, 0.0f, -15.0f), 0.0f);
	drawHedgeLine(4, HORIZONTAL, glm::vec3(30.5f, 0.0f, -17.5f), 0.0f);
	drawHedgeLine(2, VERTICAL, glm::vec3(30.5f, 0.0f, -15.5f), 0.0f);

}
void drawPathOutCourtYardNorth()
{
	drawHedgeLine(9, VERTICAL, glm::vec3(34.5f, 0.0f, -20.0f), 0);
	drawHedgeLine(4, HORIZONTAL, glm::vec3(30.5f, 0.0f, -29.0f), 0);
	drawHedgeLine(7, VERTICAL, glm::vec3(30.5f, 0.0f, -21.5f), 0);
	drawHedgeLine(2, HORIZONTAL, glm::vec3(31.0f, 0.0f, -22.0f), 0);
	drawHedgeLine(5, VERTICAL, glm::vec3(32.5f, 0.0f, -22.0f), 0);

	drawHedgeLine(8, HORIZONTAL, glm::vec3(28.0f, 0.0f, -30.5f), 0);
	drawHedgeLine(4, VERTICAL, glm::vec3(28.0f, 0.0f, -26.5f), 0);
	drawHedgeLine(22, HORIZONTAL, glm::vec3(6.0f, 0.0f, -27.0f), 0);
	drawHedgeLine(6, VERTICAL, glm::vec3(5.5f, 0.0f, -21.0f), 0);
	drawHedgeLine(2, HORIZONTAL, glm::vec3(5.5f, 0.0f, -21.0f), 0);
	drawHedgeLine(7, VERTICAL, glm::vec3(8.5f, 0.0f, -19.5f), 0);
	drawHedgeLine(5, HORIZONTAL, glm::vec3(4.0f, 0.0f, -19.5f), 0);
	drawHedgeLine(1, VERTICAL, glm::vec3(21.5f, 0.0f, -25.5f), 0);
	drawHedgeLine(4, VERTICAL, glm::vec3(7.0f, 0.0f, -21.0f), 0);

	drawHedgeLine(2, VERTICAL, glm::vec3(34.0f, 0.0f, -32.0f), 0);
	drawHedgeLine(2, VERTICAL, glm::vec3(32.0f, 0.0f, -30.5f), 0);
	drawHedgeLine(2, VERTICAL, glm::vec3(30.0f, 0.0f, -32.0f), 0);
	drawHedgeLine(2, VERTICAL, glm::vec3(28.0f, 0.0f, -30.5f), 0);
	drawHedgeLine(2, HORIZONTAL, glm::vec3(26.5f, 0.0f, -33.0f), 0);
	drawHedgeLine(4, VERTICAL, glm::vec3(26.5f, 0.0f, -28.5f), 0);
	drawHedgeLine(6, VERTICAL, glm::vec3(25.0f, 0.0f, -28.5f), 0);
	drawHedgeLine(20, HORIZONTAL, glm::vec3(5.5f, 0.0f, -28.5f), 0);
	drawHedgeLine(4, VERTICAL, glm::vec3(5.5f, 0.0f, -28.5f), 0);
	drawHedgeLine(6, HORIZONTAL, glm::vec3(5.5f, 0.0f, -33.0f), 0);
	drawHedgeLine(4, VERTICAL, glm::vec3(12.5f, 0.0f, -28.5f), 0);
	drawHedgeLine(3, VERTICAL, glm::vec3(11.0f, 0.0f, -29.5f), 0);
	drawHedgeLine(4, HORIZONTAL, glm::vec3(7.0f, 0.0f, -30.0f), 0);
	drawHedgeLine(1, VERTICAL, glm::vec3(7.0f, 0.0f, -30.0f), 0);
	drawHedgeLine(3, HORIZONTAL, glm::vec3(7.0f, 0.0f, -31.5f), 0);

	drawHedgeLine(2, HORIZONTAL, glm::vec3(13.0f, 0.0f, -32.5f), 0);
	drawHedgeLine(3, VERTICAL, glm::vec3(15.0f, 0.0f, -29.5f), 0);
	drawHedgeLine(4, VERTICAL, glm::vec3(16.5f, 0.0f, -30.0f), 0);
	drawHedgeLine(1, HORIZONTAL, glm::vec3(14.0f, 0.0f, -30.0f), 0);
	drawHedgeLine(1, HORIZONTAL, glm::vec3(16.5f, 0.0f, -30.0f), 0);
}
void drawFrontPathYard()
{
	drawHedgeLine(2, VERTICAL, glm::vec3(22.5f, 0.0f, -4.5f), 0);
	drawHedgeLine(2, HORIZONTAL, glm::vec3(23.0f, 0.0f, -6.5f), 0);
	drawHedgeLine(2, VERTICAL, glm::vec3(26.0f, 0.0f, -6.0f), 0);
	drawHedgeLine(9, HORIZONTAL, glm::vec3(26.0f, 0.0f, -6.0f), 0);
	drawDeadEnd(glm::vec3(35.0f, 0.0f, -7.5f), UP);
	drawHedge(glm::vec3(37.0f, 0.0f, -5.5f), 90.0f);

	drawHedgeLine(8, VERTICAL, glm::vec3(24.0f, 0.0f, -7.5f), 0);
	drawHedgeLine(2, HORIZONTAL, glm::vec3(24.5f, 0.0f, -8.0f), 0);

	drawHedgeLine(10, HORIZONTAL, glm::vec3(11.5f, 0.0f, -6.0f), 0);
	drawHedgeLine(10, VERTICAL, glm::vec3(11.0f, 0.0f, -5.5f), 0);
	drawHedgeLine(6, VERTICAL, glm::vec3(9.5f, 0.0f, -8.0f), 0);
	drawHedgeLine(2, HORIZONTAL, glm::vec3(7.5f, 0.0f, -14.0f), 0);
	drawHedgeLine(5, VERTICAL, glm::vec3(7.0f, 0.0f, -9.0f), 0);
	drawHedgeLine(1, HORIZONTAL, glm::vec3(7.5f, 0.0f, -9.5f), 0);

	drawHedgeLine(6, VERTICAL, glm::vec3(21.0f, 0.0f, -7.5f), 0);

drawHedgeLine(9, HORIZONTAL, glm::vec3(12.5f, 0.0f, -7.5f), 0);
drawHedgeLine(9, HORIZONTAL, glm::vec3(11.0f, 0.0f, -9.0f), 0);
drawHedgeLine(9, HORIZONTAL, glm::vec3(12.5f, 0.0f, -10.5f), 0);
drawHedgeLine(10, HORIZONTAL, glm::vec3(11.0f, 0.0f, -12.0f), 0);

drawHedgeLine(9, HORIZONTAL, glm::vec3(12.5f, 0.0f, -14.0f), 0);

drawHedgeLine(8, VERTICAL, glm::vec3(22.5f, 0.0f, -6.5f), 0);

}
void leftDrawMaze()
{
	drawHedgeLine(8, VERTICAL, glm::vec3(13.5f, 0.0f, -17.0f), 0);

	drawHedgeLine(2, HORIZONTAL, glm::vec3(14.0f, 0.0f, -17.5f), 0);

	drawHedgeLine(2, HORIZONTAL, glm::vec3(15.5f, 0.0f, -19.0f), 0);

	drawHedgeLine(2, HORIZONTAL, glm::vec3(14.0f, 0.0f, -20.5f), 0);

	drawHedgeLine(2, HORIZONTAL, glm::vec3(15.5f, 0.0f, -22.0f), 0);

	drawHedgeLine(2, HORIZONTAL, glm::vec3(14.0f, 0.0f, -23.5f), 0);

	drawHedgeLine(10, VERTICAL, glm::vec3(12.0f, 0.0f, -17.0f), 0);
	drawHedgeLine(8, VERTICAL, glm::vec3(10.5f, 0.0f, -17.5f), 0);

	drawHedgeLine(7, HORIZONTAL, glm::vec3(5.5f, 0.0f, -17.5f), 0);

	drawHedgeLine(7, HORIZONTAL, glm::vec3(5.5f, 0.0f, -16.0f), 0);
	drawHedgeLine(8, VERTICAL, glm::vec3(5.5f, 0.0f, -7.5f), 0);
	drawHedgeLine(4, HORIZONTAL, glm::vec3(6.0f, 0.0f, -8.0f), 0);
	drawHedgeLine(4, HORIZONTAL, glm::vec3(4.5f, 0.0f, -6.5f), 0);
	drawHedgeLine(3, VERTICAL, glm::vec3(9.5f, 0.0f, -4.5f), 0);
}
void drawMaze()
{
	drawGround();
	drawCourtYardPerimeter();
	drawMazePerimeter();
	drawPathOutCourtYardSouth();
	drawFrontPathYard();
	drawPathOutCourtYardNorth();
	leftDrawMaze();
}
void drawAlter()
{
	glBindTexture(GL_TEXTURE_2D, alterTx);
	g_alter.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 0.3f, 1.5f), Y_AXIS, 0.0f, glm::vec3(19.5f, 0.2f, -18.0f));
	glDrawElements(GL_TRIANGLES, g_alter.NumIndices(), GL_UNSIGNED_SHORT, 0);
	transformObject(glm::vec3(1.0f, 0.7f, 0.5f), Y_AXIS, 0.0f, glm::vec3(20.5f, 0.5f, -17.6f));
	glDrawElements(GL_TRIANGLES, g_alter.NumIndices(), GL_UNSIGNED_SHORT, 0);
	transformObject(glm::vec3(4.0f, 0.2f, 2.5f), Y_AXIS, 0.0f, glm::vec3(19.0f, 1.1f, -18.8f));
	glDrawElements(GL_TRIANGLES, g_alter.NumIndices(), GL_UNSIGNED_SHORT, 0);
}

void drawCrystal()
{
	bobCrystalY();
	if (!crystalPickedUp)
	{
		glBindTexture(GL_TEXTURE_2D, crystalTx);
		g_crystal.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.4f, 0.5f, 0.4f), X_AXIS, 180, glm::vec3(8.3f, 1.2f + bobCrystal, -11.5f));
		glDrawElements(GL_TRIANGLES, g_crystal.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, crystalTx);
		g_crystal.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.4f, 0.5f, 0.4f), X_AXIS, 0, glm::vec3(8.3f, 1.2f + bobCrystal, -11.9f));
		glDrawElements(GL_TRIANGLES, g_crystal.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	if (crystalOnAlter)
	{
		glBindTexture(GL_TEXTURE_2D, crystalTx);
		g_crystal.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.4f, 0.5f, 0.4f), X_AXIS, 180, glm::vec3(20.7f, 2.0f + bobCrystal, -17.5f));
		glDrawElements(GL_TRIANGLES, g_crystal.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, crystalTx);
		g_crystal.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.4f, 0.5f, 0.4f), X_AXIS, 0, glm::vec3(20.7f, 2.0f + bobCrystal, -17.9f));
		glDrawElements(GL_TRIANGLES, g_crystal.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
}

void checkDistanceToMilestones()
{
	if (calcDistance(glm::vec3(8.3f, 1.2f, -11.5f)) <= 1.0f)
	{
		if (!crystalPickedUp)
		{
			std::cout << "Bring the crystal to the alter in the center of the maze\n";
		}
		crystalPickedUp = true;
	}
	if (crystalPickedUp)
	{
		if (calcDistance(glm::vec3(20.7f, 2.0f, -17.5f)) <= 2.0f)
		{
			if (!crystalOnAlter)
			{
				std::cout << "The exit is now unlocked!\n";
				std::cout << "You may leave the maze if you find the correct path out!\n";
			}
			crystalOnAlter = true;
		}
	}

	if (calcDistance(glm::vec3(19.0f, 1.0f, -39.0f)) <= 3.0f)
	{
		if (crystalOnAlter && !gameCompleted)
		{
			std::cout << "You Have Completed the Maze!\n";
			std::cout << "You May Exit!\n";
			gameCompleted = true;
		}
		else if (!crystalOnAlter && !gameCompleted)
		{
			if (counter >= 120)
			{
				std::cout << "Cannot Exit Maze\n";
				counter = 0;
			}
		}
	}
	counter++;
}
void implementProgressionSystem()
{
	drawAlter();
	drawCrystal();
	checkDistanceToMilestones();
}
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vao);
	// Draw all shapes.

	glUniform3f(glGetUniformLocation(program, "sLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);

	drawCastle(20, glm::vec3(0.0f,0.0f,0.0f));
	drawMaze();
	implementProgressionSystem();

	glBindVertexArray(0); // Done writing.
	glutSwapBuffers(); // Now for a potentially smoother render.


}

void parseKeys()
{
	if (keys & KEY_FORWARD)
		position += frontVec * MOVESPEED;
	else if (keys & KEY_BACKWARD)
		position -= frontVec * MOVESPEED;
	if (keys & KEY_LEFT)
		position -= rightVec * MOVESPEED;
	else if (keys & KEY_RIGHT)
		position += rightVec * MOVESPEED;
	if (keys & KEY_UP)
		position.y += MOVESPEED;
	else if (keys & KEY_DOWN)
		position.y -= MOVESPEED;
}

void timer(int) { // essentially our update()
	parseKeys();
	glutPostRedisplay();
	glutTimerFunc(1000/FPS, timer, 0); // 60 FPS or 16.67ms.
}

//---------------------------------------------------------------------
//
// keyDown
//
void keyDown(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD; break;
	case 's':
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD; break;
	case 'a':
		if (!(keys & KEY_LEFT))
			keys |= KEY_LEFT; break;
	case 'd':
		if (!(keys & KEY_RIGHT))
			keys |= KEY_RIGHT; break;
	case 'r':
		if (!(keys & KEY_UP))
			keys |= KEY_UP; break;
	case 'f':
		if (!(keys & KEY_DOWN))
			keys |= KEY_DOWN; break;
	case 'i':
		sLight.position.z -= 0.1; break;
	case 'j':
		sLight.position.x -= 0.1; break;
	case 'k':
		sLight.position.z += 0.1; break;
	case 'l':
		sLight.position.x += 0.1; break;
	case 'p':
		sLight.position.y += 0.1; break;
	case ';':
		sLight.position.y -= 0.1; break;
	}
}

void keyDownSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD;
	}
}

void keyUp(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		keys &= ~KEY_FORWARD; break;
	case 's':
		keys &= ~KEY_BACKWARD; break;
	case 'a':
		keys &= ~KEY_LEFT; break;
	case 'd':
		keys &= ~KEY_RIGHT; break;
	case 'r':
		keys &= ~KEY_UP; break;
	case 'f':
		keys &= ~KEY_DOWN; break;
	case ' ':
		resetView();
	}
}

void keyUpSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		keys &= ~KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		keys &= ~KEY_BACKWARD;
	}
}

void mouseMove(int x, int y)
{
	if (keys & KEY_MOUSECLICKED)
	{
		pitch += (GLfloat)((y - lastY) * TURNSPEED);
		yaw -= (GLfloat)((x - lastX) * TURNSPEED);
		lastY = y;
		lastX = x;
	}
}

void mouseClick(int btn, int state, int x, int y)
{
	if (state == 0)
	{
		lastX = x;
		lastY = y;
		keys |= KEY_MOUSECLICKED; // Flip flag to true
		glutSetCursor(GLUT_CURSOR_NONE);
		//cout << "Mouse clicked." << endl;
	}
	else
	{
		keys &= ~KEY_MOUSECLICKED; // Reset flag to false
		glutSetCursor(GLUT_CURSOR_INHERIT);
		//cout << "Mouse released." << endl;
	}
}

void clean()
{
	cout << "Cleaning up!" << endl;
	glDeleteTextures(1, &gridTx);
	glDeleteTextures(1, &hedgeTx);
}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("GAME2012 - Week 7");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyDown);
	glutSpecialFunc(keyDownSpec);
	glutKeyboardUpFunc(keyUp); // New function for third example.
	glutSpecialUpFunc(keyUpSpec);

	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove); // Requires click to register.
	
	atexit(clean); // This GLUT function calls specified function before terminating program. Useful!

	glutMainLoop();

	return 0;
}
