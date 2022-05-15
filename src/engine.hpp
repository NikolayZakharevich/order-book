#pragma once

#include "commands.hpp"
#include "queue.hpp"

#include <utility>
#include <unordered_map>
#include <vector>


struct Order {
    OrderId order_id;
    int price; // x10000
    mutable int volume;
    int time;

    Order(OrderId order_id, int price, int volume, int time) : order_id(order_id), price(price), volume(volume),
                                                               time(time) {}

    bool operator==(Order const &rhs) const {
        return order_id == rhs.order_id;
    }
};

struct BuysComparator {
    bool operator()(Order const &lhs, Order const &rhs) const {
        return lhs.price != rhs.price ? lhs.price > rhs.price : lhs.time < rhs.time;
    }
};

struct SellsComparator {
    bool operator()(Order const &lhs, Order const &rhs) const {
        return lhs.price != rhs.price ? lhs.price < rhs.price : lhs.time < rhs.time;
    }
};


class CLOBEngine : public CommandVisitor {
public:

    CLOBEngine() noexcept {
        buys = std::unordered_map<Symbol, priority_queue<Order, BuysComparator>>();
        sells = std::unordered_map<Symbol, priority_queue<Order, SellsComparator>>();
        trades = std::vector<Trade>();

        order_symbols = std::unordered_map<OrderId, Symbol>();
        order_sides = std::unordered_map<OrderId, Side>();
        cur_time = 0;
    }

    void visitInsert(Insert const &insert) override;

    void visitAmend(Amend const &amend) override;

    void visitPull(Pull const &pull) override;

    std::vector<Trade> getTrades();

    std::vector<OrderBook> getOrderBooks();

private:
    int cur_time;
    std::unordered_map<Symbol, priority_queue<Order, BuysComparator>> buys;
    std::unordered_map<Symbol, priority_queue<Order, SellsComparator>> sells;
    std::vector<Trade> trades;

    std::unordered_map<OrderId, Symbol> order_symbols;
    std::unordered_map<OrderId, Side> order_sides;

    template<typename Comp>
    void amendImpl(priority_queue<Order, Comp> &queue, Symbol symbol, Amend amend, bool is_buy);

    template<typename Comp1, typename Comp2>
    void matchImpl(
            std::unordered_map<std::string, priority_queue<Order, Comp1>> &aggressive_orders,
            std::unordered_map<std::string, priority_queue<Order, Comp2>> &passive_orders,
            Symbol symbol, Order &aggressive_order, bool is_aggressive_order_buy
    );

    void matchSell(Symbol symbol, Order &sell) {
        matchImpl(sells, buys, std::move(symbol), sell, false);
    }

    void matchBuy(Symbol symbol, Order &buy) {
        matchImpl(buys, sells, std::move(symbol), buy, true);
    }

};

