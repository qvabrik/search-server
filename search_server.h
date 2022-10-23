#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>
#include <execution>
#include <string_view>
#include <thread>
#include <future>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double INACCURACY = 1e-6;

class SearchServer {
public:
    //constructors
    SearchServer() = default;
    explicit SearchServer(const std::string& set_stop_words);
    explicit SearchServer(const std::string_view stop_words_text);
    template<typename Container>
    SearchServer(const Container& container);

    template <typename StringContainer>
    std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& words) {
        std::set<std::string, std::less<>> result(words.begin(), words.end());
        if (result.count("")) { result.erase(""); }
        return result;
    }

    void SetStopWords(const std::string& text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    //ñîðòèðîâêà èòîãîâûõ äîêóìåíòîâ ïî ðåéòèíãó è îòñå÷åíèå ÒÎÏ-5 äîêóìåíòîâ
    //ïðèíèìàåò çàïðîñ è ôóíêòîð
    template < typename DocumentPredicate, class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, const DocumentPredicate& document_predicate) const;
    template< typename DocumentPredicate >
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, const DocumentPredicate& document_predicate) const;

    //FindTopDocuments äëÿ îäíîãî çàïðîñà, â ýòîì ñëó÷àå ñòàòóñ ACTUAL ïî óìîë÷àíèþ
    template< class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    //FindTopDocuments äëÿ çàïðîñà è îïðåäåë¸ííîãî ñòàòóñà
    template< class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus required_status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus required_status) const;

    size_t GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const;

    std::vector<std::tuple<int, std::vector<std::string_view>, DocumentStatus>> MatchDocuments(const std::string_view raw_query) const;

    const std::map<std::string, double, std::less<>>& GetWordFrequencies(int document_id) const;

    //int GetDocumentId(int index) const;

    std::vector<int>::iterator begin();

    std::vector<int>::iterator end();

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

private:
    struct DocumentData {
        int rating = 0;
        DocumentStatus status;
    };

    std::set<std::string, std::less<>> stop_words_;
    // ÷àñòîòà ñëîâà â êàæäîì äîêóìåíòå
    std::unordered_map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    // ÷àñòîòû êàæäîãî ñëîâà â äîêóìåíòå
    std::unordered_map<int, std::map<std::string, double, std::less<>>> document_to_words_freqs_;
    std::map<int, DocumentData> documents_;
    //âåêòîð ids äëÿ ôóíêöèé begin è end
    std::vector<int> ids_;

    bool IsStopWord(const std::string_view word) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;

        size_t size = 0;

        std::string data_;
    };

    struct Query_vector {

        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;

        size_t size = 0;
    };

    Query ParseQuery(const std::string_view text) const;
    Query_vector ParseQuery_Vector(const std::string_view text) const;

    //âû÷èñëåíèå IDF
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    //ïîèñê äîêóìåíòîâ ïî ïîäãîòîâëåííîìó çàïðîñó è çàäàííûì ïàðàìåòðàì
    template <typename Container, typename It, typename DocumentPredicate>
    void ComputeDocumentRelevance(Container& result_container, It range_begin, It range_end, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(Query const& query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, Query_vector& query, int document_id) const;

    void CheckId(int document_id) const;
};

void FindTopDocuments(SearchServer const& server, std::string const& query);

std::ostream& operator<<(std::ostream& os, std::tuple<int, std::vector<std::string>, DocumentStatus>);

void MatchDocuments(SearchServer const& server, std::string const& query);

template<typename Container>
SearchServer::SearchServer(const Container& container)
    :stop_words_(MakeUniqueNonEmptyStrings(container))
{
    if (!all_of(container.begin(), container.end(), IsValidWord))
        throw std::invalid_argument("Stop-words contains special characters.");
}

//======== FindTopDocuments ========

template < typename DocumentPredicate, class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, const DocumentPredicate& document_predicate) const {
    if (!IsValidWord(raw_query)) {
        throw std::invalid_argument("Query contains invalid characters");
    }
    
    Query query = ParseQuery(raw_query);

    std::vector<Document> matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < INACCURACY) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template < typename DocumentPredicate >
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, const DocumentPredicate& document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

//FindTopDocuments äëÿ îäíîãî çàïðîñà, â ýòîì ñëó÷àå ñòàòóñ ACTUAL ïî óìîë÷àíèþ
template< class ExecutionPolicy >
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

//FindTopDocuments äëÿ çàïðîñà è îïðåäåë¸ííîãî ñòàòóñà
template< class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus required_status) const {
    return FindTopDocuments(policy, raw_query, [required_status](int document_id, DocumentStatus status, int rating) { return status == required_status; });
}

//======== FindAllDocuments ========

template <typename Container, typename It, typename DocumentPredicate>
void SearchServer::ComputeDocumentRelevance(Container& result_container, It range_begin, It range_end, DocumentPredicate document_predicate) const {
    for (auto word_it = range_begin; word_it != range_end; word_it = std::next(word_it)) {
        if (!word_to_document_freqs_.count(*word_it))
            continue;
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(*word_it);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(*word_it)) {
            if (document_predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                result_container[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            if (document_predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
            });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const {
    concurrent_map<int, double> document_to_relevance_concurrent(std::thread::hardware_concurrency());

    size_t distance = query.plus_words.size() / std::thread::hardware_concurrency();
    if (query.plus_words.size() < std::thread::hardware_concurrency())
        distance = 1u;

    std::vector<std::future<void>> compute_relevance_futures;

    auto range_begin = query.plus_words.begin();
    auto range_end = query.plus_words.begin();

    while (range_end != query.plus_words.end()) {
        distance < std::distance(range_end, query.plus_words.end()) ? range_end = std::next(range_end, distance) : range_end = query.plus_words.end();
        compute_relevance_futures.push_back(std::async([&, range_begin, range_end] {
            ComputeDocumentRelevance(document_to_relevance_concurrent, range_begin, range_end, document_predicate);
            }));
        range_begin = range_end;
    }
    for (auto& task : compute_relevance_futures) {
        task.wait();
    }
    std::map<int, double> document_to_relevance = document_to_relevance_concurrent.BuildOrdinaryMap();

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
            });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}

//================

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);
