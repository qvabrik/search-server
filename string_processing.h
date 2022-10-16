#pragma once
#include<vector>
#include<string_view>

//делит запрос типа string на слова, используя пробелы как разделители
std::vector<std::string_view> SplitIntoWords(const std::string_view text);