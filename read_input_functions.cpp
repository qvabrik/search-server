#include "read_input_functions.h"

#include <iostream>

//считывает строку
std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

//считывает число и очищает поток вывода
int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}