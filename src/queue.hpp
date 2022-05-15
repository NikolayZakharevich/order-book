#pragma once

#include <algorithm>
#include <vector>

template<typename T, class Compare>
class priority_queue {

public:

    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;

    priority_queue() {
        cmp = Compare();
        elements = std::vector<T>();
    }

    T top();

    void remove(iterator it);

    iterator find(T element);

    bool empty();

    void pop();

    void push(const T &element);

    const_iterator end() const { return elements.end(); }

private:

    std::vector<T> elements;
    Compare cmp;

    void makeHeap();

    void siftUp(size_t i);

    void siftDown(size_t i);
};


template<typename T, class Compare>
T priority_queue<T, Compare>::top() {
    return *elements.begin();
}


template<typename T, class Compare>
void priority_queue<T, Compare>::push(T const &element) {
    elements.push_back(element);
    if (elements.size() > 1) {
        siftUp(elements.size() - 1);
    }
}

template<typename T, class Compare>
typename priority_queue<T, Compare>::iterator priority_queue<T, Compare>::find(T element) {
    return std::find(elements.begin(), elements.end(), element);
}

template<typename T, class Compare>
void priority_queue<T, Compare>::remove(priority_queue::iterator it) {
    elements.erase(it);
    makeHeap();
}

template<typename T, class Compare>
bool priority_queue<T, Compare>::empty() {
    return elements.empty();
}

template<typename T, class Compare>
void priority_queue<T, Compare>::makeHeap() {
    size_t max = elements.size() / 2;
    for (size_t i = 0; i <= max; i++) {
        siftDown(max - i);
    }
}

template<typename T, class Compare>
void priority_queue<T, Compare>::siftUp(size_t i) {
    while (cmp(elements[i], elements[(i - 1) / 2])) {
        std::swap(elements[i], elements[(i - 1) / 2]);
    }
}


template<typename T, class Compare>
void priority_queue<T, Compare>::pop() {
    elements.front() = elements.back();
    elements.pop_back();
    siftDown(0);
}

template<typename T, class Compare>
void priority_queue<T, Compare>::siftDown(size_t i) {
    while (2 * i + 1 < elements.size()) {
        size_t left = 2 * i + 1;
        size_t right = 2 * i + 2;
        size_t j = left;
        if (right < elements.size() && cmp(elements[right], elements[left])) {
            j = right;
        }
        if (!cmp(elements[j], elements[i])) {
            break;
        }
        std::swap(elements[i], elements[j]);
        i = j;
    }
}