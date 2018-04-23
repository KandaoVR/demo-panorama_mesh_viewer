#pragma once
#include "opencv2/opencv.hpp"
#include "GL/glew.h"
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "utils/camera.h"

#define GLFW_EXPOSE_NATIVE_WIN32

#ifndef _DEBUG
#pragma comment(lib, "glew32.lib")
#else
#pragma comment(lib, "glew32d.lib")
#endif
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")

namespace kandao { namespace OpenGL
{
	///////////////////////////////////// Interaction /////////////////////////////////////
	void processInput(GLFWwindow *window);
	Camera& getDefaultCamera();

	///////////////////////////////////// global functions /////////////////////////////////////
	GLFWwindow* initOpenGL(bool hide = true, int width = 800, int height = 600);
	int glCheckError();
	void terminateOpenGL();


	///////////////////////////////////// Shader Program /////////////////////////////////////
	GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path);
	GLuint LoadShadersFromString(const char * vertex_shader, const char * fragment_shader);

	// https://learnopengl.com/Introduction
	class Shader
	{
	public:
		unsigned int ID = 0;
		// constructor generates the shader on the fly
		// ------------------------------------------------------------------------
		Shader() {}
		bool loadShadersFromString(const char * vertex_shader, const char * fragment_shader);

		// activate the shader
		// ------------------------------------------------------------------------
		void use();
		void setBool(const std::string &name, bool value) const;
		void setInt(const std::string &name, int value) const;
		void setFloat(const std::string &name, float value) const;
		void setVec2(const std::string &name, const glm::vec2 &value) const;
		void setVec2(const std::string &name, float x, float y) const;
		void setVec3(const std::string &name, const glm::vec3 &value) const;
		void setVec3(const std::string &name, float x, float y, float z) const;
		void setVec4(const std::string &name, const glm::vec4 &value) const;
		void setVec4(const std::string &name, float x, float y, float z, float w);
		void setMat2(const std::string &name, const glm::mat2 &mat) const;
		void setMat3(const std::string &name, const glm::mat3 &mat) const;
		void setMat4(const std::string &name, const glm::mat4 &mat) const;

	private:
		// utility function for checking shader compilation/linking errors.
		void checkCompileErrors(GLuint shader, std::string type);
	};
} }
