#ifndef MULTIMAPTUPLE_H_
#define MULTIMAPTUPLE_H_

#include <string>

// Note: This struct simply gives back information to the user.
// Note: Must write my own version of this that uses cstrings
struct MultiMapTuple
{
    std::string key;
    std::string value;
    std::string context;
};

#endif // MULTIMAPTUPLE_H_