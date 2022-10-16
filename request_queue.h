#pragma once
#include<string>
#include<vector>
#include<deque>

#include"document.h"
#include"search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(SearchServer const& search_server) : server_(search_server) {
    }

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(std::string const& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(std::string const& raw_query);

    int GetNoResultRequests() const;

private:
    const SearchServer& server_;
    struct QueryResult {
        std::string query;
        size_t results_count;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int empty_results_ = 0;

    void Distributor(std::string const& raw_query, size_t const size);
};


template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto const& result = server_.FindTopDocuments(raw_query, document_predicate);
    Distributor(raw_query, result.size());
    return result;
}