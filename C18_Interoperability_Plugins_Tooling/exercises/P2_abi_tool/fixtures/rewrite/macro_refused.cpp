#include "c18/abi.h"

c18_status c18_process_old(c18_context*, const uint8_t*, size_t, uint8_t*, size_t, size_t*);
#define CALL_PROCESS(ctx, in, n, out, cap, written) c18_process_old(ctx, in, n, out, cap, written)

c18_status call_macro(c18_context* ctx, const uint8_t* in, size_t n, uint8_t* out, size_t cap, size_t* written) {
  return CALL_PROCESS(ctx, in, n, out, cap, written);
}
