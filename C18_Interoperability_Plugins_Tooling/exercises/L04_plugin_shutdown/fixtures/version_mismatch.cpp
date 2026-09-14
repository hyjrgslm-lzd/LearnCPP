#include "c18/abi.h"

extern "C" C18_EXPORT c18_status C18_CALL c18_get_api(uint32_t,
                                                       uint32_t host_struct_size,
                                                       c18_api* out_api) {
  if (out_api == nullptr || host_struct_size < sizeof(c18_api)) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  out_api->version = 999u;
  out_api->struct_size = sizeof(c18_api);
  out_api->create = nullptr;
  out_api->process = nullptr;
  out_api->request_stop = nullptr;
  out_api->destroy = nullptr;
  return C18_STATUS_UNSUPPORTED_VERSION;
}
