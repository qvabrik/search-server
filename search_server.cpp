#include <numeric>
#include <cmath>
#include <execution>
#include <list>

#include "search_server.h"
#include "log_duration.h"


//constructors
SearchServer::SearchServer(const std::string& set_stop_words)
    : SearchServer(SplitIntoWords(set_stop_words))
{}

SearchServer::SearchServer(const std::string_view set_stop_words)
    : SearchServer(SplitIntoWords(set_stop_words))
{}


void SearchServer::SetStopWords(const std::string& text) {
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word " + std::string(word) + " contains special characters.");
        }
        stop_words_.insert(std::string(word));
    }
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    //проверка корректности id
    if (document_id < 0 || documents_.count(document_id)) {
        throw std::invalid_argument("incorrect id");
    }
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    //добавим слова и их TF в словари
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        document_to_words_freqs_[document_id][std::string(word)] += inv_word_count;
        word_to_document_freqs_[(document_to_words_freqs_[document_id].find(word)->first)][document_id] += inv_word_count;
    }
    documents_.emplace(document_id,
        DocumentData{
            ComputeAverageRating(ratings),
            status
        });
    //добавим порядковый номер и id в index_to_id и обновим счётчик
    ids_.push_back(document_id);
}

//FindTopDocuments для одного запроса, в этом случае статус ACTUAL по умолчанию
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query);
}

//FindTopDocuments для запроса и определённого статуса
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus required_status) const {
    return FindTopDocuments(std::execution::seq, raw_query, required_status);
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

//обработка множества слов запроса, распределение на плюс и минус слова с учётом стоп-слов
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const {
    Query query = ParseQuery(raw_query);
    return MatchDocument(query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const {
    Query_vector query = ParseQuery_Vector(raw_query);
    return MatchDocument(std::execution::par, query, document_id);
}

std::vector<std::tuple<int, std::vector<std::string_view>, DocumentStatus>> SearchServer::MatchDocuments(const std::string_view raw_query) const {
    Query query = ParseQuery(raw_query);
    std::vector<std::tuple<int, std::vector<std::string_view>, DocumentStatus>> result;
    for (auto const& [id, data] : documents_) {
        auto [words, status] = MatchDocument(query, id);
        result.push_back({ id , words, status });
    }
    return result;
}

const std::map<std::string, double, std::less<>>& SearchServer::GetWordFrequencies(int document_id) const {
    if (!documents_.count(document_id)) {
        static std::map<std::string, double, std::less<>> empty;
        return empty;
    }
    return document_to_words_freqs_.at(document_id);
}

//int SearchServer::GetDocumentId(int index) const {
//    if (index_to_id_.size() > index) {
//        return index_to_id_[index];
//    }
//    throw std::out_of_range("There are no documents with this index");
//}

std::vector<int>::iterator SearchServer::begin() {
    return ids_.begin();
}

std::vector<int>::iterator SearchServer::end() {
    return ids_.end();
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    if (!documents_.count(document_id))
        return;

    std::map<std::string, double, std::less<>> words;
    {
        auto it = document_to_words_freqs_.find(document_id);
        if (it != document_to_words_freqs_.end()) {
            words = document_to_words_freqs_.at(document_id);
            document_to_words_freqs_.erase(it);
        }
    }
    {
        for (const auto [word, freq] : words) {
            if (word_to_document_freqs_.count(word) != 0) {
                auto it = word_to_document_freqs_.at(word).find(document_id);
                if (it != word_to_document_freqs_.at(word).end()) {
                    word_to_document_freqs_.at(word).erase(document_id);
                }
            }
        }
    }
    {
        auto it = documents_.find(document_id);
        if (it != documents_.end()) {
            documents_.erase(it);
        }
    }
    {
        auto it = remove(ids_.begin(), ids_.end(), document_id);
        ids_.erase(it, ids_.end());
    }
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (!documents_.count(document_id))
        return;

    std::map<std::string, double, std::less<>> const& words = document_to_words_freqs_.at(document_id);
    std::for_each(
        std::execution::par,
        words.begin(), words.end(),
        [&](auto const& word) {
            word_to_document_freqs_.at(word.first).erase(document_id);
        }
    );

    document_to_words_freqs_.erase(document_id);
    documents_.erase(document_id);
    ids_.erase(remove(ids_.begin(), ids_.end(), document_id), ids_.end());
}

//--private functions:

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("document contains special characters");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view word) const {
    //проверим, что слово не пустой минус (-) и не содержит нескольких вначале (--слово)
    if (word == "-" || (word.size() > 1 && word[1] == '-') || !IsValidWord(word)) {
        throw std::invalid_argument("Query contains empty minus or several minuses before word or special characters.");
    }
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    return {
        word,
        is_minus,
        IsStopWord(word)
    };
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const
{
    Query query;
    for (const std::string_view word : SplitIntoWords(text)) {
        //определим слово в плюс или минус слова
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
            query.size++;
        }
    }
    return query;
}

SearchServer::Query_vector SearchServer::ParseQuery_Vector(const std::string_view text) const {
    Query_vector query;
    for (const std::string_view word : SplitIntoWords(text)) {
        //определим слово в плюс или минус слова
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
            query.size++;
        }
    }
    return query;
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(Query const& query, int document_id) const {
    CheckId(document_id);

    auto check_lambda = [&](auto&& word) { return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id); };
    if (std::any_of(std::execution::seq, query.minus_words.begin(), query.minus_words.end(), check_lambda)) {
        return std::tuple{ std::vector<std::string_view>{}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words(query.size);

    std::copy_if(std::execution::seq,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(), check_lambda);

    auto it = std::find(std::execution::seq, matched_words.begin(), matched_words.end(), "");
    while (it != matched_words.end()) {
        matched_words.erase(it);
        it = std::find(std::execution::seq, matched_words.begin(), matched_words.end(), "");
    }

    std::vector<std::string_view> result(matched_words.size());
    size_t number = 0;
    for (const auto& word : matched_words) {
        result[number] = ((*word_to_document_freqs_.find(word)).first);
        ++number;
    }

    return std::tuple{ result, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, Query_vector& query, int document_id) const {
    CheckId(document_id);
    
    auto check_lambda = [&](auto&& word) { return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id); };
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), check_lambda)) {
        return std::tuple{ std::vector<std::string_view>{}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words(query.size);

    std::copy_if(std::execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(), check_lambda);

    std::sort(std::execution::par, matched_words.begin(), matched_words.end());

    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());

    auto it = std::find(std::execution::par, matched_words.begin(), matched_words.end(), "");
    if (it != matched_words.end())
        matched_words.erase(it);

    std::vector<std::string_view> result(matched_words.size());
    std::transform(
        std::execution::par,
        matched_words.begin(), matched_words.end(),
        result.begin(),
        [&](const std::string_view word) {
            return (*word_to_document_freqs_.find(word)).first;
        });

    return std::tuple{ result, documents_.at(document_id).status };
}

//вычисление IDF
double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void SearchServer::CheckId(int document_id) const {
    if (document_id < 0 || document_to_words_freqs_.count(document_id) == 0) {
        throw std::out_of_range("The index is wrong");
    }
}

//--additional functions:
void FindTopDocuments(SearchServer const& server, std::string const& query) {
    LOG_DURATION_STREAM("FindTopDocuments", std::cout);
    std::cout << "Результаты поиска по запросу: " << query << std::endl;
    for (auto const& document : server.FindTopDocuments(query)) {
        PrintDocument(document);
    }
}

std::ostream& operator<<(std::ostream& os, std::tuple<int, std::vector<std::string_view>, DocumentStatus> data) {
    auto [id, words, status] = data;
    os << "{ document_id = " << id << ", status = " << static_cast<int>(status) << ", words =";
    for (std::string_view const word : words) {
        os << " " << word;
    }
    os << "}\n";
    return os;
}

void MatchDocuments(SearchServer const& server, std::string const& query) {
    LOG_DURATION_STREAM("MatchDocuments", std::cout);
    std::cout << "Матчинг документов по запросу: " << query << std::endl;
    for (auto const& data : server.MatchDocuments(query)) {
        std::cout << data;
    }
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}