#include <utility>
#include <iostream>

#include "main.hpp"
#include "engine.hpp"
#include "serialize.hpp"


std::vector<std::string> run(std::vector<std::string> const &input) {
    CLOBEngine engine = CLOBEngine();
    for (auto const &command : parseCommands(input)) {
        command->accept(&engine);
    }
    return toString(engine.getOrderBooks());
}