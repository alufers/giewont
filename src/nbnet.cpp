#include <stdarg.h>
#include <stdio.h>
#include "nbnet_helper.h"

extern "C" {

// nbnet logging

enum { LOG_INFO, LOG_ERROR, LOG_DEBUG, LOG_TRACE, LOG_WARNING };

#define NBN_LogInfo(...) Log(LOG_INFO, __VA_ARGS__)
#define NBN_LogError(...) Log(LOG_ERROR, __VA_ARGS__)
#define NBN_LogDebug(...) Log(LOG_DEBUG, __VA_ARGS__)
#define NBN_LogTrace(...) Log(LOG_TRACE, __VA_ARGS__)
#define NBN_LogWarning(...) Log(LOG_WARNING, __VA_ARGS__)

static const char *log_type_strings[] = {"INFO", "ERROR", "DEBUG", "TRACE",
                                         "WARNING"};

// Basic logging function
void Log(int type, const char *fmt, ...) {
  if(type == LOG_DEBUG || type == LOG_TRACE) {
    return;
  }
  va_list args;

  va_start(args, fmt);

  printf("[nbnet - %s] ", log_type_strings[type]);
  vprintf(fmt, args);
  printf("\n");

  va_end(args);
}

#define NBNET_IMPL

#include "nbnet.h"

#ifdef PLATFORM_WEB
#include "net_drivers/webrtc.h"
#else
#include "net_drivers/webrtc_c.h"
#endif


}

using namespace giewont;

void giewont::install_nbnet_webrtc_driver() {
  NBN_WebRTC_C_Register(NBN_WebRTC_C_Config{
    .enable_tls = false,
  });
}
