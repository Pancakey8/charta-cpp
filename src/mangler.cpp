#include "mangler.hpp"
#include <print>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }
    std::println("{}", mangle(std::string{argv[1]}));
}
