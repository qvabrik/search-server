/*фреймворк ASSERT*/
#pragma once
#include<iostream>
#include<vector>
#include<set>
#include<map>

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, const std::pair<Key, Value>& container) {
    os << container.first << ": " << container.second;
    return os;
}

template<typename Container>
std::ostream& Print(std::ostream& os, const Container& container) {
    bool isFirst = true;
    for (auto const& element : container) {
        if (isFirst) {
            os << element;
            isFirst = false;
            continue;
        }
        os << ", " << element;
    }
    return os;
}

template<typename Term>
std::ostream& operator<<(std::ostream& os, const std::vector<Term>& container) {
    os << "[";
    Print(os, container);
    os << "]";
    return os;
}


template<typename Term>
std::ostream& operator<<(std::ostream& os, const std::set<Term>& container) {
    os << "{";
    Print(os, container);
    os << "}";
    return os;
}

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, const std::map<Key, Value>& container) {
    os << "{";
    Print(os, container);
    os << "}";
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "(" << line << "): " << func << ": ";
        std::cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cerr << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cerr << " Hint: " << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, hint)

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint);

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, #hint)

template <typename Func>
void RunTestImpl(const Func& func, const std::string& func_name) {
    func();
    std::cerr << func_name << " OK" << std::endl;
}

#define RUN_TEST(func) RunTestImpl ((func), #func)

// -------- конец фреймворка ASSERT ----------