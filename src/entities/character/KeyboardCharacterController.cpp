#include "KeyboardCharacterController.h"
#include "CameraEntity.h"
#include "CharacterEntity.h"
#include "Log.h"
#include "schema.capnp.h"
#include <array>
#include <utility>
#ifdef GIEWONT_HAS_GRAPHICS
#include "client/ClientGame.h"
#endif
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

  auto &camera = game.camera_ref.get_as<CameraEntity>(game);

  Vec2 worldCursorPos = camera.screenToWorldPos(GetMousePosition());

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

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    capnp::MallocMessageBuilder message;
    auto interact = message.initRoot<net::InteractNetMessage>();

    interact.setType(net::InteractionType::PRIMARY_CLICK);
    interact.setInteractorNetId(character.net_id);
    ClientGame &client_game = dynamic_cast<ClientGame &>(game);

    if (game.camera_ref.valid_as<CameraEntity>(game)) {

      Vec2 delta = worldCursorPos - character.position;
      auto net_offset = interact.initCharacterToMouseOffset();
      delta.serialize(net_offset);
    }

    game.perform_interaction(interact);
  }

  check_interactables_counter++;
  if (check_interactables_counter % 10 == 0) {
    // Check for interactables under the cursor
    EntityRef new_highlighted_interactable;
    float closest_dist = INFINITY;
    for (auto &entity : game.valid_entities()) {
      if (entity->check_interaction_possible(game, character.get_ref())) {
        auto dist = entity->position.distance(worldCursorPos);
        if (dist < closest_dist) {
          closest_dist = dist;
          new_highlighted_interactable = entity->get_ref();
        }
      }
    }
    this->highlighted_interactable = new_highlighted_interactable;
  }

  if (highlighted_interactable.valid_as<PhysEntity>(game)) {
    auto &highlighted_entity =
        highlighted_interactable.get_as<PhysEntity>(game);
    highlighted_interactable_aabb =
        highlighted_entity.get_aabb().translated(highlighted_entity.position);
  } else {
    highlighted_interactable_aabb = std::nullopt;
  }

  character.perform_movement(game, delta_time, command);
#endif
}

void KeyboardCharacterController::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  if (auto highlight_aabb = highlighted_interactable_aabb) {
    Rectangle raylib_rect = {highlight_aabb->min.x, highlight_aabb->min.y,
                             highlight_aabb->width(), highlight_aabb->height()};
    DrawRectangleLinesEx(raylib_rect, 2.0f, RED);
  }
#endif
}
