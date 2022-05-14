#include "engine.hpp"

#include <vector>
#include <unordered_map>
#include <queue>


void push(std::unordered_map<std::string, std::vector<Order>> &orders, Symbol const &symbol, Order order);

void remove(std::unordered_map<std::string, std::vector<Order>> &orders, Symbol const &symbol, OrderId order_id);

void CLOBEngine::visitInsert(Insert const &insert) {
    Order order = Order(insert.order_id, insert.price, insert.volume);
    switch (insert.side) {
        case BUY:
            push(buys, insert.symbol, order);
            break;
        case SELL:
            push(sells, insert.symbol, order);
            break;
    }
    order_symbols[order.order_id] = insert.symbol;
    order_sides[order.order_id] = insert.side;
}

void CLOBEngine::visitAmend(Amend const &amend) {
    auto it_symbol = order_symbols.find(amend.order_id);
    if (it_symbol == order_symbols.end()) {
        return;
    }

    Symbol symbol = it_symbol->second;
    Side side = order_sides[amend.order_id];
    Order order = Order(amend.order_id, amend.price, amend.volume);

    switch (side) {
        case Side::BUY:
            remove(buys, symbol, order.order_id);
            push(buys, symbol, order);
            break;
        case Side::SELL:
            remove(sells, symbol, order.order_id);
            push(buys, symbol, order);
            break;
    }
}

void CLOBEngine::visitPull(Pull const &pull) {
    auto it = order_symbols.find(pull.order_id);
    if (it == order_symbols.end()) {
        return;
    }

    Symbol symbol = it->second;
    remove(buys, symbol, pull.order_id);
    remove(sells, symbol, pull.order_id);
}


std::vector<OrderBook> CLOBEngine::getOrderBooks() {
    std::vector<Symbol> symbols = std::vector<Symbol>();
    symbols.reserve(buys.size() + sells.size());
    for (auto const &it : buys) {
        symbols.push_back(it.first);
    }
    for (auto const &r : sells) {
        symbols.push_back(r.first);
    }

    std::sort(symbols.begin(), symbols.end());

    std::vector<OrderBook> info = std::vector<OrderBook>();
    for (Symbol const &symbol : symbols) {
        auto it_buys = buys.find(symbol);
        auto it_sells = sells.find(symbol);

        std::vector<OrderBook::OrderBookItem> items = std::vector<OrderBook::OrderBookItem>();
        if (it_buys == buys.end()) {
            for (auto const &order : it_sells->second) {
                OrderBook::OrderBookItem item = OrderBook::OrderBookItem();
                item.ask_price = order.price;
                item.ask_volume = order.volume;
                items.push_back(item);
            }
        } else if (it_sells == sells.end()) {
            for (auto const &order : it_buys->second) {
                OrderBook::OrderBookItem item = OrderBook::OrderBookItem();
                item.bid_price = order.price;
                item.bid_volume = order.volume;
                items.push_back(item);
            }
        } else {
            auto it_buys_cur = it_buys->second.begin();
            auto it_sells_cur = it_sells->second.begin();

            while (it_buys_cur != it_buys->second.end() && it_sells_cur != it_sells->second.end()) {
                OrderBook::OrderBookItem item = OrderBook::OrderBookItem();
                item.bid_price = it_buys_cur->price;
                item.bid_volume = it_buys_cur->volume;
                item.ask_price = it_sells_cur->price;
                item.ask_volume = it_sells_cur->volume;
                items.push_back(item);

                ++it_buys_cur;
                ++it_sells_cur;
            }

            while (it_buys_cur != it_buys->second.end()) {
                OrderBook::OrderBookItem item = OrderBook::OrderBookItem();
                item.bid_price = it_buys_cur->price;
                item.bid_volume = it_buys_cur->volume;
                items.push_back(item);

                ++it_buys_cur;
            }
            while (it_sells_cur != it_sells->second.end()) {
                OrderBook::OrderBookItem item = OrderBook::OrderBookItem();
                item.ask_price = it_sells_cur->price;
                item.ask_volume = it_sells_cur->volume;
                items.push_back(item);

                ++it_sells_cur;
            }
        }

        info.emplace_back(symbol, items);
    }
    return info;
}

std::vector<Trade> CLOBEngine::getTrades() {
    return std::vector<Trade>();
}


void push(std::unordered_map<std::string, std::vector<Order>> &orders, Symbol const &symbol, Order order) {
    auto it = orders.find(symbol);
    if (it == orders.end()) {
        orders[symbol] = std::vector<Order>{order};
    } else {
        it->second.push_back(order);
    }
    std::cerr << "there\n";
}

void remove(std::unordered_map<std::string, std::vector<Order>> &orders, Symbol const &symbol, OrderId order_id) {
    std::vector<Order> orders_by_symbol = orders[symbol];
    orders_by_symbol.erase(std::remove_if(
            orders_by_symbol.begin(), orders_by_symbol.end(),
            [&order_id](Order const &item) {
                return item.order_id == order_id;
            }), orders_by_symbol.end()
    );
    if (orders_by_symbol.empty()) {
        orders.erase(symbol);
    }
}