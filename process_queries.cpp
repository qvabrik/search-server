#include "process_queries.h"

#include <execution>
#include <functional>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](std::string const& query) { return search_server.FindTopDocuments(query); });
    return result;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    return std::transform_reduce(
        std::execution::par,
        queries.begin(), queries.end(),
        std::vector<Document>{},
        [](std::vector<Document> T1, std::vector<Document> const& T2) {
            T1.insert(T1.end(), T2.begin(), T2.end());
            return T1;
        },
        [&search_server](std::string const& query) { return search_server.FindTopDocuments(query); }
    );
}