#include "raylib.h"
#include <cstdint>
#include <iostream>
#include <vector>

// Function to convert RGB uint8_t* image to a Texture2D
Texture2D CreateTextureFromRGBData(const uint8_t* imageData, int width, int height) {
    // Create an empty Image object
    Image image = {};
    image.data = malloc(width * height * 3);  // Allocate memory for RGB data
    if (!image.data) {
        std::cerr << "Failed to allocate memory for the image!\n";
        exit(EXIT_FAILURE);
    }

    // Copy the image data into the Image object
    memcpy(image.data, imageData, width * height * 3);
    image.width = width;
    image.height = height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8; // Specify RGB format

    // Generate a texture from the image
    Texture2D texture = LoadTextureFromImage(image);

    // Free the temporary image data
    UnloadImage(image);

    return texture;
}

int main() {
    // Initialize raylib
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "raylib - Draw RGB Image");

    // Dummy RGB image data (a simple gradient for demonstration purposes)
    const int imageWidth = 256;
    const int imageHeight = 256;
    std::vector<uint8_t> imageData(imageWidth * imageHeight * 3);

    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            int index = (y * imageWidth + x) * 3;
            imageData[index] = static_cast<uint8_t>((x / (float)imageWidth) * 255);      // Red gradient
            imageData[index + 1] = static_cast<uint8_t>((y / (float)imageHeight) * 255); // Green gradient
            imageData[index + 2] = 128;                                                // Constant blue
        }
    }

    // Create a texture from the RGB image data
    Texture2D texture = CreateTextureFromRGBData(imageData.data(), imageWidth, imageHeight);

    // Main game loop
    while (!WindowShouldClose()) {
        // Draw
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText("RGB Image Example", 10, 10, 20, DARKGRAY);

        // Draw the texture centered on the screen
        DrawTexture(texture, (screenWidth - imageWidth) / 2, (screenHeight - imageHeight) / 2, WHITE);

        EndDrawing();
    }

    // Clean up
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}
