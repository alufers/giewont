#pragma once

#include "Game.h"

namespace giewont {
class ECSContainerBase {
public:
  virtual ~ECSContainerBase() = default;

  virtual void update() {}
  virtual void draw() {}
};

} // namespace giewont
