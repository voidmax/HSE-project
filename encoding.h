typedef pair<string, string> feature;
typedef vector<feature> feature_list;

void operator += (feature_list& a, const feature_list& b) {
    for (auto i : b) {
        a.push_back(i);
    }
} 

feature_list create_feature(string a, string b) {
    return {{a, b}};
}

string to_string_p(double d, int p) {
    stringstream stream;
    stream << setprecision(p) << d;
    return stream.str();
}

double to_double(string s) {
    if (s.empty()) return 0;
    return atof(s.c_str());
}

feature_list create_feature(string a, double b) {
    return create_feature(a, to_string_p(b, 10));
}



feature_list encode_ordered_cat(string name, string key, vector<string> order) {
    for (int i = 0; i < order.size(); ++i) {
        if (order[i] == key) {
            return create_feature(name, i + 1);
        }
    }
    return create_feature(name, 0);
}

feature_list encode_one_hot(string prefix, string key, vector<string> order) {
    feature_list ans;
    prefix += '_';
    for (string c : order) {
        ans += create_feature(prefix + c, c == key);
    }
    return ans;
}    

feature_list encode_hist(string prefix, vector<string> events, vector<string> values) {
    int n = events.size();
    unordered_map<string, vector<double>> cnt;
    for (int i = 0; i < events.size(); ++i) {
        cnt[events[i]].push_back(i);
    }
    feature_list features;
    for (auto v : values) {
        features += create_feature(prefix + "_" + v, cnt[v].size());
        features += create_feature(prefix + "_r_" + v, n ? (1. * cnt[v].size() / n) : 0);
        features += create_feature(prefix + "_m_" + v, mean(cnt[v]) / n);
        features += create_feature(prefix + "_d_" + v, std_norm(cnt[v]) / n);
    }
    return features;
}

feature_list coor_encoding(vector<double> node_coors, vector<vector<double>> edge_coors) {
    feature_list features;
    vector<double> edge_dists;
    for (int i = 0; i < node_coors.size(); ++i) {
        features += create_feature("coor_" + to_string(i + 1), node_coors[i]);
    }
    for (int i = 0; i < edge_coors.size(); ++i) {
        features += create_feature("coor_" + to_string(i + 1) + "_mean" , mean(edge_coors[i]));
        features += create_feature("coor_" + to_string(i + 1) + "_min" , min(edge_coors[i]));
        features += create_feature("coor_" + to_string(i + 1) + "_max" , max(edge_coors[i]));
        features += create_feature("coor_" + to_string(i + 1) + "q2", quantile(edge_coors[i], 0.2));
        features += create_feature("coor_" + to_string(i + 1) + "q5", quantile(edge_coors[i], 0.5));
        features += create_feature("coor_" + to_string(i + 1) + "q8", quantile(edge_coors[i], 0.8));
    }
    // for (int i = 0; i < edge_coors[0].size(); ++i) {
    //     double d = 0;
    //     for (int j = 0; j < node_coors.size(); ++j) {
    //         d += pow(edge_coors[j][i] - node_coors[j], 2);
    //     }
    //     edge_dists.push_back(sqrt(d));
    // }
    // features += create_feature("dist_mean", mean(edge_dists));
    // features += create_feature("dist_median", median(edge_dists));
    return features;
}