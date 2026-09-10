#include <iostream>

#if defined(__has_include)
#if __has_include(<meta>)
#include <meta>
#define C04_HAS_META_HEADER 1
#endif
#endif

#if defined(C04_FORCE_ANNOTATIONS_PROBE)
#define C04_HAS_ANNOTATION_SYNTAX 1
#endif
#ifndef C04_HAS_META_HEADER
#define C04_HAS_META_HEADER 0
#endif
#ifndef C04_HAS_ANNOTATION_SYNTAX
#define C04_HAS_ANNOTATION_SYNTAX 0
#endif

struct FormatSkip {
    bool value;
};

struct AnnotatedRecord {
#if C04_HAS_ANNOTATION_SYNTAX
    [[= FormatSkip{true}]]
#endif
    int secret;
    int visible;
};

#if C04_HAS_META_HEADER && C04_HAS_ANNOTATION_SYNTAX
consteval bool annotations_work() {
    using namespace std::meta;
    auto fields = nonstatic_data_members_of(^^AnnotatedRecord, access_context::current());
    auto secret_annotations = annotations_of(fields[0]);
    auto visible_annotations = annotations_of_with_type(fields[1], ^^FormatSkip);
    return secret_annotations.size() == 1
        && is_annotation(secret_annotations[0])
        && extract<FormatSkip>(secret_annotations[0]).value
        && visible_annotations.empty();
}
#endif

int main() {
    std::cout << "probe=annotations header=" << C04_HAS_META_HEADER
              << " macro=" << C04_HAS_ANNOTATION_SYNTAX
              << " body=" << (C04_HAS_META_HEADER && C04_HAS_ANNOTATION_SYNTAX) << "\n";
#if !C04_HAS_META_HEADER || !C04_HAS_ANNOTATION_SYNTAX
    std::cout << "SKIP annotations: missing <meta> or explicit annotation syntax macro\n";
    return 77;
#else
    static_assert(annotations_work());
    std::cout << "PASS annotations_of, annotations_of_with_type, extract<T>\n";
    return 0;
#endif
}
