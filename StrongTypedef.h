#pragma once

template<char ... Chars>
class wrapped_name_strongtypedef {};

template <char... s>
constexpr auto operator""_wrapstrongtypedef()
{ return wrapped_name_strongtypedef< char(s)... >(); }

template<u64 STRLEN>
consteval u64 name1(const char(&c)[STRLEN])
{
    static_assert(STRLEN <= 18);
    u64 ret = 0;
    for (u64 i = 0; i < STRLEN; i += 2)
        ret = (ret * 128) + int(c[i]);
    return ret;
}
template<u64 STRLEN>
consteval u64 name2(const char(&c)[STRLEN])
{
    static_assert(STRLEN <= 18);
    u64 ret = 0;
    for (u64 i = 1; i < STRLEN; i += 2)
        ret = (ret * 128) + int(c[i]);
    return ret;
}

namespace StrongTypedef
{
    template<typename T, u64 n1, u64 n2>
    struct Strong
    {
    private:
        T t{};
    public:
        T& toTmut() { return t; }
        const T& toT() const { return t; }

        constexpr Strong() = default;
        constexpr Strong( const Strong & ) = default;
        constexpr Strong( Strong && ) = default;
        constexpr Strong& operator=( const Strong & ) = default;

        constexpr Strong(const T& val): t(val) {}

        auto operator<=>(const Strong&) const = default;

        //reimplement all the operators! \o/
        Strong& operator++(){ ++t; return *this; }

    #define implement_op(OP) \
        template<typename STR> \
        Strong& operator OP ## = (const STR& rhs)\
        {\
            if constexpr(std::is_same_v<Strong,STR>)\
                t OP ## = rhs.t;\
            else\
                t OP ## = rhs;\
            return *this;\
        }\
        template<typename STR> \
        Strong operator OP (const STR& rhs) const\
        {\
            Strong ret = *this;\
            if constexpr(std::is_same_v<Strong,STR>)\
                ret OP ## = rhs.t;\
            else\
                ret OP ## = rhs;\
            return ret;\
        }

        implement_op(+);
        implement_op(-);
        implement_op(*);
        implement_op(/);
        implement_op(%);
        implement_op(<<);
        implement_op(>>);
    #undef implement_op

        explicit operator bool() const
        {
            static_assert(std::same_as<T,bool>, "StrongTypedef::operator bool() only available when T = bool!");
            return bool(t);
        }
    };
}

template<typename T, u64 N1, u64 N2>
std::ostream& operator<<(std::ostream& o, const StrongTypedef::Strong<T,N1,N2>& t)
{
    o << t.toT();
    return o;
}

//#define MakeStrong(name, contained) using name = StrongTypedef::Strong<contained, decltype(#name ## _wrapstrongtypedef)>;
#define MakeStrong(name, contained) using name = StrongTypedef::Strong<contained, name1(#name), name2(#name)>;
