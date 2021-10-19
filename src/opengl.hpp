	/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 *
 * OpenGL 4.5 renderer.
 */

#pragma once

#include <glad/glad.h>

#include "common/image.hpp"
#include "common/utils.hpp"
#include "common/renderer.hpp"
#include "common/mesh.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <unordered_map>

namespace OpenGL {

class NonCopyable
{
public:
	NonCopyable() {}
	virtual ~NonCopyable() { Release(); }

	virtual void Release() {}

protected:
	NonCopyable(const NonCopyable &) = delete;
	void operator = (const NonCopyable &) = delete;
};

class Shader : public NonCopyable
{
public:
	Shader(GLenum ShaderType, const std::string ShaderSourceStr)
	{

		mShader = glCreateShader(ShaderType);
		const GLchar *const p = &ShaderSourceStr[0];
		glShaderSource(mShader, 1, &p, NULL);
		glCompileShader(mShader);

		int  success;
		glGetShaderiv(mShader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			std::vector<char> infoLog(512);
			glGetShaderInfoLog(mShader, infoLog.size() - 1, NULL, &infoLog[0]);
			const std::unordered_map<GLenum, std::string> typeNameStr =
			{
				{ GL_VERTEX_SHADER, "vertex shader" },
				{ GL_FRAGMENT_SHADER, "fragment shader" },
				{ GL_GEOMETRY_SHADER, "geometry shader" },
				{ GL_COMPUTE_SHADER, "compute shader" }
			};

			std::string tn = typeNameStr.at(ShaderType);
			std::cout << "ERROR:shader compilation failed: " << std::endl << tn << std::endl << &infoLog[0] << std::endl;
		}
	}

	Shader(Shader &&Other)
		: mShader(std::move(Other.mShader))
	{
		Other.mShader = 0;
	}

	Shader &operator = (Shader &&Other)
	{
		if (&Other != this)
		{
			Release();

			std::swap(mShader, Other.mShader);
		}

		return *this;
	}

	bool IsUsable() const { return 0 != mShader; }

	void AttachTo(GLuint Program) const
	{
		glAttachShader(Program, mShader);
	}

	static std::string GetFileContents(std::string PathStr)
	{
		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			file.open(PathStr);
			std::stringstream stream;
			stream << file.rdbuf();
			file.close();
			return stream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR: getShaderFileContents: read failed (" << e.what() << ")" << std::endl;
		}
		return "";
	}

	void Release() override
	{
		glDeleteShader(mShader);
		mShader = 0;
	}

protected:
	GLuint mShader;
};

class ShaderProgram : public NonCopyable
{
public:
	using List = std::initializer_list<std::tuple<GLenum, std::string>>;

	ShaderProgram()
		: mProgram(0)
	{
	}

	ShaderProgram(ShaderProgram &&Other)
		: mProgram(Other.mProgram)
	{
		Other.mProgram = 0;
	}

	ShaderProgram &operator = (ShaderProgram &&Other)
	{
		if (&Other != this)
		{
			Release();

			std::swap(mProgram, Other.mProgram);
		}

		return *this;
	}

	ShaderProgram(const List &ShaderList)
	{
		mProgram = glCreateProgram();
		std::vector<Shader> shaderList;
		for (const auto &shader : ShaderList)
		{
			shaderList.emplace_back(std::get<0>(shader), std::get<1>(shader));
		}
		for (const auto &shader : shaderList)
		{
			shader.AttachTo(mProgram);
		}

		glLinkProgram(mProgram);

		int success;
		glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
		if (!success)
		{
			std::vector<char> infoLog(512);
			glGetProgramInfoLog(mProgram, infoLog.size(), NULL, &infoLog[0]);
			std::cout << "ERROR: shader program compilation failed" << std::endl << &infoLog[0] << std::endl;
		}
	}

	bool IsUsable() const { return 0 != mProgram; }

	void Use() const
	{
		if (IsUsable())
		{
			glUseProgram(mProgram);
		}
	}

	static void DispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)
	{
		glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
	}

	void Release() override
	{
		glDeleteProgram(mProgram);
		mProgram = 0;
	}

	void SetFloat(GLint location, GLfloat v0)
	{
		glProgramUniform1f(mProgram, location, v0);
	}

	void SetVector(GLint location, glm::vec2 v0)
	{
		glProgramUniform2f(mProgram, location, v0.x, v0.y);
	}

	void SetVector(GLint location, glm::vec3 v0)
	{
		glProgramUniform3f(mProgram, location, v0.x, v0.y, v0.z);
	}

	void SetVector(GLint location, glm::vec4 v0)
	{
		glProgramUniform4f(mProgram, location, v0.x, v0.y, v0.z, v0.w);
	}

protected:
	GLuint mProgram;
};

class RenderTarget : public NonCopyable
{
public:
	RenderTarget() : NonCopyable(), mId(0), mWidth(0), mHeight(0) {}

	~RenderTarget() override { Release(); }

	RenderTarget(RenderTarget &&Other)
		: mId(Other.mId), mWidth(Other.mWidth), mHeight(Other.mHeight)
	{
		Other.mId = 0;
		Other.mWidth = Other.mHeight = 0;
	}

	RenderTarget &operator = (RenderTarget &&Other)
	{
		if (&Other != this)
		{
			Release();

			std::swap(mId, Other.mId);
			std::swap(mWidth, Other.mWidth);
			std::swap(mHeight, Other.mHeight);
		}
		return *this;
	}

	void Release() override
	{
		mId = 0;
		mWidth = 0;
		mHeight = 0;
	}

	virtual bool IsUsable() { return 0 != mId; }
	virtual GLint GetWidth() const { return mWidth; }
	virtual GLint GetHeight() const { return mHeight; }
	virtual void AttachTo(GLuint /*Fb*/, GLenum /*Attachment*/) const = 0;

protected:
	GLuint mId;
	GLint mWidth, mHeight;
};

class Texture : public RenderTarget
{
public:
	Texture()
		: RenderTarget()
	{
		mLevels = 0;
	}

	~Texture() override { Release(); }

	Texture(Texture &&Other)
		: RenderTarget(std::move(Other)), mLevels(Other.mLevels)
	{
		Other.mLevels = 0;
	}

	Texture &operator = (Texture &&Other)
	{
		if (&Other != this)
		{
			Release();

			RenderTarget::operator = (std::move(Other));
			std::swap(mLevels, Other.mLevels);
		}
		return *this;
	}

	Texture(GLenum Target)
		: RenderTarget()
	{
		mLevels = 0;
		glCreateTextures(Target, 1, &mId);
	}

	Texture(GLenum Target, int Width, int Height, GLenum InternalFormat, int Levels = 0)
	{
		createTexture(Target, Width, Height, InternalFormat, Levels);
	}

	Texture(const std::shared_ptr<class Image>& Img, GLenum Format, GLenum InternalFormat, int Levels = 0)
	{
		createTexture(GL_TEXTURE_2D, Img->width(), Img->height(), InternalFormat, Levels);
		glTextureSubImage2D(mId, 0, 0, 0, mWidth, mHeight, Format,
							Img->isHDR() ? GL_FLOAT : GL_UNSIGNED_BYTE,
							Img->pixels<void>());
		if (mLevels > 1)
		{
			GenerateMipmap();
		}
	}

	Texture(GLenum Target, GLint Width, GLint Height, GLenum Format, GLenum InternalFormat,
			int Levels = 0, GLenum Type = GL_UNSIGNED_BYTE, void *DataPtr = nullptr)
	{
		createTexture(Target, Width, Height, InternalFormat, Levels);
		glTextureSubImage2D(mId, 0, 0, 0, mWidth, mHeight, Format, Type, DataPtr);
		if (mLevels > 1)
		{
			GenerateMipmap();
		}
	}

	GLint GetLevels() const { return mLevels; }

	void AttachTo(GLuint Fb, GLenum Attachment) const override
	{
		glNamedFramebufferTexture(Fb, Attachment, mId, 0);
	}

	void Storage(GLenum InternalFormat, GLint Width, GLint Height, GLint Levels)
	{
		if (0 == mWidth && 0 == mHeight)
		{
			glTextureStorage2D(mId, Levels, InternalFormat, Width, Height);
			mWidth = Width;
			mHeight = Height;
			mLevels = Levels;
		}
	}

	void BindTextureUnit(GLuint unit) const
	{
		glBindTextureUnit(unit, mId);
	}

	void BindImageTexture(GLuint Unit, GLint Level, GLboolean Layered, GLint Layer, GLenum Access, GLenum Format) const
	{
		glBindImageTexture(Unit, mId, Level, Layered, Layer, Access, Format);
	}

	void GenerateMipmap() const
	{
		glGenerateTextureMipmap(mId);
	}

	void CopyImageSubData(GLenum SrcTarget, GLint SrcLevel, GLint SrcX, GLint SrcY, GLint SrcZ,
		const Texture &DstTex, GLenum DstTarget, GLint DstLevel, GLint DstX, GLint DstY, GLint DstZ,
		GLsizei SrcDepth) const
	{
		glCopyImageSubData(mId, SrcTarget, SrcLevel, SrcX, SrcY, SrcZ, DstTex.mId, DstTarget, DstLevel, DstX, DstY, DstZ, mWidth, mHeight, SrcDepth);
	}

	void SetWrap(GLint WrapS, GLint WrapT) const
	{
		glTextureParameteri(mId, GL_TEXTURE_WRAP_S, WrapS);
		glTextureParameteri(mId, GL_TEXTURE_WRAP_T, WrapT);
	}

	void Release() override
	{
		if (0 != mId)
		{
			glDeleteTextures(1, &mId);
			mId = 0;
			RenderTarget::Release();
		}
		mLevels = 0;
	}

protected:
	void createTexture(GLenum Target, int Width, int Height, GLenum InternalFormat, int Levels = 0)
	{
		mWidth  = Width;
		mHeight = Height;
		mLevels = (Levels > 0) ? Levels : int(1. + log(std::max(Width, Height)) / log(2.f));//Utility::numMipmapLevels(Width, Height);

		glCreateTextures(Target, 1, &mId);
		glTextureStorage2D(mId, mLevels, InternalFormat, mWidth, mHeight);
		glTextureParameteri(mId, GL_TEXTURE_MIN_FILTER, (mLevels > 1) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTextureParameteri(mId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		static float maxAnisotropy = -1;
		if (maxAnisotropy < 0)
		{
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
		}
		glTextureParameterf(mId, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	}

	GLint mLevels;
};

class Environment : public Texture
{
protected:
	static constexpr int kEnvMapSize = 1024;
	static constexpr int kIrradianceMapSize = 32;
	static constexpr int kBRDF_LUT_Size = 256;

public:
	Environment()
		: Texture() {}

	~Environment() override { Release(); }

	Environment(Environment &&Other)
		: Texture(std::move(Other))
			, mIrmap(std::move(Other.mIrmap))
			, mSpBrdfLut(std::move(Other.mSpBrdfLut))
	{
	}

	Environment &operator = (Environment &&Other)
	{
		if (&Other != this)
		{
			Release();

			Texture::operator = (std::move(Other));
			mIrmap = std::move(Other.mIrmap);
			mSpBrdfLut = std::move(Other.mSpBrdfLut);
		}
		return *this;
	}

	Environment(const std::shared_ptr<class Image>& Img)
		: Texture(GL_TEXTURE_CUBE_MAP, kEnvMapSize, kEnvMapSize, GL_RGBA16F)
	{	//------------------------------------------------------------------------------------------------------------------
		Texture envTextureEquirect{ Img, GL_RGB, GL_RGB16F, 1 };
		Texture envTextureUnfiltered{ GL_TEXTURE_CUBE_MAP, kEnvMapSize, kEnvMapSize, GL_RGBA16F };
		ShaderProgram equirectToCubeProgram =
			ShaderProgram{{ std::make_tuple(GL_COMPUTE_SHADER, Shader::GetFileContents("shaders/equirect2cube_cs.glsl")) }};

		equirectToCubeProgram.Use();
		envTextureEquirect.BindTextureUnit(0);
		envTextureUnfiltered.BindImageTexture(0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

		ShaderProgram::DispatchCompute(envTextureUnfiltered.GetWidth() / 32, envTextureUnfiltered.GetHeight() / 32, 6);

		envTextureEquirect.Release();
		equirectToCubeProgram.Release();
		envTextureUnfiltered.GenerateMipmap();
		//-------------------------------------------------------------------------------------------------------------------
		ShaderProgram spmapProgram =
			ShaderProgram{{ std::make_tuple(GL_COMPUTE_SHADER, Shader::GetFileContents("shaders/spmap_cs.glsl")) }};

		// Copy 0th mipmap level into destination environment map
		envTextureUnfiltered.CopyImageSubData(GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0, /*m_envTexture*/
											  *this, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0, 6);

		spmapProgram.Use();
		envTextureUnfiltered.BindTextureUnit(0);

		// Pre-filter rest of the mip chain.
		const float deltaRoughness = 1.0f / glm::max(float(/*m_envTexture.*/this->GetLevels() - 1), 1.0f);
		for (int level = 1, size = kEnvMapSize / 2; level <= /*m_envTexture.*/this->GetLevels(); ++level, size /= 2)
		{
			const GLuint numGroups = glm::max(1, size / 32);
			/*m_envTexture.*/this->BindImageTexture(0, level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			spmapProgram.SetFloat(0, level * deltaRoughness);
			ShaderProgram::DispatchCompute(numGroups, numGroups, 6);
		}
		spmapProgram.Release();
		envTextureUnfiltered.Release();
		//-------------------------------------------------------------------------------------------------------------------
		mIrmap = Texture{ GL_TEXTURE_CUBE_MAP, kIrradianceMapSize, kIrradianceMapSize, GL_RGBA16F, 1 };

		ShaderProgram irmapProgram =
			ShaderProgram{{ std::make_tuple(GL_COMPUTE_SHADER, Shader::GetFileContents("shaders/irmap_cs.glsl")) }};

		irmapProgram.Use();
		/*m_envTexture.*/BindTextureUnit(0);
		mIrmap.BindImageTexture(0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		ShaderProgram::DispatchCompute(mIrmap.GetWidth() / 32, mIrmap.GetHeight() / 32, 6);
		irmapProgram.Release();
		//-------------------------------------------------------------------------------------------------------------------
		mSpBrdfLut = Texture{ GL_TEXTURE_2D, kBRDF_LUT_Size, kBRDF_LUT_Size, GL_RG16F, 1 };
		mSpBrdfLut.SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		ShaderProgram spBRDFProgram =
			ShaderProgram{{ std::make_tuple(GL_COMPUTE_SHADER, Shader::GetFileContents("shaders/spbrdf_cs.glsl")) }};

		spBRDFProgram.Use();
		mSpBrdfLut.BindImageTexture(0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);
		ShaderProgram::DispatchCompute(mSpBrdfLut.GetWidth() / 32, mSpBrdfLut.GetHeight() / 32, 1);
		spBRDFProgram.Release();
		//-------------------------------------------------------------------------------------------------------------------
		glFinish();
	}

	void Release() override
	{
		Texture::Release();
		mIrmap.Release();
		mSpBrdfLut.Release();
	}

	const Texture &GetIrmapTexture() const { return mIrmap; }
	const Texture &GetSpBrdfLutTexture() const { return mSpBrdfLut; }

protected:
	Texture mIrmap, mSpBrdfLut;
};

class Renderbuffer : public RenderTarget
{
public:
	Renderbuffer()
		: RenderTarget()
	{
		glCreateRenderbuffers(1, &mId);
	}

	~Renderbuffer() override { Release(); }

// 	Renderbuffer(GLenum Format, GLint Width, GLint Height, GLint Samples = 0)
// 		: RenderTarget()
// 	{
// 		Storage(Format, Width, Height, Samples);
// 	}

	Renderbuffer(Renderbuffer &&Other)
		: RenderTarget(std::move(Other))
	{
	}

	Renderbuffer &operator = (Renderbuffer &&Other)
	{
		if (&Other != this)
		{
			Release();

			RenderTarget::operator = (std::move(Other));
		}

		return *this;
	}

	void AttachTo(GLuint Fb, GLenum Attachment) const override
	{
		glNamedFramebufferRenderbuffer(Fb, Attachment, GL_RENDERBUFFER, mId);
	}

	void Storage(GLenum Format, GLint Width, GLint Height, GLint Samples = 0)
	{
		if (Samples > 0)
		{
			glNamedRenderbufferStorageMultisample(mId, Samples, Format, Width, Height);
		}
		else
		{
			glNamedRenderbufferStorage(mId, Format, Width, Height);
		}
		mWidth = Width;
		mHeight = Height;
	}

	void Release() override
	{
		if (0 != mId)
		{
			glDeleteRenderbuffers(1, &mId);
			mId = 0;
			RenderTarget::Release();
		}
	}

protected:
};

class Framebuffer : public NonCopyable
{
protected:
	enum RenderTargetType { TypeRenderBuffer = 0, TypeTexture };
public:
	Framebuffer()
	{
		glCreateFramebuffers(1, &mId);
	}

	~Framebuffer() override { Release(); }

	Framebuffer(Framebuffer &&Other)
		: mId(Other.mId)
	{
		Other.mId = 0;
	}

	Framebuffer & operator = (Framebuffer && Other)
	{
		if (&Other != this)
		{
			Release();

			std::swap(mId, Other.mId);
		}
		return *this;
	}

	GLuint GetId() { return mId; }

	void Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mId);
	}

	void Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void AttachRenderbuffer(GLenum Attachment, GLenum Format, GLint Width, GLint Height, GLint Samples = 0)
	{
		mRbParams[Attachment] = std::make_tuple(RenderTargetType::TypeRenderBuffer, Format, Samples);
		recreateIfNeeded(Attachment, Width, Height);
		updateDrawBuffers();
	}

	void AttachTexture(GLenum Attachment, GLenum Format, GLint Width, GLint Height)
	{
		mRbParams[Attachment] = std::make_tuple(RenderTargetType::TypeTexture, Format, 0);
		recreateIfNeeded(Attachment, Width, Height);
		updateDrawBuffers();
	}

	void ResizeAll(GLint Width, GLint Height)
	{
		for (const auto &rb : mRenderbuffers)
		{
			recreateIfNeeded(rb.first, Width, Height);
		}
	}

	const std::shared_ptr<const RenderTarget> GetRenderTarget(GLenum Attachment) const
	{
		return mRenderbuffers.at(Attachment);
	}

	GLenum CheckStatus() const
	{
		return glCheckNamedFramebufferStatus(mId, GL_DRAW_FRAMEBUFFER);
	}

	void Blit(glm::ivec2 SrcP0, glm::ivec2 SrcP1, const Framebuffer& Dst, glm::ivec2 DstP0, glm::ivec2 DstP1, GLbitfield Mask, GLenum Filter) const
	{
		glBlitNamedFramebuffer(mId, Dst.mId, SrcP0.x, SrcP0.y, SrcP1.x, SrcP1.y, DstP0.x, DstP0.y, DstP1.x, DstP1.y, Mask, Filter);
	}

	void InvalidateAttachments(std::vector<GLenum> Attachments) const
	{
		glInvalidateNamedFramebufferData(mId, GLsizei(Attachments.size()), (Attachments.size() > 0) ? &Attachments[0] : nullptr);
	}

	void ReadBuffer(GLenum Attachment) const
	{
		glNamedFramebufferReadBuffer(mId, Attachment);
	}

	void DrawBuffer(GLenum Attachment) const
	{
		glNamedFramebufferDrawBuffer(mId, Attachment);
	}

	void DrawBuffers(std::vector<GLenum> Buffers) const
	{
		glNamedFramebufferDrawBuffers(mId, Buffers.size(), Buffers.size() ? &Buffers[0] : nullptr);
	}

	void Resolve(const Framebuffer& Dst, std::vector<std::tuple<GLenum, GLenum, GLbitfield, GLenum>> List) const
	{
		for (const auto & copyParams: List)
		{
			auto attach1 = std::get<0>(copyParams);
			auto attach2 = std::get<1>(copyParams);
			auto bitfieldMask = std::get<2>(copyParams);
			auto filter = std::get<3>(copyParams);
			const auto &rt1 = GetRenderTarget(attach1);
			const auto &rt2 = Dst.GetRenderTarget(attach2);
			ReadBuffer(attach1);
			Dst.DrawBuffer(attach2);
			Blit(glm::ivec2{0, 0}, glm::ivec2{rt1->GetWidth(), rt1->GetHeight()},
				 Dst, glm::ivec2{0, 0}, glm::ivec2{rt2->GetWidth(), rt2->GetHeight()}, bitfieldMask, filter);
		}
	}

	void Release() override
	{
		if (0 != mId)
		{
			mRenderbuffers.clear();
			glDeleteFramebuffers(1, &mId);
			mId = 0;
		}
	}

protected:
	void updateDrawBuffers()
	{
		std::vector<GLenum> buffers;
		for (const auto &buffer : mRenderbuffers)
		{
			if (GL_DEPTH_ATTACHMENT != buffer.first)
			{
				buffers.emplace_back(buffer.first);
			}
		}
		glNamedFramebufferDrawBuffers(mId, buffers.size(), buffers.size() ? &buffers[0] : nullptr);
	}

	void recreateIfNeeded(GLenum Attachment, GLint Width, GLint Height)
	{
		if (0 == mRenderbuffers.count(Attachment)
			|| mRenderbuffers[Attachment]->GetWidth() != Width
			|| mRenderbuffers[Attachment]->GetHeight() != Height)
		{
			const auto &params = mRbParams[Attachment];
			const auto type = std::get<0>(params);
			const auto format = std::get<1>(params);
			const auto samples = std::get<2>(params);
			switch (type)
			{
				case RenderTargetType::TypeRenderBuffer:
					{
						auto renderbufferPtr = std::make_shared<Renderbuffer>();
						mRenderbuffers[Attachment] = std::static_pointer_cast<RenderTarget>(renderbufferPtr);
						renderbufferPtr->Storage(format, Width, Height, samples);
					}
					break;
				case RenderTargetType::TypeTexture:
					{
						auto texturePtr = std::make_shared<Texture>(GL_TEXTURE_2D);
						mRenderbuffers[Attachment] = std::static_pointer_cast<RenderTarget>(texturePtr);
						texturePtr->Storage(format, Width, Height, 1);
					}
					break;
			}
			mRenderbuffers[Attachment]->AttachTo(mId, Attachment);
		}
	}

	GLuint mId;
	std::unordered_map<GLenum, std::shared_ptr<RenderTarget>> mRenderbuffers;
	std::unordered_map<GLenum, std::tuple<RenderTargetType, GLenum, GLint>> mRbParams;
};

template <class T>
class UniformBuffer : public NonCopyable
{
public:
	UniformBuffer()
		: mId(0)
	{}

	UniformBuffer(UniformBuffer &&Other)
		: mId(Other.mId)
	{
		Other.mId = 0;
	}

	UniformBuffer &operator = (UniformBuffer &&Other)
	{
		if (&Other != this)
		{
			Release();

			std::swap(mId, Other.mId);
		}
		return *this;
	}

	void Create()
	{
		glCreateBuffers(1, &mId);
		glNamedBufferStorage(mId, sizeof(*this), nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	void Update()
	{
		glNamedBufferSubData(mId, 0, sizeof(T), &mData);
	}

	void Bind(GLuint Slot)
	{
		Update();//glNamedBufferSubData(mId, 0, sizeof(T), &mData);
		glBindBufferBase(GL_UNIFORM_BUFFER, Slot, mId);
	}

	T &GetReference() { return mData; }

	void Release() override
	{
		glDeleteBuffers(1, &mId);
		mId = 0;
	}

protected:
	GLuint mId;
	T mData;
};

struct MeshBuffer
{
	MeshBuffer() : vbo(0), ibo(0), vao(0), numElements(0) {}
	GLuint vbo, ibo, vao;
	GLuint numElements;
};

class MeshGeometry : public NonCopyable
{
public:
	MeshGeometry() : mEmpty(false), mVbo(0), mIbo(0), mVao(0), mNumElements(0)
	{
	}

	MeshGeometry(MeshGeometry &&Other)
		: mEmpty(Other.mEmpty), mVbo(Other.mVbo), mIbo(Other.mIbo), mVao(Other.mVao), mNumElements(Other.mNumElements)
	{
		Other.mEmpty = false;
		Other.mVao = 0;
		Other.mVbo = 0;
		Other.mIbo = 0;
		Other.mNumElements = 0;
	}

	MeshGeometry &operator = (MeshGeometry &&Other)
	{
		if (&Other != this)
		{
			Release();

			std::swap(mEmpty, Other.mEmpty);
			std::swap(mVao, Other.mVao);
			std::swap(mVbo, Other.mVbo);
			std::swap(mIbo, Other.mIbo);
			std::swap(mNumElements, Other.mNumElements);
		}
		return *this;
	}

	MeshGeometry(const std::shared_ptr<Mesh> &MeshPtr, bool FullScreenTriangle = false)
	{
		mEmpty = FullScreenTriangle;
		if (false != FullScreenTriangle)
		{
			mNumElements = 0;
			mVbo = 0;
			mIbo = 0;
			glCreateVertexArrays(1, &mVao);
// 			std::vector<glm::vec2> vert = { glm::vec2 {-1.f, -1.f}, glm::vec2{1.f, -1.f}, glm::vec2{-1.f,  1.f}, glm::vec2{1.f,  1.f} };
// 			std::vector<GLuint> ind = { 0, 1, 2, 3, 2, 1 };
//
// 			mNumElements = ind.size();
//
// 			glCreateBuffers(1, &mVbo);
// 			glNamedBufferStorage(mVbo, vert.size() * sizeof(vert[0]), reinterpret_cast<const void*>(&vert[0]), 0);
//
// 			glCreateBuffers(1, &mIbo);
// 			glNamedBufferStorage(mIbo, ind.size() * sizeof(ind[0]), reinterpret_cast<const void*>(&ind[0]), 0);
//
// 			glCreateVertexArrays(1, &mVao);
//
// 			glVertexArrayElementBuffer(mVao, mIbo);
// 			glVertexArrayVertexBuffer(mVao, 0, mVbo, 0, sizeof(vert[0]));
// 			glEnableVertexArrayAttrib(mVao, 0);
// 			glVertexArrayAttribFormat(mVao, 0, sizeof(vert[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0);
// 			glVertexArrayAttribBinding(mVao, 0, 0);
		}
		else
		{
			mNumElements = static_cast<GLuint>(MeshPtr->faces().size()) * 3;

			const size_t vertexDataSize = MeshPtr->vertices().size() * sizeof(Mesh::Vertex);
			const size_t indexDataSize  = MeshPtr->faces().size() * sizeof(Mesh::Face);

			glCreateBuffers(1, &mVbo);
			glNamedBufferStorage(mVbo, vertexDataSize, reinterpret_cast<const void*>(&MeshPtr->vertices()[0]), 0);
			glCreateBuffers(1, &mIbo);
			glNamedBufferStorage(mIbo, indexDataSize, reinterpret_cast<const void*>(&MeshPtr->faces()[0]), 0);

			glCreateVertexArrays(1, &mVao);
			glVertexArrayElementBuffer(mVao, mIbo);
			std::array<GLint, Mesh::NumAttributes> sizes =
			{
				sizeof(Mesh::Vertex::position),
				sizeof(Mesh::Vertex::normal),
				sizeof(Mesh::Vertex::tangent),
				sizeof(Mesh::Vertex::bitangent),
				sizeof(Mesh::Vertex::texcoord),
			};
			for (int i = 0; i < Mesh::NumAttributes; i++)
			{
				glVertexArrayVertexBuffer(mVao, i, mVbo, i * sizeof(glm::vec3), sizeof(Mesh::Vertex));
				glEnableVertexArrayAttrib(mVao, i);
				glVertexArrayAttribFormat(mVao, i, sizes[i] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0);
				glVertexArrayAttribBinding(mVao, i, i);
			}
		}
	}

	void Release() override
	{
		if (0 != mVao)
		{
			glDeleteVertexArrays(1, &mVao);
			mVao = 0;
		}
		if (0 != mVbo)
		{
			glDeleteBuffers(1, &mVbo);
			mVbo = 0;
		}
		if (0 != mIbo)
		{
			glDeleteBuffers(1, &mIbo);
			mIbo = 0;
		}
		mNumElements = 0;
		mEmpty = false;
	}

	void Render()
	{
		glBindVertexArray(mVao);
		if (false == mEmpty)
		{
			glDrawElements(GL_TRIANGLES, mNumElements, GL_UNSIGNED_INT, 0);
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}

protected:
	GLboolean mEmpty;
	GLuint mVbo, mIbo, mVao;
	GLuint mNumElements;
};

class PbrMesh : public MeshGeometry
{
public:
	PbrMesh()
		: MeshGeometry()
	{}

	PbrMesh(PbrMesh &&Other)
		: MeshGeometry(std::move(Other))
	{
		mAlbedo = std::move(Other.mAlbedo);
		mNormals = std::move(Other.mNormals);
		mMetalness = std::move(Other.mMetalness);
		mRoughness = std::move(Other.mRoughness);
		mEnvironmentPtr = std::move(Other.mEnvironmentPtr);
	}

	PbrMesh &operator = (PbrMesh &&Other)
	{
		if (&Other != this)
		{
			MeshGeometry::operator = (std::move(Other));
			mAlbedo = std::move(Other.mAlbedo);
			mNormals = std::move(Other.mNormals);
			mMetalness = std::move(Other.mMetalness);
			mRoughness = std::move(Other.mRoughness);
			mEnvironmentPtr = std::move(Other.mEnvironmentPtr);
		}

		return *this;
	}

	PbrMesh(const std::shared_ptr<Mesh> &MeshPtr, const std::shared_ptr<const Environment> &EnvironmentPtr = nullptr)
		: MeshGeometry(MeshPtr, false)
	{
		mEnvironmentPtr = EnvironmentPtr;
		auto albedoFileName = "textures/"+ MeshPtr->textureName(Mesh::TextureType::Albedo);
		if (albedoFileName.empty())
		{
			GLubyte pix[] = { 128, 128, 128, 255 };
			mAlbedo = Texture{ GL_TEXTURE_2D, 1, 1, GL_RGBA, GL_RGBA8, 0, GL_UNSIGNED_BYTE, &pix };
		}
		else
		{
			mAlbedo = Texture{ Image::fromFile(albedoFileName, 4), GL_RGBA, GL_SRGB8_ALPHA8 };
		}

		auto normalsFileName = "textures/"+ MeshPtr->textureName(Mesh::TextureType::Normals);
		if (normalsFileName.empty())
		{
			GLubyte pix[] = { 0, 0, 255 };
			mNormals = Texture{ GL_TEXTURE_2D, 1, 1, GL_RGB, GL_RGB8, 0, GL_UNSIGNED_BYTE, &pix };
		}
		else
		{
			mNormals = Texture{ Image::fromFile(normalsFileName, 3), GL_RGB, GL_RGB8 };
		}

		auto metalnessFileName = "textures/"+ MeshPtr->textureName(Mesh::TextureType::Metalness);
		if (metalnessFileName.empty())
		{
			GLubyte pix[] = { 128 };
			mMetalness = Texture{ GL_TEXTURE_2D, 1, 1, GL_RED, GL_R8, 0, GL_UNSIGNED_BYTE, &pix };
		}
		else
		{
			mMetalness = Texture{ Image::fromFile(metalnessFileName, 1), GL_RED, GL_R8 };
		}

		auto roughnessFileName = "textures/"+ MeshPtr->textureName(Mesh::TextureType::Roughness);
		if (roughnessFileName.empty())
		{
			GLubyte pix[] = { 128 };
			mRoughness = Texture{ GL_TEXTURE_2D, 1, 1, GL_RED, GL_R8, 0, GL_UNSIGNED_BYTE, &pix };
		}
		else
		{
			mRoughness = Texture{ Image::fromFile(roughnessFileName, 1), GL_RED, GL_R8 };
		}
	}

	void Render()
	{
		static ShaderProgram pbrProgram;
		if (!pbrProgram.IsUsable())
		{
			pbrProgram =
				ShaderProgram{{ std::make_tuple(GL_VERTEX_SHADER, Shader::GetFileContents("shaders/pbr_vs.glsl")),
								std::make_tuple(GL_FRAGMENT_SHADER, Shader::GetFileContents("shaders/pbr_fs.glsl")) }};
		}
		pbrProgram.Use();

		mAlbedo.BindTextureUnit(0);
		mNormals.BindTextureUnit(1);
		mMetalness.BindTextureUnit(2);
		mRoughness.BindTextureUnit(3);
		if (nullptr != mEnvironmentPtr)
		{
			mEnvironmentPtr->BindTextureUnit(4);
			mEnvironmentPtr->GetIrmapTexture().BindTextureUnit(5);
			mEnvironmentPtr->GetSpBrdfLutTexture().BindTextureUnit(6);
		}
		MeshGeometry::Render();
	}

protected:
	Texture mAlbedo, mNormals, mMetalness, mRoughness;
	std::shared_ptr<const Environment> mEnvironmentPtr;
};

//==========================================================================================================================
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//==========================================================================================================================
class Renderer final : public RendererInterface
{
public:
	GLFWwindow* initialize(int width, int height, int maxSamples) override;
	void shutdown() override;
	std::function<void (int w, int h)> setup() override;
	void render(GLFWwindow* window, const ViewSettings& view, const SceneSettings& scene) override;

protected:
	void renderScene(const ViewSettings& view, const SceneSettings& scene);

#if _DEBUG
	static void logMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
#endif

// 	struct
// 	{
// 		float maxAnisotropy = 1.0f;
// 	}
// 	mCapabilities;

	std::shared_ptr<Framebuffer> mFramebuffer, mResolveFramebuffer;

	MeshGeometry mFullScreenQuad;
	MeshGeometry mSkybox;
	PbrMesh mPbrModel;
	PbrMesh mGlass;

	MeshGeometry mEmptyVao;

// 	Camera mCamera { glm::vec3(0, 0.3f, 5), glm::vec3(0, 0.3f, 0), glm::vec3(0, 1, 0) };

	ShaderProgram mSkyboxProgram;
	ShaderProgram mTonemapProgram;

	std::shared_ptr<Environment> mEnvPtr;

	struct SkyboxUB
	{
		glm::mat4 skyViewProjectionMatrix;
	};
	UniformBuffer<SkyboxUB> mSkyboxUB;

	struct TransformUB
	{
		glm::mat4 viewProjectionMatrix;
		glm::mat4 modelMatrix;
	};
	UniformBuffer<TransformUB> mTransformUB;

	struct ShadingUB
	{
		struct
		{
			glm::vec4 direction;
			glm::vec4 radiance;
		} lights[SceneSettings::NumLights];
		glm::vec4 eyePosition;
	};
	UniformBuffer<ShadingUB> mShadingUB;

	struct BaseInfoUB
	{
		int opaquePass;
	};
	UniformBuffer<BaseInfoUB> mBaseInfoUB;
};

} // OpenGL
