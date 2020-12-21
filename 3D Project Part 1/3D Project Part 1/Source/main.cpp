#include <iostream>
#include <vector>

#include "GLM/glm.hpp"
#include "GLM/common.hpp"
#include "GLM/gtc/type_ptr.hpp"
#include "GLAD/glad.h"
#include "GLFW/glfw3.h"

#include "opengl_utilities.h"
#include "mesh_generation.h"

/* Keep the global state inside this struct */
static struct 
{
	glm::dvec2 mouse_position;
	glm::ivec2 screen_dimensions = glm::ivec2(960, 960);
} Globals;

/* GLFW Callback functions */
static void ErrorCallback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

static void CursorPositionCallback(GLFWwindow* window, double x, double y)
{
	Globals.mouse_position.x = x;
	Globals.mouse_position.y = y;
}

static void WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	Globals.screen_dimensions.x = width;
	Globals.screen_dimensions.y = height;

	glViewport(0, 0, width, height);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//std::cout << key << std::endl;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

/* Scene Generation Functions*/

int main(int argc, char* argv[])
{
	/* Set GLFW error callback */
	glfwSetErrorCallback(ErrorCallback);

	/* Initialize the library */
	if (!glfwInit())
	{
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	/* Create a windowed mode window and its OpenGL context */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(
		Globals.screen_dimensions.x, Globals.screen_dimensions.y,
		"Sadi Celik", NULL, NULL
	);
	if (!window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	/* Move window to a certain position [do not change] */
	glfwSetWindowPos(window, 10, 50);
	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	/* Enable VSync */
	glfwSwapInterval(1);

	/* Load OpenGL extensions with GLAD */
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Set GLFW Callbacks */
	glfwSetCursorPosCallback(window, CursorPositionCallback);
	glfwSetWindowSizeCallback(window, WindowSizeCallback);
	glfwSetKeyCallback(window, KeyCallback);

	/* Configure OpenGL */
	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);

	/* Creating OpenGL objects */
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<GLuint> indices;

	/* Creating Meshes */
	GenerateParametricShapeFrom2D(positions, normals, indices, ParametricHalfCircle, 16, 16);
	VAO sphereVAO(positions, normals, indices);

	positions.clear();
	normals.clear();
	indices.clear();

	GenerateParametricShapeFrom2D(positions, normals, indices, ParametricCircle, 16, 16);
	VAO torusVAO(positions, normals, indices);

	positions.clear();
	normals.clear();
	indices.clear();

	GenerateParametricShapeFrom2D(positions, normals, indices, ParametricSpikes, 64, 32);
	VAO parametric_one_VAO(positions, normals, indices);

	positions.clear();
	normals.clear();
	indices.clear();

	GenerateParametricShapeFrom2Dv2(positions, normals, indices, ParametricSpikes, 1024, 1024);
	VAO parametric_two_VAO(positions, normals, indices);

	/* Creating Programs and Shaders */
	const GLchar* vertex_shader_scene_ottffs = R"VERTEX(
		#version 330 core

		layout(location = 0) in vec3 a_position;
		layout(location = 1) in vec3 a_normal;

		uniform mat4 u_transform;

		out vec3 vertex_position;
		out vec3 vertex_normal;

		void main()
		{
			gl_Position = u_transform * vec4(a_position, 1);
			vertex_normal = (u_transform * vec4(a_normal, 0)).xyz;
			vertex_position = gl_Position.xyz;
		}
		)VERTEX";

	/*********************************************************************************************************************************/

	GLuint scene_one = CreateProgramFromSources(vertex_shader_scene_ottffs,
		R"FRAGMENT(
		#version 330 core

		out vec4 out_color;

		void main()
		{
			out_color = vec4(1, 1, 1, 1);
		}
		)FRAGMENT");
	if (scene_one == NULL)
	{
		glfwTerminate();
		return -1;
	}
	// Wireframe Mode ON
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glUseProgram(scene_one);

	/*********************************************************************************************************************************/

	GLuint scene_two = CreateProgramFromSources(vertex_shader_scene_ottffs,
		R"FRAGMENT(
		#version 330 core

		in vec3 vertex_normal;

		out vec4 out_color;

		void main()
		{
			vec3 color = normalize(vertex_normal);
			out_color = vec4(color, 1);
		}
		)FRAGMENT");
	if (scene_two == NULL)
	{
		glfwTerminate();
		return -1;
	}

	/*********************************************************************************************************************************/

	GLuint scene_three = CreateProgramFromSources(vertex_shader_scene_ottffs,
		R"FRAGMENT(
		#version 330 core

		in vec3 vertex_position;
		in vec3 vertex_normal;

		out vec4 out_color;

		void main()
		{
			vec3 color = vec3(0);

			vec3 surface_color = vec3(0.5, 0.5, 0.5);
			vec3 surface_position = vertex_position;
			vec3 surface_normal = normalize(vertex_normal);

			// Ambient light
			float ambient_k = 1;
			vec3 ambient_color = vec3(0.5, 0.5, 0.5);
			color += ambient_k * ambient_color * surface_color;

			vec3 light_direction = normalize(vec3(-1, -1, 1));
			vec3 to_light = -light_direction;
			vec3 light_color =  vec3(0.4, 0.4, 0.4);

			// Diffuse light
			float diffuse_k = 1;
			float diffuse_intensity = max(0, dot(to_light, surface_normal));
			color += diffuse_k * diffuse_intensity * light_color * surface_color;

			// Specular Lighting
			vec3 view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			vec3 halfway_dir = normalize(view_dir + to_light);

			float specular_k = 1;
			float shininess = 64;
			float specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * light_color;

			out_color = vec4(color, 1);
		}
		)FRAGMENT");
	if (scene_three == NULL)
	{
		glfwTerminate();
		return -1;
	}

	/*********************************************************************************************************************************/

	const GLchar* fragment_shader_gray = R"FRAGMENT(
		#version 330 core

		uniform vec2 u_mouse_position;

		in vec3 vertex_position;
		in vec3 vertex_normal;

		out vec4 out_color;

		void main()
		{
			vec3 color = vec3(0);

			vec3 surface_color = vec3(0.5, 0.5, 0.5);
			vec3 surface_position = vertex_position;
			vec3 surface_normal = normalize(vertex_normal);

			// Ambient light
			float ambient_k = 1;
			vec3 ambient_color = vec3(0.5, 0.5, 0.5);
			color += ambient_k * ambient_color * surface_color;

			vec3 light_direction = normalize(vec3(-1, -1, 1));
			vec3 to_light = -light_direction;
			vec3 light_color =  vec3(0.4, 0.4, 0.4);

			// Diffuse light
			float diffuse_k = 1;
			float diffuse_intensity = max(0, dot(to_light, surface_normal));
			color += diffuse_k * diffuse_intensity * light_color * surface_color;

			// Specular Lighting
			vec3 view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			vec3 halfway_dir = normalize(view_dir + to_light);

			float specular_k = 1;
			float shininess = 128;
			float specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * light_color;

			// Light 2
			vec3 point_light_position = vec3(u_mouse_position, -1);
			vec3 point_light_color =  vec3(0.5, 0.5, 0.5);
			vec3 to_point_light = normalize(point_light_position - surface_position);
			
			// Diffuse light
			diffuse_k = 1;
			diffuse_intensity = max(0, dot(to_point_light, surface_normal));
			color += diffuse_k * diffuse_intensity * point_light_color * surface_color;

			// Specular Lighting
			view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			halfway_dir = normalize(view_dir + to_point_light);

			specular_k = 1;
			shininess = 128;
			specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * point_light_color;

			out_color = vec4(color, 1);
		}
		)FRAGMENT";

	const GLchar* fragment_shader_red = R"FRAGMENT(
		#version 330 core

		uniform vec2 u_mouse_position;

		in vec3 vertex_position;
		in vec3 vertex_normal;

		out vec4 out_color;

		void main()
		{
			vec3 color = vec3(0);

			vec3 surface_color = vec3(1, 0, 0);
			vec3 surface_position = vertex_position;
			vec3 surface_normal = normalize(vertex_normal);

			// Ambient light
			float ambient_k = 1;
			vec3 ambient_color = vec3(0.5, 0.5, 0.5);
			color += ambient_k * ambient_color * surface_color;

			vec3 light_direction = normalize(vec3(-1, -1, 1));
			vec3 to_light = -light_direction;
			vec3 light_color =  vec3(0.4, 0.4, 0.4);

			// Diffuse light
			float diffuse_k = 1;
			float diffuse_intensity = max(0, dot(to_light, surface_normal));
			color += diffuse_k * diffuse_intensity * light_color * surface_color;

			// Specular Lighting
			vec3 view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			vec3 halfway_dir = normalize(view_dir + to_light);

			float specular_k = 1;
			float shininess = 32;
			float specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * light_color;

			// Light 2
			vec3 point_light_position = vec3(u_mouse_position, -1);
			vec3 point_light_color =  vec3(0.5, 0.5, 0.5);
			vec3 to_point_light = normalize(point_light_position - surface_position);
			
			// Diffuse light
			diffuse_k = 1;
			diffuse_intensity = max(0, dot(to_point_light, surface_normal));
			color += diffuse_k * diffuse_intensity * point_light_color * surface_color;

			// Specular Lighting
			view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			halfway_dir = normalize(view_dir + to_point_light);

			specular_k = 1;
			shininess = 32;
			specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * point_light_color;

			out_color = vec4(color, 1);
		}
		)FRAGMENT";

	const GLchar* fragment_shader_blue = R"FRAGMENT(
		#version 330 core

		uniform vec2 u_mouse_position;

		in vec3 vertex_position;
		in vec3 vertex_normal;

		out vec4 out_color;

		void main()
		{
			vec3 color = vec3(0);

			vec3 surface_color = vec3(0, 0, 1);
			vec3 surface_position = vertex_position;
			vec3 surface_normal = normalize(vertex_normal);

			// Ambient light
			float ambient_k = 1;
			vec3 ambient_color = vec3(0.5, 0.5, 0.5);
			color += ambient_k * ambient_color * surface_color;

			vec3 light_direction = normalize(vec3(-1, -1, 1));
			vec3 to_light = -light_direction;
			vec3 light_color =  vec3(0.4, 0.4, 0.4);

			// Diffuse light
			float diffuse_k = 1;
			float diffuse_intensity = max(0, dot(to_light, surface_normal));
			color += diffuse_k * diffuse_intensity * light_color * surface_color;

			// Specular Lighting
			vec3 view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			vec3 halfway_dir = normalize(view_dir + to_light);

			float specular_k = 1;
			float shininess = 32;
			float specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * light_color;

			// Light 2
			vec3 point_light_position = vec3(u_mouse_position, -1);
			vec3 point_light_color =  vec3(0.5, 0.5, 0.5);
			vec3 to_point_light = normalize(point_light_position - surface_position);
			
			// Diffuse light
			diffuse_k = 1;
			diffuse_intensity = max(0, dot(to_point_light, surface_normal));
			color += diffuse_k * diffuse_intensity * point_light_color * surface_color;

			// Specular Lighting
			view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			halfway_dir = normalize(view_dir + to_point_light);

			specular_k = 1;
			shininess = 32;
			specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * point_light_color;

			out_color = vec4(color, 1);
		}
		)FRAGMENT";

	const GLchar* fragment_shader_green = R"FRAGMENT(
		#version 330 core

		uniform vec2 u_mouse_position;

		in vec3 vertex_position;
		in vec3 vertex_normal;

		out vec4 out_color;

		void main()
		{
			vec3 color = vec3(0);

			vec3 surface_color = vec3(0, 1, 0);
			vec3 surface_position = vertex_position;
			vec3 surface_normal = normalize(vertex_normal);

			// Ambient light
			float ambient_k = 1;
			vec3 ambient_color = vec3(0.5, 0.5, 0.5);
			color += ambient_k * ambient_color * surface_color;

			vec3 light_direction = normalize(vec3(-1, -1, 1));
			vec3 to_light = -light_direction;
			vec3 light_color =  vec3(0.4, 0.4, 0.4);

			// Diffuse light
			float diffuse_k = 1;
			float diffuse_intensity = max(0, dot(to_light, surface_normal));
			color += diffuse_k * diffuse_intensity * light_color * surface_color;

			// Specular Lighting
			vec3 view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			vec3 halfway_dir = normalize(view_dir + to_light);

			float specular_k = 1;
			float shininess = 32;
			float specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * light_color;

			// Light 2
			vec3 point_light_position = vec3(u_mouse_position, -1);
			vec3 point_light_color =  vec3(0.5, 0.5, 0.5);
			vec3 to_point_light = normalize(point_light_position - surface_position);
			
			// Diffuse light
			diffuse_k = 1;
			diffuse_intensity = max(0, dot(to_point_light, surface_normal));
			color += diffuse_k * diffuse_intensity * point_light_color * surface_color;

			// Specular Lighting
			view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			halfway_dir = normalize(view_dir + to_point_light);

			specular_k = 1;
			shininess = 32;
			specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * point_light_color;

			out_color = vec4(color, 1);
		}
		)FRAGMENT";

	GLuint scene_four_obj1 = CreateProgramFromSources(vertex_shader_scene_ottffs, fragment_shader_gray);
	GLuint scene_four_obj2 = CreateProgramFromSources(vertex_shader_scene_ottffs, fragment_shader_red);
	GLuint scene_four_obj3 = CreateProgramFromSources(vertex_shader_scene_ottffs, fragment_shader_green);
	GLuint scene_four_obj4 = CreateProgramFromSources(vertex_shader_scene_ottffs, fragment_shader_blue);

	if (scene_four_obj1 == NULL)
	{
		glfwTerminate();
		return -1;
	}
	if (scene_four_obj2 == NULL)
	{
		glfwTerminate();
		return -1;
	}
	if (scene_four_obj3 == NULL)
	{
		glfwTerminate();
		return -1;
	}
	if (scene_four_obj4 == NULL)
	{
		glfwTerminate();
		return -1;
	}
	/*********************************************************************************************************************************/

	GLuint scene_five = CreateProgramFromSources(vertex_shader_scene_ottffs,
		R"FRAGMENT(
		#version 330 core

		uniform vec2 u_mouse_position;

		in vec3 vertex_position;
		in vec3 vertex_normal;

		out vec4 out_color;

		void main()
		{
			vec3 color = vec3(0);

			vec3 surface_color = vec3(0, 1, 0);
			vec3 surface_position = vertex_position;
			vec3 surface_normal = normalize(vertex_normal);

			// Ambient light
			float ambient_k = 1;
			vec3 ambient_color = vec3(0.5, 0.5, 0.5);
			color += ambient_k * ambient_color * surface_color;

			vec3 light_direction = normalize(vec3(-1, -1, 1));
			vec3 to_light = -light_direction;
			vec3 light_color =  vec3(0.4, 0.4, 0.4);

			// Diffuse light
			float diffuse_k = 1;
			float diffuse_intensity = max(0, dot(to_light, surface_normal));
			color += diffuse_k * diffuse_intensity * light_color * surface_color;

			// Specular Lighting
			vec3 view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			vec3 halfway_dir = normalize(view_dir + to_light);

			float specular_k = 1;
			float shininess = 32;
			float specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * light_color;

			// Light 2
			vec3 point_light_position = vec3(u_mouse_position, -1);
			vec3 point_light_color =  vec3(0.5, 0.5, 0.5);
			vec3 to_point_light = normalize(point_light_position - surface_position);
			
			// Diffuse light
			diffuse_k = 1;
			diffuse_intensity = max(0, dot(to_point_light, surface_normal));
			color += diffuse_k * diffuse_intensity * point_light_color * surface_color;

			// Specular Lighting
			view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			halfway_dir = normalize(view_dir + to_point_light);

			specular_k = 1;
			shininess = 32;
			specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * point_light_color;

			out_color = vec4(color, 1);
		}
		)FRAGMENT");

	if (scene_five == NULL)
	{
		glfwTerminate();
		return -1;
	}

	/*********************************************************************************************************************************/

	GLuint scene_six = CreateProgramFromSources(vertex_shader_scene_ottffs,
		R"FRAGMENT(
		#version 330 core

		uniform vec2 u_mouse_position;

		in vec3 vertex_position;
		in vec3 vertex_normal;

		out vec4 out_color;

		void main()
		{
			vec3 color = vec3(0);

			vec3 surface_color = vec3(1, 1, 1);
			vec3 surface_position = vertex_position;
			vec3 surface_normal = normalize(vertex_normal);

			// Ambient light
			float ambient_k = 0.5;
			vec3 ambient_color = vec3(0, 1, 0);
			color += ambient_k * ambient_color * surface_color;

			vec3 light_direction = normalize(vec3(1, 1, 1));
			vec3 to_light = -light_direction;
			vec3 light_color =  vec3(0, 0, 1);

			// Diffuse light
			float diffuse_k = 1;
			float diffuse_intensity = max(0, dot(to_light, surface_normal));
			color += diffuse_k * diffuse_intensity * light_color * surface_color;

			// Specular Lighting
			vec3 view_dir = vec3(0, 0, -1);	//	Because we are using an orthograpic projection, and because of the direction of the projection
			vec3 halfway_dir = normalize(view_dir + to_light);

			float specular_k = 1;
			float shininess = 64;
			float specular_intensity = max(0, dot(halfway_dir, surface_normal));
			color += specular_k * pow(specular_intensity, shininess) * light_color;

			// Light 2
			vec3 point_light_position = vec3(u_mouse_position, -1);
			vec3 point_light_color =  vec3(1, 0, 0);
			vec3 to_point_light = normalize(point_light_position - surface_position);
			
			// Diffuse light
			diffuse_k = 1;
			diffuse_intensity = max(0, dot(to_point_light, surface_normal));
			color += diffuse_k * diffuse_intensity * point_light_color * surface_color;

			out_color = vec4(normalize(color), 1);
		}
		)FRAGMENT");
	if (scene_six == NULL)
	{
		glfwTerminate();
		return -1;
	}

	// u_mouse_position and u_transform
	auto mouse_location = glGetUniformLocation(scene_four_obj1, "u_mouse_position");
	auto u_transform_location = glGetUniformLocation(scene_one, "u_transform");

	// Key Flags
	bool flag_q = GL_FALSE;
	bool flag_w = GL_FALSE;
	bool flag_e = GL_FALSE;
	bool flag_r = GL_FALSE;
	bool flag_t = GL_FALSE;
	bool flag_y = GL_FALSE;
	bool flag_init = GL_FALSE;

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* States of the keys*/
		int state_q = glfwGetKey(window, GLFW_KEY_Q);
		int state_w = glfwGetKey(window, GLFW_KEY_W);
		int state_e = glfwGetKey(window, GLFW_KEY_E);
		int state_r = glfwGetKey(window, GLFW_KEY_R);
		int state_t = glfwGetKey(window, GLFW_KEY_T);
		int state_y = glfwGetKey(window, GLFW_KEY_Y);

		/* INIT FLAG ONLY TRUE WHEN EVERY OTHER FLAGS ARE FALSE @ the start */
		if (flag_q == GL_FALSE && flag_w == GL_FALSE && flag_e == GL_FALSE && flag_r == GL_FALSE && flag_t == GL_FALSE && flag_y == GL_FALSE)
		{
			flag_init = GL_TRUE;
		}

		/****** Scene One ******/
		if (state_q == GLFW_PRESS)
		{
			//Flag true
			flag_q = GL_TRUE;
			//Flag false
			flag_w = GL_FALSE;
			flag_e = GL_FALSE;
			flag_r = GL_FALSE;
			flag_t = GL_FALSE;
			flag_y = GL_FALSE;
			flag_init = GL_FALSE;

			// Wireframe Mode ON
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			u_transform_location = glGetUniformLocation(scene_one, "u_transform");
			glUseProgram(scene_one);
		}

		/****** Scene Two ******/
		if (state_w == GLFW_PRESS)
		{
			//Flag true
			flag_w = GL_TRUE;
			//Flag false
			flag_q = GL_FALSE;
			flag_e = GL_FALSE;
			flag_r = GL_FALSE;
			flag_t = GL_FALSE;
			flag_y = GL_FALSE;
			flag_init = GL_FALSE;

			// Wireframe Mode OFF
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			u_transform_location = glGetUniformLocation(scene_two, "u_transform");
			glUseProgram(scene_two);
		}

		/****** Scene Three ******/
		if (state_e == GLFW_PRESS)
		{
			//Flag true
			flag_e = GL_TRUE;
			//Flag false
			flag_w = GL_FALSE;
			flag_q = GL_FALSE;
			flag_r = GL_FALSE;
			flag_t = GL_FALSE;
			flag_y = GL_FALSE;
			flag_init = GL_FALSE;

			// Wireframe Mode OFF
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			u_transform_location = glGetUniformLocation(scene_three, "u_transform");
			glUseProgram(scene_three);
		}

		/****** Scene Four ******/
		if (state_r == GLFW_PRESS)
		{
			//Flag true
			flag_r = GL_TRUE;
			//Flag false
			flag_w = GL_FALSE;
			flag_e = GL_FALSE;
			flag_q = GL_FALSE;
			flag_t = GL_FALSE;
			flag_y = GL_FALSE;
			flag_init = GL_FALSE;

			// Wireframe Mode OFF
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		/****** Scene Five ******/
		if (state_t == GLFW_PRESS)
		{
			//Flag true
			flag_t = GL_TRUE;
			//Flag false
			flag_w = GL_FALSE;
			flag_e = GL_FALSE;
			flag_r = GL_FALSE;
			flag_y = GL_FALSE;
			flag_q = GL_FALSE;
			flag_init = GL_FALSE;

			// Wireframe Mode OFF
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		/****** Scene Six ******/
		if (state_y == GLFW_PRESS)
		{
			//Flag true
			flag_y = GL_TRUE;
			//Flag false
			flag_w = GL_FALSE;
			flag_e = GL_FALSE;
			flag_r = GL_FALSE;
			flag_t = GL_FALSE;
			flag_q = GL_FALSE;
			flag_init = GL_FALSE;

			// Wireframe Mode OFF
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			mouse_location = glGetUniformLocation(scene_six, "u_mouse_position");
			u_transform_location = glGetUniformLocation(scene_six, "u_transform");
			glUseProgram(scene_six);
		}

		// Calculate mouse position
		auto mouse_position = Globals.mouse_position / glm::dvec2(Globals.screen_dimensions);
		mouse_position.y = 1. - mouse_position.y;
		mouse_position = mouse_position * 2. - 1.;

		/****** Render Scene One, Two, Three with 4 Meshes ******/
		if (flag_init == GL_TRUE || flag_q == GL_TRUE || flag_w == GL_TRUE || flag_e == GL_TRUE)
		{
			glClearColor(0, 0, 0, 1);

			glm::mat4 transform;

			// Draw Sphere
			glBindVertexArray(sphereVAO.id);

			transform = glm::translate(glm::vec3(-0.5, 0.5, 0));
			transform = glm::scale(transform, glm::vec3(0.45f));
			transform = glm::rotate(transform, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform));

			glDrawElements(GL_TRIANGLES, sphereVAO.element_array_count, GL_UNSIGNED_INT, NULL);

			// Draw Torus
			glBindVertexArray(torusVAO.id);

			transform = glm::translate(glm::vec3(0.5, 0.5, 0));
			transform = glm::scale(transform, glm::vec3(0.45f));
			transform = glm::rotate(transform, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform));

			glDrawElements(GL_TRIANGLES, torusVAO.element_array_count, GL_UNSIGNED_INT, NULL);

			// Draw Parametric One
			glBindVertexArray(parametric_one_VAO.id);

			transform = glm::translate(glm::vec3(-0.5, -0.5, 0));
			transform = glm::scale(transform, glm::vec3(0.45f));
			transform = glm::rotate(transform, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform));

			glDrawElements(GL_TRIANGLES, parametric_one_VAO.element_array_count, GL_UNSIGNED_INT, NULL);

			// Draw Parametric Two
			glBindVertexArray(parametric_two_VAO.id);

			transform = glm::translate(glm::vec3(0.5, -0.5, 0));
			transform = glm::scale(transform, glm::vec3(0.45f));
			transform = glm::rotate(transform, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform));

			glDrawElements(GL_TRIANGLES, parametric_two_VAO.element_array_count, GL_UNSIGNED_INT, NULL);
		}

		/****** Render Scene Four with 4 Meshes ******/
		if (flag_r == GL_TRUE)
		{
			glClearColor(0, 0, 0, 1);

			glm::mat4 transform_v4;

			mouse_location = glGetUniformLocation(scene_four_obj1, "u_mouse_position");
			u_transform_location = glGetUniformLocation(scene_four_obj1, "u_transform");
			glUseProgram(scene_four_obj1);

			glUniform2fv(mouse_location, 1, glm::value_ptr(glm::vec2(mouse_position)));

			// Draw Sphere
			glBindVertexArray(sphereVAO.id);

			transform_v4 = glm::translate(glm::vec3(-0.5, 0.5, 0));
			transform_v4 = glm::scale(transform_v4, glm::vec3(0.45f));
			transform_v4 = glm::rotate(transform_v4, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform_v4));

			glDrawElements(GL_TRIANGLES, sphereVAO.element_array_count, GL_UNSIGNED_INT, NULL);

			glUseProgram(scene_four_obj2);

			glUniform2fv(mouse_location, 1, glm::value_ptr(glm::vec2(mouse_position)));

			// Draw Torus
			glBindVertexArray(torusVAO.id);

			transform_v4 = glm::translate(glm::vec3(0.5, 0.5, 0));
			transform_v4 = glm::scale(transform_v4, glm::vec3(0.45f));
			transform_v4 = glm::rotate(transform_v4, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform_v4));

			glDrawElements(GL_TRIANGLES, torusVAO.element_array_count, GL_UNSIGNED_INT, NULL);

			glUseProgram(scene_four_obj3);

			glUniform2fv(mouse_location, 1, glm::value_ptr(glm::vec2(mouse_position)));

			// Draw Parametric One
			glBindVertexArray(parametric_one_VAO.id);

			transform_v4 = glm::translate(glm::vec3(-0.5, -0.5, 0));
			transform_v4 = glm::scale(transform_v4, glm::vec3(0.45f));
			transform_v4 = glm::rotate(transform_v4, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform_v4));

			glDrawElements(GL_TRIANGLES, parametric_one_VAO.element_array_count, GL_UNSIGNED_INT, NULL);

			glUseProgram(scene_four_obj4);

			glUniform2fv(mouse_location, 1, glm::value_ptr(glm::vec2(mouse_position)));

			// Draw Parametric Two
			glBindVertexArray(parametric_two_VAO.id);

			transform_v4 = glm::translate(glm::vec3(0.5, -0.5, 0));
			transform_v4 = glm::scale(transform_v4, glm::vec3(0.45f));
			transform_v4 = glm::rotate(transform_v4, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform_v4));

			glDrawElements(GL_TRIANGLES, parametric_two_VAO.element_array_count, GL_UNSIGNED_INT, NULL);
		}

		/****** Render Scene Five The Game ******/
		if (flag_t == GL_TRUE)
		{
			glClearColor(0, 0, 0, 1);

			glm::mat4 transform_v3;

			mouse_location = glGetUniformLocation(scene_five, "u_mouse_position");
			glUniform2fv(mouse_location, 1, glm::value_ptr(glm::vec2(mouse_position)));

			glm::dvec2 chasing_pos = glm::mix(mouse_position, chasing_pos, 0.99f);
			double distance_bet = abs(glm::distance(mouse_position, chasing_pos));

			if (distance_bet >= 0.3 * 2)
			{
				u_transform_location = glGetUniformLocation(scene_four_obj3, "u_transform");
				glUseProgram(scene_four_obj3);
			}
			else
			{
				u_transform_location = glGetUniformLocation(scene_four_obj2, "u_transform");
				glUseProgram(scene_four_obj2);
			}

			//::cout << chasing_pos.g << std::endl;

			// Draw Sphere 1
			glBindVertexArray(sphereVAO.id);

			transform_v3 = glm::translate(glm::vec3(mouse_position,1));
			transform_v3 = glm::scale(transform_v3, glm::vec3(0.3f));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform_v3));

			glDrawElements(GL_TRIANGLES, sphereVAO.element_array_count, GL_UNSIGNED_INT, NULL);

			// Draw Sphere 2
			u_transform_location = glGetUniformLocation(scene_four_obj1, "u_transform");
			glUseProgram(scene_four_obj1);

			glBindVertexArray(sphereVAO.id);

			transform_v3 = glm::translate(glm::vec3(chasing_pos, 1));
			transform_v3 = glm::scale(transform_v3, glm::vec3(0.3f));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform_v3));

			glDrawElements(GL_TRIANGLES, sphereVAO.element_array_count, GL_UNSIGNED_INT, NULL);
		}

		/****** Render Scene Six Impress ******/
		if (flag_y == GL_TRUE)
		{
			// Transparent window
			glClearColor(0, 0, 0, 0);

			glm::mat4 transform_v2;

			glUniform2fv(mouse_location, 1, glm::value_ptr(glm::vec2(mouse_position)));

			// Draw Parametric Two
			glBindVertexArray(parametric_two_VAO.id);

			transform_v2 = glm::translate(glm::vec3(0, 0, 0));
			transform_v2 = glm::rotate(transform_v2, glm::radians(float(glfwGetTime() * 10)), glm::vec3(1, 1, 0));
			glUniformMatrix4fv(u_transform_location, 1, GL_FALSE, glm::value_ptr(transform_v2));

			glDrawElements(GL_TRIANGLES, parametric_two_VAO.element_array_count, GL_UNSIGNED_INT, NULL);
		}
		
		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}