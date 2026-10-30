
#include "httplib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
using namespace std;
using namespace httplib;

const string STAFF_FILE = "staff.txt";
const string CUSTOMER_FILE = "customers.txt";
const string ACCOUNT_FILE = "accounts.txt";

struct Row {
    vector<string> data;
};

string jsonEscape(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if ((unsigned char)c >= 32) out += c;
    }
    return out;
}

string js(const string& s) {
    return "\"" + jsonEscape(s) + "\"";
}

string getValue(const Request& req, const string& key) {
    return req.has_param(key) ? req.get_param_value(key) : "";
}

bool safeValue(const string& s) {
    return s.find('|') == string::npos &&
           s.find('\n') == string::npos &&
           s.find('\r') == string::npos;
}

vector<Row> readFile(const string& file, int expected) {
    vector<Row> rows;
    ifstream in(file);
    string line;

    while (getline(in, line)) {
        stringstream ss(line);
        string value;
        Row row;

        while (getline(ss, value, '|'))
            row.data.push_back(value);

        // Old customer format:
        // id|cnic|name|accountType|accountNumber|balance|active
        if (file == CUSTOMER_FILE && row.data.size() == 7) {
            auto old = row.data;
            row.data = {
                old[0], old[1], old[2],
                "", "", "", "", "", "",
                old[3], old[4], old[5], old[6]
            };
        }

        // Old staff format: 11 fields
        if (file == STAFF_FILE && row.data.size() == 11) {
            row.data.push_back("0");
            row.data.push_back("0");
        }

        if ((int)row.data.size() == expected)
            rows.push_back(row);
    }

    return rows;
}

bool saveFile(const string& file, const vector<Row>& rows) {
    string temp = file + ".tmp";

    {
        ofstream out(temp);
        if (!out) return false;

        for (const auto& row : rows) {
            for (size_t i = 0; i < row.data.size(); i++) {
                if (i) out << "|";
                out << row.data[i];
            }
            out << "\n";
        }

        if (!out.good()) return false;
    }

    if (rename(temp.c_str(), file.c_str()) != 0) {
        remove(temp.c_str());
        return false;
    }

    return true;
}

string nextId(const vector<Row>& rows) {
    int highest = 0;

    for (const auto& r : rows) {
        try {
            highest = max(highest, stoi(r.data[0]));
        } catch (...) {}
    }

    return to_string(highest + 1);
}

string nextAccountNumber(
    const vector<Row>& customers,
    const vector<Row>& accounts
) {
    unsigned long long highest = 100000000000ULL;

    for (const auto& r : customers) {
        try {
            highest = max(highest, stoull(r.data[10]));
        } catch (...) {}
    }

    for (const auto& r : accounts) {
        try {
            highest = max(highest, stoull(r.data[2]));
        } catch (...) {}
    }

    return to_string(highest + 1);
}

void sendError(Response& res, int code, string message) {
    res.status = code;
    res.set_content(
        "{\"error\":" + js(message) + "}",
        "application/json"
    );
}

string customersJSON(const vector<Row>& rows) {
    string out = "[";

    for (size_t i = 0; i < rows.size(); i++) {
        const auto& v = rows[i].data;

        if (i) out += ",";

        out +=
            "{\"id\":" + v[0] +
            ",\"cnic\":" + js(v[1]) +
            ",\"fullName\":" + js(v[2]) +
            ",\"fatherName\":" + js(v[3]) +
            ",\"motherName\":" + js(v[4]) +
            ",\"dob\":" + js(v[5]) +
            ",\"phone\":" + js(v[6]) +
            ",\"address\":" + js(v[7]) +
            ",\"branch\":" + js(v[8]) +
            ",\"accountType\":" + js(v[9]) +
            ",\"accountNumber\":" + js(v[10]) +
            ",\"balance\":" + v[11] +
            ",\"active\":" + string(v[12] == "1" ? "true" : "false") +
            "}";
    }

    return out + "]";
}

int main() {
    Server server;

    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    server.Options(R"(/api/.*)", [](const Request&, Response& res) {
        res.status = 204;
    });

    server.Get("/api/status", [](const Request&, Response& res) {
        res.set_content(R"({"status":"running"})", "application/json");
    });

    // STAFF DIRECTORY
    server.Get("/api/staff", [](const Request&, Response& res) {
        auto rows = readFile(STAFF_FILE, 13);
        string out = "[";

        for (size_t i = 0; i < rows.size(); i++) {
            auto& v = rows[i].data;
            if (i) out += ",";

            out += "{\"id\":" + v[0] +
                ",\"firstName\":" + js(v[1]) +
                ",\"lastName\":" + js(v[2]) +
                ",\"title\":" + js(v[3]) +
                ",\"phone\":" + js(v[4]) +
                ",\"email\":" + js(v[5]) +
                ",\"branch\":" + js(v[6]);

            const char* keys[] = {
                "canViewCustomers", "canManageAccounts",
                "canViewReports", "canManageStaff",
                "canAddEmployees", "canDeleteEmployees"
            };

            for (int k = 0; k < 6; k++)
                out += string(",\"") + keys[k] + "\":" +
                    (v[k + 7] == "1" ? "true" : "false");

            out += "}";
        }

        res.set_content(out + "]", "application/json");
    });

    server.Post("/api/staff", [](const Request& req, Response& res) {
        vector<string> v = {
            getValue(req, "firstName"),
            getValue(req, "lastName"),
            getValue(req, "title"),
            getValue(req, "phone"),
            getValue(req, "email"),
            getValue(req, "branch")
        };

        for (auto& x : v)
            if (!safeValue(x)) {
                sendError(res, 400, "Invalid field.");
                return;
            }

        if (v[0].empty() || v[1].empty() ||
            v[2].empty() || v[4].empty()) {
            sendError(res, 400, "Required fields missing.");
            return;
        }

        auto rows = readFile(STAFF_FILE, 13);
        Row r;
        r.data = {nextId(rows)};
        r.data.insert(r.data.end(), v.begin(), v.end());
        r.data.insert(r.data.end(), {"1","0","0","0","0","0"});

        rows.push_back(r);

        if (!saveFile(STAFF_FILE, rows)) {
            sendError(res, 500, "Could not save employee.");
            return;
        }

        res.status = 201;
        res.set_content(R"({"success":true})", "application/json");
    });

    server.Delete(R"(/api/staff/(\d+))",
    [](const Request& req, Response& res) {
        string id = req.matches[1];
        auto rows = readFile(STAFF_FILE, 13);
        size_t oldSize = rows.size();

        rows.erase(remove_if(rows.begin(), rows.end(),
            [&](const Row& r) { return r.data[0] == id; }),
            rows.end());

        if (rows.size() == oldSize) {
            sendError(res, 404, "Employee not found.");
            return;
        }

        if (!saveFile(STAFF_FILE, rows)) {
            sendError(res, 500, "Could not save staff data.");
            return;
        }

        res.set_content(R"({"success":true})", "application/json");
    });

    server.Post(R"(/api/staff/(\d+)/permissions)",
    [](const Request& req, Response& res) {
        string id = req.matches[1];
        auto rows = readFile(STAFF_FILE, 13);

        auto it = find_if(rows.begin(), rows.end(),
            [&](const Row& r) { return r.data[0] == id; });

        if (it == rows.end()) {
            sendError(res, 404, "Employee not found.");
            return;
        }

        const char* keys[] = {
            "canViewCustomers", "canManageAccounts",
            "canViewReports", "canManageStaff",
            "canAddEmployees", "canDeleteEmployees"
        };

        for (int i = 0; i < 6; i++) {
            string value = getValue(req, keys[i]);

            if (value != "0" && value != "1") {
                sendError(res, 400, "Invalid permission.");
                return;
            }

            it->data[i + 7] = value;
        }

        if (!saveFile(STAFF_FILE, rows)) {
            sendError(res, 500, "Could not save permissions.");
            return;
        }

        res.set_content(R"({"success":true})", "application/json");
    });

    // CUSTOMER SEARCH
    server.Get("/api/customers", [](const Request& req, Response& res) {
        auto rows = readFile(CUSTOMER_FILE, 13);

        if (req.has_param("cnic")) {
            string cnic = getValue(req, "cnic");

            rows.erase(remove_if(rows.begin(), rows.end(),
                [&](const Row& r) { return r.data[1] != cnic; }),
                rows.end());
        }

        res.set_content(customersJSON(rows), "application/json");
    });

    // ADD CUSTOMER - ACCOUNT NUMBER IS AUTOMATIC
    server.Post("/api/customers", [](const Request& req, Response& res) {
        vector<string> v = {
            getValue(req, "cnic"),
            getValue(req, "fullName"),
            getValue(req, "fatherName"),
            getValue(req, "motherName"),
            getValue(req, "dob"),
            getValue(req, "phone"),
            getValue(req, "address"),
            getValue(req, "branch"),
            getValue(req, "accountType"),
            "",
            getValue(req, "balance"),
            "1"
        };

        for (auto& x : v)
            if (!safeValue(x)) {
                sendError(res, 400, "Invalid customer data.");
                return;
            }

        if (v[0].empty() || v[1].empty()) {
            sendError(res, 400, "CNIC and name are required.");
            return;
        }

        try {
            if (stod(v[10]) < 0) throw 1;
        } catch (...) {
            sendError(res, 400, "Invalid balance.");
            return;
        }

        auto customers = readFile(CUSTOMER_FILE, 13);

        for (auto& c : customers) {
            if (c.data[1] == v[0]) {
                sendError(res, 409, "CNIC already exists.");
                return;
            }
        }

        auto accounts = readFile(ACCOUNT_FILE, 5);

        v[9] = v[9].empty() ? "Savings" : v[9];
        v[10] = nextAccountNumber(customers, accounts);

        Row customer;
        customer.data = {nextId(customers)};
        customer.data.insert(customer.data.end(), v.begin(), v.end());

        customers.push_back(customer);

        if (!saveFile(CUSTOMER_FILE, customers)) {
            sendError(res, 500, "Could not save customer.");
            return;
        }

        Row account;
        account.data = {
            nextId(accounts), v[0], v[10], v[9], "1"
        };

        accounts.push_back(account);

        if (!saveFile(ACCOUNT_FILE, accounts)) {
            sendError(res, 500, "Customer saved but account file failed.");
            return;
        }

        res.status = 201;
        res.set_content(customersJSON({customer}), "application/json");
    });

    // EDIT CUSTOMER - CNIC AND ACCOUNT NUMBER ARE NOT EDITABLE
    server.Put(R"(/api/customers/(\d+))",
    [](const Request& req, Response& res) {
        string id = req.matches[1];
        auto rows = readFile(CUSTOMER_FILE, 13);

        auto it = find_if(rows.begin(), rows.end(),
            [&](const Row& r) { return r.data[0] == id; });

        if (it == rows.end()) {
            sendError(res, 404, "Customer not found.");
            return;
        }

        const char* keys[] = {
            "fullName", "fatherName", "motherName",
            "dob", "phone", "address", "branch",
            "accountType", "balance"
        };

        int indexes[] = {2,3,4,5,6,7,8,9,11};

        for (int i = 0; i < 9; i++) {
            string value = getValue(req, keys[i]);

            if (!safeValue(value)) {
                sendError(res, 400, "Invalid field.");
                return;
            }

            if (i == 0 && value.empty()) {
                sendError(res, 400, "Customer name required.");
                return;
            }

            if (i == 8) {
                try {
                    if (stod(value) < 0) throw 1;
                } catch (...) {
                    sendError(res, 400, "Invalid balance.");
                    return;
                }
            }

            it->data[indexes[i]] = value;
        }

        if (!saveFile(CUSTOMER_FILE, rows)) {
            sendError(res, 500, "Could not update customer.");
            return;
        }

        res.set_content(customersJSON({*it}), "application/json");
    });

    // ACTIVATE / DEACTIVATE
    server.Post(R"(/api/customers/(\d+)/status)",
    [](const Request& req, Response& res) {
        string id = req.matches[1];
        string active = getValue(req, "active");

        if (active != "0" && active != "1") {
            sendError(res, 400, "Invalid status.");
            return;
        }

        auto rows = readFile(CUSTOMER_FILE, 13);

        auto it = find_if(rows.begin(), rows.end(),
            [&](const Row& r) { return r.data[0] == id; });

        if (it == rows.end()) {
            sendError(res, 404, "Customer not found.");
            return;
        }

        it->data[12] = active;

        if (!saveFile(CUSTOMER_FILE, rows)) {
            sendError(res, 500, "Could not update status.");
            return;
        }

        res.set_content(R"({"success":true})", "application/json");
    });

    // DELETE CUSTOMER AND LINKED ACCOUNT
    server.Delete(R"(/api/customers/(\d+))",
    [](const Request& req, Response& res) {
        string id = req.matches[1];
        auto customers = readFile(CUSTOMER_FILE, 13);

        auto it = find_if(customers.begin(), customers.end(),
            [&](const Row& r) { return r.data[0] == id; });

        if (it == customers.end()) {
            sendError(res, 404, "Customer not found.");
            return;
        }

        string cnic = it->data[1];
        string number = it->data[10];

        customers.erase(it);

        if (!saveFile(CUSTOMER_FILE, customers)) {
            sendError(res, 500, "Could not delete customer.");
            return;
        }

        auto accounts = readFile(ACCOUNT_FILE, 5);

        accounts.erase(remove_if(accounts.begin(), accounts.end(),
            [&](const Row& r) {
                return r.data[1] == cnic || r.data[2] == number;
            }), accounts.end());

        if (!saveFile(ACCOUNT_FILE, accounts)) {
            sendError(res, 500, "Account cleanup failed.");
            return;
        }

        res.set_content(R"({"success":true})", "application/json");
    });

    cout << "Bank Admin Portal running on port 8080" << endl;

    if (!server.listen("0.0.0.0", 8080)) {
        cerr << "Server could not start." << endl;
        return 1;
    }

    return 0;
}
