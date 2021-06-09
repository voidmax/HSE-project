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