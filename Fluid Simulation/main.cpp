#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "Fluid.h"

Fluid* fluid;

//const unsigned int SCR_WIDTH = 800;
//const unsigned int SCR_HEIGHT = 800;
//const unsigned int grid_size = 80;

const unsigned int SCR_WIDTH = 1080;
const unsigned int SCR_HEIGHT = 1080;
const unsigned int grid_size = 216;
//const unsigned int grid_size = 512;

void* SSBOptrData;

float lastX = SCR_WIDTH / 2;
float lastY = SCR_HEIGHT / 2;
bool firstMouse = true;

float timeStart = 0.0f, timeEnd = 0.0f;
float deltaTime = 0.0f;

std::string read_shader(const std::string& filename);
unsigned int compile_shader(unsigned int type, const std::string& shader_path);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void process_input(GLFWwindow* window);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GLFW Window", nullptr, nullptr);
	if (!window) {
		std::cout << "Failed to create GLFW window!\n";
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialise GLAD!\n";
		glfwTerminate();
		return 1;
	}

	float positions[] = {
		-1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f
	};

	unsigned int indices[] = {
		0, 1, 2,
		2, 3, 1
	};

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), positions, GL_STATIC_DRAW);

	unsigned int EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	int max_gpu_storage;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_gpu_storage);
	if (max_gpu_storage < grid_size * grid_size * sizeof(glm::vec4)) {
		std::cout << "GPU cannot store fluid data!\n";
		return 1;
	}

	fluid = new Fluid(grid_size, 0.00001f, 0.001f);

	unsigned int SSBO;
	glGenBuffers(1, &SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, fluid->densityPixel.size() * sizeof(glm::vec4), glm::value_ptr(fluid->densityPixel[0]), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);
	SSBOptrData = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	unsigned int vertexShader = compile_shader(GL_VERTEX_SHADER, "quadVertex.glsl");
	unsigned int fragmentShader = compile_shader(GL_FRAGMENT_SHADER, "fluidFragment.glsl");

	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glUseProgram(shaderProgram);

	int resolution = glGetUniformLocation(shaderProgram, "u_Resolution");
	glUniform2f(resolution, (float)SCR_WIDTH, (float)SCR_HEIGHT);

	int gridDim = glGetUniformLocation(shaderProgram, "u_GridDim");
	glUniform2f(gridDim, (float)grid_size, (float)grid_size);

	while (!glfwWindowShouldClose(window)) {
		process_input(window);

		glClear(GL_COLOR_BUFFER_BIT);

		fluid->Update(deltaTime);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		SSBOptrData = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

		if (SSBOptrData)
			fluid->Draw(SSBOptrData);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();

		timeEnd = (float)glfwGetTime();
		deltaTime = timeEnd - timeStart;
		timeStart = timeEnd;
	}

	delete fluid;

	glDeleteProgram(shaderProgram);
	glDeleteBuffers(1, &SSBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);

	glfwTerminate();
	return 0;
}

// Callback and Input Functions

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	std::cout << width << "x" << height << "\n";
}

void mouse_position_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float offsetX = xpos - lastX;
	float offsetY = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		glm::vec2 tile = glm::vec2(SCR_WIDTH / grid_size, SCR_HEIGHT / grid_size);
		glm::vec2 tileCoord = glm::vec2(int(xpos / tile.x), int(ypos / tile.y));
		fluid->AddDensity(tileCoord.x, grid_size - tileCoord.y, 1000 * 3);
		fluid->AddVelocity(tileCoord.x, grid_size - tileCoord.y, glm::vec2(offsetX, offsetY) * 100.f);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		fluid->SetGrayscaleSpace();

	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		fluid->SetHSVSpace();
}

void process_input(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		fluid->Clean();
}

// Utility Functions

std::string read_shader(const std::string& filename) {
	std::ifstream fileReader(filename);
	if (!fileReader.is_open()) {
		std::cout << "Failed to read file: " << filename << "\n";
		return std::string{};
	}

	std::stringstream ss; ss << fileReader.rdbuf();
	return ss.str().c_str();
}

unsigned int compile_shader(unsigned int type, const std::string& shader_path) {
	std::string shaderSource = read_shader(shader_path);
	const char* source = shaderSource.c_str();

	unsigned int shaderId = glCreateShader(type);
	glShaderSource(shaderId, 1, &source, nullptr);
	glCompileShader(shaderId);

	int result;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
	if (!result) {
		int length;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)_malloca(length * sizeof(char));
		glGetShaderInfoLog(shaderId, length, &length, message);
		std::cout << "Failed to compile shader!\n" << message << "\n";
		glDeleteShader(shaderId);
		return 0;
	}

	return shaderId;
}
