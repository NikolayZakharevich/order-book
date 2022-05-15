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

template<typename Compare>
using Queue = priority_queue<Order, Compare>;

template<typename Compare>
using Queues = std::unordered_map<std::string, Queue<Compare>>;

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

    CLOBEngine() noexcept;

    void visitInsert(Insert const &insert) override;

    void visitAmend(Amend const &amend) override;

    void visitPull(Pull const &pull) override;

    std::vector<Trade> getTrades();

    std::vector<OrderBook> getOrderBooks();

private:
    int cur_time;
    Queues<BuysComparator> buys;
    Queues<SellsComparator> sells;
    std::vector<Trade> trades;

    std::unordered_map<OrderId, Symbol> order_symbols;
    std::unordered_map<OrderId, Side> order_sides;

    template<typename Compare>
    void amendImpl(Queue<Compare> &queue, Symbol symbol, Amend amend, bool is_buy);

    template<typename CompareAggressive, typename ComparePassive>
    void matchImpl(
            Queues<CompareAggressive> &aggressive_queues,
            Queues<ComparePassive> &passive_queues,
            Symbol symbol, Order &aggressive_order, bool is_buy
    );

    void matchSell(Symbol symbol, Order &sell) {
        matchImpl(sells, buys, std::move(symbol), sell, false);
    }

    void matchBuy(Symbol symbol, Order &buy) {
        matchImpl(buys, sells, std::move(symbol), buy, true);
    }

};

