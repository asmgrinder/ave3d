/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 */

#include <cstdio>
#include <string>
#include <memory>
#include <vector>

#include "application.hpp"

#include "../opengl.hpp"

int main(int /*argc*/, char* /*argv*/[])
{
	RendererInterface* renderer = new OpenGL::Renderer;

	try
	{
		Application().run(std::unique_ptr<RendererInterface>{ renderer });
	}
	catch(const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
