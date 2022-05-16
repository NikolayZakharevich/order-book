#include "engine.hpp"

#include <vector>
#include <unordered_map>

/* push wrapper */

/**
 * Create a queue if it's not exists and push order
 */
template<typename Compare>
void push(Queues<Compare> &queues, Symbol const &symbol, Order const &order);

/* remove wrappers */

/**
 * Remove order from queue by order_id, erase queue if it becomes empty
 */
template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, OrderId order_id);

/**
 * Remove order from queue by iterator, erase queue if it becomes empty
 */
template<typename Compare>
void remove(Queues<Compare> &queues, Symbol const &symbol, std::vector<Order>::iterator it_order);

/* order books helper */

/**
 * Formats order books
 */
template<typename Compare>
std::vector<OrderBook::Item> formatItems(Queue<Compare> queue);

/* CLOBEngine definition  */

CLOBEngine::CLOBEngine() {
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
            insertImpl(buys, sells, insert.symbol, order, true);
            break;
        case Side::SELL:
            insertImpl(sells, buys, insert.symbol, order, false);
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
            items_bids = formatItems(it_buys->second);
        }
        auto it_sells = sells.find(symbol);
        if (it_sells != sells.end()) {
            items_asks = formatItems(it_sells->second);
        }
        order_books.emplace_back(symbol, items_bids, items_asks);
    }
    return order_books;
}

std::vector<Trade> CLOBEngine::getTrades() {
    return trades;
}

/* CLOBEngine implementation details */

template<typename CompareAggressive, typename ComparePassive>
void CLOBEngine::insertImpl(
        Queues<CompareAggressive> &aggressive_queues,
        Queues<ComparePassive> &passive_queues,
        Symbol symbol,
        Order &aggressive_order,
        bool is_buy
) {
    // check for passive orders queue
    auto it_passive_orders = passive_queues.find(symbol);
    // if there are no passive orders, push order to queue
    if (it_passive_orders == passive_queues.end()) {
        push(aggressive_queues, symbol, aggressive_order);
        return;
    }

    // if volume is 0 then order is either invalid or already matched
    while (aggressive_order.volume > 0) {

        // if there are no passive orders left, cleanup and push order to queue
        if (it_passive_orders->second.empty()) {
            passive_queues.erase(it_passive_orders);
            push(aggressive_queues, symbol, aggressive_order);
            return;
        }

        // unsafe access by reference useful if want just update it's volume
        Order &best_passive_order = it_passive_orders->second.top();

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
            it_passive_orders->second.pop();
        }
    }
}


template<typename Compare>
void CLOBEngine::amendImpl(Queues<Compare> &queues, Symbol const &symbol, Amend amend, bool is_buy) {
    auto it_queue = queues.find(symbol);
    if (it_queue == queues.end()) {
        // mustn't happen
        return;
    }
    auto it_order = it_queue->second.find(amend.order_id);
    if (it_order == it_queue->second.end()) {
        // unknown order_id passed
        return;
    }

    // order doesn't lose time priority if the only change is the volume decrease
    if (it_order->price == amend.price && it_order->volume > amend.volume) {
        *it_order = Order(it_order->order_id, it_order->price, amend.volume, it_order->time);
        return;
    }

    // if there are any other changes amend is equal to insert
    remove(queues, symbol, it_order);
    Order order = Order(amend.order_id, amend.price, amend.volume, ++cur_time);
    if (is_buy) {
        insertImpl(buys, sells, symbol, order, is_buy);
    } else {
        insertImpl(sells, buys, symbol, order, is_buy);
    }
}

/* push wrapper implementation */

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

/* remove wrappers implementation */

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
void remove(Queues<Compare> &queues, Symbol const &symbol, OrderId order_id) {
    auto it_queue = queues.find(symbol);
    if (it_queue == queues.end()) {
        // mustn't happen because of client's code checks
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
        // mustn't happen because of client's code checks
        return;
    }
    remove(queues, it_queue, it_order);
}

/**
 * Queue is copied because method removes it's elements (heap sort)
 */
template<typename Compare>
std::vector<OrderBook::Item> formatItems(Queue<Compare> queue) {
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