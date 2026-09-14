#pragma once
#include "c18/abi.h"
inline c18_status validate(const c18_api* table) {
    if (table == nullptr) return C18_STATUS_BAD_ARGUMENT;
    const bool version_matches = table->version == 1u;
    if (!version_matches) return C18_STATUS_UNSUPPORTED_VERSION;
    const bool complete = table->struct_size >= sizeof(*table) && table->create != nullptr &&
                          table->process != nullptr && table->request_stop != nullptr && table->destroy != nullptr;
    return complete ? C18_STATUS_OK : C18_STATUS_BAD_ARGUMENT;
}
