#include "reference.hpp"
int main() {
    aba_lab::run(false);
    aba_lab::run(true);
    aba_lab::wrap_model();
    std::cout << "G2 OK: safe ABA history, tagged rejection, finite-width counterexample\n";
}
