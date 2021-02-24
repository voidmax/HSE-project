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

#include "tqdm.h"

using namespace std;

const int N = 1e9;

struct table {
	vector<string> columns;
	unordered_map<string, int> id;
	vector<vector<string>> rows;

	void init() {
		for (int i = 0; i < columns.size(); ++i) {
			id[columns[i]] = i;
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
		return rows[i][id[key]];
	}
};

vector<string> split(string a) {
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
		output << i;
	}
	output << '\n';
}

void read_csv(string filename, table& data) {
	cout << "Reading " << filename << "..." << endl;
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





int encode_ordered_cat(char key, vector<char> order = {'a', 'b', 'c', 'd', 'e', 'f'}) {
	for (int i = 0; i < order.size(); ++i) {
		if (order[i] == key) {
			return i + 1;
		}
	}
	return 0;
}

typedef pair<string, string> feature;

void operator += (vector<feature>& a, const vector<feature>& b) {
	for (auto i : b) {
		a.push_back(i);
	}
} 

vector<feature> create_feature(string a, string b) {
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

vector<feature> create_feature(string a, double b) {
	return create_feature(a, to_string_p(b, 10));
}

vector<feature> encode_one_hot(
	string prefix, char key, vector<char> order = {'a', 'b', 'c', 'd', 'e', 'f'}) {
	vector<feature> ans;
	prefix += '_';
	for (char c : order) {
		string name = prefix, res;
		name += c;
		res += '0' + (c == key);
		ans.push_back({name, res});
	}
	return ans;
}    

vector<feature> encode_hist(string prefix, vector<string> events_value, vector<char> values) {
	int n = events_value.size();
	unordered_map<char, int> cnt;
	for (auto i : events_value) {
		cnt[i[0]]++;
	}
	vector<feature> features;
	for (char c : values) {
		string namec;
		namec += c;
		features += create_feature(prefix + "_" + namec, cnt[c]);
		features += create_feature(prefix + "_r_" + namec, n ? (1. * cnt[c] / n) : 0);
	}
	return features;
}

double median(vector<double>& x) {
	if (x.empty()) {
		return 0;
	}
	return x[x.size() / 2];
}

double mean(vector<double>& x) {
	if (x.empty()) {
		return 0;
	}
	double sum = 0;
	for (auto i : x) {
		sum += i;
	}
	return sum / x.size();
}

double min(vector<double>& x) {
	if (x.empty()) {
		return 0;
	}
	return x[0];
}

double max(vector<double>& x) {
	if (x.empty()) {
		return 0;
	}
	return x.back();
}

double quantile(vector<double>& x, double p) {
	if (x.empty()) {
		return 0;
	}
	int id = x.size() * p;
	return x[id];
}

double std_norm(vector<double>& x) {
	if (x.empty()) {
		return 0;
	}
	double m = mean(x);
	double sum = 0;
	for (auto i : x) {
		sum += (i - m) * (i - m);
	}
	return sqrt(sum);
}

unordered_set<string> used_features;
vector<string> features_names;

void extract_features( 
	table& users, table& trans, string filename, bool trans_target = false, bool init_names = true) {

	if (init_names) {
		cout << "\nIninitializing features for " << filename << "..." << endl;
	} else {
		cout << "\nPreparing features for " << filename << "..." << endl;
	}

	ofstream output;
	output.open(filename.c_str());
	tqdm bar;
	int lst = 0;

	if (!init_names) {
		string all_features;
		for (auto i : features_names) {
			all_features += i + ",";
		}
		all_features.pop_back();
		output << all_features << '\n';
	} 

	for (int i = 0; i < users.rows.size(); ++i) {
		bar.progress(i, users.rows.size());
		int L = lst;
		while (L < trans.rows.size() && trans[L][0] < users[i][0]) {
			++L;
		} 
		int R = L;
		while (R < trans.rows.size() && trans[R][0] == users[i][0]) {
			++R;
		}
		lst = R;

		double mx = to_double(users.get(i, "LOC_GEO_X"));
		double my = to_double(users.get(i, "LOC_GEO_Y"));

		vector<double> cx, cy;
		for (int M = L; M < R; ++M) {
			string x = trans.get(M, "GEO_X");
			if (x.empty()) continue;
			string y = trans.get(M, "GEO_Y");
			if (y.empty()) continue;
			cx.push_back(to_double(x));
			cy.push_back(to_double(y));
		}
		sort(cx.begin(), cx.end());
		sort(cy.begin(), cy.end());

		vector<feature> features;
		features += create_feature("user_id", users[i][0]);


		// target features
		string target = users.get(i, "TARGET_TASK_2");
		if (target.empty()) {
			target = "0000-00-00";
		}
		if (trans_target) {
			features += create_feature("apply_already", target < "2014-07-01");
			features += create_feature("apply_target", target > "2014-06-30" && target < "2015-01-01");
			features += create_feature("apply_future", target >= "2014-07-01");
		}


		// categorical features
		features += create_feature("age_cat", 
			to_string(encode_ordered_cat(users.get(i, "AGE_CAT")[0])));
		features += create_feature("loc_cat", 
			to_string(encode_ordered_cat(users.get(i, "LOC_CAT")[0])));
		features += create_feature("inc_cat", 
			to_string(encode_ordered_cat(users.get(i, "INC_CAT")[0], {'d', 'a', 'b', 'c'})));
		features += encode_one_hot(
			"age_cat", users.get(i, "AGE_CAT")[0], {'a', 'b', 'c'});
		features += encode_one_hot(
			"inc_cat", users.get(i, "INC_CAT")[0], {'a', 'b', 'c', 'd'});
		features += encode_one_hot(
			"gender", users.get(i, "GEN")[0], {'m', 'f'});


		// geo features
		features += create_feature("loc_x", mx);
		features += create_feature("loc_y", my);

		vector<double> dist, angle;
		vector<double> cdist, cangle;
		vector<double> dx, dy;
		for (int i = 0; i < cx.size(); ++i) {
			dx.push_back(cx[i] - mx);
			dy.push_back(cy[i] - my);
			dist.push_back(hypot(dx.back(), dy.back()));
			angle.push_back(atan2(dx.back(), dy.back()));
			cdist.push_back(hypot(cx[i], cy[i]));
			cangle.push_back(atan2(cx[i], cy[i]));
		}

		features += create_feature("trans_loc_x_med", median(cx));
		features += create_feature("trans_loc_y_med", median(cy));
		features += create_feature("trans_loc_dx_mean", mean(dx));
		features += create_feature("trans_loc_dy_mean", mean(dy));

		features += create_feature("trans_loc_dist_min", min(dist));
		features += create_feature("trans_loc_dist_max", max(dist));
		features += create_feature("trans_loc_dist_mean", mean(dist));
		features += create_feature("trans_loc_dist_std", std_norm(dist));
		features += create_feature("trans_loc_dist_q2", quantile(dist, 0.2));
		features += create_feature("trans_loc_dist_q5", quantile(dist, 0.5));
		features += create_feature("trans_loc_dist_q8", quantile(dist, 0.8));

		features += create_feature("trans_loc_angle_mean", mean(angle));
		features += create_feature("trans_loc_angle_std", std_norm(angle));
		features += create_feature("trans_loc_angle_min", min(angle));
		features += create_feature("trans_loc_angle_max", max(angle));
		features += create_feature("trans_loc_angle_diff", max(angle) - min(angle));
		features += create_feature("trans_loc_angle_med", median(angle));

		features += create_feature("trans_center_dist_min", min(cdist));
		features += create_feature("trans_center_dist_max", max(cdist));
		features += create_feature("trans_center_dist_mean", mean(cdist));
		features += create_feature("trans_center_dist_std", std_norm(cdist));
		features += create_feature("trans_center_dist_q2", quantile(cdist, 0.2));
		features += create_feature("trans_center_dist_q5", quantile(cdist, 0.5));
		features += create_feature("trans_center_dist_q8", quantile(cdist, 0.8));

		features += create_feature("trans_center_angle_mean", mean(cangle));
		features += create_feature("trans_center_angle_std", std_norm(cangle));
		features += create_feature("trans_center_angle_min", min(cangle));
		features += create_feature("trans_center_angle_max", max(cangle));
		features += create_feature("trans_center_angle_diff", max(cangle) - min(cangle));
		features += create_feature("trans_center_angle_med", median(cangle));


		// card & wealth  
		string card = users.get(i, "CARD_MONTHLY");
		string wealth = users.get(i, "WEALTH_MONTHLY");
		vector<int> card_monthly, wealth_monthly;
		for (int i = 0; i < 6; ++i) card_monthly.push_back(card[i] - '0');
		for (int i = 0; i < 6; ++i) wealth_monthly.push_back(wealth[i] - '0');
	
		double card_months = 0;
		for (int i = 0; i < 6; ++i) card_months += card_monthly[i];
		features += create_feature("card_months", card_months);

		features += create_feature("card_last", card_monthly.back());

		double card_inc = 0;
		for (int i = 1; i < 6; ++i) card_inc += card_monthly[i] > card_monthly[i - 1];
		features += create_feature("card_inc", card_inc);

		double card_dec = 0;
		for (int i = 1; i < 6; ++i) card_dec += card_monthly[i] < card_monthly[i - 1];
		features += create_feature("card_dec", card_dec);

		double wealth_months = 0;
		for (int i = 0; i < 6; ++i) wealth_months += wealth_monthly[i];
		features += create_feature("wealth_months", wealth_months);

		features += create_feature("wealth_last", wealth_monthly.back());

		double wealth_inc = 0;
		for (int i = 1; i < 6; ++i) wealth_inc += wealth_monthly[i] > wealth_monthly[i - 1];
		features += create_feature("wealth_inc", wealth_inc);

		double wealth_dec = 0;
		for (int i = 1; i < 6; ++i) wealth_dec += wealth_monthly[i] < wealth_monthly[i - 1];
		features += create_feature("wealth_dec", wealth_dec);


		// events
		features += create_feature("events", R - L);

		unordered_set<string> b, n, p, all;
		for (int M = L; M < R; ++M) {
			string POI_ID = trans.get(M, "POI_ID");
			all.insert(POI_ID);
			string CHANNEL = trans.get(M, "CHANNEL");
			if (CHANNEL == "b") {
				b.insert(POI_ID);
			} else if (CHANNEL == "n") {
				n.insert(POI_ID);
			} else if (CHANNEL == "p") {
				p.insert(POI_ID);
			}
		}
		features += create_feature("uniq_poi", all.size());
		features += create_feature("uniq_poi_b", b.size());
		features += create_feature("uniq_poi_n", n.size());
		features += create_feature("uniq_poi_p", p.size());


		vector<string> CHANNEL, TIME_CAT, LOC_CAT, MC_CAT, CARD_CAT, AMT_CAT;
		for (int M = L; M < R; ++M) {
			CHANNEL.push_back(trans.get(M, "CHANNEL"));
			TIME_CAT.push_back(trans.get(M, "TIME_CAT"));
			LOC_CAT.push_back(trans.get(M, "LOC_CAT"));
			MC_CAT.push_back(trans.get(M, "MC_CAT"));
			CARD_CAT.push_back(trans.get(M, "CARD_CAT"));
			AMT_CAT.push_back(trans.get(M, "AMT_CAT"));
		}

        features += encode_hist("event_channel", CHANNEL, {'b', 'n', 'p'});
        features += encode_hist("event_time", TIME_CAT, {'a', 'b', 'c'});
        features += encode_hist("event_loc", LOC_CAT, {'a', 'b', 'c'});
        vector<char> values;
        for (char c = 'a'; c <= 'j'; ++c) {
        	values.push_back(c);
        }
        features += encode_hist("event_mc", MC_CAT, values);
        features += encode_hist("event_card", CARD_CAT, {'c', 'd'});
        features += encode_hist("event_amt", AMT_CAT, {'a', 'b', 'c'});

		if (init_names) {
			for (auto [a, b] : features) {
				if (!used_features.count(a)) {
					used_features.insert(a);
					features_names.push_back(a);
				}
			}
		} else {
			unordered_map<string, string> res(features.begin(), features.end());
			string line = "";
			for (auto name : features_names) {
				line += res[name] + ",";
			}
			line.pop_back();
			output << line << '\n';
 		}
 	}
	output.close();
}

int main() {
	ios::sync_with_stdio(0);
	for (int year = 2014; year <= 2015; ++year) {
		used_features.clear();
		features_names.clear();
		cout << "Preparing year = " << year << "..." << endl;
		table users, trans;
		read_csv("data/users_" + to_string(year) + ".csv", users);
		read_csv("data/train_" + to_string(year) + ".csv", trans);
		cout << "Sorting users..." << endl;
		sort(users.rows.begin(), users.rows.end());
		cout << "Sorting transactions..." << endl;
		sort(trans.rows.begin(), trans.rows.end());
		extract_features(users, trans, "features/features_users_" + to_string(year) + ".csv", year == 2014, true);
		extract_features(users, trans, "features/features_users_" + to_string(year) + ".csv", year == 2014, false);
		cout << endl << endl;
	}
}
