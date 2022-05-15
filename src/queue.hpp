#pragma once

#include <algorithm>
#include <vector>

template<typename Key, typename Value, class Compare>
class priority_queue {

public:

    typedef typename std::vector<Value>::iterator iterator;
    typedef typename std::vector<Value>::const_iterator const_iterator;

    priority_queue() {}
    priority_queue(std::function<Key(Value)> const &value_to_key) : value_to_key(value_to_key) {
        cmp = Compare();
        values = std::vector<Value>();
    }

    Value top() const;

    Value &top();

    void remove(iterator it);

    iterator find(Value value);

    bool empty();

    void pop();

    void push(const Value &value);

    const_iterator end() const { return values.end(); }

private:

    std::vector<Value> values;
    std::function<Key(Value)> const value_to_key;
    Compare cmp;

    void makeHeap();

    void siftUp(size_t index_from);

    void siftDown(size_t index_from);
};


template<typename Key, typename Value, class Compare>
Value &priority_queue<Key, Value, Compare>::top() {
    return *values.begin();
}

template<typename Key, typename Value, class Compare>
Value priority_queue<Key, Value, Compare>::top() const {
    return top();
}


template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::push(Value const &value) {
    values.push_back(value);
    if (values.size() > 1) {
        siftUp(values.size() - 1);
    }
}

template<typename Key, typename Value, class Compare>
typename priority_queue<Key, Value, Compare>::iterator priority_queue<Key, Value, Compare>::find(Value value) {
    return std::find(values.begin(), values.end(), value);
}

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::remove(priority_queue::iterator it) {
    values.erase(it);
    makeHeap();
}

template<typename Key, typename Value, class Compare>
bool priority_queue<Key, Value, Compare>::empty() {
    return values.empty();
}

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::makeHeap() {
    size_t max = values.size() / 2;
    for (size_t i = 0; i <= max; i++) {
        siftDown(max - i);
    }
}

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::siftUp(size_t index_from) {
    while (index_from > 0 && cmp(values[index_from], values[(index_from - 1) / 2])) {
        std::swap(values[index_from], values[(index_from - 1) / 2]);
        index_from = (index_from - 1) / 2;
    }
}


template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::pop() {
    values.front() = values.back();
    values.pop_back();
    siftDown(0);
}

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::siftDown(size_t index_from) {
    while (2 * index_from + 1 < values.size()) {
        size_t index_left = 2 * index_from + 1;
        size_t index_right = 2 * index_from + 2;
        size_t index_to = index_left;
        if (index_right < values.size() && cmp(values[index_right], values[index_left])) {
            index_to = index_right;
        }
        if (!cmp(values[index_to], values[index_from])) {
            break;
        }
        std::swap(values[index_from], values[index_to]);
        index_from = index_to;
    }
}