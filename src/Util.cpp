#include "Util.h"
#include <algorithm>
#include <string>

using namespace std;

bool giewont::string_contains_case_insensitive(const std::string &str,
                                               const std::string &substr) {
  std::string str_lower = str;
  std::string substr_lower = substr;

  // Convert both strings to lowercase
  std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(),
                 ::tolower);
  std::transform(substr_lower.begin(), substr_lower.end(), substr_lower.begin(),
                 ::tolower);

  // Check if the lowercase version of the substring is in the lowercase version
  // of the string
  return str_lower.find(substr_lower) != std::string::npos;
}
