/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 */

#include <stdexcept>
#include <stb_image.h>

#include "image.hpp"
#include <iostream>

Image::Image()
	: m_width(0)
	, m_height(0)
	, m_channels(0)
	, m_hdr(false)
{
}

Image::~Image()
{
}


std::shared_ptr<Image> Image::fromFile(const std::string& filename, int channels)
{
	std::cout << "Loading image: " << filename << std::endl;

	std::shared_ptr<Image> image { new Image };

	if (stbi_is_hdr(filename.c_str()))
	{
		std::shared_ptr<void> pixels = 
			std::shared_ptr<void>(stbi_loadf(filename.c_str(), &image->m_width, &image->m_height, &image->m_channels, channels),
				[](void *Ptr) { if (nullptr != Ptr) { stbi_image_free(Ptr); } });
		if (pixels)
		{
			image->m_pixels = pixels;
			image->m_hdr = true;
		}
	}
	else
	{
		std::shared_ptr<void> pixels = 
			std::shared_ptr<void>(stbi_load(filename.c_str(), &image->m_width, &image->m_height, &image->m_channels, channels),
				[](void *Ptr) { if (nullptr != Ptr) { stbi_image_free(Ptr); } });
		if (pixels)
		{
			image->m_pixels = pixels;
			image->m_hdr = false;
		}
	}
	if (channels > 0)
	{
		image->m_channels = channels;
	}

	if (!image->m_pixels)
	{
		throw std::runtime_error("Failed to load image file: " + filename);
	}
	return image;
}
