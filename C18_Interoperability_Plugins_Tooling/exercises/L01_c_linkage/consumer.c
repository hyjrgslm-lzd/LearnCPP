#include "api.h"
#include <stdio.h>
#include <string.h>
int main(void) {
    const unsigned char input[] = {'a', 0, 'z', 255, 'A', '0'};
    const unsigned char expected[] = {'A', 0, 'Z', 255, 'A', '0'};
    unsigned char output[sizeof input];
    memset(output, 0x5a, sizeof output);
    if (c18_l01_upper(input, sizeof input, output) != 0 || memcmp(output, expected, sizeof output) != 0) {
        puts("check failed: complete byte conversion");
        return 1;
    }
    if (c18_l01_upper(NULL, 0, NULL) != 0) {
        puts("check failed: empty bytes");
        return 1;
    }
    puts("C11 consumer called C++ implementation: PASS");
    return 0;
}
