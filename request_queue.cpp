#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(std::string const& raw_query, DocumentStatus status) {
    auto const& result = server_.FindTopDocuments(raw_query, status);
    Distributor(raw_query, result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(std::string const& raw_query) {
    auto const& result = server_.FindTopDocuments(raw_query);
    Distributor(raw_query, result.size());
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_results_;
}

//---private:

void RequestQueue::Distributor(std::string const& raw_query, size_t const size) {
    // ���� requests_ ��� �������� 1440 ���������, ��������� ������� ������,
    // ����� ��������� ��������� ������ ������� - ���� ����������� ��� ���, �� --empty_results_
    if (requests_.size() == min_in_day_) {
        if (requests_.front().results_count == 0) {
            --empty_results_;
        }
        requests_.pop_front();
    }

    //������� QueryResult � ���������, ���� ��� size == 0, ++empty_results_
    if (size == 0) {
        ++empty_results_;
    }
    requests_.push_back(QueryResult{ raw_query, size });
}