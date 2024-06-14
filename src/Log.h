#ifndef LOG_H_
#define LOG_H_

#include <iostream>

#define LOG_WARN() std::cerr << __FILE__ << ":" << __LINE__ << " [WARN] "

#define LOG_DEBUG() std::cerr << __FILE__ << ":" << __LINE__ << " [DEBUG] "

#define LOG_ERROR() std::cerr << __FILE__ << ":" << __LINE__ << " [ERROR] "

#endif // LOG_H_
