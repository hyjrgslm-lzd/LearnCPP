#include "lesson_api.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    lesson_engine* engine = NULL;
    int value = 0;
    lesson_status status = lesson_create(LESSON_ABI_VERSION, 40, &engine);
    if (status != LESSON_OK || engine == NULL) {
        fprintf(stderr, "create failed: %s\n", lesson_status_message(status));
        return EXIT_FAILURE;
    }
    status = lesson_eval(engine, 2, &value);
    lesson_destroy(engine);
    if (status != LESSON_OK || value != 42) {
        fprintf(stderr, "eval failed: %s value=%d\n", lesson_status_message(status), value);
        return EXIT_FAILURE;
    }
    printf("C consumer passed: value=%d\n", value);
    return EXIT_SUCCESS;
}
