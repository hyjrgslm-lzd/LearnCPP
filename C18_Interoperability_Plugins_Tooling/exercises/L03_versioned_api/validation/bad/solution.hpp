#pragma once
#include "c18/abi.h"
inline c18_status validate(const c18_api* api) {
    // Wrong: a matching version alone does not make all operations available.
    if (!api) return C18_STATUS_BAD_ARGUMENT;
    return api->version == C18_ABI_VERSION ? C18_STATUS_OK : C18_STATUS_UNSUPPORTED_VERSION;
}
