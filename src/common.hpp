#pragma once

#include <vector>
#include <string>
#include <utility>

/**
 * Type for order unique identifier
 */
typedef int64_t OrderId;

/**
 * Type for order symbol e.g. AAPL or TSLA
 */
typedef std::string Symbol;

/**
 * Type for order price
 */
typedef int32_t Price;

/**
 * Type for order volume
 */
typedef int32_t Volume;

/**
 * Type for order side (buy or sell)
 */
enum Side {
    BUY, SELL
};

/**
 * An insert pushes the order to the order book.
 * The order will be matched with the opposite side until either the volume of
 * the new order is exhausted or until there are no orders on the opposite side
 * with which the new order can match.
 */
struct Insert;

/**
 * An amend changes the price and/or volume of the order. An amend causes
 * the order to lose time priority in the order book, unless the only change
 * to the order is that the volume is decreased. If the price of the order
 * is amended, it needs to be re-evaluated for potential matches
 */
struct Amend;

/**
 * A pull removes the order from the order book.
 */
struct Pull;


struct CommandVisitor {
    virtual void visitInsert(Insert const &insert) = 0;

    virtual void visitAmend(Amend const &amend) = 0;

    virtual void visitPull(Pull const &pull) = 0;

    virtual ~CommandVisitor() = default;
};

/**
 * Common interface for commands.
 * {@see Insert}
 * {@see Amend}
 * {@see Pull}
 */
struct Command {
    virtual void accept(CommandVisitor *visitor) const = 0;

    virtual ~Command() = default;
};

struct Insert : Command {
    OrderId order_id;
    Symbol symbol;
    Side side;
    Price price; // shifted price
    Volume volume;

    Insert(OrderId order_id, Symbol symbol, Side side,
           Price price, Volume volume) : order_id(order_id),
                                         symbol(std::move(symbol)),
                                         side(side),
                                         price(price),
                                         volume(volume) {}

    void accept(CommandVisitor *vistitor) const override { vistitor->visitInsert(*this); }
};

struct Amend : Command {
    OrderId order_id;
    Price price; // shifted price
    Volume volume;

    Amend(OrderId order_id, Price price, Volume volume) : order_id(order_id), price(price), volume(volume) {}

    void accept(CommandVisitor *visitor) const override { visitor->visitAmend(*this); }
};

struct Pull : Command {
    OrderId order_id;

    explicit Pull(OrderId order_id) : order_id(order_id) {}

    void accept(CommandVisitor *visitor) const override { visitor->visitPull(*this); }

};

struct OrderBook {
    struct Item {
        Price price;
        Volume volume;

        Item(Price price, Volume volume) : price(price), volume(volume) {}
    };

    Symbol symbol;
    std::vector<Item> bids;
    std::vector<Item> asks;

    OrderBook(Symbol symbol, std::vector<Item> bids,
              std::vector<Item> asks) : symbol(std::move(symbol)), bids(std::move(bids)), asks(std::move(asks)) {}
};


struct Trade {
    Symbol symbol;
    Price price; // x10000
    Volume volume;
    OrderId aggressive_order_id;
    OrderId passive_order_id;

    Trade(Symbol symbol, Price price, Volume volume, OrderId aggressive_order_id,
          OrderId passive_order_id) : symbol(std::move(symbol)), price(price), volume(volume),
                                      aggressive_order_id(aggressive_order_id), passive_order_id(passive_order_id) {}
};