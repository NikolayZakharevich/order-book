#pragma once

#include "commands.hpp"

#include <unordered_map>
#include <vector>


struct Order {
    OrderId order_id;
    int price; // x10000
    int volume;

    Order(OrderId order_id, int price, int volume) : order_id(order_id), price(price), volume(volume) {}
};


class CLOBEngine : public CommandVisitor {
public:

    CLOBEngine() noexcept {
        buys = std::unordered_map<Symbol, std::vector<Order >>();
        sells = std::unordered_map<Symbol, std::vector<Order >>();

        order_symbols = std::unordered_map<OrderId, Symbol>();
        order_sides = std::unordered_map<OrderId, Side>();
    }

    void visitInsert(Insert const &insert) override;

    void visitAmend(Amend const &amend) override;

    void visitPull(Pull const &pull) override;

    std::vector<Trade> getTrades();

    std::vector<OrderBook> getOrderBooks();

private:
    std::unordered_map<Symbol, std::vector<Order>> buys;
    std::unordered_map<Symbol, std::vector<Order>> sells;

    std::unordered_map<OrderId, Symbol> order_symbols;
    std::unordered_map<OrderId, Side> order_sides;
};

