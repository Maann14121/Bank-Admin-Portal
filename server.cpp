
#include "httplib.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace httplib;
using namespace std;

const string STAFF_FILE = "staff.txt";

struct Employee {
    string id;
    string firstName;
    string lastName;
    string title;
    string phone;
    string email;
    string branch;
    string canViewCustomers;
    string canManageAccounts;
    string canViewReports;
    string canManageStaff;
};

string jsonEscape(const string& s) {
    string out;

    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
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

bool validField(const string& value) {
    return value.find('|') == string::npos &&
           value.find('\n') == string::npos &&
           value.find('\r') == string::npos;
}

bool readEmployees(vector<Employee>& employees) {
    employees.clear();

    ifstream file(STAFF_FILE);
    if (!file.is_open()) {
        return true; // New file will be created when saving.
    }

    string line;

    while (getline(file, line)) {
        vector<string> p = splitLine(line);

        if (p.size() != 11) {
            continue;
        }

        Employee e;
        e.id = p[0];
        e.firstName = p[1];
        e.lastName = p[2];
        e.title = p[3];
        e.phone = p[4];
        e.email = p[5];
        e.branch = p[6];
        e.canViewCustomers = p[7];
        e.canManageAccounts = p[8];
        e.canViewReports = p[9];
        e.canManageStaff = p[10];

        employees.push_back(e);
    }

    return true;
}

bool saveEmployees(const vector<Employee>& employees) {
    const string tempFile = STAFF_FILE + ".tmp";

    {
        ofstream file(tempFile, ios::trunc);

        if (!file.is_open()) {
            return false;
        }

        for (const Employee& e : employees) {
            file << e.id << '|'
                 << e.firstName << '|'
                 << e.lastName << '|'
                 << e.title << '|'
                 << e.phone << '|'
                 << e.email << '|'
                 << e.branch << '|'
                 << e.canViewCustomers << '|'
                 << e.canManageAccounts << '|'
                 << e.canViewReports << '|'
                 << e.canManageStaff << '\n';
        }

        if (!file.good()) {
            file.close();
            remove(tempFile.c_str());
            return false;
        }
    }

    if (rename(tempFile.c_str(), STAFF_FILE.c_str()) != 0) {
        remove(tempFile.c_str());
        return false;
    }

    return true;
}

string employeeToJson(const Employee& e) {
    return "{"
        "\"id\":" + e.id + ","
        "\"firstName\":\"" + jsonEscape(e.firstName) + "\","
        "\"lastName\":\"" + jsonEscape(e.lastName) + "\","
        "\"title\":\"" + jsonEscape(e.title) + "\","
        "\"phone\":\"" + jsonEscape(e.phone) + "\","
        "\"email\":\"" + jsonEscape(e.email) + "\","
        "\"branch\":\"" + jsonEscape(e.branch) + "\""
        "}";
}

string employeesToJson(const vector<Employee>& employees) {
    string json = "[";

    for (size_t i = 0; i < employees.size(); ++i) {
        if (i > 0) {
            json += ",";
        }

        json += employeeToJson(employees[i]);
    }

    json += "]";
    return json;
}

bool getFormValue(
    const Request& req,
    const string& key,
    string& value
) {
    if (!req.has_param(key)) {
        return false;
    }

    value = req.get_param_value(key);
    return true;
}

bool validEmail(const string& email) {
    size_t at = email.find('@');

    return at != string::npos &&
           at > 0 &&
           email.find('.', at) != string::npos &&
           email.find(' ') == string::npos;
}

int main() {
    Server server;

    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    server.Options("/api/.*", [](const Request&, Response& res) {
        res.status = 204;
    });

    // Existing route: backend status
    server.Get("/api/status", [](const Request&, Response& res) {
        res.set_content(
            R"({"status":"running","message":"Bank Admin Portal backend is connected"})",
            "application/json"
        );
    });

    // Existing route: list all employees
    server.Get("/api/staff", [](const Request&, Response& res) {
        vector<Employee> employees;
        readEmployees(employees);

        ifstream check(STAFF_FILE);
        if (!check.is_open()) {
            res.status = 404;
            res.set_content(
                R"({"error":"staff.txt not found. Create the file or add an employee first."})",
                "application/json"
            );
            return;
        }

        res.set_content(employeesToJson(employees), "application/json");
    });

    // Add employee and save permanently to staff.txt
    server.Post("/api/staff", [](const Request& req, Response& res) {
        Employee e;

        if (!getFormValue(req, "firstName", e.firstName) ||
            !getFormValue(req, "lastName", e.lastName) ||
            !getFormValue(req, "title", e.title) ||
            !getFormValue(req, "email", e.email) ||
            !getFormValue(req, "phone", e.phone) ||
            !getFormValue(req, "branch", e.branch)) {

            res.status = 400;
            res.set_content(
                R"({"error":"Please provide all required employee fields."})",
                "application/json"
            );
            return;
        }

        if (e.firstName.empty() || e.lastName.empty() ||
            e.title.empty() || e.email.empty() || e.branch.empty()) {
            res.status = 400;
            res.set_content(
                R"({"error":"First name, last name, title, email and branch are required."})",
                "application/json"
            );
            return;
        }

        if (!validEmail(e.email)) {
            res.status = 400;
            res.set_content(
                R"({"error":"Please enter a valid email address."})",
                "application/json"
            );
            return;
        }

        if (!validField(e.firstName) || !validField(e.lastName) ||
            !validField(e.title) || !validField(e.phone) ||
            !validField(e.email) || !validField(e.branch)) {
            res.status = 400;
            res.set_content(
                R"({"error":"Fields cannot contain pipe or newline characters."})",
                "application/json"
            );
            return;
        }

        vector<Employee> employees;
        readEmployees(employees);

        int nextId = 1;

        for (const Employee& existing : employees) {
            try {
                nextId = max(nextId, stoi(existing.id) + 1);
            } catch (...) {
                // Ignore malformed IDs.
            }
        }

        e.id = to_string(nextId);

        // Default permissions for newly added employees.
        e.canViewCustomers = "1";
        e.canManageAccounts = "0";
        e.canViewReports = "0";
        e.canManageStaff = "0";

        employees.push_back(e);

        if (!saveEmployees(employees)) {
            res.status = 500;
            res.set_content(
                R"({"error":"Could not save staff.txt. Check file permissions."})",
                "application/json"
            );
            return;
        }

        res.status = 201;
        res.set_content(employeeToJson(e), "application/json");
    });

    // Delete employee by ID and update staff.txt
    server.Delete(R"(/api/staff/(\d+))",
        [](const Request& req, Response& res) {
            string id = req.matches[1].str();

            vector<Employee> employees;
            readEmployees(employees);

            auto it = find_if(
                employees.begin(),
                employees.end(),
                [&id](const Employee& e) {
                    return e.id == id;
                }
            );

            if (it == employees.end()) {
                res.status = 404;
                res.set_content(
                    R"({"error":"Employee not found."})",
                    "application/json"
                );
                return;
            }

            employees.erase(it);

            if (!saveEmployees(employees)) {
                res.status = 500;
                res.set_content(
                    R"({"error":"Could not update staff.txt."})",
                    "application/json"
                );
                return;
            }

            res.set_content(
                R"({"success":true,"message":"Employee deleted successfully."})",
                "application/json"
            );
        }
    );

    cout << "Bank Admin Portal API starting..." << endl;
    cout << "Listening on port 8080" << endl;

    if (!server.listen("0.0.0.0", 8080)) {
        cerr << "Could not start server on port 8080." << endl;
        return 1;
    }

    return 0;
}
