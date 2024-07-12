#include "Entity.h"
#include "Game.h"


using namespace giewont;

void Entity::destroy() { marked_for_deletion = true; }

void Entity::build_sync_message(net::SyncEntityNetMessage::Builder &sync_message) {
  sync_message.setNetId(net_id);
  sync_message.setEntityType(get_net_type());
  sync_message.setNetOwnerId(net_owner_peer_id);
  auto net_position = sync_message.initPosition();
  position.serialize(net_position);
  
}

void Entity::update_from_sync_message(Game const &game, const net::SyncEntityNetMessage::Reader &sync_message) {

  if(sync_message.getNetId() != net_id) {
    throw std::runtime_error("Entity ID mismatch");
  }

  if(sync_message.getEntityType() != get_net_type()) {
    throw std::runtime_error("Entity type mismatch");
  }

  if(sync_message.getNetOwnerId() != game.my_peer_id || is_being_created) {
   
  this->position = sync_message.getPosition();
  
  }
}

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

