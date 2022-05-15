#pragma once

#include <vector>
#include <string>

typedef int OrderId;
typedef std::string Symbol;

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
    Side side; //
    int price; // x10000
    int volume;

    Insert(OrderId order_id, Symbol symbol, Side side,
           int price, int volume) : order_id(order_id),
                                    symbol(std::move(symbol)),
                                    side(side),
                                    price(price),
                                    volume(volume) {}

    void accept(CommandVisitor *vistitor) const override { vistitor->visitInsert(*this); }
};

struct Amend : Command {
    OrderId order_id;
    int price; // x10000
    int volume;

    Amend(OrderId order_id, int price, int volume) : order_id(order_id), price(price), volume(volume) {}

    void accept(CommandVisitor *visitor) const override { visitor->visitAmend(*this); }
};

struct Pull : Command {
    OrderId order_id;

    explicit Pull(OrderId order_id) : order_id(order_id) {}

    void accept(CommandVisitor *visitor) const override { visitor->visitPull(*this); }

};


struct Trade {
    Symbol symbol;
    int price; // x10000
    int volume;
    OrderId aggressive_order_id;
    OrderId passive_order_id;

    Trade(Symbol symbol, int price, int volume, OrderId aggressive_order_id,
          OrderId passive_order_id) : symbol(std::move(symbol)), price(price), volume(volume),
                                      aggressive_order_id(aggressive_order_id), passive_order_id(passive_order_id) {}
};

struct OrderBook {

    struct OrderBookItem {
        int static const NONE = -1;
        int bid_price;
        int bid_volume;
        int ask_price;
        int ask_volume;

        static OrderBookItem bid(int bid_price, int bid_volume) { return {bid_price, bid_volume, NONE, NONE}; }

        static OrderBookItem ask(int ask_price, int ask_volume) { return {NONE, NONE, ask_price, ask_volume}; }

        OrderBookItem(int bid_price, int bid_volume,
                      int ask_price, int ask_volume) : bid_price(bid_price),
                                                       bid_volume(bid_volume),
                                                       ask_price(ask_price),
                                                       ask_volume(ask_volume) {}
    };

    Symbol symbol;
    std::vector<OrderBookItem> rows;

    OrderBook(Symbol symbol, std::vector<OrderBookItem> rows) : symbol(std::move(symbol)), rows(std::move(rows)) {}
};

