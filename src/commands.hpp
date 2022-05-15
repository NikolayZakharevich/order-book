#pragma once

#include <vector>
#include <string>
#include <utility>

typedef int64_t OrderId;
typedef std::string Symbol;
typedef int32_t Price;
typedef int32_t Volume;

enum Side {
    BUY, SELL
};

struct Insert;
struct Amend;
struct Pull;

struct CommandVisitor {
    virtual void visitInsert(Insert const &insert) = 0;

    virtual void visitAmend(Amend const &amend) = 0;

    virtual void visitPull(Pull const &pull) = 0;

    virtual ~CommandVisitor() = default;
};

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

