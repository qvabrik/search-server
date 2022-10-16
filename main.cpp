#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "assert.h"
#include "tests.h"
#include "log_duration.h"
#include "remove_duplicates.h"
#include "process_queries.h"

using namespace std;

#include <execution>
#include <random>
#include <string>

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution(97, 122)(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void TestMatchDocument(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(std::string{ mark });
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id) {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += words.size();
    }
    cout << word_count << endl;
}

template <typename ExecutionPolicy>
void TestFindTopDocuments(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(std::string{ mark });
    const int document_count = search_server.GetDocumentCount();
    auto words = search_server.FindTopDocuments(policy, query);
    size_t word_count = words.size();
    cout << word_count << endl;
}

#define TEST_MD(policy) TestMatchDocument(#policy, search_server, query, execution::policy)
#define TEST_FTD(policy) TestFindTopDocuments(#policy, search_server, query, execution::policy)

int main() {
    setlocale(LC_ALL, "Russian"); // задаём русский текст
    system("chcp 1251"); // настраиваем кодировку консоли

    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cerr << "Search server testing finished\n---"s << endl;

    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

    const string query = GenerateQuery(generator, dictionary, 500, 0.1);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    TEST_FTD(seq);
    TEST_FTD(par);
}