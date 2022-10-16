#include "string_processing.h"
#include <iostream>
#include <algorithm>

//����� ������ ���� string �� �����, ��������� ������� ��� �����������

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    text.remove_prefix(std::min(text.find_first_not_of(" "), text.size())); // �� ������� �������
    while (!text.empty()) {
        size_t position_first_space = text.find(' ');
        std::string_view tmp_substr = text.substr(0, position_first_space);
        result.push_back(tmp_substr);
        text.remove_prefix(tmp_substr.size());
        text.remove_prefix(std::min(text.find_first_not_of(" "), text.size())); // �� ������� �������
    }
    return result;
}