#pragma once
#include<vector>
#include<string_view>

//����� ������ ���� string �� �����, ��������� ������� ��� �����������
std::vector<std::string_view> SplitIntoWords(const std::string_view text);