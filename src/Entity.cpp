#include "Entity.h"

using giewont::Entity;


void Entity::destroy() {
  marked_for_deletion = true;
}
