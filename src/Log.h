#ifndef LOG_H_
#define LOG_H_

#include <iostream>

#define COLOR_RED 


#define LOG_WARN() std::cerr << __FILE__ << ":" << __LINE__ << " [WARN] "

#define LOG_DEBUG() std::cerr << __FILE__ << ":" << __LINE__ << " [DEBUG] "

#define LOG_ERROR() std::cerr << "\033[31m" << __FILE__ << ":" << __LINE__ << " [ERROR] " << "\033[0m"

#define LOG_INFO() std::cerr << __FILE__ << ":" << __LINE__ << " [INFO] "

#endif // LOG_H_
