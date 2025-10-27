#include "StringUtils.h"

bool isValidPositiveInteger(const String &str)
{
    if (str.length() == 0)
        return false;
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (!isDigit(str[i]))
            return false;
    }
    return true;
}