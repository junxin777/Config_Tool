#ifndef __UTILS_H__
#define __UTILS_H__

#include <ctime>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/x509.h>
#include <openssl/x509v3.h>

using namespace std;

namespace project
{

string util_delta_to_days_and_secs(time_t);
size_t util_split_str(const string &, char, vector<string> &);
size_t util_trim_leading_char(string &, const char);
size_t util_trim_tailing_char(string &, const char);
string util_extract_path(const string &);
bool util_is_file_existing(const string &);

template<typename T>
    string util_val_to_str(const T &var, const T &invalid_val, const string &invalid_str)
{
    ostringstream oss;
    if (var == invalid_val)
        oss << invalid_str;
    else
        oss << var;
    return oss.str();
}

} // namespace project

#endif // __UTILS_H__
