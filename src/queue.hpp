#pragma once

#include <algorithm>
#include <vector>

template<typename Key, typename Element, class Compare>
class priority_queue {

public:

    typedef typename std::vector<Element>::iterator iterator;
    typedef typename std::vector<Element>::const_iterator const_iterator;

    priority_queue() {}
    priority_queue(std::function<Key(Element)> const &element_to_key) : element_to_key(element_to_key) {
        cmp = Compare();
        elements = std::vector<Element>();
    }

    Element top() const;

    Element &top();

    void remove(iterator it);

    iterator find(Element element);

    bool empty();

    void pop();

    void push(const Element &element);

    const_iterator end() const { return elements.end(); }

private:

    std::vector<Element> elements;
    std::function<Key(Element)> const element_to_key;
    Compare cmp;

    void makeHeap();

    void siftUp(size_t i);

    void siftDown(size_t i);
};


template<typename Key, typename Element, class Compare>
Element &priority_queue<Key, Element, Compare>::top() {
    return *elements.begin();
}

template<typename Key, typename Element, class Compare>
Element priority_queue<Key, Element, Compare>::top() const {
    return top();
}


template<typename Key, typename Element, class Compare>
void priority_queue<Key, Element, Compare>::push(Element const &element) {
    elements.push_back(element);
    if (elements.size() > 1) {
        siftUp(elements.size() - 1);
    }
}

template<typename Key, typename Element, class Compare>
typename priority_queue<Key, Element, Compare>::iterator priority_queue<Key, Element, Compare>::find(Element element) {
    return std::find(elements.begin(), elements.end(), element);
}

template<typename Key, typename Element, class Compare>
void priority_queue<Key, Element, Compare>::remove(priority_queue::iterator it) {
    elements.erase(it);
    makeHeap();
}

template<typename Key, typename Element, class Compare>
bool priority_queue<Key, Element, Compare>::empty() {
    return elements.empty();
}

template<typename Key, typename Element, class Compare>
void priority_queue<Key, Element, Compare>::makeHeap() {
    size_t max = elements.size() / 2;
    for (size_t i = 0; i <= max; i++) {
        siftDown(max - i);
    }
}

template<typename Key, typename Element, class Compare>
void priority_queue<Key, Element, Compare>::siftUp(size_t i) {
    while (i > 0 && cmp(elements[i], elements[(i - 1) / 2])) {
        std::swap(elements[i], elements[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}


template<typename Key, typename Element, class Compare>
void priority_queue<Key, Element, Compare>::pop() {
    elements.front() = elements.back();
    elements.pop_back();
    siftDown(0);
}

template<typename Key, typename Element, class Compare>
void priority_queue<Key, Element, Compare>::siftDown(size_t i) {
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