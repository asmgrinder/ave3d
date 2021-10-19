/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 */

#pragma once
#include <glm/mat4x4.hpp>
#include <functional>

struct GLFWwindow;

struct ViewSettings
{
	float pitch = 0.0f;
	float yaw = 90.0f;
	float distance;
	float fov;
};

struct SceneSettings
{
	float pitch = 0.0f;
	float yaw = 0.0f;

	static const int NumLights = 3;
	struct Light {
		glm::vec3 direction;
		glm::vec3 radiance;
		bool enabled = false;
	} lights[NumLights];
};

class RendererInterface
{
public:
	virtual ~RendererInterface() = default;

	virtual GLFWwindow* initialize(int width, int height, int maxSamples) = 0;
	virtual void shutdown() = 0;
	virtual std::function<void (int w, int h)> setup() = 0;
	virtual void render(GLFWwindow* window, const ViewSettings& view, const SceneSettings& scene) = 0;
};
