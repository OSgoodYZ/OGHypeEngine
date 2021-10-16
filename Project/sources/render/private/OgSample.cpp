#include "render/OgSample.h"

OG_NAMESPACE_RENDER_BEGIN

void OgSample::Run() {
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void OgSample::initWindow() 
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	_window = glfwCreateWindow(_width, _height, "Vulkan", nullptr, nullptr);
}

void OgSample::initVulkan() 
{

}

void OgSample::mainLoop() 
{
	while (!glfwWindowShouldClose(_window)) 
	{
		glfwPollEvents();
	}
}

void OgSample::cleanup() 
{
	glfwDestroyWindow(_window);
	glfwTerminate();
}

OG_NAMESPACE_RENDER_END