/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 *
 * OpenGL 4.5 renderer.
 */

#include <stdexcept>
#include <memory>

#include <GLFW/glfw3.h>

#include "opengl.hpp"


namespace OpenGL
{


GLFWwindow* Renderer::initialize(int width, int height, int maxSamples)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#if _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	glfwWindowHint(GLFW_DEPTH_BITS, 0);
	glfwWindowHint(GLFW_STENCIL_BITS, 0);
	glfwWindowHint(GLFW_SAMPLES, 0);

	GLFWwindow* window = glfwCreateWindow(width, height, "Physically Based Rendering (OpenGL 4.5)", nullptr, nullptr);
	if (!window)
	{
		throw std::runtime_error("Failed to create OpenGL context");
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(-1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize OpenGL extensions loader");
	}
	
// 	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &mCapabilities.maxAnisotropy);

#if _DEBUG
	glDebugMessageCallback(Renderer::logMessage, nullptr);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

	GLint maxSupportedSamples;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSupportedSamples);

	const int samples = glm::min(maxSamples, maxSupportedSamples);
	mFramebuffer = std::make_shared<Framebuffer>();
	{
		mFramebuffer->AttachRenderbuffer(GL_COLOR_ATTACHMENT0, GL_RGBA16F, width, height, samples);
		mFramebuffer->AttachRenderbuffer(GL_COLOR_ATTACHMENT1, GL_RGBA16F, width, height, samples);
		mFramebuffer->AttachRenderbuffer(GL_COLOR_ATTACHMENT2, GL_R16F,    width, height, samples);
		mFramebuffer->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT32, width, height, samples);
		mFramebuffer->DrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 });
		auto status = mFramebuffer->CheckStatus();
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error("Framebuffer is not complete: " + std::to_string(status));
		}
	}

	mResolveFramebuffer = std::make_shared<Framebuffer>();
	{
		mResolveFramebuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GL_RGBA16F, width, height);
		mResolveFramebuffer->AttachTexture(GL_COLOR_ATTACHMENT1, GL_RGBA16F, width, height);
		mResolveFramebuffer->AttachTexture(GL_COLOR_ATTACHMENT2, GL_R16F,    width, height);
		mResolveFramebuffer->DrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 });
		auto status = mResolveFramebuffer->CheckStatus();
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error("Framebuffer is not complete: " + std::to_string(status));
		}
	}

	glViewport(0, 0, width, height);

	std::cout << "OpenGL 4.5 renderer [" << glGetString(GL_RENDERER) << "]" << std::endl;
	return window;
}

void Renderer::shutdown()
{
	mResolveFramebuffer->Release();
	mFramebuffer->Release();

	mEmptyVao.Release();

	mSkyboxUB.Release();
	mTransformUB.Release();
	mShadingUB.Release();
	mBaseInfoUB.Release();


	mSkybox.Release();
	mPbrModel.Release();
	mGlass.Release();
	mFullScreenQuad.Release();

	mTonemapProgram.Release();
	mSkyboxProgram.Release();

	mEnvPtr->Release();
}

std::function<void (int w, int h)> Renderer::setup()
{
	// Set global OpenGL state.
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glFrontFace(GL_CCW);

	// Create empty VAO for rendering full screen triangle
	mEmptyVao = MeshGeometry(nullptr, true);

	// Create uniform buffers.
	mSkyboxUB.Create();
	mTransformUB.Create();
	mShadingUB.Create();
	mBaseInfoUB.Create();
	// Load assets & compile/link rendering programs
	mTonemapProgram =
		ShaderProgram{{ std::make_tuple(GL_VERTEX_SHADER, Shader::GetFileContents("shaders/tonemap_vs.glsl")),
						std::make_tuple(GL_FRAGMENT_SHADER, Shader::GetFileContents("shaders/tonemap_fs.glsl")) }};

	mSkyboxProgram =
		ShaderProgram{{ std::make_tuple(GL_VERTEX_SHADER, Shader::GetFileContents("shaders/skybox_vs.glsl")),
						std::make_tuple(GL_FRAGMENT_SHADER, Shader::GetFileContents("shaders/skybox_fs.glsl")) }};

	mEnvPtr = std::make_shared<Environment>(Image::fromFile("environment.hdr", 3));

	mSkybox   = MeshGeometry{ Mesh::fromFile("meshes/skybox.obj") };
	mPbrModel = PbrMesh{ Mesh::fromFile("meshes/cerberus2.fbx"), mEnvPtr };
	mGlass    = PbrMesh{ Mesh::fromFile("meshes/plate.fbx"), mEnvPtr };

	return [&](int w, int h) { glViewport(0, 0, w, h); };
}

void Renderer::renderScene(const ViewSettings& view, const SceneSettings& scene)
{
	// update uniforms
	auto &transformUniforms = mTransformUB.GetReference();
	transformUniforms.modelMatrix = /*glm::translate(glm::mat4{ 1.0f }, { 0.f, 0.0f, 40.0f })
									* */glm::eulerAngleXY(glm::radians(scene.pitch), glm::radians(scene.yaw));
	mTransformUB.Bind(0);	// Update and bind uniform buffer

	mPbrModel.Render();

// 		// update uniforms
// 		transformUniforms.modelMatrix = /*glm::translate(glm::mat4{ 1.0f }, { 10.f, 0.0f, 0.0f })
// 										* */glm::eulerAngleXY(glm::radians(scene.pitch), glm::radians(scene.yaw));
// 		mTransformUB.Bind(0);	// Update and bind uniform buffer

	mGlass.Render();
}

void Renderer::render(GLFWwindow* window, const ViewSettings& view, const SceneSettings& scene)
{
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

	mFramebuffer->ResizeAll(fbWidth, fbHeight);
	mResolveFramebuffer->ResizeAll(fbWidth, fbHeight);

	const auto &colorRb = mFramebuffer->GetRenderTarget(GL_COLOR_ATTACHMENT0);
	assert(colorRb->GetWidth() == fbWidth);
	assert(colorRb->GetHeight() == fbHeight);

	const glm::mat4 projectionMatrix = glm::perspectiveFov(glm::radians(view.fov), float(colorRb->GetWidth()), float(colorRb->GetHeight()), 1.f, 10000.0f);
// 	const glm::mat4 projectionMatrix = glm::perspective<float>(glm::radians(view.fov), float(colorRb->GetWidth()) / float(colorRb->GetHeight()), 0.1f, 1000.0f);
	const glm::mat4 viewRotationMatrix = glm::eulerAngleXY(glm::radians(view.pitch), glm::radians(view.yaw));
	const glm::mat4 viewMatrix = glm::translate(glm::mat4{ 1.0f }, { 0.0f, 0.0f, -view.distance }) * viewRotationMatrix;

	// Prepare framebuffer for rendering
	mFramebuffer->Bind();
	// opaque pass
	glDepthMask(GL_TRUE);								// enable write to depth buffer to clear it
	glClearColor(0.f, 0.f, 0.f, 0.f);					// zero color buffer, necessary only for transparency targets,
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	//  since every pixel will be overwritten by skybox

	// Draw skybox
	glDisable(GL_BLEND);								// disable blending
	glDepthMask(GL_FALSE);								// disable write to depth buffer for skybox
	glDisable(GL_DEPTH_TEST);

	// Update skybox uniform buffer
	{
		auto &skyboxUniforms = mSkyboxUB.GetReference();
		skyboxUniforms.skyViewProjectionMatrix = projectionMatrix * viewRotationMatrix;
		mSkyboxUB.Bind(0);
	}
	mSkyboxProgram.Use();
	mEnvPtr->BindTextureUnit(0);
	mSkybox.Render();

	// Update shading uniform buffer
	{
		const glm::vec3 eyePosition = glm::vec3{ glm::inverse(viewMatrix) * glm::vec4{ 0, 0, 0, 1.f } };
		auto &shadingUniforms = mShadingUB.GetReference();
		shadingUniforms.eyePosition = glm::vec4(eyePosition, 0.0f);
		for (int i = 0; i < SceneSettings::NumLights; ++i)
		{
			const SceneSettings::Light& light = scene.lights[i];
			shadingUniforms.lights[i].direction = glm::vec4{light.direction, 0.0f};
			shadingUniforms.lights[i].radiance = light.enabled ? glm::vec4{light.radiance, 0.0f} : glm::vec4{};
		}
		mShadingUB.Bind(1);					// update and bind uniform buffers
	}

	glDepthMask(GL_TRUE);					// enable write to depth buffer to clear it
	glEnable(GL_DEPTH_TEST);

	// Update transform uniform buffer
	{
		auto &transformUniforms = mTransformUB.GetReference();
		transformUniforms.viewProjectionMatrix = projectionMatrix * viewMatrix;
		transformUniforms.modelMatrix = glm::translate(glm::mat4{ 1.0f }, { 10.f, 0.0f, 0.0f })
										* glm::eulerAngleXY(glm::radians(scene.pitch), glm::radians(scene.yaw));
	}

	// Update base info buffer
	{
		auto &baseInfoUniforms = mBaseInfoUB.GetReference();
		baseInfoUniforms.opaquePass = 1;	// don't draw transparent geometry
		mBaseInfoUB.Bind(2);
	}
	// Draw scene
	renderScene(view, scene);

	// transparency pass (Order Independent Transparency (OIT), see https://developer.download.nvidia.com/SDK/10/opengl/src/dual_depth_peeling/doc/DualDepthPeeling.pdf)
	glDepthMask(GL_FALSE);					// do not write new data to depth buffer
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);						// enable blending so that transparent geometry sum up
	glBlendFunc(GL_ONE, GL_ONE);			// weights for data in FB and new data

	{
		auto &baseInfoUniforms = mBaseInfoUB.GetReference();
		baseInfoUniforms.opaquePass = 0;	// draw transparent geometry
		mBaseInfoUB.Bind(2);				// update and bind uniform buffer
	}

	// Draw scene
	renderScene(view, scene);

	mFramebuffer->Unbind();

	glDisable(GL_BLEND);					// disable blending

	// Resolve multisample framebuffer (copy renderbuffers to textures)
	mFramebuffer->Resolve(*mResolveFramebuffer,
		{
			{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0, GL_COLOR_BUFFER_BIT, GL_NEAREST },
			{ GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT1, GL_COLOR_BUFFER_BIT, GL_NEAREST },
			{ GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT2, GL_COLOR_BUFFER_BIT, GL_NEAREST },
		});
	mFramebuffer->InvalidateAttachments({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 });

	// Draw a full screen triangle for postprocessing/tone mapping
	mTonemapProgram.Use();
	try
	{
		std::dynamic_pointer_cast<const Texture>(mResolveFramebuffer->GetRenderTarget(GL_COLOR_ATTACHMENT0))->BindTextureUnit(0);
		std::dynamic_pointer_cast<const Texture>(mResolveFramebuffer->GetRenderTarget(GL_COLOR_ATTACHMENT1))->BindTextureUnit(1);
		std::dynamic_pointer_cast<const Texture>(mResolveFramebuffer->GetRenderTarget(GL_COLOR_ATTACHMENT2))->BindTextureUnit(2);
		mEmptyVao.Render();
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}

	glfwSwapBuffers(window);
}

#if _DEBUG
void Renderer::logMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	const std::unordered_map<GLenum, std::string> errorType =
	{
		{ GL_DEBUG_TYPE_ERROR, "** GL ERROR **" },
		{ GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "** DEPRECATED BEHAVIOUR **" },
		{ GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "** UNDEFINED BEHAVIOUR **" },
		{ GL_DEBUG_TYPE_PORTABILITY, "** PORTABILITY **" },
		{ GL_DEBUG_TYPE_PERFORMANCE, "** PERFORMANCE **" },
		{ GL_DEBUG_TYPE_OTHER, "** OTHER **" },
	};
	const std::unordered_map<GLenum, std::string> errorSeverity =
	{
		{ GL_DEBUG_SEVERITY_HIGH, "high severity"},
		{ GL_DEBUG_SEVERITY_MEDIUM, "medium severity"},
		{ GL_DEBUG_SEVERITY_LOW, "low severity"},
		{ GL_DEBUG_SEVERITY_NOTIFICATION, "notification"},
	};
	std::cerr << "GL CALLBACK: " << errorType.at(type) << " (" << errorSeverity.at(severity) << "), message = " << message << std::endl;
}
#endif

} // OpenGL

