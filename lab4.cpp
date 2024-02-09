#pragma warning(disable:4996)

#include <cmath>
#include <iostream>
#include <GL\glew.h>
#include <SFML/Window.hpp>
#include <SFML/System/Time.hpp>



#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

in vec3 position;
in vec3 color;
in vec2 aTexCoord;
in vec3 aNormal;

out vec2 TexCoord;
out vec3 Color;
out vec3 Normal;
out vec3 FragPos;


void main(){
	gl_Position = proj * view * model * vec4(position, 1.0);
	Color = vec3(0.102,0.573, 0.929);
	TexCoord = aTexCoord;
	Normal = aNormal;
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core

out vec4 outColor;

in vec3 Color;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
uniform vec3 lightPos;

	
uniform sampler2D texture1;
uniform float ambientStrength;

//float ambientStrength = 0.1;

void main()
{

vec3 ambientlightColor = vec3(1.0,1.0,1.0);
vec4 ambient = ambientStrength * vec4(ambientlightColor,1.0);
vec3 difflightColor = vec3(1.0,1.0,1.0);
vec3 norm = normalize(Normal);
vec3 lightDir = normalize(lightPos - FragPos);
float diff = max(dot(norm, lightDir), 0.0);
vec3 diffuse = diff * difflightColor;

//outColor = vec4(Color, 1.0);
//outColor = texture(texture1, TexCoord);
outColor = (ambient+vec4(diffuse,1.0)) * texture(texture1, TexCoord);

}
)glsl";

glm::mat4 model;
glm::mat4 proj;
glm::mat4 view;

double rotation = 0;
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 8.0f);
glm::vec3 cameraPositionUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraPositionFront = glm::vec3(0.0f, 0.0f, -1.0f);
//glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 lightPos(1.2f, 2.0f, 2.0f);
float ambientStrength = 1;

bool fMouse = true;
int last_x, last_y;
double yaw = -90;
double pitch = 0;

//loading picture from obj file
std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

bool loadOBJ(
	const char* path,
	std::vector < glm::vec3 >& out_vertices,
	std::vector < glm::vec2 >& out_uvs,
	std::vector < glm::vec3 >& out_normals
) {

	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

	FILE* file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}
	while (1) {
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.
		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			std::cout << "SCANNED VERTEX: " << vertex.x << " , " << vertex.y << " , " << vertex.z << std::endl;
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);

		}

	}
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		out_vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < uvIndices.size(); i++) {
		unsigned int uvIndex = uvIndices[i];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		out_uvs.push_back(uv);
	}

	for (unsigned int i = 0; i < normalIndices.size(); i++) {
		unsigned int normalIndex = normalIndices[i];
		glm::vec3 normal = temp_vertices[normalIndex - 1];
		out_normals.push_back(normal);
	}
}

void ustawKamereKlawisze(GLint view, float time) {
	float cameraSpeed = 0.000001f * time;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
		//std::cout << "Up" << std::endl;
		cameraPosition += cameraSpeed * cameraPositionFront;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
		//std::cout << "Down" << std::endl;
		cameraPosition -= cameraSpeed * cameraPositionFront;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
		//std::cout << "Left" << std::endl;
		//	rotation -= cameraSpeed;
		//	cameraPositionFront.x = sin(rotation);
		//	cameraPositionFront.z = -cos(rotation);
		cameraPosition -= glm::normalize(glm::cross(cameraPositionFront, cameraPositionUp)) * cameraSpeed;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
		//  std::cout << "Right" << std::endl;
		//	rotation += cameraSpeed;
		//	cameraPositionFront.x = sin(rotation);
		//	cameraPositionFront.z = -cos(rotation);
		cameraPosition += glm::normalize(glm::cross(cameraPositionFront, cameraPositionUp)) * cameraSpeed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tab)) {
		cameraPosition += cameraPositionUp * cameraSpeed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
		cameraPosition -= cameraPositionUp * cameraSpeed;
	}
	glm::mat4 thisView;
	//add position to camera front to always look at front of cube
	thisView = glm::lookAt(cameraPosition, cameraPosition + cameraPositionFront, cameraPositionUp);
	// sending to shader
	glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(thisView));
}

void ustawKamereMysz(GLint uniformView, float time, const sf::Window& window) {
	sf::Vector2i localPos = sf::Mouse::getPosition(window);

	bool reloc = false;
	sf::Vector2i position;
	if (localPos.x <= 0) {
		position.x = window.getSize().x;
		position.y = localPos.y;
		reloc = true;
	}
	if (localPos.x >= window.getSize().x - 1) {
		position.x = 0;
		position.y = localPos.y;
		reloc = true;
	}
	if (localPos.y <= 0) {
		position.y = window.getSize().y;
		position.x = localPos.x;
		reloc = true;
	}
	if (localPos.y >= window.getSize().y - 1) {
		position.y = 0;
		position.x = localPos.x;
		reloc = true;
	}
	if (reloc) {
		sf::Mouse::setPosition(position, window);
		fMouse = true;
	}
	localPos = sf::Mouse::getPosition(window);

	if (fMouse) {
		last_x = localPos.x;
		last_y = localPos.y;
		fMouse = false;
	}

	float offset_x = localPos.x - last_x;
	float offset_y = localPos.y - last_y;
	last_x = localPos.x;
	last_y = localPos.y;

	double sensit = 0.2f;
	double cameraSpeed = 0.00002f * time;
	offset_x *= sensit;
	offset_y *= sensit;

	yaw += offset_x * cameraSpeed;
	pitch -= offset_y * cameraSpeed;
	if (yaw > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch - -89.0f;

	glm::vec3 front;

	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	cameraPositionFront = glm::normalize(front);

	glm::mat4 view;
	view = glm::lookAt(cameraPosition, cameraPosition + cameraPositionFront, cameraPositionUp);
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
}

int main()
{
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;

	// Okno renderingu
	sf::Window window(sf::VideoMode(1200, 1000, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

	window.setMouseCursorVisible(false);
	window.setMouseCursorGrabbed(true);

	// Inicjalizacja GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Utworzenie VAO (Vertex Array Object)
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Utworzenie VBO (Vertex Buffer Object)
	// i skopiowanie do niego danych wierzchołkowych

	float cameraSpeed = 0.00002f;
	float obrot = 0.005f;

	bool res = loadOBJ("untitled.obj", vertices, uvs, normals);



	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	std::cout << vertices.size() << std::endl;

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	GLint isCompiled = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);
		std::cout << "Compilation vertexShader ERROR\n";
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &errorLog[0]);
	}
	else {
		std::cout << "Compilation vertexShader OK\n";
	}

	// Utworzenie i skompilowanie shadera fragmentów
	GLuint fragmentShader =
		glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);
		std::cout << "Compilation fragmentShader ERROR\n";

		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &errorLog[0]);
	}
	else {
		std::cout << "Compilation fragment Shader OK\n";
	}

	// Zlinkowanie obu shaderów w jeden wspólny program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// Specifikacja formatu danych wierzchołkowych
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), 0);

	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	GLint TexCoord = glGetAttribLocation(shaderProgram, "aTexCoord");
	glEnableVertexAttribArray(TexCoord);
	glVertexAttribPointer(TexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

	GLint NorAttrib = glGetAttribLocation(shaderProgram, "aNormal");
	glEnableVertexAttribArray(NorAttrib);
	glVertexAttribPointer(NorAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	GLint uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
	glUniform3fv(uniLightPos, 1, &lightPos[0]);

	unsigned int texture1;

	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);

	unsigned char* data = stbi_load("texture.jpg", &width, &height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	// Rozpoczęcie pętli zdarzeń
	bool running = true;
	sf::Mouse myszka;
	GLenum prymityw = GL_TRIANGLES;

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));

	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);


	GLint uniView = glGetUniformLocation(shaderProgram, "view");

	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	proj = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.06f, 100.0f);
	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	window.setMouseCursorGrabbed(true); //przechwycenie kursora myszy w oknie ------------
	window.setMouseCursorVisible(false); //ukrycie kursora myszy

	window.setFramerateLimit(20);
	sf::Time time;
	sf::Clock clock;
	int counter = 0;
	bool freezeMouse = false;

	//stbi_image_free(data);

	//glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

	while (running) {

		time = clock.getElapsedTime();
		time = clock.restart();
		counter++;
		float fps = 1000000 / time.asMicroseconds();
		if (counter > fps) {
			window.setTitle(std::to_string(fps));
			counter = 0;
		}

		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			switch (windowEvent.type) {
			case sf::Event::Closed:
				running = false;
				break;
			}
		}

		sf::Event::KeyPressed;
		switch (windowEvent.key.code)
		{
		case sf::Keyboard::Escape:
			running = false;
			break;

		case sf::Keyboard::Num1:
			ambientStrength += 0.1f;
			if (ambientStrength >= 1.0f) {
				ambientStrength = 1.0f;
			}
			std::cout << ambientStrength << " ";
			glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), ambientStrength);
			break;

		case sf::Keyboard::Num2:
			ambientStrength -= 0.1f;
			if (ambientStrength <= 0.0f) {
				ambientStrength = 0.0f;
			}
			std::cout << ambientStrength << " ";
			glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), ambientStrength);

			break;

		case sf::Keyboard::Num3:
			glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 0.0f);
			break;

		case sf::Keyboard::Num4:
			glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 1.0f);
			break;

		case sf::Keyboard::Num5:
			prymityw = GL_TRIANGLES;
			break;

		case sf::Keyboard::Num6:
			prymityw = GL_TRIANGLE_STRIP;
			break;

		case sf::Keyboard::Num7:
			prymityw = GL_TRIANGLE_FAN;
			break;

		case sf::Keyboard::Num8:
			prymityw = GL_QUADS;
			break;

		case sf::Keyboard::Num9:
			prymityw = GL_QUAD_STRIP;
			break;
		case sf::Keyboard::Num0:
			prymityw = GL_POLYGON;
			break;
		case sf::Keyboard::RShift:
			if (freezeMouse) freezeMouse = false;
			else freezeMouse = true;
			break;
		}

		if (!freezeMouse) {
			sf::Vector2i mouse = myszka.getPosition(window);
			sf::Event::MouseMoved;
			switch (windowEvent.type)
			{

			case sf::Event::MouseMoved:
				ustawKamereMysz(uniView, time.asMicroseconds(), window);
				break;
			}
		}

		ustawKamereKlawisze(uniView, time.asMicroseconds());
		// Nadanie scenie koloru czarnego
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);
		//glActiveTexture(GL_TEXTURE1);

		//glDrawArrays(GL_LINE_LOOP, 0, vertices.size());
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		// Wymiana buforów tylni/przedni
		window.display();
		//std :: cout << punkty_<<"\n";
	}

	// Kasowanie programu i czyszczenie buforów
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	// Zamknięcie okna renderingu
	window.close();
	return 0;
}
