#include "CharacterController.h"

namespace giewont {

CharacterMovementCommand operator|(CharacterMovementCommand a,
                                   CharacterMovementCommand b) {
  return static_cast<CharacterMovementCommand>(static_cast<int>(a) |
                                               static_cast<int>(b));
}

bool operator&(CharacterMovementCommand a, CharacterMovementCommand b) {
  return static_cast<int>(a) & static_cast<int>(b);
}

CharacterMovementCommand operator|=(CharacterMovementCommand &a,
                                    CharacterMovementCommand b) {
  a = static_cast<CharacterMovementCommand>(static_cast<int>(a) |
                                            static_cast<int>(b));
  return a;
}

}; // namespace giewont
