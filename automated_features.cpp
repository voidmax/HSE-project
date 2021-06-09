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

#include "tqdm.h"
#include "markup.h"
#include "functions.h"
#include "table.h"
#include "encoding.h"

unordered_map<string, table> database;

void extract_features(table& node, table& edge, string save_path) {
    ofstream output;
    output.open(save_path.c_str());
    tqdm bar;
    int lst = 0;

    unordered_map<string, vector<pair<string, column>>> node_columns;
    for (auto [name, col] : node.info.columns) {
        node_columns[col["type"]].push_back({name, col});
        // cerr << "node " << name << endl;
    } 
    unordered_map<string, vector<pair<string, column>>> edge_columns;
    for (auto [name, col] : edge.info.columns) {
        edge_columns[col["type"]].push_back({name, col});
        // cerr << "edge " << name << endl;
    } 

    string node_key = node_columns["key"][0].first;
    string edge_key = edge_columns["node_key"][0].first;
    string edge_order = edge_columns.count("date") ? edge_columns["date"][0].first : "";
    cerr << "\nSorting " << node.info.name << "..." << endl;
    node.sorting(node_key);
    cerr << "Sorting " << edge.info.name << "..." << endl;
    edge.sorting(edge_key);
    cerr << "Preparing features for " << node.info.name << "..." << endl;

    double min_elem = 1e9, max_elem = -1e9;
    for (int i = 0; i < node.rows.size(); ++i) {
        vector<double> node_coors;
        for (auto [name, col] : node_columns["coordinates"]) {
            if (node.get(i, name).size()) {
                double cur = to_double(node.get(i, name));
                min_elem = min(min_elem, cur);
                max_elem = max(max_elem, cur);
            }
        }
    }
    for (auto [name, col] : edge_columns["coordinates"]) {
        for (int i = 0; i < edge.rows.size(); ++i) {
            if (edge.get(i, name).size()) {
                double cur = to_double(edge.get(i, name));
                min_elem = min(min_elem, cur);
                max_elem = max(max_elem, cur);
            }
        }
    }
    if (min_elem == max_elem) {
        min_elem -= 1e-6;
    }
    if (min_elem > max_elem) {
        min_elem = 0;
        max_elem = 1;
    }

    for (int i = 0; i < node.rows.size(); ++i) {
        bar.progress(i, node.rows.size());
        int L = lst;
        while (L < edge.rows.size() &&
            edge.get(L, edge_key) < node.get(i, node_key)) {
            ++L;
        } 
        int R = L;
        while (R < edge.rows.size() && 
            edge.get(R, edge_key) == node.get(i, node_key)) {
            ++R;
        }
        lst = R;
        if (edge_order.size()) {
            edge.sorting(L, R, edge_order);
        }
        feature_list features;

        for (auto [name, col] : node_columns["key"]) {
            features += create_feature(name, node.get(i, name));
        }

        for (auto [name, col] : node_columns["ordered_category"]) {
            auto order = from_regex(col["values"]);
            features += encode_ordered_cat(name, node.get(i, name), order);
        }

        for (auto [name, col] : node_columns["category"]) {
            auto order = from_regex(col["values"]);
            features += encode_one_hot(name, node.get(i, name), order);
        }

        for (auto [name, col] : node_columns["binary"]) {
            auto order = from_regex(col["values"]);
            features += encode_ordered_cat(name, node.get(i, name), order);
        }

        for (auto [name, col] : node_columns["binary_sequence"]) {
            string s = node.get(i, name);
            int sum = 0, lst, inc = 0, dec = 0;
            for (int i = 0; i < s.size(); ++i) {
                if (i) {
                    inc += s[i] > s[i - 1];
                    dec += s[i] < s[i - 1];
                }
                sum += s[i] - '0';
            }
            lst = s.back() - '0';
            features += create_feature(name + "_sum", sum);
            features += create_feature(name + "_lst", lst);
            features += create_feature(name + "_inc", inc);
            features += create_feature(name + "_dec", dec);
        }

        for (auto [name, col] : node_columns["target"]) {
            features += create_feature("target", node.get(i, name));
        }

        for (auto [name, col] : edge_columns["category"]) {
            auto order = from_regex(col["values"]);
            vector<string> values;
            for (int M = L; M < R; ++M) {
                 values.push_back(edge.get(M, name));
            }
            features += encode_hist(name, values, order);
        }

        for (auto [name, col] : edge_columns["ordered_category"]) {
            auto order = from_regex(col["values"]);
            vector<string> values;
            for (int M = L; M < R; ++M) {
                 values.push_back(edge.get(M, name));
            }
            features += encode_hist(name, values, order);
        }

        vector<double> node_coors;
        for (auto [name, col] : node_columns["coordinates"]) {
            if (node.get(i, name).size()) {
                node_coors.push_back(to_double(node.get(i, name)));
            } else {
                node_coors.push_back(min_elem);
            }
        }
        for (auto& v : node_coors) {
            v = (v - min_elem) / (max_elem - min_elem);
        }
        vector<vector<double>> edge_coors;
        for (auto [name, col] : edge_columns["coordinates"]) {
            edge_coors.push_back({});
            for (int M = L; M < R; ++M) {
                if (edge.get(M, name).size()) {
                    assert(edge.id.count(name));
                    assert(M < edge.rows.size());
                    edge_coors.back().push_back(to_double(edge.get(M, name)));
                }
            }
        }
        for (auto& a : edge_coors) {
            for (auto& v : a) {
                v = (v - min_elem) / (max_elem - min_elem);
            }
        }
        features += coor_encoding(node_coors, edge_coors);

        if (i == 0) {
            bool first_print = true;
            for (auto f : features) {
                if (first_print) {
                    first_print = false;
                } else {
                    output << ",";
                }
                output << f.first;
            }
            output << '\n';
        } 
        bool first_print = true;
        for (auto f : features) {
            if (first_print) {
                first_print = false;
            } else {
                output << ",";
            }
            output << f.second;
        }
        output << '\n';
    }
    bar.finish();
    output.close();
}

void fill_table_columns(string tname) { 
    vector<pair<string, string>> add;
    vector<string> del;
    for (auto [name, col] : database[tname].info.columns) {
        if (col.info["type"] == "info") {
            del.push_back(name);
            add.push_back({name, col.info["table"]});
            fill_table_columns(add.back().second);
        }
    }
    for (auto name : del) {
        database[tname].info.columns.erase(name);
    }
    for (auto [from, t] : add) {
        for (auto [name, col] : database[t].info.columns) {
            if (col.info["type"] == "key" || col.info["type"] == "node_key") {
                continue;
            }
            tqdm bar;
            string feature_name = t + "_" + name;
            if (database[tname].info.columns.count(feature_name)) {
                int id = 1;
                string pref = to_string(id) + "_";
                while (database[tname].info.columns.count(pref + feature_name)) {
                    pref = to_string(id++) + "_";
                }
                feature_name = pref + feature_name;
            }
            cerr << "Moving feature " << name << " from " << t << " to " << tname << " as " << feature_name << "..." << endl;
            database[tname].columns.push_back(feature_name);
            database[tname].id[feature_name] = database[tname].rows[0].size();
            database[tname].info.columns[feature_name] = col;
            for (int i = 0; i < database[tname].rows.size(); ++i) {
                bar.progress(i, database[tname].rows.size());
                string key = database[tname].get(i, from);
                // cerr << key << endl;
                // cerr << t << " " << key << ' ' << name << endl;
                if (database[t].row_id.count(key) == 0) {
                    database[tname].rows[i].push_back("");
                } else {
                    // cerr << "Success " << database[t].get(key, name) << '\n';
                    database[tname].rows[i].push_back(database[t].get(key, name));
                    // database[tname].print(i);
                }
            }
            bar.finish();
        }
    }
}

int main(int argc, char* argv[]) {
    assert(argc == 3);
    string config = argv[1];
    string prefix = argv[2];

    ios::sync_with_stdio(0);
    vector<table_info> input = init_config(config);
    for (auto cur : input) {
        database[cur.name].info = cur;
        cerr << "Table name: " << cur.name << endl;
        read_csv(cur.info["path"].data[0], database[cur.name]);
    }

    for (auto [name, table] : database) {
        fill_table_columns(name);
    }
    for (auto [table_name, edge] : database) {
        if (edge.info.info["category"].data[0] != "edge") {
            continue;
        }
        table node;
        for (auto [name, col] : edge.info.columns) {
            if (col["type"] == "node_key") {
                assert(database.count(col["path"]));
                node = database[col["path"]];
            }
        }
        assert(node.info.name.size());
        extract_features(node, edge, prefix + "_" + node.info.name + ".csv");
    }
}   