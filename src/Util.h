#ifndef UTIL_H_
#define UTIL_H_

#include <string>

namespace giewont {
bool string_contains_case_insensitive(const std::string &str,
                                      const std::string &substr);
}

#endif // UTIL_H_
