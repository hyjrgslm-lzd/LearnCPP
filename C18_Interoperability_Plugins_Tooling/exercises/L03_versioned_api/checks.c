#include "c18/abi.h"
#include <stdio.h>
#include <string.h>
c18_status c18_l03_validate(const c18_api* api);
#define CHECK(condition, message) do { if (!(condition)) { puts("check failed: " message); return 1; } } while (0)
int main(void) {
    c18_api api;
    unsigned char before[sizeof api];
    memset(&api, 0x5a, sizeof api);
    memcpy(before, &api, sizeof api);
    CHECK(c18_get_api(999, sizeof api, &api) == C18_STATUS_UNSUPPORTED_VERSION, "reject unknown version");
    CHECK(memcmp(before, &api, sizeof api) == 0, "failed query does not commit");
    CHECK(c18_get_api(C18_ABI_VERSION, sizeof api - 1, &api) == C18_STATUS_BAD_ARGUMENT, "reject short table");
    CHECK(memcmp(before, &api, sizeof api) == 0, "short table does not write");
    CHECK(c18_get_api(C18_ABI_VERSION, sizeof api, &api) == C18_STATUS_OK, "query real plugin from C11");
    CHECK(c18_l03_validate(&api) == C18_STATUS_OK, "good function table");
    {
        c18_api faulty = api;
        faulty.destroy = NULL;
        CHECK(c18_l03_validate(&faulty) == C18_STATUS_BAD_ARGUMENT, "required function table");
        faulty = api;
        faulty.struct_size = sizeof api - 1;
        CHECK(c18_l03_validate(&faulty) == C18_STATUS_BAD_ARGUMENT, "table prefix bound");
        faulty = api;
        faulty.version = 999;
        CHECK(c18_l03_validate(&faulty) == C18_STATUS_UNSUPPORTED_VERSION, "semantic version");
        CHECK(c18_l03_validate(NULL) == C18_STATUS_BAD_ARGUMENT, "null table");
    }
    {
        c18_host_api host = {C18_ABI_VERSION, sizeof(c18_host_api), NULL, NULL};
        c18_context* context = NULL;
        c18_context* failed_output;
        c18_status status, destroyed;
        CHECK(api.create(&host, &context) == C18_STATUS_OK && context != NULL, "create opaque context");
        failed_output = context;
        host.version = 999;
        status = api.create(&host, &failed_output);
        api.request_stop(context);
        destroyed = api.destroy(context);
        CHECK(status == C18_STATUS_BAD_ARGUMENT && failed_output == NULL, "failed create clears output handle");
        CHECK(destroyed == C18_STATUS_OK, "C consumer cleanup");
    }
    printf("C11 ABI consumer: PASS table=%zu host=%zu status=%zu\n", sizeof api, sizeof(c18_host_api), sizeof(c18_status));
    return 0;
}
