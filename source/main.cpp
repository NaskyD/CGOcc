#define GLFW_INCLUDE_NONE

#include<functional>
#include<iostream>

#include<GLFW/glfw3.h>
#include<glbinding\gl\gl.h>
#include<glbinding/ContextInfo.h>
#include<glbinding/Version.h>
#include<globjects\logging.h>
#include<globjects\globjects.h>
#include<globjects/base/File.h>

#include<Painter.h>

using namespace gl;

namespace{
	bool g_toggleFS = false;
	bool g_isFS = false;
	short g_renderMode = 0;
	const int c_windowWidth = 1920;//1024;
	const int c_windowHeight = 1080;// 768;
	Painter g_painter(c_windowWidth, c_windowHeight);
}

void key_callback(GLFWwindow * window, int key, int /*scancode*/, int action, int /*modes*/)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
		glfwSetWindowShouldClose(window, true);
	else if (key == GLFW_KEY_F5 && action == GLFW_RELEASE)
		globjects::File::reloadAll();
	else if (key == GLFW_KEY_1 && action == GLFW_RELEASE)
		g_renderMode = 0;
	else if (key == GLFW_KEY_2 && action == GLFW_RELEASE)
		g_renderMode = 1;
	else if (key == GLFW_KEY_3 && action == GLFW_RELEASE)
		g_renderMode = 2;
	else if (key == GLFW_KEY_4 && action == GLFW_RELEASE)
		g_renderMode = 3;
	else if (key == GLFW_KEY_5 && action == GLFW_RELEASE)
		g_renderMode = 4;
	else if (key == GLFW_KEY_6 && action == GLFW_RELEASE)
		g_renderMode = 5;
	else if (key == GLFW_KEY_7 && action == GLFW_RELEASE)
		g_renderMode = 6;
}

void window_size_callback(GLFWwindow * window, int width, int height)
{
	g_painter.resizeWindow(width, height);
}

void setResizeCallback(GLFWwindow * window, Painter & painter)
{
	//glfwSetWindowSizeCallback(window, &painter.resizeWindow);
	std::function <void(GLFWwindow*, int, int)> fn = [painter](GLFWwindow * window, int width, int height) mutable {painter.resizeWindow(width, height); };
	glfwSetWindowSizeCallback(window, window_size_callback);// std::function<void(GLFWwindow*, int, int)>{[painter](GLFWwindow * window, int width, int height) mutable {painter.resizeWindow(width, height); }});

}

GLFWwindow * createWindow(bool fs = false)
{
	// Set GLFW window hints
	glfwSetErrorCallback([](int /*error*/, const char * description) { puts(description); });
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

	// Create a context and, if valid, make it current
	GLFWwindow * window = glfwCreateWindow(fs ? 1920 : c_windowHeight, fs ? 1080 : c_windowHeight, "Occlusion Management", fs ? glfwGetPrimaryMonitor() : NULL, NULL);
	if (window == nullptr)
	{
		globjects::critical() << "Context creation failed. Terminate execution.";

		glfwTerminate();
		exit(1);
	}
	
	glfwMakeContextCurrent(window);

	// Create callback that when user presses ESC, the context should be destroyed and window closed
	glfwSetKeyCallback(window, key_callback);

	// Initialize globjects (internally initializes glbinding, and registers the current context)
	globjects::init();

	// Do only on startup
	if (!g_toggleFS)
	{
		// Dump information about context and graphics card
		globjects::info() << std::endl
			<< "OpenGL Version:  " << glbinding::ContextInfo::version() << std::endl
			<< "OpenGL Vendor:   " << glbinding::ContextInfo::vendor() << std::endl
			<< "OpenGL Renderer: " << glbinding::ContextInfo::renderer() << std::endl;
	}


	glClearColor(0.2f, 0.3f, 0.4f, 1.f);

	g_isFS = fs;
	return window;
}

void destroyWindow(GLFWwindow * window)
{
	globjects::detachAllObjects();
	glfwDestroyWindow(window);
}

int main(int /*argc*/, char * /*argv*/[])
{
	glfwInit();
	GLFWwindow * window = createWindow(false);
	setResizeCallback(window, g_painter);
	g_painter.initialize();

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (g_toggleFS)
		{
			destroyWindow(window);
			window = createWindow(!g_isFS);
			setResizeCallback(window, g_painter);

			g_toggleFS = false;
		}

		g_painter.draw(g_renderMode);
		glfwSwapBuffers(window);
		//glfwSwapInterval(0);		for performance messures
	}

	// Properly shutdown GLFW
	glfwTerminate();

	return 0;
}