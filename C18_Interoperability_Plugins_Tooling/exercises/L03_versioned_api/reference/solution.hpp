#pragma once
#include "c18/abi.h"
inline c18_status validate(const c18_api* api) {
    if (!api) return C18_STATUS_BAD_ARGUMENT;
    if (api->version != C18_ABI_VERSION) return C18_STATUS_UNSUPPORTED_VERSION;
    if (api->struct_size < sizeof(c18_api)) return C18_STATUS_BAD_ARGUMENT;
    if (!api->create || !api->process || !api->request_stop || !api->destroy) return C18_STATUS_BAD_ARGUMENT;
    return C18_STATUS_OK;
}
