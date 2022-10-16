// -------- класс  Paginator и его периферия ----------
#pragma once
#include<iosfwd>

//класс для хранения двух итераторов - начала и конца страницы
template <typename Iterator>
class IteratorRange {
public:

    IteratorRange(Iterator range_begin, Iterator range_end)
        :begin_(range_begin), end_(range_end)
    {
    }

    size_t size() const {
        return distance(begin_, end_);
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

private:
    Iterator begin_;
    Iterator end_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {
        Iterator current_it = range_begin;
        do {
            if (std::distance(current_it, range_end) <= page_size) {
                pages_.push_back(IteratorRange(current_it, range_end));
                break;
            }
            Iterator previous_it = current_it;
            advance(current_it, page_size);
            pages_.push_back(IteratorRange(previous_it, current_it));
        } while (true);
    }

    auto GetPages() const {
        return pages_;
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

//фукнция для удобного обращения к классу Paginator
template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(c.begin(), c.end(), page_size);
}

//оператор вывода класса IteratorRange
template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator> documents) {
    auto current_it = documents.begin();
    while (current_it != documents.end()) {
        os << *current_it;
        std::advance(current_it, 1);
    }
    return os;
}