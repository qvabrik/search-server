#pragma once

void TestExcludeStopWordsFromAddedDocumentContent();
void TestGetWordFrequencies();
void TestExcludeDocumentsWithMinusWords();
void TestMatchingDocument();
void TestSortByRelevance();
void TestComputeAverageRating();
void TestFindTopDocumentByPredicate();
void TestFindTopDocumentByStatus();
void TestParallelFindTopDocumentByPredicate();
void TestParallelFindTopDocumentByStatus();
void TestCalculateRelevance();
//void TestGetDocumentId();
void TestRequestQueue();
void TestRemoveDuplicates();
void TestProcessQueries();

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();
