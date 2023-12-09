#include <sys/stat.h>

#include <cassert>
#include <cstring>
#include <string>

#include "utils.h"

using namespace project;
using namespace std;

namespace project
{

string util_delta_to_days_and_secs(time_t delta)
{
    ostringstream oss;
    int n_day = (int)(delta / 86400);
    int n_sec = (int)(delta % 86400);
    if (n_day)
        oss << n_day << " day"
            << (n_day > 1 ? "s" : "");
    if (n_sec)
        oss << (n_day ? " and " : "")
            << n_sec << " second"
            << (n_sec > 1 ? "s" : "");
    if (!delta)
        oss << "0 seconds";
    return oss.str();
}

size_t util_split_str(const string &str, char delim, vector<string> &tokens)
{
    string::size_type prev = 0, pos = 0;
    while ((pos = str.find_first_of(delim, prev)) != string::npos)
    {
        tokens.push_back(string(str, prev, pos - prev));
        prev = pos + 1;
    }
    tokens.push_back(string(str, prev, str.length() - prev));
    return tokens.size();
}

size_t util_trim_leading_char(string &str, const char c)
{
    size_t pos = str.find_first_not_of(c);
    if (pos != string::npos)
        str = str.substr(pos);
    else if (!str.empty())
        str.clear();
    return str.size();
}

size_t util_trim_tailing_char(string &str, const char c)
{
    size_t pos = str.find_last_not_of(c);
    if (pos != string::npos)
        str.erase(pos + 1);
    else if (!str.empty())
        str.clear();
    return str.size();
}

string util_extract_path(const string &fpath)
{
    size_t pos = fpath.find_last_of("/\\");
    return fpath.substr(0, pos);
}

bool util_is_file_existing(const string &fpath)
{
    struct stat st;
    return (stat(fpath.c_str(), &st) == 0);
}

} // namespace project
