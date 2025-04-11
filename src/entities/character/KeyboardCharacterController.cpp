#include "KeyboardCharacterController.h"
#include "CharacterEntity.h"
#include "Log.h"
#include "schema.capnp.h"
#include <array>
#include <utility>

using namespace giewont;

#ifdef GIEWONT_HAS_GRAPHICS
constexpr std::array<std::pair<int, net::InteractionType>, 2> interaction_map =
    {std::make_pair(KEY_E, net::InteractionType::USE_INTERACTION),
     std::make_pair(KEY_Q, net::InteractionType::THROW_INTERACTION)};
#endif

void KeyboardCharacterController::update(Game &game, CharacterEntity &character,
                                         float delta_time) {

  if (game.is_server()) {
    LOG_WARN() << "KeyboardCharacterController::update: called on server"
               << std::endl;
  }
#ifdef GIEWONT_HAS_GRAPHICS
  CharacterMovementCommand command = giewont::CharacterMovementCommand::NONE;
  if (IsKeyDown(KEY_A)) {
    command |= giewont::CharacterMovementCommand::MOVE_LEFT;
  } else if (IsKeyDown(KEY_D)) {
    command |= giewont::CharacterMovementCommand::MOVE_RIGHT;
  }
  if (IsKeyDown(KEY_SPACE)) {
    command |= giewont::CharacterMovementCommand::JUMP;
  }

  for (auto [key, interaction_type] : interaction_map) {
    if (IsKeyReleased(key)) {
      if (!game.is_server()) {
        capnp::MallocMessageBuilder message;
        auto interact = message.initRoot<net::InteractNetMessage>();

        interact.setType(interaction_type);
        interact.setInteractorNetId(character.net_id);
        game.perform_interaction(interact);
      }
    }
  }

  character.perform_movement(game, delta_time, command);
#endif
}
