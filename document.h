#pragma once
#include <iosfwd>

struct Document {

    Document() = default;

    Document(const int new_id, const double new_relevance, const int new_rating)
        :id(new_id),
        relevance(new_relevance),
        rating(new_rating)
    {
    }

    int id = 0;
    double relevance = 0.;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

void PrintDocument(const Document& document);

std::ostream& operator<<(std::ostream& os, const Document& document);