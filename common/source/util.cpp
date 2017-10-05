#include <util.hpp>

using namespace std;

vector<string> split(string str, const string& delimiter) {
    vector<string> parts;
    size_t last = 0;
    size_t next = 0;
    
    while ((next = str.find(delimiter, last)) != string::npos) {
        parts.emplace_back(str.substr(last, next - last));
        last = next + 1;
    }
    
    parts.emplace_back(str.substr(last));
    return parts;
}
