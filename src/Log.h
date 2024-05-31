#ifndef LOG_H_
#define LOG_H_

#include <iostream>

#define LOG_WARN() std::cerr << __FILE__ << ":" << __LINE__ << " [WARN] "

#define LOG_DEBUG() std::cerr << __FILE__ << ":" << __LINE__ << " [DEBUG] "

#endif // LOG_H_
