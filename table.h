struct table {
    table_info info;
    vector<string> columns;
    unordered_map<string, int> id;
    unordered_map<string, int> row_id;
    vector<vector<string>> rows;
    string key_name;

    void update_places(int L, int R) {
        if (!key_name.empty()) {
            int need = id[key_name];
            for (int i = L; i < R; ++i) {
                row_id[rows[i][need]] = i;
            }
        }
    }

    void init() {
        // cout << info.name << ' ' << columns.size() << ": ";
        // for (int i = 0; i < columns.size(); ++i) {
        //     cout << columns[i] << ' ';
        // } 
        // cout << endl;
        for (int i = 0; i < columns.size(); ++i) {
            // if (info.columns.count(columns[i])) {
            //     cout << columns[i] << ':' << info.columns[columns[i]]["type"] << ")" << endl;
            // }
            if (info.columns.count(columns[i]) &&                 
                info.columns[columns[i]]["type"] == "key") {
                key_name = columns[i];
                // cout << key_name << endl;
                break;
            }
        } 
        for (int i = 0; i < columns.size(); ++i) {
            id[columns[i]] = i;
        }
        update_places(0, rows.size());
        for (int i = 0; i < rows.size(); ++i) {
            assert(rows[i].size() == columns.size());
        }
    }

    vector<string>& operator [](int i) {
        return rows[i];
    }

    string get(int i, string key) {
        // assert(id.count(key));
        if (!id.count(key)) {
            return "";
        }
        assert(i < rows.size());
        assert(rows[i].size() > id[key]);
        return rows[i][id[key]];
    }

    string get(string _i, string key) {
        assert(row_id.count(_i));
        return get(row_id[_i], key);
    }

    void sorting(string key) {
        assert(id.count(key));
        int cur = id[key];
        sort(rows.begin(), rows.end(), [&](vector<string>& a, vector<string>& b) {
            return a[cur] < b[cur];
        });
        update_places(0, rows.size());
    }

    void sorting(int L, int R, string key) {
        assert(id.count(key));
        int cur = id[key];
        sort(rows.begin() + L, rows.begin() + R, [&](vector<string>& a, vector<string>& b) {
            return a[cur] < b[cur];
        });
        update_places(L, R);
    }

    void print(string key) { 
        assert(!key_name.empty());
        assert(row_id.count(key));
        int cur = row_id[key];
        cout << key << " =";
        for (auto [name, value] : info.columns) {
            cout << " {" << name << " : " << rows[cur][id[name]] << "}";
        }
        cout << '\n';
    }

    void print(int i) { 
        for (auto [name, value] : info.columns) {
            cout << " {" << name << " : " << rows[i][id[name]] << "}";
        }
        cout << '\n';
    }
};

vector<string> split(string a) {
    a += ",";
    vector<string> ans;
    int L = 0;
    while (L < a.size()) {
        int R = L;
        while (R < a.size() && a[R] != ',') {
            ++R;
        }
        ans.push_back(string(a.begin() + L, a.begin() + R));
        L = R + 1;
    }
    return ans;
}

void print(ofstream& output, vector<string> data) {
    bool have = false;
    for (auto i : data) {
        if (have) {
            output << ",";
        } else {
            have = true;
        }
        if (i == "nan") {
            output << "0";
        } else {
            output << i;
        }
    }
    output << '\n';
}

void read_csv(string filename, table& data) {
    cerr << "Reading " << filename << "..." << endl;
    ifstream input;
    input.open(filename.c_str());
    string s;
    input >> s;
    data.columns = split(s);
    while (input >> s) {
        vector<string> res;
        res = split(s);
        data.rows.push_back(res);
    }
    cerr << "Total rows read: " << data.rows.size() << '\n';
    cerr << '\n';
    data.init();
    input.close();
}

void print_csv(string filename, table& data) {
    ofstream output;
    output.open(filename.c_str());
    print(output, data.columns);
    for (auto cur : data.rows) {
        print(output, cur);
    }
    output.close();
}