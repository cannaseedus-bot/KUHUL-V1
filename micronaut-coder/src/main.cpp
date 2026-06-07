#include "scx/core_engine.hpp"

/**
 * Micronaut Coder v0.7.x — Grammar Learning Engine
 * 
 * This tool synthesises EBNF, PEG and JSON-Schema grammars from source code
 * using local codebase scans and web research.
 */
int main(int argc, char** argv) {
    return scx::engine::run(argc, argv);
}
