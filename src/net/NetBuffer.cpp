#include "NetBuffer.h"
#include <cstring>
#include <stdexcept>

using namespace giewont;

NetBuffer &NetBuffer::operator<<(uint8_t value) {
  buffer.push_back(value);
  return *this;
}

NetBuffer &NetBuffer::operator>>(uint8_t &value) {
  if (read_pos >= buffer.size()) {
    throw std::runtime_error("NetBuffer read out of bounds");
  }
  value = buffer[read_pos];
  read_pos++;
  return *this;
}

NetBuffer &NetBuffer::operator<<(uint16_t value) {
  buffer.push_back(value & 0xFF);
  buffer.push_back((value >> 8) & 0xFF);
  return *this;
}

NetBuffer &NetBuffer::operator>>(uint16_t &value) {
  if (read_pos + 2 > buffer.size()) {
    throw std::runtime_error("NetBuffer read out of bounds");
  }
  value = buffer[read_pos] | (buffer[read_pos + 1] << 8);
  read_pos += 2;
  return *this;
}

NetBuffer &NetBuffer::operator<<(uint32_t value) {
  buffer.push_back(value & 0xFF);
  buffer.push_back((value >> 8) & 0xFF);
  buffer.push_back((value >> 16) & 0xFF);
  buffer.push_back((value >> 24) & 0xFF);
  return *this;
}

NetBuffer &NetBuffer::operator>>(uint32_t &value) {
  if (read_pos + 4 > buffer.size()) {
    throw std::runtime_error("NetBuffer read out of bounds");
  }
  value = buffer[read_pos] | (buffer[read_pos + 1] << 8) |
          (buffer[read_pos + 2] << 16) | (buffer[read_pos + 3] << 24);
  read_pos += 4;
  return *this;
}

NetBuffer &NetBuffer::operator<<(uint64_t value) {
  buffer.push_back(value & 0xFF);
  buffer.push_back((value >> 8) & 0xFF);
  buffer.push_back((value >> 16) & 0xFF);
  buffer.push_back((value >> 24) & 0xFF);
  buffer.push_back((value >> 32) & 0xFF);
  buffer.push_back((value >> 40) & 0xFF);
  buffer.push_back((value >> 48) & 0xFF);
  buffer.push_back((value >> 56) & 0xFF);
  return *this;
}

NetBuffer &NetBuffer::operator>>(uint64_t &value) {
  if (read_pos + 8 > buffer.size()) {
    throw std::runtime_error("NetBuffer read out of bounds");
  }
  value = buffer[read_pos] | (buffer[read_pos + 1] << 8) |
          (buffer[read_pos + 2] << 16) | (buffer[read_pos + 3] << 24) |
          (buffer[read_pos + 4] << 32) | (buffer[read_pos + 5] << 40) |
          (buffer[read_pos + 6] << 48) | (buffer[read_pos + 7] << 56);
  read_pos += 8;
  return *this;
}

NetBuffer &NetBuffer::operator<<(float value) {
  uint32_t int_value;
  memcpy((void *)&int_value, (void *)&value, sizeof(float));
  return *this << int_value;
}

NetBuffer &NetBuffer::operator>>(float &value) {
  uint32_t int_value;
  *this >> int_value;
  memcpy((void *)&value, &int_value, sizeof(float));
  return *this;
}

NetBuffer &NetBuffer::operator<<(const std::string &value) {
  *this << (uint32_t)value.size();
  for (char c : value) {
    buffer.push_back(c);
  }
  return *this;
}

NetBuffer &NetBuffer::operator>>(std::string &value) {
  uint32_t size;
  *this >> size;
  if (read_pos + size > buffer.size()) {
    throw std::runtime_error("NetBuffer read out of bounds (reading string)");
  }
  value = std::string((char *)&buffer[read_pos], size);
  read_pos += size;
  return *this;
}
