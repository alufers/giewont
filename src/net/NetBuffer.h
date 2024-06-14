#ifndef NETBUFFER_H_
#define NETBUFFER_H_
#include <stdint.h>
#include <string>
#include <vector>

namespace giewont {

class NetBuffer {
public:
  NetBuffer() = default;
  NetBuffer(uint8_t *data, size_t size) : buffer(data, data + size) {}
  NetBuffer(const std::vector<uint8_t> &data) : buffer(data) {}

  std::vector<uint8_t> buffer;
  size_t read_pos = 0;

  NetBuffer &operator<<(uint8_t value);
  NetBuffer &operator>>(uint8_t &value);

  NetBuffer &operator<<(uint16_t value);
  NetBuffer &operator>>(uint16_t &value);

  NetBuffer &operator<<(uint32_t value);
  NetBuffer &operator>>(uint32_t &value);

  NetBuffer &operator<<(uint64_t value);
  NetBuffer &operator>>(uint64_t &value);
  
  NetBuffer &operator<<(float value);
  NetBuffer &operator>>(float &value);

  NetBuffer &operator<<(const std::string &value);
  NetBuffer &operator>>(std::string &value);
};

};     // namespace giewont
#endif // NETBUFFER_H_
