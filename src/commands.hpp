#pragma once

#include <vector>

// TODO: remove when remove debug
#include <iostream>

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

    virtual void print() const = 0;

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

    void print() const override {
        std::cerr << "insert " << order_id << " " << symbol << " " << side << " " << price << " " << volume << "\n";
    }
};

struct Amend : Command {
    OrderId order_id;
    int price; // x10000
    int volume;

    Amend(OrderId order_id, int price, int volume) : order_id(order_id), price(price), volume(volume) {}

    void accept(CommandVisitor *visitor) const override { visitor->visitAmend(*this); }

    void print() const override {
        std::cerr << "amend " << order_id << " " << price << " " << volume << "\n";
    }
};

struct Pull : Command {
    OrderId order_id;

    explicit Pull(OrderId order_id) : order_id(order_id) {}

    void accept(CommandVisitor *visitor) const override { visitor->visitPull(*this); }

    void print() const override {
        std::cerr << "pull " << order_id << "\n";
    }
};


struct Trade {
    Symbol symbol;
    int price; // x10000
    std::string volume;
    OrderId aggressive_order_id;
    OrderId passive_order_id;
};

struct OrderBook {

    struct OrderBookItem {
        int static const NONE = -1;
        int bid_price = NONE;
        int bid_volume = NONE;
        int ask_price = NONE;
        int ask_volume = NONE;
    };

    Symbol symbol;
    std::vector<OrderBookItem> rows;

    OrderBook(Symbol symbol, std::vector<OrderBookItem> rows) : symbol(std::move(symbol)), rows(std::move(rows)) {}
};

