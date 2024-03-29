#pragma once

#include "common.hpp"
#include "queue.hpp"

#include <utility>
#include <unordered_map>
#include <vector>

struct Order {
    OrderId order_id;
    Price price; // shifted price
    Volume volume;
    uint64_t time;

    Order(OrderId order_id, Price price, Volume volume,
          uint64_t time) : order_id(order_id), price(price), volume(volume), time(time) {}
};

struct OrderInfo {
    Symbol symbol;
    Side side;

    OrderInfo(Symbol symbol, Side side) : symbol(std::move(symbol)), side(side) {}
};

template<typename Compare>
using Queue = priority_queue<OrderId, Order, Compare>;

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

/**
 *  Central limit order book (CLOB) for managing orders
 */
class CLOBEngine : public CommandVisitor {
public:

    CLOBEngine();

    /**
     * Inserts order to the order book
     */
    void visitInsert(Insert const &insert) override;

    /**
     * Changes the price and/or volume of the order
     */
    void visitAmend(Amend const &amend) override;

    /**
     * Removes the order from the order book.
     */
    void visitPull(Pull const &pull) override;

    /**
     * Returns all trades
     */
    std::vector<Trade> getTrades();

    /**
     * Returns current order books
     */
    std::vector<OrderBook> getOrderBooks();

private:
    /**
     * Incremental counter, which value is passed to order to define priority among orders with equal price
     */
    uint64_t cur_time;

    /**
     * Orders from the side {@see Side::BUY}
     */
    Queues<BuysComparator> buys;

    /**
     * Orders from the side {@see Side::SELL}
     */
    Queues<SellsComparator> sells;

    /**
     * Trades between orders
     */
    std::vector<Trade> trades;

    /**
     * Meta information about orders. There is no need to store the whole information in queues.
     * Also used to prevent duplicates (e.g. two orders with the same order_id from distinct sides, pull and insert
     * of orders with same order_id)
     */
    std::unordered_map<OrderId, OrderInfo> order_infos;

    template<typename CompareAggressive, typename ComparePassive>
    void insertImpl(Queues<CompareAggressive> &aggressive_queues, Queues<ComparePassive> &passive_queues,
                    Symbol symbol, Order &aggressive_order, bool is_buy);

    template<typename Compare>
    void amendImpl(Queues<Compare> &queues, Symbol const &symbol, Amend amend, bool is_buy);
};

