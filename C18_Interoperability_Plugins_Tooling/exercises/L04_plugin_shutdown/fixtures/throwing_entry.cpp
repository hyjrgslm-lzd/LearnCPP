#include "c18/abi.h"

#include <stdexcept>

extern "C" C18_EXPORT c18_status C18_CALL c18_get_api(uint32_t, uint32_t, c18_api*) {
  try {
    throw std::runtime_error("entry threw");
  } catch (...) {
    return C18_STATUS_PLUGIN_ERROR;
  }
}
