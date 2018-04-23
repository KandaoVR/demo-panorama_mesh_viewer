/* Demo for display panorama with depth using OpenGL.
*  All rights reserved. KandaoVR 2018.
*/
#include "opencv2/opencv.hpp"
#include "utils/utils.opengl.h"
#include "utils/utils.opencv.h"
#include "utils/shaders.h"
#include "utils/timer.h"

using namespace std;
using namespace cv;
using namespace kandao;

void buildVAO_Equirectangular(const cv::Mat &frame, const cv::Mat &depth,
	unsigned int &VAO, unsigned int &n_indices, int n_cols = 200, int n_rows = 100);
unsigned int makeTextureFromMat(const cv::Mat &src, GLint src_fmt, GLint src_type, GLint dst_fmt);

int main(int argc, char **argv)
{
	string in_fn = argc > 1 ? argv[1] : "../data/sampla_with_disp_tb.jpg";
	Mat in_dat = imread(in_fn);
	if (in_dat.empty()) {
		printf("read input frame failed\n");
		return -1;
	}

	Mat frame = in_dat.rowRange(0, in_dat.rows / 2);
	Mat disp = in_dat.rowRange(in_dat.rows / 2, in_dat.rows);
	Mat depth = opencv::viewableDisp2Original(disp, 0.01f);

	///////////////////////////////////// opengl /////////////////////////////////////
	int SCR_WIDTH = 1000, SCR_HEIGHT = 1000;
	GLFWwindow *window = OpenGL::initOpenGL(false, SCR_WIDTH, SCR_HEIGHT);

	///////////////////////////////////// vertex /////////////////////////////////////
	int n_cols = 1000, n_rows = 500;
	unsigned int VAO = 0, n_indices = 0;
	buildVAO_Equirectangular(frame, depth, VAO, n_indices, n_cols, n_rows);

	///////////////////////////////////// texture /////////////////////////////////////
	unsigned int tex_frame = makeTextureFromMat(frame, GL_BGR, GL_UNSIGNED_BYTE, GL_RGB);
	unsigned int tex_depth = makeTextureFromMat(depth, GL_RED, GL_FLOAT, GL_R32F);

	///////////////////////////////////// shader /////////////////////////////////////
	OpenGL::Shader shader;
	shader.loadShadersFromString(show_equi_vs, show_texture_fs);

	///////////////////////////////////// main loop /////////////////////////////////////
	Camera& camera = OpenGL::getDefaultCamera();
	camera.setPosition(0.f, 0.f, 0.f);

	while (!glfwWindowShouldClose(window))
	{
		// input
		OpenGL::processInput(window);

		// render shader
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		// pass projection matrix to shader (note that in this case it could change every frame)
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		shader.setMat4("projection", projection);

		// camera/view transformation
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("view", view);

		glm::mat4 model(1.f);
		shader.setMat4("model", model);

		// draw
		shader.use();
		glBindTexture(GL_TEXTURE_2D, tex_frame);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, n_indices, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	OpenGL::terminateOpenGL();
	return 0;
}

static void backward2Point_equi(float u, float v, float &X, float &Y, float &Z)
{
	float sinv = sinf(v);
	X = sinv * sinf(u);
	Y = -cosf(v);
	Z = sinv * cosf(u);
}

static void makeQuadrangleEqui(const cv::Mat &depth, float x, float y, float w, float h, 
	std::vector<cv::Vec3f> &quad_3d, std::vector<cv::Vec2f> &quad_2d)
{
	int width = depth.cols, height = depth.rows;
	quad_3d.resize(4);
	quad_2d.resize(4);

	// 4 vertex points on source
	vector<Vec2f> src_xy = {
		Vec2f(x, y),
		Vec2f(x + w, y),
		Vec2f(x + w, y + h),
		Vec2f(x, y + h),
	};

	// 4 corresponding 3d points
	for (int i = 0; i < 4; ++i) {
		// vertex on texture
		quad_2d[i][0] = src_xy[i][0] / width;
		quad_2d[i][1] = src_xy[i][1] / height;

		// to discrete coordinates on frame
		int xx = round(src_xy[i][0] - 0.5);
		int yy = round(src_xy[i][1] - 0.5);
		xx = (xx + width) % width;
		yy = min(max(yy, 0), height - 1);
		float d = depth.ptr<float>(yy)[xx];

		// to equirectangular coordinates
		float u = src_xy[i][0] / width * CV_PI * 2.f - CV_PI;
		float v = src_xy[i][1] / height * CV_PI;

		// to 3d points
		float X, Y, Z;
		backward2Point_equi(u, v, X, Y, Z);

		// to opengl coordinates
		quad_3d[i][0] = X  * d;
		quad_3d[i][1] = -Y * d;
		quad_3d[i][2] = -Z * d;
	}
}

void buildVAO_Equirectangular(const cv::Mat &frame, const cv::Mat &depth,
	unsigned int &VAO, unsigned int &n_indices, int n_cols, int n_rows)
{
	// build upon grids of HW * NH 
	vector<float> vertices;
	vertices.reserve(n_cols * n_rows * 5);
	vector<unsigned int> indices;
	indices.reserve(n_cols * n_rows * 1.5);

	float width = frame.cols, height = frame.rows;
	float w = width / (n_cols - 1), h = height / (n_rows - 1);

	startCpuTimer(gen_vertices);
	for (int i = 0; i < n_rows - 1; ++i) {
		for (int j = 0; j < n_cols - 1; ++j) {
			vector<Vec3f> quad_3d;
			vector<Vec2f> quad_2d;

			float y = i * h, x = j * w;
			makeQuadrangleEqui(depth, x, y, w, h, quad_3d, quad_2d);

			int k = vertices.size() / 5;

			for (int i = 0; i < 4; ++i) {
				vertices.push_back(quad_3d[i][0]);
				vertices.push_back(quad_3d[i][1]);
				vertices.push_back(quad_3d[i][2]);
				vertices.push_back(quad_2d[i][0]);
				vertices.push_back(quad_2d[i][1]);
			}

			// index to draw triangles
			indices.push_back(k + 0);
			indices.push_back(k + 1);
			indices.push_back(k + 3);
			indices.push_back(k + 1);
			indices.push_back(k + 2);
			indices.push_back(k + 3);
		}
	}
	stopCpuTimer(gen_vertices);

	/////////////////////////////////////// VAO /////////////////////////////////////
	//unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	unsigned int EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	n_indices = indices.size();
}

unsigned int makeTextureFromMat(const cv::Mat &src, GLint src_fmt, GLint src_type, GLint dst_fmt)
{
	int width = src.cols, height = src.rows;

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, dst_fmt, width, height, 0, src_fmt, src_type, src.data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}
