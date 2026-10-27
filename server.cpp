

#include "httplib.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

using namespace httplib;
using namespace std;

string jsonEscape(const string& s) {
    string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c >= 32) out += static_cast<char>(c);
        }
    }
    return out;
}

vector<string> splitLine(const string& line) {
    vector<string> parts;
    string part;
    stringstream ss(line);
    while (getline(ss, part, '|')) {
        parts.push_back(part);
    }
    return parts;
}

int main() {
    Server server;

    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    server.Options("/api/.*", [](const Request&, Response& res) {
        res.status = 204;
    });

    server.Get("/api/status", [](const Request&, Response& res) {
        res.set_content(
            R"({"status":"running","message":"Bank Admin Portal backend is connected"})",
            "application/json"
        );
    });

    server.Get("/api/staff", [](const Request&, Response& res) {
        ifstream file("staff.txt");

        if (!file.is_open()) {
            res.status = 404;
            res.set_content(
                R"({"error":"staff.txt not found. Run the C++ bank program first and save its data."})",
                "application/json"
            );
            return;
        }

        string line;
        string json = "[";
        bool first = true;

        while (getline(file, line)) {
            vector<string> p = splitLine(line);

            if (p.size() != 11) continue;

            if (!first) json += ",";
            first = false;

            json += "{";
            json += "\"id\":" + p[0] + ",";
            json += "\"firstName\":\"" + jsonEscape(p[1]) + "\",";
            json += "\"lastName\":\"" + jsonEscape(p[2]) + "\",";
            json += "\"title\":\"" + jsonEscape(p[3]) + "\",";
            json += "\"phone\":\"" + jsonEscape(p[4]) + "\",";
            json += "\"email\":\"" + jsonEscape(p[5]) + "\",";
            json += "\"branch\":\"" + jsonEscape(p[6]) + "\"";
            json += "}";
        }

        json += "]";
        res.set_content(json, "application/json");
    });

    cout << "Bank Admin Portal API starting..." << endl;
    cout << "Listening on port 8080" << endl;

    if (!server.listen("0.0.0.0", 8080)) {
        cerr << "Could not start server on port 8080." << endl;
        return 1;
    }

    return 0;
}