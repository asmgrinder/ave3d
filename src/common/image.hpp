/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 */

#pragma once

#include <cassert>
#include <memory>
#include <string>

class Image
{
public:
	~Image();

	static std::shared_ptr<Image> fromFile(const std::string& filename, int channels = 4);

	int width() const { return m_width; }
	int height() const { return m_height; }
	int channels() const { return m_channels; }
	int bytesPerPixel() const { return m_channels * (m_hdr ? sizeof(float) : sizeof(unsigned char)); }
	int pitch() const { return m_width * bytesPerPixel(); }

	bool isHDR() const { return m_hdr; }

	template<typename T>
	const T* pixels() const
	{
		return reinterpret_cast<const T*>(m_pixels);
	}

private:
	Image();

	int m_width;
	int m_height;
	int m_channels;
	bool m_hdr;
	void *m_pixels;
};
