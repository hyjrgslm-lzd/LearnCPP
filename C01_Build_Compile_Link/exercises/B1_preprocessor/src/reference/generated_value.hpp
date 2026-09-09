#ifndef B1_PREPROCESSOR_GENERATED_VALUE_HPP
#define B1_PREPROCESSOR_GENERATED_VALUE_HPP

#ifndef B1_LOCAL_OFFSET
#define B1_LOCAL_OFFSET 0
#endif

#define B1_JOIN_VALUE(base) ((base) + B1_LOCAL_OFFSET)

static inline int generated_value()
{
    return B1_JOIN_VALUE(100);
}

#endif
