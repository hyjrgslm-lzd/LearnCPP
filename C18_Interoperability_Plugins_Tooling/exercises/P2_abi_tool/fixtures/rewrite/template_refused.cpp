#include "c18/abi.h"

c18_status c18_process_old(c18_context*, const uint8_t*, size_t, uint8_t*, size_t, size_t*);

template <class T>
c18_status call_template(c18_context* ctx, const T* in, size_t n, uint8_t* out, size_t cap, size_t* written) {
  return c18_process_old(ctx, reinterpret_cast<const uint8_t*>(in), n, out, cap, written);
}
