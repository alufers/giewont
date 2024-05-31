#include "Entity.h"
#include "Game.h"


using namespace giewont;

void Entity::destroy() { marked_for_deletion = true; }

EntityRef Entity::get_ref() const {
  EntityRef ref;
  ref.id = id;
  ref.generation = generation;
  return ref;
}

bool EntityRef::valid(Game const &game) const {
  if (id == 0)
    return false; // reference to null entity
  return game.entities[id] != nullptr &&
         game.entities[id]->generation == generation;
}

Entity &EntityRef::get(Game &game) const {
  if (valid(game)) {
    return *game.entities[id];
  } else {
    throw std::runtime_error("EntityRef is not valid");
  }
}

