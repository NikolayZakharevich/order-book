#include "engine.hpp"

#include <vector>
#include <unordered_map>

// push wrapper

template<typename Compare>
void push(Queues<Compare> &queues, Symbol const &symbol, Order const &order);

// remove wrappers

template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, OrderId order_id);

template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, std::vector<Order>::iterator it_order);

template<typename Compare>
void remove(Queues<Compare> &queues, typename Queues<Compare>::iterator it_queue,
            std::vector<Order>::iterator it_order);

// order books helper

template<typename Compare>
std::vector<OrderBook::Item> extractItems(Queue<Compare> queue);

// implementation

CLOBEngine::CLOBEngine() noexcept {
    buys = std::unordered_map<Symbol, Queue<BuysComparator>>();
    sells = std::unordered_map<Symbol, Queue<SellsComparator>>();
    trades = std::vector<Trade>();

    order_infos = std::unordered_map<OrderId, OrderInfo>();
    cur_time = 0;
}

void CLOBEngine::visitInsert(Insert const &insert) {
    Order order(insert.order_id, insert.price, insert.volume, ++cur_time);
    order_infos.insert({order.order_id, OrderInfo(insert.symbol, insert.side)});
    switch (insert.side) {
        case Side::BUY:
            matchImpl(buys, sells, insert.symbol, order, true);
            break;
        case Side::SELL:
            matchImpl(sells, buys, insert.symbol, order, false);
            break;
    }
}

void CLOBEngine::visitAmend(Amend const &amend) {
    auto it_info = order_infos.find(amend.order_id);
    if (it_info == order_infos.end()) {
        return;
    }
    Symbol symbol = it_info->second.symbol;
    switch (it_info->second.side) {
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
    auto it_info = order_infos.find(pull.order_id);
    if (it_info == order_infos.end()) {
        return;
    }
    Symbol symbol = it_info->second.symbol;
    switch (it_info->second.side) {
        case Side::BUY:
            remove(buys, symbol, pull.order_id);
            break;
        case Side::SELL:
            remove(sells, symbol, pull.order_id);
            break;
    }
}

std::vector<OrderBook> CLOBEngine::getOrderBooks() {
    std::vector<Symbol> symbols;
    symbols.reserve(buys.size() + sells.size());

    for (auto const &it : buys) {
        symbols.push_back(it.first);
    }
    for (auto const &r : sells) {
        symbols.push_back(r.first);
    }

    std::sort(symbols.begin(), symbols.end());
    symbols.erase(std::unique(symbols.begin(), symbols.end()), symbols.end());

    std::vector<OrderBook> order_books;
    order_books.reserve(symbols.size());

    for (Symbol const &symbol : symbols) {
        std::vector<OrderBook::Item> items_bids, items_asks;
        auto it_buys = buys.find(symbol);
        if (it_buys != buys.end()) {
            items_bids = extractItems(it_buys->second);
        }
        auto it_sells = sells.find(symbol);
        if (it_sells != sells.end()) {
            items_asks = extractItems(it_sells->second);
        }
        order_books.emplace_back(symbol, items_bids, items_asks);
    }
    return order_books;
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
    // if volume is 0 then order is either invalid order or already matched
    while (aggressive_order.volume > 0) {
        // check for passive orders queue
        auto it_passive_orders = passive_queues.find(symbol);
        // if there are no passive orders, push order to queue
        if (it_passive_orders == passive_queues.end() || it_passive_orders->second.empty()) {
            push(aggressive_queues, symbol, aggressive_order);
            return;
        }

        // get best passive order from queue
        Queue<ComparePassive> &passive_orders = it_passive_orders->second;
        // unsafe access by reference useful if want just update it's volume
        Order &best_passive_order = passive_orders.top();

        // orders matched if buy price is lower or equal than sell price
        bool is_match = (is_buy && best_passive_order.price <= aggressive_order.price) ||
                        (!is_buy && aggressive_order.price <= best_passive_order.price);
        // if there is no match, push order to queue
        if (!is_match) {
            push(aggressive_queues, symbol, aggressive_order);
            return;
        }

        // if there is a match, save a trade
        Price price = (is_buy ? aggressive_order : best_passive_order).price;
        Volume volume = std::min(best_passive_order.volume, aggressive_order.volume);
        trades.emplace_back(symbol, price, volume, aggressive_order.order_id, best_passive_order.order_id);

        // update orders volume
        best_passive_order.volume -= volume;
        aggressive_order.volume -= volume;

        // drop current best passive order if need
        if (best_passive_order.volume == 0) {
            passive_orders.pop();
        }
    }
}


template<typename Compare>
void CLOBEngine::amendImpl(Queues<Compare> &queues, Symbol const &symbol, Amend amend, bool is_buy) {
    auto it_queue = queues.find(symbol);
    if (it_queue == queues.end()) {
        return;
    }
    Queue<Compare> &queue = it_queue->second;
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
        matchImpl(buys, sells, symbol, order, is_buy);
    } else {
        matchImpl(sells, buys, symbol, order, is_buy);
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

// remove wrappers implementation

template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, OrderId order_id) {
    auto it_queue = queues.find(symbol);
    if (it_queue == queues.end()) {
        // mustn't happen
        return;
    }
    auto it_order = it_queue->second.find(order_id);
    if (it_order != it_queue->second.end()) {
        remove(queues, it_queue, it_order);
    }
}

template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, std::vector<Order>::iterator it_order) {
    auto it_queue = queues.find(symbol);
    if (it_queue == queues.end()) {
        // mustn't happen
        return;
    }
    remove(queues, it_queue, it_order);
}

template<typename Compare>
void remove(Queues<Compare> &queues, typename Queues<Compare>::iterator it_queue,
            std::vector<Order>::iterator it_order) {
    // remove order from queue
    it_queue->second.remove(it_order);
    // if no orders left in this queue, remove it
    if (it_queue->second.empty()) {
        queues.erase(it_queue);
    }
}

template<typename Compare>
std::vector<OrderBook::Item> extractItems(Queue<Compare> queue) {
    std::vector<OrderBook::Item> items = std::vector<OrderBook::Item>();
    if (queue.empty()) {
        return items;
    }
    Order order = queue.top();
    queue.pop();
    Price cur_price = order.price;
    Volume cur_volume = order.volume;
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