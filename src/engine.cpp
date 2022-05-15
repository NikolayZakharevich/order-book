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


template<typename Comp1, typename Comp2>
void CLOBEngine::match(
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

void CLOBEngine::visitInsert(Insert const &insert) {
    Order order = Order(insert.order_id, insert.price, insert.volume);
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
    Order order = Order(amend.order_id, amend.price, amend.volume);

    switch (side) {
        case Side::BUY:
            remove(buys, symbol, order.order_id);
            matchBuy(symbol, order);
            break;
        case Side::SELL:
            remove(sells, symbol, order.order_id);
            matchSell(symbol, order);
            break;
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


template<typename Comp>
std::vector<OrderBook::OrderBookItem> extractBuyItems(priority_queue<Order, Comp> &buys) {
    std::vector<OrderBook::OrderBookItem> items = std::vector<OrderBook::OrderBookItem>();
    while (!buys.empty()) {
        Order const buy = buys.top();
        buys.pop();
        OrderBook::OrderBookItem item = OrderBook::OrderBookItem();
        item.bid_price = buy.price;
        item.bid_volume = buy.volume;
        items.push_back(item);
    }
    return items;
}

template<typename Comp>
std::vector<OrderBook::OrderBookItem> extractSellItems(priority_queue<Order, Comp> &sells) {
    std::vector<OrderBook::OrderBookItem> items = std::vector<OrderBook::OrderBookItem>();
    while (!sells.empty()) {
        Order const sell = sells.top();
        sells.pop();
        OrderBook::OrderBookItem item = OrderBook::OrderBookItem();
        item.ask_price = sell.price;
        item.ask_volume = sell.volume;
        items.push_back(item);
    }
    return items;
}

template<typename Comp1, typename Comp2>
std::vector<OrderBook::OrderBookItem> extractItems(
        priority_queue<Order, Comp1> &buys,
        priority_queue<Order, Comp2> &sells
) {
    std::vector<OrderBook::OrderBookItem> items = std::vector<OrderBook::OrderBookItem>();
    while (!buys.empty() && !sells.empty()) {
        Order const buy = buys.top();
        Order const sell = sells.top();
        buys.pop();
        sells.pop();

        OrderBook::OrderBookItem item = OrderBook::OrderBookItem();
        item.bid_price = buy.price;
        item.bid_volume = buy.volume;
        item.ask_price = sell.price;
        item.ask_volume = sell.volume;
        items.push_back(item);
    }
    return items;
}

std::vector<OrderBook> CLOBEngine::getOrderBooks() {
    std::vector<Symbol> symbols = std::vector<Symbol>();
    symbols.reserve(buys.size() + sells.size());

    auto buysCopy = buys;
    auto sellsCopy = sells;

    for (auto const &it : buysCopy) {
        symbols.push_back(it.first);
    }
    for (auto const &r : sellsCopy) {
        symbols.push_back(r.first);
    }

    std::sort(symbols.begin(), symbols.end());
    symbols.erase(std::unique(symbols.begin(), symbols.end()), symbols.end());

    std::vector<OrderBook> info = std::vector<OrderBook>();
    for (Symbol const &symbol : symbols) {
        auto it_buys = buysCopy.find(symbol);
        auto it_sells = sellsCopy.find(symbol);

        std::vector<OrderBook::OrderBookItem> items = std::vector<OrderBook::OrderBookItem>();
        if (it_buys == buysCopy.end()) {
            auto items_sells = extractSellItems(it_sells->second);
            items.insert(items.end(), items_sells.begin(), items_sells.end());
        } else if (it_sells == sellsCopy.end()) {
            auto items_buys = extractBuyItems(it_buys->second);
            items.insert(items.end(), items_buys.begin(), items_buys.end());
        } else {
            auto items_both = extractItems(it_buys->second, it_sells->second);
            auto items_sells = extractSellItems(it_sells->second);
            auto items_buys = extractBuyItems(it_buys->second);
            items.insert(items.end(), items_both.begin(), items_both.end());
            items.insert(items.end(), items_sells.begin(), items_sells.end());
            items.insert(items.end(), items_buys.begin(), items_buys.end());
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
    orders_by_symbol.remove_if([&order_id](Order const &item) {
        return item.order_id == order_id;
    });
    if (orders_by_symbol.empty()) {
        orders.erase(symbol);
    }
}

std::ostream &operator<<(std::ostream &os, Order const &order) {
    os << "order_id: " << order.order_id << " price: " << order.price << " volume: " << order.volume;
    return os;
}
