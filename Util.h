#pragma once

#include <sstream>
#include <type_traits>

template<typename T>
T FromString(string key)
{
    if constexpr(std::is_same_v<T,string>)
        return key;

    std::stringstream sstr(key);
	T ret;
	sstr >> ret;
	return ret;
}

template<typename T>
string ToString(T key)
{
    if constexpr(std::is_same_v<T,string>)
        return key;

    std::stringstream sstr;
	sstr << key;
	return sstr.str();
}

template<typename T>
T Concat(const vector<T>& lines, T linefeed)
{
    T ret;
    for(auto& line: lines)
    {
        ret += line;
        ret += linefeed;
    }
    return ret;
}
