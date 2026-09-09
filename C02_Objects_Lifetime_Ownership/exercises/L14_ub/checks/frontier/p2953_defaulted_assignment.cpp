struct OnlyAssignableLvalue {
    OnlyAssignableLvalue() = default;
    OnlyAssignableLvalue(const OnlyAssignableLvalue&) = default;
    OnlyAssignableLvalue& operator=(const OnlyAssignableLvalue&) & = default;
};

struct RvalueQualifiedDefaultedAssignment {
    RvalueQualifiedDefaultedAssignment() = default;
    RvalueQualifiedDefaultedAssignment(const RvalueQualifiedDefaultedAssignment&) = default;
    RvalueQualifiedDefaultedAssignment& operator=(const RvalueQualifiedDefaultedAssignment&) && = default;
};

void use_lvalue_assignment() {
    OnlyAssignableLvalue a;
    OnlyAssignableLvalue b;
    a = b;
}
