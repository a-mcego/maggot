#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <regex>
#include <algorithm>
#include <map>

#include <cstdint>
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

using std::cout, std::endl, std::string, std::vector, std::ifstream, std::map;

inline void KILL(const string& s)
{
    std::cerr << "ERROR: " << s << endl;
    std::terminate();
}

struct Token
{
    string type, data;
};

