#include "BitmapImage.h"

using namespace giewont::gfx::media;

BitmapImage::BitmapImage(std::size_t width, std::size_t height, PixelFormat format)
    : m_width(width), m_height(height), m_format(format)
{
    m_data.resize(width * height * 3);
}

BitmapImage::~BitmapImage()
{
}
