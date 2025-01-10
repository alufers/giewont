#pragma once

#include <vector>
#include <cstdint>

namespace giewont::gfx::media
{

    
    enum class PixelFormat {
        RGB24,
        RGBA32,
    };

    /**
     * @brief Represents a bitmap image in memory.
     */
    class BitmapImage
    {

        
    private:
        std::vector<std::uint8_t> m_data;
        std::size_t m_width = 0;
        std::size_t m_height = 0;
        PixelFormat m_format = PixelFormat::RGBA32;

    public:
        BitmapImage(std::size_t width, std::size_t height, PixelFormat format);
        BitmapImage(std::size_t width, std::size_t height, PixelFormat format, const std::vector<std::uint8_t>& data);
        ~BitmapImage();
    };

}
