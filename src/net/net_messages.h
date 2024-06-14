#ifndef NET_MESSAGES_H_
#define NET_MESSAGES_H_

#include "Log.h"
#include "NetBuffer.h"
#include <stdint.h>
#include <string>

namespace giewont::net {

enum class BaseMessageTypes : uint32_t { LOAD_LEVEL = 0, LEVEL_LOADED = 1 };

class LoadLevelMessage {
public:
  const BaseMessageTypes type = BaseMessageTypes::LOAD_LEVEL;
  std::string tmj_path;

  LoadLevelMessage(std::string tmj_path) : tmj_path(tmj_path) {}
  LoadLevelMessage() {}

  void serialize(NetBuffer &buffer) { buffer << tmj_path; }
  static LoadLevelMessage deserialize(NetBuffer &buffer) {
    LoadLevelMessage msg;
    buffer >> msg.tmj_path;

    return msg;
  }
};
} // namespace giewont::net

#endif // NET_MESSAGES_H_
