/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 */

#pragma once

#include <memory>
#include "renderer.hpp"

class Application
{
public:
	Application();
	~Application();

	void run(const std::unique_ptr<RendererInterface>& renderer);

private:
	static void mousePositionCallback(GLFWwindow* window, double xpos, double ypos);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

	GLFWwindow* m_window;
	double m_prevCursorX;
	double m_prevCursorY;

	ViewSettings m_viewSettings;
	SceneSettings m_sceneSettings;

	enum class InputMode
	{
		None,
		RotatingView,
		RotatingScene,
	};
	InputMode m_mode;

	std::function<void (int w, int h)> m_onResize;
};
