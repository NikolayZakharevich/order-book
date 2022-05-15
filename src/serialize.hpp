#pragma once

#include "commands.hpp"
#include <vector>
#include <memory>

std::vector<std::shared_ptr<Command>> parseCommands(std::vector<std::string> const &input);

std::vector<std::string> toString(std::vector<Trade> trades, std::vector<OrderBook> order_books);