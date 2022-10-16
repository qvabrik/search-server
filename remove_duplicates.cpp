#include <map>
#include <set>
#include <vector>
#include <iostream>

#include <vector>

#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	std::map<std::set<std::string>, int> unic_documents;
	std::vector<int> ids_for_delete;

	bool isFirst = true;
	for (const auto actual_id : search_server) {
		//добавим слова документа во временное хранилище
		auto actual_document_map = search_server.GetWordFrequencies(actual_id);
		std::set<std::string> actual_document;
		for (auto it = actual_document_map.begin(); it != actual_document_map.end(); ++it) {
			actual_document.insert(std::string(it->first));
		}

		// если первый, то просто добавим документ в unic_documents
		if (isFirst) {
			unic_documents.emplace(actual_document, actual_id);
			isFirst = false;
			continue;
		}
		bool isDuplicate = false;

		if (unic_documents.contains(actual_document)) {
			actual_id > unic_documents.at(actual_document) ? ids_for_delete.push_back(actual_id) : ids_for_delete.push_back(unic_documents.at(actual_document));
			actual_id > unic_documents.at(actual_document) ? std::cout << "Found duplicate document id " << actual_id << "\n" : std::cout << "Found duplicate document id " << unic_documents.at(actual_document) << "\n";
			isDuplicate = true;
		}

		// если документ не дубликат, добавим его в unic_documents
		if (!isDuplicate) {
			unic_documents.emplace(actual_document, actual_id);
		}
	}
	// удалим документы с id из вектора ids_for_delete
	for (const int id : ids_for_delete) {
		search_server.RemoveDocument(id);
	}
}