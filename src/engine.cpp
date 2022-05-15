#include "engine.hpp"

#include <vector>
#include <unordered_map>

template<typename Compare>
void push(Queues<Compare> &queues, Symbol const &symbol, Order const &order);

template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, OrderId order_id);

template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, std::vector<Order>::iterator it_order);

CLOBEngine::CLOBEngine() noexcept {
    buys = std::unordered_map<Symbol, Queue<BuysComparator>>();
    sells = std::unordered_map<Symbol, Queue<SellsComparator>>();
    trades = std::vector<Trade>();

    order_symbols = std::unordered_map<OrderId, Symbol>();
    order_sides = std::unordered_map<OrderId, Side>();
    cur_time = 0;
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
            amendImpl(buys, symbol, amend, true);
            break;
        case Side::SELL: {
            amendImpl(sells, symbol, amend, false);
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


template<typename Compare>
std::vector<OrderBook::Item> extractItems(Queue<Compare> &queue) {
    std::vector<OrderBook::Item> items = std::vector<OrderBook::Item>();
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

        std::vector<OrderBook::Item> items_bids, items_asks;
        if (it_buys != buys_copy.end()) {
            items_bids = extractItems(it_buys->second);
        } else {
            items_bids = std::vector<OrderBook::Item>();
        }
        if (it_sells != sells_copy.end()) {
            items_asks = extractItems(it_sells->second);
        } else {
            items_asks = std::vector<OrderBook::Item>();
        }

        info.emplace_back(symbol, items_bids, items_asks);
    }
    return info;
}

std::vector<Trade> CLOBEngine::getTrades() {
    return trades;
}

template<typename CompareAggressive, typename ComparePassive>
void CLOBEngine::matchImpl(
        Queues<CompareAggressive> &aggressive_queues,
        Queues<ComparePassive> &passive_queues,
        Symbol symbol,
        Order &aggressive_order,
        bool is_buy
) {
    while (true) {

        auto it_passive_orders = passive_queues.find(symbol);
        if (it_passive_orders == passive_queues.end() || it_passive_orders->second.empty()) {
            push(aggressive_queues, symbol, aggressive_order);
            return;
        }
        Queue<ComparePassive> &passive_orders = it_passive_orders->second;

        Order best_passive_order = passive_orders.top();
        bool is_not_match = (is_buy && aggressive_order.price < best_passive_order.price) ||
                            (!is_buy && best_passive_order.price < aggressive_order.price);
        if (is_not_match) {
            push(aggressive_queues, symbol, aggressive_order);
            return;
        }

        int price = (is_buy ? aggressive_order : best_passive_order).price;
        int volume = std::min(best_passive_order.volume, aggressive_order.volume);
        trades.emplace_back(symbol, price, volume, aggressive_order.order_id, best_passive_order.order_id);

        if (best_passive_order.volume > aggressive_order.volume) {
            best_passive_order.volume -= aggressive_order.volume;
            auto it = passive_orders.find(best_passive_order.order_id);
            remove(passive_queues, symbol, it);
            push(passive_queues, symbol, best_passive_order);
            return;
        } else if (aggressive_order.volume > best_passive_order.volume) {
            aggressive_order.volume -= best_passive_order.volume;
            passive_orders.pop();
        } else {
            passive_orders.pop();
            return;
        }
    }
}


template<typename Compare>
void CLOBEngine::amendImpl(Queues<Compare> &queues, Symbol const &symbol, Amend amend, bool is_buy) {
    Queue<Compare> &queue = queues[symbol];
    auto it = queue.find(amend.order_id);
    if (it == queue.end()) {
        return;
    }
    if (it->price == amend.price && it->volume > amend.volume) {
        *it = Order(it->order_id, it->price, amend.volume, it->time);
        return;
    }

    Order order = Order(amend.order_id, amend.price, amend.volume, ++cur_time);
    remove(queues, symbol, it);

    if (is_buy) {
        matchBuy(symbol, order);
    } else {
        matchSell(symbol, order);
    }

}

template<typename Compare>
void push(Queues<Compare> &queues, Symbol const &symbol, Order const &order) {
    auto it = queues.find(symbol);
    if (it == queues.end()) {
        auto queue = Queue<Compare>([](const Order &order) { return order.order_id; });
        queue.push(order);
        queues.insert({symbol, queue});
    } else {
        it->second.push(order);
    }
}

template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, OrderId order_id) {
    auto it_order = queues[symbol].find(order_id);
    if (it_order != queues[symbol].end()) {
        remove(queues, symbol, it_order);
    }
}

template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, std::vector<Order>::iterator it_order) {
    queues[symbol].remove(it_order);
    if (queues[symbol].empty()) {
        queues.erase(symbol);
    }
}