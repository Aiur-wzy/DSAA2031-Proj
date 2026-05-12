#pragma once

#include <iostream>

namespace onebase {

#define LOG_INFO(msg)  (std::cout << "[INFO]  " << (msg) << '\n')
#define LOG_WARN(msg)  (std::cout << "[WARN]  " << (msg) << '\n')
#define LOG_ERROR(msg) (std::cerr << "[ERROR] " << (msg) << '\n')

#ifdef NDEBUG
#define LOG_DEBUG(...) ((void)0)
#else
#define LOG_DEBUG(msg) (std::cout << "[DEBUG] " << (msg) << '\n')
#endif

}  // namespace onebase
