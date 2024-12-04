#pragma once

#include <string>

namespace ccxt {

class Precise {
public:
    Precise(const std::string& str = "0");
    Precise(const char* str);
    Precise(int value);
    Precise(long value);
    Precise(double value);
    
    static Precise string_mul(const std::string& string1, const std::string& string2);
    static Precise string_div(const std::string& string1, const std::string& string2, int precision = 18);
    static Precise string_add(const std::string& string1, const std::string& string2);
    static Precise string_sub(const std::string& string1, const std::string& string2);
    static Precise string_mod(const std::string& string1, const std::string& string2);
    static Precise string_pow(const std::string& string1, const std::string& string2);
    static int string_eq(const std::string& string1, const std::string& string2);
    static int string_gt(const std::string& string1, const std::string& string2);
    static int string_ge(const std::string& string1, const std::string& string2);
    static int string_lt(const std::string& string1, const std::string& string2);
    static int string_le(const std::string& string1, const std::string& string2);
    
    Precise mul(const Precise& other) const;
    Precise div(const Precise& other, int precision = 18) const;
    Precise add(const Precise& other) const;
    Precise sub(const Precise& other) const;
    Precise mod(const Precise& other) const;
    Precise pow(const Precise& other) const;
    bool eq(const Precise& other) const;
    bool gt(const Precise& other) const;
    bool ge(const Precise& other) const;
    bool lt(const Precise& other) const;
    bool le(const Precise& other) const;
    
    std::string toString() const;
    double toDouble() const;
    
private:
    std::string value;
    static std::string normalize(const std::string& string);
};

} // namespace ccxt
