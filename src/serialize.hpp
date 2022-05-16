#pragma once

#include "common.hpp"
#include <vector>
#include <memory>

static int32_t PRICE_SHIFT = 10000;
static int32_t PRICE_SHIFT_PLACES = 4;

std::vector<std::shared_ptr<Command>> parseCommands(std::vector<std::string> const &input);

std::vector<std::string> toString(std::vector<Trade> trades, std::vector<OrderBook> order_books);