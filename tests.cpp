#include <iostream>
#include <vector>
#include <set>
#include <list>
#include <execution>

#include "tests.h"
#include "search_server.h"
#include "request_queue.h"
#include "assert.h"
#include "remove_duplicates.h"
#include "process_queries.h"

using namespace std;

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto matched_documents = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(matched_documents.size(), 1u);
        ASSERT_EQUAL(matched_documents[0].id, doc_id);
    }
    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto matched_documents = server.FindTopDocuments("in"s);
        ASSERT_HINT(matched_documents.empty(), "Stop words must be excluded from document"s);
    }

    //тестируем добавление стоп-слов при создании объекта класса SearchServer, передаём их как строку
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto matched_documents = server.FindTopDocuments("in"s);
        ASSERT_HINT(matched_documents.empty(), "Stop words added like string while create SearchServer must be excluded from document"s);
    }

    //тестируем добавление стоп-слов при создании объекта класса SearchServer, передаём их как вектор
    {
        const vector<string> set_stop_words = { "in"s, "the"s };
        SearchServer server(set_stop_words);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto matched_documents = server.FindTopDocuments("in"s);
        ASSERT_HINT(matched_documents.empty(), "Stop words added like vector while create SearchServer must be excluded from document"s);
    }

    //тестируем добавление стоп-слов при создании объекта класса SearchServer, передаём их как множество
    {
        const set<string> set_stop_words = { "in"s, "the"s };
        SearchServer server(set_stop_words);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto matched_documents = server.FindTopDocuments("in"s);
        ASSERT_HINT(matched_documents.empty(), "Stop words added like vector while create SearchServer must be excluded from document"s);
    }
}

void TestGetWordFrequencies() {
    SearchServer search_server("and in at"s);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "big big dick in Lola's ass"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    ASSERT_EQUAL(search_server.GetWordFrequencies(1).at("cat"), 0.25);
    ASSERT_EQUAL(search_server.GetWordFrequencies(2).at("big"), 0.4);
    ASSERT_EQUAL(search_server.GetWordFrequencies(3).size(), 0u);
}

void TestExcludeDocumentsWithMinusWords() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 43;
    const string content_1 = "cat in the city London"s;
    const string content_2 = "cat in the city Moscow"s;
    const vector<int> ratings_1 = { 1, 2, 3 };
    const vector<int> ratings_2 = { 2, 3, 4 };
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);

        const auto matched_documents = server.FindTopDocuments("cat -London"s);
        ASSERT_EQUAL(matched_documents[0].id, 43);
        ASSERT_EQUAL(matched_documents.size(), 1u);
    }
}

void TestMatchingDocument() {
    const int doc_id = 42;
    const string content = "cat in the city Moscow"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        //тест на возврат 0 совпадающих слов
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto [matched_words, status] = server.MatchDocument("Hello in there"s, doc_id);
        //в этом случае совпадений нет, возвращаемый вектор должен быть пустой
        ASSERT_EQUAL(matched_words.size(), 0u);
    }
    {
        //тест на возврат 1 слова
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto [matched_words, status] = server.MatchDocument("Hello cat in there"s, doc_id);
        //в этом случае функция должна вернуть вектор с 1м словом - cat
        ASSERT_EQUAL(matched_words.size(), 1u);
        ASSERT_EQUAL(matched_words[0], "cat"s);
    }
    {
        //тест на возврат 3 слов
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);

        const auto [matched_words, status] = server.MatchDocument("cat in the city Moscow"s, doc_id);
        //в этом случае функция должна вернуть 3 слова - Moscow, cat, city (именно такой порядок, алфавитный, т.к. обработанный запрос возвращается в виде set)
        ASSERT_EQUAL(matched_words.size(), 3u);
        ASSERT_EQUAL(matched_words[0], "Moscow"s);
        ASSERT_EQUAL(matched_words[1], "cat"s);
        ASSERT_EQUAL(matched_words[2], "city"s);
    }
    {
        //тест на возврат документа, содержащего минус-слово
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto [matched_words, status] = server.MatchDocument("cat in the city -Moscow"s, doc_id);
        //в этом случае совпадения есть, но так же есть минус слово, поэтому возвращаемый вектор должен быть пустой
        ASSERT_EQUAL(matched_words.size(), 0u);
    }
}

void TestParallelMatchingDocument() {
    const int doc_id = 42;
    const string content = "cat in the city Moscow"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        //тест на возврат 0 совпадающих слов
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto [matched_words, status] = server.MatchDocument(std::execution::par, "Hello in there"s, doc_id);
        //в этом случае совпадений нет, возвращаемый вектор должен быть пустой
        ASSERT_EQUAL(matched_words.size(), 0u);
    }
    {
        //тест на возврат 1 слова
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto [matched_words, status] = server.MatchDocument(std::execution::par, "Hello cat in there"s, doc_id);
        //в этом случае функция должна вернуть вектор с 1м словом - cat
        ASSERT_EQUAL(matched_words.size(), 1u);
    }
    {
        //тест на возврат 3 слов
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);

        const auto [matched_words, status] = server.MatchDocument(std::execution::par, "cat in the city Moscow"s, doc_id);
        ASSERT_EQUAL(matched_words.size(), 3u);
    }
    {
        //тест на возврат документа, содержащего минус-слово
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto [matched_words, status] = server.MatchDocument(std::execution::par, "cat in the city -Moscow"s, doc_id);
        //в этом случае совпадения есть, но так же есть минус слово, поэтому возвращаемый вектор должен быть пустой
        ASSERT_EQUAL(matched_words.size(), 0u);
    }
    {
        //тест на возврат документа по запросу с дублями
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto [matched_words, status] = server.MatchDocument(std::execution::par, "cat cat cat in the city Moscow"s, doc_id);
        //в этом случае совпадения есть, но так же есть минус слово, поэтому возвращаемый вектор должен быть пустой
        ASSERT_EQUAL(matched_words.size(), 3u);
    }
}

void TestSortByRelevance() {
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city London"s;
    const vector<int> ratings_1 = { 1, 2, 3 };
    const int doc_id_2 = 2;
    const string content_2 = "cat in the city Moscow"s;
    const vector<int> ratings_2 = { 1, 2, 3 };
    const int doc_id_3 = 3;
    const string content_3 = "fluffer cat in the city Moscow"s;
    const vector<int> ratings_3 = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);

    int id = 3;

    const auto matched_documents = server.FindTopDocuments("fluffer cat in Moscow"s);
    for (const auto& document : matched_documents) {
        ASSERT_HINT(document.id == id, "Sort by relevance doesn't working OK"s);
        --id;
    }
}

void TestComputeAverageRating() {
    //тут запустим три теста - с пустым вектором рейтинга, с одним значением = 0 и с тремя обычными значениями
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city London"s;
    const vector<int> ratings_1 = { };
    const int doc_id_2 = 2;
    const string content_2 = "cat in the city Moscow"s;
    const vector<int> ratings_2 = { 0 };
    const int doc_id_3 = 3;
    const string content_3 = "fluffer cat in the city Moscow"s;
    const vector<int> ratings_3 = { 1, 2, 3, 1000 };

    //считаем среднее значение для ratings_3, другие рейтинги нулевые
    int average_rating_3 = (1 + 2 + 3 + 1000) / 4;

    SearchServer server;
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);

    const auto matched_documents = server.FindTopDocuments("fluffer cat in Moscow"s);
    ASSERT_HINT(matched_documents[0].rating == average_rating_3, "Average rating with few inputs doesn't working OK"s);
    ASSERT_HINT(matched_documents[1].rating == 0, "Average rating with no inputs doesn't working OK"s);
    ASSERT_HINT(matched_documents[2].rating == 0, "Average rating with 1 input doesn't working OK"s);
}

void TestFindTopDocumentByPredicate() {
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city London"s;
    const vector<int> ratings_1 = { 1, 3, 5 };
    const int doc_id_2 = 2;
    const string content_2 = "cat in the city Moscow"s;
    const vector<int> ratings_2 = { 10, 20, 30 };
    const int doc_id_3 = 3;
    const string content_3 = "fluffer cat in the city Moscow"s;
    const vector<int> ratings_3 = { 1, 2, 3, 1000 };

    SearchServer server;
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);

    //тестируем приём рейтинга через предикат
    {
        const auto matched_documents = server.FindTopDocuments("fluffer cat in Moscow"s, [](int document_id, DocumentStatus status, int rating) { return rating > 100; });
        ASSERT_HINT(matched_documents.size() == 1u, "Predicat doesn't working OK with ratings"s);
    }

    //тестируем приём id через предикат
    {
        const auto matched_documents = server.FindTopDocuments("fluffer cat in Moscow"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 1; });
        ASSERT_HINT(matched_documents.size() == 2u, "Predicat doesn't working OK with ids"s);
    }
}

void TestFindTopDocumentByStatus() {
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city London"s;
    const vector<int> ratings_1 = { 1, 3, 5 };
    const int doc_id_2 = 2;
    const string content_2 = "cat in the city Moscow"s;
    const vector<int> ratings_2 = { 10, 20, 30 };
    const int doc_id_3 = 3;
    const string content_3 = "fluffer cat in the city Moscow"s;
    const vector<int> ratings_3 = { 1, 2, 3, 1000 };

    SearchServer server;
    server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::IRRELEVANT, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::IRRELEVANT, ratings_3);

    {
        const auto matched_documents = server.FindTopDocuments("fluffer cat in Moscow"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(matched_documents[0].id, 1);
    }
    {
        const auto matched_documents = server.FindTopDocuments("fluffer cat in Moscow"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(matched_documents.size(), 2u);
    }
    {
        const auto matched_documents = server.FindTopDocuments("fluffer cat in Moscow"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL_HINT(matched_documents.size(), 0u, "FindTopDocumentsWithStatus return missing values"s);
    }
}

void TestParallelFindTopDocumentByPredicate() {
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city London"s;
    const vector<int> ratings_1 = { 1, 3, 5 };
    const int doc_id_2 = 2;
    const string content_2 = "cat in the city Moscow"s;
    const vector<int> ratings_2 = { 10, 20, 30 };
    const int doc_id_3 = 3;
    const string content_3 = "fluffer cat in the city Moscow"s;
    const vector<int> ratings_3 = { 1, 2, 3, 1000 };

    SearchServer server;
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);

    //тестируем приём рейтинга через предикат
    {
        const auto matched_documents = server.FindTopDocuments(std::execution::par, "fluffer cat in Moscow"s, [](int document_id, DocumentStatus status, int rating) { return rating > 100; });
        ASSERT_HINT(matched_documents.size() == 1u, "Predicat doesn't working OK with ratings"s);
    }

    //тестируем приём id через предикат
    {
        const auto matched_documents = server.FindTopDocuments(std::execution::par, "fluffer cat in Moscow"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 1; });
        ASSERT_HINT(matched_documents.size() == 2u, "Predicat doesn't working OK with ids"s);
    }
}

void TestParallelFindTopDocumentByStatus() {
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city London"s;
    const vector<int> ratings_1 = { 1, 3, 5 };
    const int doc_id_2 = 2;
    const string content_2 = "cat in the city Moscow"s;
    const vector<int> ratings_2 = { 10, 20, 30 };
    const int doc_id_3 = 3;
    const string content_3 = "fluffer cat in the city Moscow"s;
    const vector<int> ratings_3 = { 1, 2, 3, 1000 };

    SearchServer server;
    server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::IRRELEVANT, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::IRRELEVANT, ratings_3);

    {
        const auto matched_documents = server.FindTopDocuments(std::execution::par, "fluffer cat in Moscow"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(matched_documents[0].id, 1);
    }
    {
        const auto matched_documents = server.FindTopDocuments(std::execution::par, "fluffer cat in Moscow"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(matched_documents.size(), 2u);
    }
    {
        const auto matched_documents = server.FindTopDocuments(std::execution::par, "fluffer cat in Moscow"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL_HINT(matched_documents.size(), 0u, "FindTopDocumentsWithStatus return missing values"s);
    }
};

void TestCalculateRelevance() {
    const int doc_id_0 = 0;
    const string content_0 = "big chicken in the city Krakov"s;
    const int doc_id_1 = 1;
    const string content_1 = "angry dog in the city Samara"s;
    const int doc_id_2 = 2;
    const string content_2 = "cat in the city Moscow"s;
    const int doc_id_3 = 3;
    const string content_3 = "fluffer cat and big cat in the city Moscow"s;
    const vector<int> ratings = { 1, 3, 5 };

    SearchServer server;
    server.SetStopWords("in the and");
    server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings);

    const auto matched_documents = server.FindTopDocuments("fluffer cat in Moscow"s);
    //релевантность 0 не включается в результат
    ASSERT_EQUAL_HINT(matched_documents.size(), 2u, "Documents with relevance == 0 should not be uncluded in result"s);

    //релевантность не 0, на примере "cat in the city Moscow"s
    double TF_cat = 2. / 3.;
    double IDF_cat = log(4. / 2.);
    double TF_IDF = TF_cat * IDF_cat;
    ASSERT_EQUAL(matched_documents[1].relevance, TF_IDF);
}

//void TestGetDocumentId() {
//    SearchServer server;
//    (void)server.AddDocument(1, "Flaffer cat in Moscow"s, DocumentStatus::ACTUAL, { 1, 3, 5 });
//    (void)server.AddDocument(3, "Cat in Moscow"s, DocumentStatus::ACTUAL, { 1, 3, 5 });
//    (void)server.AddDocument(5, "Moscow"s, DocumentStatus::ACTUAL, { 1, 3, 5 });
//    ASSERT_EQUAL(server.GetDocumentId(2), 5);
//}

void TestRequestQueue() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }

    // все еще 1439 запросов с нулевым результатом
    ASSERT_EQUAL(request_queue.GetNoResultRequests(), 1439u);

    request_queue.AddFindRequest("curly dog"s);
    request_queue.AddFindRequest("big collar"s);
    request_queue.AddFindRequest("sparrow"s);

    // 1437 запросов с нулевым результатом
    ASSERT_EQUAL(request_queue.GetNoResultRequests(), 1437u);
}

void TestRemoveDuplicates() {
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    ASSERT_EQUAL(search_server.GetDocumentCount(), 9);
    cout.clear(ios_base::badbit);
    RemoveDuplicates(search_server);
    cout.clear(ios_base::goodbit);
    ASSERT_EQUAL(search_server.GetDocumentCount(), 5);
}

void TestProcessQueries() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    int results[] = { 3, 5, 2 };
    id = 0;
    for (const auto& documents : ProcessQueries(search_server, queries)) {
        ASSERT_EQUAL(documents.size(), results[id++]);
    }
    {
        SearchServer search_server("and with"s);
        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }
        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
        ASSERT_EQUAL(ProcessQueriesJoined(search_server, queries).size(), 10u);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestGetWordFrequencies);
    RUN_TEST(TestExcludeDocumentsWithMinusWords);
    RUN_TEST(TestMatchingDocument);
    RUN_TEST(TestParallelMatchingDocument);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestFindTopDocumentByPredicate);
    RUN_TEST(TestFindTopDocumentByStatus);
    RUN_TEST(TestParallelFindTopDocumentByPredicate);
    RUN_TEST(TestParallelFindTopDocumentByStatus);
    RUN_TEST(TestCalculateRelevance);
    //RUN_TEST(TestGetDocumentId);
    RUN_TEST(TestRequestQueue);
    RUN_TEST(TestRemoveDuplicates);
    RUN_TEST(TestProcessQueries);
}

