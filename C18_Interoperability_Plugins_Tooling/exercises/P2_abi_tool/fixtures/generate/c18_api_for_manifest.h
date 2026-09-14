#pragma once
#include "c18/abi.h"

extern c18_status C18_CALL c18_get_api(uint32_t requested_version, uint32_t host_struct_size, c18_api* out_api);
