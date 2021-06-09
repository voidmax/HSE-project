#include <iostream>
#include <complex>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <cmath>
#include <bitset>
#include <cassert>
#include <queue>
#include <stack>
#include <deque>
#include <random>
#include <fstream>
using namespace std;

typedef unordered_map<string, string> dict;

enum{ IS_JSON, IS_ARRAY, IS_STRING };

struct json {
    int type = IS_JSON;
    vector<string> data;
    map<string, json*> info;

    json() {}

    json operator[] (string s) {
        assert(info.count(s));
        return *info[s]; 
    }
};

struct column {
    dict info;

    string& operator[] (string s) {
        assert(info.count(s));
        return info[s]; 
    }

    bool have(string s) {
        return info.count(s);
    }
};

struct table_info {
    string name;
    unordered_map<string, column> columns;
    json info;

    column& operator[] (string s) {
        assert(columns.count(s));
        return columns[s]; 
    }

    bool have(string s) {
        return columns.count(s);
    }
};

void ignore_spaces(int& it, string& s, string finish = " \t") {
    set<char> bad(finish.begin(), finish.end());
    while (it < s.size() && bad.count(s[it])) {
        ++it;
    }
}

string read_string(int& it, string& s) {
    ignore_spaces(it, s);
    assert(it < s.size() && s[it] == '"');
    int L = it++;
    while (it < s.size() && s[it] != '"') {
        ++it;
    }
    assert(it < s.size());
    ++it;
    return string(s.begin() + L + 1, s.begin() + it - 1);
}

json* read_json(int& it, string& s) {
    ignore_spaces(it, s);
    if (it == s.size()) {
        return new json();
    }
    json* cur = new json();

    assert(s[it] == '{');
    do {
        assert(s[it] == ',' || (s[it] == '{' && cur->info.empty()));
        ignore_spaces(++it, s);
        string key = read_string(it, s);
        assert(cur->info.count(key) == 0);
        ignore_spaces(it, s);
        assert(it + 1 < s.size() && s[it] == ':');
        ignore_spaces(++it, s);
        if (s[it] == '{') {
            cur->info[key] = read_json(it, s);
        } else if (s[it] == '[') {
            json* tmp = new json();
            tmp->type = IS_ARRAY;
            while (it < s.size() && s[it] != ']') {
                assert(s[it] == ',' || (s[it] == '[' && tmp->data.empty()));
                ignore_spaces(++it, s);
                string str = read_string(it, s);
                tmp->data.push_back(str);
            }
            assert(it < s.size() && s[it] == ']');
            ++it;
            cur->info[key] = tmp;
        } else {
            json* tmp = new json();
            tmp->type = IS_STRING;
            string str = read_string(it, s);
            tmp->data.push_back(str);
            cur->info[key] = tmp;
        }
        ignore_spaces(it, s);
    } while (it < s.size() && s[it] == ',');
    ++it;
    return cur;
}

void print(json* cur, int level = 0) {
    if (cur->type == IS_STRING) {
        cout << cur->data[0];
    } else if (cur->type == IS_ARRAY) {
        cout << "[";
        for (int i = 0; i < cur->data.size(); ++i) {
            if (i) {
                cout << ", ";
            }
            cout << cur->data[i];
        }
        cout << "]";
    } else {
        string tab = string(level * 4, ' ');
        cout << "{\n";
        bool first = true;
        for (auto [key, value] : cur->info) {
            if (!first) {
                cout << ",\n";
            } else {
                first = false;
            }
            cout << tab << "    ";
            cout << key << ": ";
            print(value, level + 1);
        }
        cout << '\n';
        cout << tab << "}";
    }
}

vector<table_info> init_config(string config_path = "config.txt") {    
    ifstream input;
    input.open(config_path.c_str());
    string config, s;
    while (getline(input, s)) {
        config += s;
        config += " "; 
    }
    int it = 0;
    auto data = *read_json(it, config);
    

    vector<string> tables;
    for (auto [key, value] : data["tables"].info) {
        tables.push_back(key);
    }
    sort(tables.begin(), tables.end(), [&](string& a, string& b) {
        return data["tables"][a].info.count("copy") < data["tables"][b].info.count("copy");
    });
    vector<table_info> result;
    map<string, int> id;
    for (auto name : tables) {
        id[name] = result.size();

        table_info table;
        table.name = name;
        auto cur = data["tables"][name];
        table.info = cur["info"];

        if (cur.info.count("copy")) {
            string from = cur["copy"]["table"].data[0];
            table.columns = result[id[from]].columns;
            if (cur["copy"].info.count("except")) {
                for (auto col : cur["copy"]["except"].data) {
                    table.columns.erase(col);
                }
            }
        }
        if (cur.info.count("columns")) {
            for (auto [key, value] : cur["columns"].info) {
                column tmp;
                for (auto [param, str] : value->info) {
                    if (str->type == IS_STRING) {
                        // cout << name << ' ' << param << ' ' << str->data[0] << endl;
                        tmp.info[param] = str->data[0];
                    }
                }
                vector<string> who = {};
                if (value->info.count("copy")) {
                    who = value->info["copy"]->data;
                }
                who.push_back(key);
                for (auto cur_key : who) {
                    table.columns[cur_key] = tmp;
                }
            }
        } 
        result.push_back(table);
    }
    // print(&data);
    return result;
}

vector<string> from_regex(string s) {
    vector<string> result;
    vector<vector<string>> res;
    for (int L = 0; L < s.size(); ++L) {
        if (s[L] != '[') {
            string c;
            c += s[L];
            res.push_back({c});
        } else {
            vector<string> possible;
            ++L;
            assert(L < s.size());
            while (s[L] != ']') {
                assert(L + 1 < s.size());
                if (s[L] == ',') {
                    ++L;
                    continue;
                }
                if (s[L + 1] == ',' || s[L + 1] == ']') {
                    string c;
                    c += s[L];
                    possible.push_back({c});
                    ++L;
                } else if (s[L + 1] == '-') {
                    assert(L + 3 < s.size());
                    int A = s[L];
                    int B = s[L + 2];
                    while (true) {
                        string c;
                        c += A;
                        possible.push_back(c);
                        if (A == B) {
                            break;
                        }
                        A += (A < B ? 1 : -1);
                    }
                    L += 3;
                }
            }
            res.push_back(possible);
        }
    }   
    int number = 1;
    for (auto r : res) {
        number *= r.size();
    }
    for (int i = 0; i < number; ++i) {
        int mask = i;
        string cur = "";
        for (auto r : res) {
            cur += r[mask % r.size()];
            mask /= r.size();
        }
        result.push_back(cur);
    }
    return result;
}