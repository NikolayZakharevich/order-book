#include "engine.hpp"

#include <vector>
#include <unordered_map>
#include <queue>


template<typename Comp>
void push(std::unordered_map<std::string, priority_queue<Order, Comp>> &orders, Symbol const &symbol,
          Order const &order);

template<typename Comp>
void remove(std::unordered_map<std::string, priority_queue<Order, Comp>> &orders, Symbol const &symbol,
            OrderId order_id);


template<typename Comp>
void printQueue(std::unordered_map<std::string, priority_queue<Order, Comp>> &orders) {
    auto ordersCopy = orders;
    std::cerr << "\n";
    for (auto it : ordersCopy) {
        std::cerr << it.first << " {\n";
        while (!it.second.empty()) {
            Order order = it.second.top();
            it.second.pop();
            std::cerr << order << "\n";
        }
        std::cerr << "}\n";
    }
    std::cerr << "\n";
}


std::function<bool(Order)> predicateOrderId(OrderId order_id) {
    return [&order_id](Order const &item) {
        return item.order_id == order_id;
    };
}

template<typename Comp1, typename Comp2>
void CLOBEngine::matchImpl(
        std::unordered_map<std::string, priority_queue<Order, Comp1>> &aggressive_orders,
        std::unordered_map<std::string, priority_queue<Order, Comp2>> &passive_orders,
        Symbol symbol,
        Order &aggressive_order,
        bool is_aggressive_order_buy
) {
    while (true) {

        auto it_passive_orders = passive_orders.find(symbol);
        if (it_passive_orders == passive_orders.end() || it_passive_orders->second.empty()) {
            push(aggressive_orders, symbol, aggressive_order);
            return;
        }

        Order &best_passive_order = it_passive_orders->second.top();
        bool is_not_match = (is_aggressive_order_buy && aggressive_order.price < best_passive_order.price) ||
                            (!is_aggressive_order_buy && best_passive_order.price < aggressive_order.price);
        if (is_not_match) {
            push(aggressive_orders, symbol, aggressive_order);
            return;
        }

        int price = (is_aggressive_order_buy ? aggressive_order : best_passive_order).price;
        int volume = std::min(best_passive_order.volume, aggressive_order.volume);
        trades.emplace_back(symbol, price, volume, aggressive_order.order_id, best_passive_order.order_id);

        if (best_passive_order.volume > aggressive_order.volume) {
            best_passive_order.volume -= aggressive_order.volume;
            return;
        } else if (aggressive_order.volume > best_passive_order.volume) {
            aggressive_order.volume -= best_passive_order.volume;
            it_passive_orders->second.pop();
        } else {
            it_passive_orders->second.pop();
            return;
        }
    }
}


template<typename Comp>
void CLOBEngine::amendImpl(priority_queue<Order, Comp> &queue, Symbol symbol, Amend amend, bool is_buy) {
    auto it = queue.find_if(predicateOrderId(amend.order_id));
    if (it == queue.end()) {
        return;
    }
    if (it->price == amend.price && it->volume > amend.volume) {
        *it = Order(amend.order_id, amend.price, amend.volume, it->time);
        return;
    }

    Order order = Order(amend.order_id, amend.price, amend.volume, ++cur_time);
    queue.remove(it);

    if (is_buy) {
        matchBuy(symbol, order);
    } else {
        matchSell(symbol, order);
    }

}

void CLOBEngine::visitInsert(Insert const &insert) {
    Order order = Order(insert.order_id, insert.price, insert.volume, ++cur_time);
    order_symbols[order.order_id] = insert.symbol;
    order_sides[order.order_id] = insert.side;

    switch (insert.side) {
        case Side::BUY:
            matchBuy(insert.symbol, order);
            break;
        case Side::SELL:
            matchSell(insert.symbol, order);
            break;
    }

}

void CLOBEngine::visitAmend(Amend const &amend) {
    auto it_symbol = order_symbols.find(amend.order_id);
    if (it_symbol == order_symbols.end()) {
        return;
    }

    Symbol symbol = it_symbol->second;
    Side side = order_sides[amend.order_id];

    switch (side) {
        case Side::BUY:
            amendImpl(buys[symbol], symbol, amend, true);
            break;
        case Side::SELL: {
            amendImpl(sells[symbol], symbol, amend, false);
            break;
        }
    }
}

void CLOBEngine::visitPull(Pull const &pull) {
    auto it = order_symbols.find(pull.order_id);
    if (it == order_symbols.end()) {
        return;
    }
    Symbol symbol = it->second;
    Side side = order_sides[pull.order_id];

    switch (side) {
        case Side::BUY:
            remove(buys, symbol, pull.order_id);
            break;
        case Side::SELL:
            remove(sells, symbol, pull.order_id);
            break;
    }

}

struct Item {
    int price;
    int volume;

    Item(int price, int volume) : price(price), volume(volume) {}
};

template<typename Comp>
std::vector<Item> extractItems(priority_queue<Order, Comp> &queue) {
    std::vector<Item> items = std::vector<Item>();
    if (queue.empty()) {
        return items;
    }
    Order order = queue.top();
    queue.pop();
    int cur_price = order.price;
    int cur_volume = order.volume;
    while (!queue.empty()) {
        order = queue.top();
        queue.pop();
        if (order.price == cur_price) {
            cur_volume += order.volume;
        } else {
            items.emplace_back(cur_price, cur_volume);
            cur_price = order.price;
            cur_volume = order.volume;
        }
    }
    items.emplace_back(cur_price, cur_volume);
    return items;
}

std::vector<OrderBook> CLOBEngine::getOrderBooks() {
    std::vector<Symbol> symbols = std::vector<Symbol>();
    symbols.reserve(buys.size() + sells.size());

    auto buys_copy = buys;
    auto sells_copy = sells;

    for (auto const &it : buys_copy) {
        symbols.push_back(it.first);
    }
    for (auto const &r : sells_copy) {
        symbols.push_back(r.first);
    }

    std::sort(symbols.begin(), symbols.end());
    symbols.erase(std::unique(symbols.begin(), symbols.end()), symbols.end());

    std::vector<OrderBook> info = std::vector<OrderBook>();
    for (Symbol const &symbol : symbols) {
        auto it_buys = buys_copy.find(symbol);
        auto it_sells = sells_copy.find(symbol);

        std::vector<Item> items_bids, items_asks;
        if (it_buys != buys_copy.end()) {
            items_bids = extractItems(it_buys->second);
        } else {
            items_bids = std::vector<Item>();
        }
        if (it_sells != sells_copy.end()) {
            items_asks = extractItems(it_sells->second);
        } else {
            items_asks = std::vector<Item>();
        }

        std::vector<OrderBook::OrderBookItem> items = std::vector<OrderBook::OrderBookItem>();
        auto it_items_bids = items_bids.begin();
        auto it_items_asks = items_asks.begin();
        while (it_items_bids != items_bids.end() && it_items_asks != items_asks.end()) {
            auto bid = *it_items_bids++;
            auto ask = *it_items_asks++;
            items.emplace_back(bid.price, bid.volume, ask.price, ask.volume);
        }
        while (it_items_bids != items_bids.end()) {
            auto bid = *it_items_bids++;
            items.push_back(OrderBook::OrderBookItem::bid(bid.price, bid.volume));
        }
        while (it_items_asks != items_asks.end()) {
            auto ask = *it_items_asks++;
            items.push_back(OrderBook::OrderBookItem::ask(ask.price, ask.volume));
        }

        info.emplace_back(symbol, items);
    }
    return info;
}

std::vector<Trade> CLOBEngine::getTrades() {
    return trades;
}

template<typename Comp>
void push(std::unordered_map<std::string, priority_queue<Order, Comp>> &orders, Symbol const &symbol,
          Order const &order) {
    auto it = orders.find(symbol);
    if (it == orders.end()) {
        auto queue = priority_queue<Order, Comp>();
        queue.push(order);
        orders[symbol] = queue;
    } else {
        it->second.push(order);
    }
}

template<typename Comp>
void remove(std::unordered_map<std::string, priority_queue<Order, Comp>> &orders, Symbol const &symbol,
            OrderId order_id) {
    priority_queue<Order, Comp> &orders_by_symbol = orders[symbol];
    orders_by_symbol.remove_if(predicateOrderId(order_id));
    if (orders_by_symbol.empty()) {
        orders.erase(symbol);
    }
}

std::ostream &operator<<(std::ostream &os, Order const &order) {
    os << "order_id: " << order.order_id << " price: " << order.price << " volume: " << order.volume << " time: "
       << order.time;
    return os;
}
