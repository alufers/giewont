#pragma once
#include "Game.h"

namespace giewont {
    class DrawableGame : public Game {
    public:
        DrawableGame();
        virtual void draw() = 0;
    };
}
