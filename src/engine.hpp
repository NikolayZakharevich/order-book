#pragma once

#include "commands.hpp"

#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <ostream>


template<typename T, class Compare>
class priority_queue : public std::priority_queue<T, std::vector<T>, Compare> {
public:
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;


    T &top() { return this->c.front(); }

    bool remove_if(std::function<bool(T)> predicate) {
        auto it = std::find_if(this->c.begin(), this->c.end(), predicate);
        if (it != this->c.end()) {
            this->c.erase(it);
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
            return true;
        } else {
            return false;
        }
    }
};


struct Order {
    OrderId order_id;
    int price; // x10000
    int volume;

    Order(OrderId order_id, int price, int volume) : order_id(order_id), price(price), volume(volume) {}


    friend std::ostream &operator<<(std::ostream &os, Order const &order);
};

struct BuysComparator {
    bool operator()(Order const &lhs, Order const &rhs) {
        return lhs.price != rhs.price ? lhs.price < rhs.price : lhs.order_id > rhs.order_id;
    }
};

struct SellsComparator {
    bool operator()(Order const &lhs, Order const &rhs) {
        return lhs.price != rhs.price ? lhs.price > rhs.price : lhs.order_id > rhs.order_id;
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
    }

    void visitInsert(Insert const &insert) override;

    void visitAmend(Amend const &amend) override;

    void visitPull(Pull const &pull) override;

    std::vector<Trade> getTrades();

    std::vector<OrderBook> getOrderBooks();

private:
    std::unordered_map<Symbol, priority_queue<Order, BuysComparator>> buys;
    std::unordered_map<Symbol, priority_queue<Order, SellsComparator>> sells;
    std::vector<Trade> trades;

    std::unordered_map<OrderId, Symbol> order_symbols;
    std::unordered_map<OrderId, Side> order_sides;

    template<typename Comp1, typename Comp2>
    void match(
            std::unordered_map<std::string, priority_queue<Order, Comp1>> &aggressive_orders,
            std::unordered_map<std::string, priority_queue<Order, Comp2>> &passive_orders,
            Symbol symbol,
            Order &aggressive_order,
            bool is_aggressive_order_buy
    );

    void matchSell(Symbol symbol, Order &sell) {
        match(sells, buys, symbol, sell, false);
    }

    void matchBuy(Symbol symbol, Order &buy) {
        match(buys, sells, symbol, buy, true);
    }

};

