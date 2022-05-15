#pragma once

#include <vector>
#include <unordered_map>

template<typename Key, typename Value, class Compare>
class priority_queue {

public:

    typedef typename std::vector<Value>::iterator iterator;
    typedef typename std::vector<Value>::const_iterator const_iterator;

    priority_queue() {}

    priority_queue(std::function<Key(Value)> const &value_to_key) : value_to_key(value_to_key) {
        cmp = Compare();
        values = std::vector<Value>();
        key_indexes = std::unordered_map<Key, size_t>();
    }

    /**
     * Inserts the element to the queue.
     * O(log(n)) time complexity
     */
    void push(const Value &value);

    /**
     * Returns the minimum element from the queue
     * O(1) time complexity
     */
    Value top() const;

    /**
     * Returns the minimum element from the queue. Allows to change the element so it can broke the class invariant
     * O(1) time complexity
     */
    Value &top();

    /**
     * If such element exists in the queue returns the iterator pointing at it.
     * Otherwise, returns iterator the end of queue {@see end()}
     * O(1) time complexity
     */
    iterator find(Value value);

    /**
     * If element with such key exists in the queue returns the iterator pointing at it.
     * Otherwise, returns iterator the end of queue {@see end()}
     * O(1) time complexity
     */
    iterator find(Key key);

    /**
     * If iterator is valid, removes the element from queue
     * O(log(n)) time complexity
     */
    void remove(iterator it);

    /**
     * Removes the minimum element from queue. To get the minimum element {@see top()}
     * O(log(n)) time complexity
     */
    void pop();

    /**
     * `true` if there are no elements in queue, `false` otherwise
     * O(1) time complexity
     */
    bool empty();

    /**
     * Return the iterator to the queue end
     */
    iterator end() { return values.end(); }

    /**
     * Return the iterator to the queue end
     */
    const iterator end() const { return values.end(); }

private:

    std::vector<Value> values;
    std::function<Key(Value)> const value_to_key;
    std::unordered_map<Key, size_t> key_indexes;
    Compare cmp;

    void siftUp(size_t index_from);

    void siftDown(size_t index_from);

    void remove(size_t index_from);

    void swap(size_t index_from, size_t index_to);
};

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::push(Value const &value) {
    values.push_back(value);
    size_t index_back = values.size() - 1;
    key_indexes[value_to_key(value)] = index_back;
    if (index_back > 0) {
        siftUp(index_back);
    }
}

template<typename Key, typename Value, class Compare>
Value &priority_queue<Key, Value, Compare>::top() {
    return *values.begin();
}

template<typename Key, typename Value, class Compare>
Value priority_queue<Key, Value, Compare>::top() const {
    return top();
}

template<typename Key, typename Value, class Compare>
typename priority_queue<Key, Value, Compare>::iterator priority_queue<Key, Value, Compare>::find(Value value) {
    return find(value_to_key(value));
}

template<typename Key, typename Value, class Compare>
typename priority_queue<Key, Value, Compare>::iterator priority_queue<Key, Value, Compare>::find(Key key) {
    auto it = key_indexes.find(key);
    if (it == key_indexes.end()) {
        return end();
    }
    return values.begin() + it->second;
}

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::pop() {
    remove(0);
}

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::remove(priority_queue::iterator it) {
    remove(it - values.begin());
}

template<typename Key, typename Value, class Compare>
bool priority_queue<Key, Value, Compare>::empty() {
    return values.empty();
}


template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::siftUp(size_t index_from) {
    while (index_from > 0 && cmp(values[index_from], values[(index_from - 1) / 2])) {
        swap(index_from, (index_from - 1) / 2);
        index_from = (index_from - 1) / 2;
    }
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
        swap(index_from, index_to);
        index_from = index_to;
    }
}

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::swap(size_t index_from, size_t index_to) {
    Key key_from = value_to_key(values[index_from]);
    Key key_to = value_to_key(values[index_to]);
    key_indexes[key_from] = index_to;
    key_indexes[key_to] = index_from;
    std::swap(values[index_from], values[index_to]);
}

template<typename Key, typename Value, class Compare>
void priority_queue<Key, Value, Compare>::remove(size_t index_from) {
    size_t index_back = values.size() - 1;
    swap(index_from, index_back);
    key_indexes.erase(index_back);
    values.pop_back();
    siftDown(index_from);
}
