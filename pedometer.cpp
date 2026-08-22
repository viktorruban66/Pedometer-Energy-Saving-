// pedometer.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

struct Config {
    double stride_cm = 78.0;
    double weight_kg = 70.0;
    int goal = 10000;
};

struct Entry {
    string date;
    int steps;
};

const string CONFIG_FILE = "pedometer_config.json";
const string DATA_FILE = "steps.json";

Config loadConfig() {
    Config cfg;
    ifstream f(CONFIG_FILE);
    if (f.is_open()) {
        json j;
        f >> j;
        if (j.contains("stride_cm")) cfg.stride_cm = j["stride_cm"];
        if (j.contains("weight_kg")) cfg.weight_kg = j["weight_kg"];
        if (j.contains("goal")) cfg.goal = j["goal"];
        f.close();
    }
    return cfg;
}

void saveConfig(const Config& cfg) {
    json j = {{"stride_cm", cfg.stride_cm}, {"weight_kg", cfg.weight_kg}, {"goal", cfg.goal}};
    ofstream f(CONFIG_FILE);
    f << setw(2) << j << endl;
}

vector<Entry> loadEntries() {
    vector<Entry> entries;
    ifstream f(DATA_FILE);
    if (f.is_open()) {
        json j;
        f >> j;
        for (auto& item : j) {
            entries.push_back({item["date"], item["steps"]});
        }
        f.close();
    }
    return entries;
}

void saveEntries(const vector<Entry>& entries) {
    json j = json::array();
    for (auto& e : entries) {
        j.push_back({{"date", e.date}, {"steps", e.steps}});
    }
    ofstream f(DATA_FILE);
    f << setw(2) << j << endl;
}

string today() {
    time_t t = time(nullptr);
    char buf[11];
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
    return string(buf);
}

Entry* getEntry(vector<Entry>& entries, const string& date) {
    for (auto& e : entries) {
        if (e.date == date) return &e;
    }
    return nullptr;
}

void addSteps(vector<Entry>& entries, const Config& cfg, int steps, const string& dateStr) {
    string d = dateStr.empty() ? today() : dateStr;
    Entry* e = getEntry(entries, d);
    if (e) {
        e->steps = steps;
    } else {
        entries.push_back({d, steps});
    }
    saveEntries(entries);
    double dist = steps * cfg.stride_cm / 100000.0;
    double cal = steps * 0.04 * cfg.weight_kg / 70.0;
    cout << "✅ Logged " << steps << " steps on " << d << " – " << fixed << setprecision(2) << dist << " km, " << (int)cal << " kcal\n";
}

void stats(const vector<Entry>& entries, const Config& cfg) {
    if (entries.empty()) {
        cout << "No entries.\n";
        return;
    }
    int totalSteps = 0;
    for (auto& e : entries) totalSteps += e.steps;
    double totalDist = totalSteps * cfg.stride_cm / 100000.0;
    double totalCal = totalSteps * 0.04 * cfg.weight_kg / 70.0;
    int avgSteps = totalSteps / entries.size();
    string td = today();
    const Entry* te = nullptr;
    for (auto& e : entries) if (e.date == td) { te = &e; break; }
    int todaySteps = te ? te->steps : 0;
    int pct = min(100, (int)((double)todaySteps / cfg.goal * 100));
    double fuelSaved = totalDist / 10.0;
    double co2Saved = fuelSaved * 2.31;

    cout << "\n🚶 Pedometer (Energy Saving)\n";
    cout << "Today: " << todaySteps << " steps\n";
    cout << "Goal: " << cfg.goal << " steps (" << pct << "% complete)\n";
    cout << "\n📊 Statistics\n";
    cout << "Total steps: " << totalSteps << "\n";
    cout << "Total distance: " << fixed << setprecision(2) << totalDist << " km\n";
    cout << "Total calories: " << (int)totalCal << " kcal\n";
    cout << "Average: " << avgSteps << " steps/day\n";
    cout << "Energy saved: " << fixed << setprecision(2) << fuelSaved << " L of fuel\n";
    cout << "CO₂ saved: " << fixed << setprecision(2) << co2Saved << " kg\n";
}

void listEntries(const vector<Entry>& entries, const Config& cfg) {
    if (entries.empty()) {
        cout << "No entries.\n";
        return;
    }
    vector<Entry> sorted = entries;
    sort(sorted.begin(), sorted.end(), [](const Entry& a, const Entry& b) {
        return a.date > b.date;
    });
    cout << "\n📋 Daily Log:\n";
    for (auto& e : sorted) {
        double dist = e.steps * cfg.stride_cm / 100000.0;
        double cal = e.steps * 0.04 * cfg.weight_kg / 70.0;
        int pct = min(100, (int)((double)e.steps / cfg.goal * 100));
        cout << e.date << " | " << e.steps << " steps | " << fixed << setprecision(2) << dist << " km | " << (int)cal << " kcal | " << pct << "%\n";
    }
}

void exportCSV(const vector<Entry>& entries, const Config& cfg, const string& filename) {
    ofstream f(filename);
    f << "Date,Steps,Distance (km),Calories,Goal %\n";
    for (auto& e : entries) {
        double dist = e.steps * cfg.stride_cm / 100000.0;
        double cal = e.steps * 0.04 * cfg.weight_kg / 70.0;
        int pct = min(100, (int)((double)e.steps / cfg.goal * 100));
        f << e.date << "," << e.steps << "," << fixed << setprecision(2) << dist << "," << (int)cal << "," << pct << "\n";
    }
    f.close();
    cout << "✅ Exported " << entries.size() << " entries to " << filename << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: pedometer [config|add|stats|list|export]\n";
        return 1;
    }
    Config cfg = loadConfig();
    vector<Entry> entries = loadEntries();
    string cmd = argv[1];

    if (cmd == "config") {
        for (int i=2; i<argc; i++) {
            if (string(argv[i]) == "--stride" && i+1 < argc) {
                cfg.stride_cm = stod(argv[++i]);
            }
            if (string(argv[i]) == "--weight" && i+1 < argc) {
                cfg.weight_kg = stod(argv[++i]);
            }
            if (string(argv[i]) == "--goal" && i+1 < argc) {
                cfg.goal = stoi(argv[++i]);
            }
        }
        saveConfig(cfg);
        cout << "✅ Config updated: stride=" << cfg.stride_cm << "cm, weight=" << cfg.weight_kg << "kg, goal=" << cfg.goal << "\n";
    } else if (cmd == "add") {
        if (argc < 3) { cerr << "Usage: add <steps> [--date YYYY-MM-DD]\n"; return 1; }
        int steps = stoi(argv[2]);
        string dateStr;
        for (int i=3; i<argc; i++) {
            if (string(argv[i]) == "--date" && i+1 < argc) {
                dateStr = argv[++i];
            }
        }
        addSteps(entries, cfg, steps, dateStr);
    } else if (cmd == "stats") {
        stats(entries, cfg);
    } else if (cmd == "list") {
        listEntries(entries, cfg);
    } else if (cmd == "export") {
        string filename = argc > 2 ? argv[2] : "steps.csv";
        exportCSV(entries, cfg, filename);
    } else {
        cerr << "Unknown command\n";
        return 1;
    }
    return 0;
}
