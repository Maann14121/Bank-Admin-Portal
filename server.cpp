
#include "httplib.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace httplib;

using Row = vector<string>;
using Rows = vector<Row>;

const string SF = "staff.txt";
const string CF = "customers.txt";
const string AF = "accounts.txt";
const string BF = "branches.txt";

// -------------------- COMMON FUNCTIONS --------------------

string esc(const string& s) {
    string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else if ((unsigned char)c >= 32) o += c;
    }
    return o;
}

string q(const string& s) {
    return "\"" + esc(s) + "\"";
}

string p(const Request& r, const string& k) {
    return r.has_param(k) ? r.get_param_value(k) : "";
}

bool safe(const string& s) {
    return s.find('|') == string::npos &&
           s.find('\n') == string::npos &&
           s.find('\r') == string::npos;
}

Rows read(const string& f) {
    Rows a;
    ifstream in(f);
    string line;

    while (getline(in, line)) {
        Row r;
        stringstream ss(line);
        string v;

        while (getline(ss, v, '|')) {
            r.push_back(v);
        }

        if (!r.empty()) a.push_back(r);
    }

    return a;
}

bool write(const string& f, const Rows& a) {
    string t = f + ".tmp";

    {
        ofstream o(t);

        if (!o) return false;

        for (auto& r : a) {
            for (size_t i = 0; i < r.size(); i++) {
                if (i) o << '|';
                o << r[i];
            }
            o << '\n';
        }

        if (!o.good()) return false;
    }

    if (rename(t.c_str(), f.c_str())) {
        remove(t.c_str());
        return false;
    }

    return true;
}

string nextId(const Rows& a) {
    long long n = 0;

    for (auto& r : a) {
        try {
            n = max(n, stoll(r[0]));
        } catch (...) {}
    }

    return to_string(n + 1);
}

string bt(string s) {
    return (s == "1" || s == "true" || s == "TRUE" ||
            s == "yes" || s == "on") ? "true" : "false";
}

bool valid(const Row& r) {
    for (auto& s : r) {
        if (!safe(s)) return false;
    }
    return true;
}

bool validBalance(const string& s) {
    if (s.empty()) return false;

    bool digit = false;
    bool dot = false;

    for (char c : s) {
        if (isdigit((unsigned char)c)) {
            digit = true;
        } else if (c == '.' && !dot) {
            dot = true;
        } else {
            return false;
        }
    }

    return digit;
}

string arr(const Rows& a) {
    string s = "[";

    for (size_t i = 0; i < a.size(); i++) {
        if (i) s += ",";

        s += "[";

        for (size_t j = 0; j < a[i].size(); j++) {
            if (j) s += ",";
            s += q(a[i][j]);
        }

        s += "]";
    }

    return s + "]";
}

void send(Response& r, string s, int code = 200) {
    r.status = code;
    r.set_content(s, "application/json; charset=utf-8");
}

string err(string s) {
    return "{\"error\":" + q(s) + "}";
}

// -------------------- STAFF DATA --------------------

Rows staff() {
    Rows out;

    for (auto r : read(SF)) {

        // Old 11-field staff records
        if (r.size() == 11) {
            r.push_back("0");
            r.push_back("0");
        }

        if (r.size() == 13) {

            if (r[4].find('@') == string::npos &&
                r[5].find('@') != string::npos) {
                swap(r[4], r[5]);
            }

            for (int i = 7; i < 13; i++) {
                r[i] = bt(r[i]);
            }

            out.push_back(r);
        }
    }

    write(SF, out);
    return out;
}

// -------------------- CUSTOMER DATA --------------------

Rows customers() {
    Rows out;

    for (auto r : read(CF)) {

        // Convert old 7-field customer records
        if (r.size() == 7) {
            r = {
                r[0], r[1], r[2],
                "", "", "", "", "", "", "",
                r[3], r[4], r[5],
                r[6] == "1" ? "Active" : r[6]
            };
        }

        // Convert old 13-field records
        if (r.size() == 13) {
            r.insert(r.begin() + 6, "");
        }

        if (r.size() == 14) {

            if (r[13] == "1") r[13] = "Active";
            if (r[13] == "0") r[13] = "Inactive";

            out.push_back(r);
        }
    }

    write(CF, out);
    return out;
}

// -------------------- ACCOUNT DATA --------------------

Rows accounts() {
    Rows out;

    for (auto r : read(AF)) {

        // Convert old 5-field account records
        if (r.size() == 5) {

            string bal = "0";

            for (auto& c : customers()) {
                if (c[1] == r[1] && c[11] == r[2]) {
                    bal = c[12];
                }
            }

            r.push_back(bal);

            r[4] = r[4] == "1" ? "Active" :
                   r[4] == "0" ? "Inactive" : r[4];
        }

        if (r.size() == 6) {
            out.push_back(r);
        }
    }

    write(AF, out);
    return out;
}

// -------------------- BRANCH DATA --------------------

Rows branches() {
    Rows out;

    for (auto r : read(BF)) {
        if (r.size() == 5) out.push_back(r);
    }

    return out;
}

// -------------------- ACCOUNT NUMBER --------------------

string accountNo(const Rows& a) {
    unsigned long long n = 100000000000ULL;

    for (auto& r : a) {
        try {
            n = max(n, stoull(r[2]));
        } catch (...) {}
    }

    return to_string(n + 1);
}

// -------------------- CUSTOMER STATUS UPDATE --------------------

void refreshCustomerStatuses() {

    auto c = customers();
    auto a = accounts();

    for (auto& x : c) {

        bool active = false;

        for (auto& y : a) {

            if (y[1] == x[1] && y[4] == "Active") {
                active = true;
                break;
            }
        }

        x[13] = active ? "Active" : "Inactive";
    }

    write(CF, c);
}

// ==================== MAIN SERVER ====================

int main() {

    Server s;

    s.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    s.Options(R"(.*)", [](const Request&, Response& r) {
        r.status = 204;
    });

    // -------------------- API STATUS --------------------

    s.Get("/api/status", [](const Request&, Response& r) {
        send(r, "{\"status\":\"ok\"}");
    });

    // ==================== STAFF ====================

    s.Get("/api/staff", [](const Request&, Response& r) {
        send(r, arr(staff()));
    });

    // ADD STAFF
    s.Post("/api/staff", [](const Request& req, Response& r) {

        auto a = staff();

        Row x = {
            nextId(a),
            p(req, "firstName"),
            p(req, "lastName"),
            p(req, "jobTitle"),
            p(req, "email"),
            p(req, "phone"),
            p(req, "branch"),
            bt(p(req, "canViewCustomers")),
            bt(p(req, "canManageAccounts")),
            bt(p(req, "canViewReports")),
            bt(p(req, "canManageStaff")),
            bt(p(req, "canAddEmployees")),
            bt(p(req, "canDeleteEmployees"))
        };

        if (x[1].empty() || x[2].empty() || !valid(x)) {
            send(r, err("First and last name are required; fields cannot contain |"), 400);
            return;
        }

        a.push_back(x);

        if (!write(SF, a)) {
            send(r, err("Cannot save staff file"), 500);
            return;
        }

        send(r, "{\"success\":true,\"id\":" + q(x[0]) + "}", 201);
    });

    // EDIT STAFF DETAILS AND PERMISSIONS
    s.Put(R"(/api/staff/(\d+))", [](const Request& req, Response& r) {

        auto a = staff();
        string id = req.matches[1];
        bool found = false;

        for (auto& x : a) {

            if (x[0] != id) continue;

            found = true;

            // Staff details
            if (req.has_param("firstName")) x[1] = p(req, "firstName");
            if (req.has_param("lastName"))  x[2] = p(req, "lastName");
            if (req.has_param("jobTitle"))  x[3] = p(req, "jobTitle");
            if (req.has_param("email"))     x[4] = p(req, "email");
            if (req.has_param("phone"))     x[5] = p(req, "phone");
            if (req.has_param("branch"))    x[6] = p(req, "branch");

            // Permissions
            if (req.has_param("canViewCustomers"))
                x[7] = bt(p(req, "canViewCustomers"));

            if (req.has_param("canManageAccounts"))
                x[8] = bt(p(req, "canManageAccounts"));

            if (req.has_param("canViewReports"))
                x[9] = bt(p(req, "canViewReports"));

            if (req.has_param("canManageStaff"))
                x[10] = bt(p(req, "canManageStaff"));

            if (req.has_param("canAddEmployees"))
                x[11] = bt(p(req, "canAddEmployees"));

            if (req.has_param("canDeleteEmployees"))
                x[12] = bt(p(req, "canDeleteEmployees"));

            if (!valid(x)) {
                send(r, err("Invalid staff information"), 400);
                return;
            }
        }

        if (!found) {
            send(r, err("Employee not found"), 404);
            return;
        }

        if (!write(SF, a)) {
            send(r, err("Could not save staff permissions"), 500);
            return;
        }

        send(r, "{\"success\":true}");
    });

    // DELETE STAFF
    s.Delete(R"(/api/staff/(\d+))", [](const Request& req, Response& r) {

        auto a = staff();
        Rows b;
        string id = req.matches[1];

        for (auto& x : a) {
            if (x[0] != id) b.push_back(x);
        }

        if (a.size() == b.size()) {
            send(r, err("Employee not found"), 404);
            return;
        }

        if (!write(SF, b)) {
            send(r, err("Could not save staff file"), 500);
            return;
        }

        send(r, "{\"success\":true}");
    });

    // ==================== CUSTOMERS ====================

    s.Get("/api/customers", [](const Request&, Response& r) {
        send(r, arr(customers()));
    });

    // ADD CUSTOMER AND FIRST ACCOUNT
    s.Post("/api/customers", [](const Request& req, Response& r) {

        auto c = customers();
        auto a = accounts();

        string cn = p(req, "cnic");
        string name = p(req, "fullName");

        if (cn.size() != 13 ||
            !all_of(cn.begin(), cn.end(), [](unsigned char z) {
                return isdigit(z);
            })) {
            send(r, err("New CNIC must be exactly 13 digits"), 400);
            return;
        }

        for (auto& x : c) {
            if (x[1] == cn) {
                send(r, err("CNIC already exists. Use Additional Account."), 409);
                return;
            }
        }

        string no = accountNo(a);
        string type = p(req, "accountType");
        string bal = p(req, "initialBalance");

        if (type.empty()) type = "Savings";
        if (bal.empty()) bal = "0";

        if (!validBalance(bal)) {
            send(r, err("Invalid initial balance"), 400);
            return;
        }

        Row x = {
            nextId(c), cn, name,
            p(req, "fatherName"),
            p(req, "motherName"),
            p(req, "dob"),
            p(req, "email"),
            p(req, "phone"),
            p(req, "address"),
            p(req, "branch"),
            type, no, bal, "Active"
        };

        if (name.empty() || !valid(x)) {
            send(r, err("Full name required or invalid field"), 400);
            return;
        }

        c.push_back(x);
        a.push_back({nextId(a), cn, no, type, "Active", bal});

        if (!write(CF, c) || !write(AF, a)) {
            send(r, err("Could not save customer/account"), 500);
            return;
        }

        send(r, "{\"success\":true,\"accountNumber\":" + q(no) + "}", 201);
    });

    // EDIT CUSTOMER
    s.Put(R"(/api/customers/(\d+))", [](const Request& req, Response& r) {

        auto c = customers();
        string id = req.matches[1];
        bool found = false;

        vector<pair<int, string>> fields = {
            {2, "fullName"},
            {3, "fatherName"},
            {4, "motherName"},
            {5, "dob"},
            {6, "email"},
            {7, "phone"},
            {8, "address"},
            {9, "branch"},
            {10, "accountType"},
            {12, "balance"}
        };

        for (auto& x : c) {

            if (x[0] != id) continue;

            found = true;

            for (auto& z : fields) {
                if (req.has_param(z.second)) {
                    x[z.first] = p(req, z.second);
                }
            }

            if (!valid(x) || !validBalance(x[12])) {
                send(r, err("Invalid customer field or balance"), 400);
                return;
            }
        }

        if (!found) {
            send(r, err("Customer not found"), 404);
            return;
        }

        if (!write(CF, c)) {
            send(r, err("Cannot save customer file"), 500);
            return;
        }

        // Keep first account details synchronized
        auto a = accounts();

        for (auto& x : c) {
            if (x[0] == id) {
                for (auto& y : a) {
                    if (y[1] == x[1] && y[2] == x[11]) {
                        y[3] = x[10];
                        y[5] = x[12];
                    }
                }
            }
        }

        if (!write(AF, a)) {
            send(r, err("Cannot save account file"), 500);
            return;
        }

        send(r, "{\"success\":true}");
    });

    // CUSTOMER STATUS
    s.Put(R"(/api/customers/(\d+)/status)", [](const Request& req, Response& r) {

        auto c = customers();

        string id = req.matches[1];
        string st = p(req, "status");

        if (st != "Active" && st != "Inactive") {
            send(r, err("Status must be Active or Inactive"), 400);
            return;
        }

        bool ok = false;

        for (auto& x : c) {
            if (x[0] == id) {
                x[13] = st;
                ok = true;
            }
        }

        if (!ok) {
            send(r, err("Customer not found"), 404);
            return;
        }

        if (!write(CF, c)) {
            send(r, err("Cannot save customer file"), 500);
            return;
        }

        send(r, "{\"success\":true}");
    });

    // DELETE CUSTOMER AND ALL LINKED ACCOUNTS
    s.Delete(R"(/api/customers/(\d+))", [](const Request& req, Response& r) {

        auto c = customers();
        Rows co;

        string id = req.matches[1];
        string cn;

        for (auto& x : c) {
            if (x[0] == id) cn = x[1];
            else co.push_back(x);
        }

        if (cn.empty()) {
            send(r, err("Customer not found"), 404);
            return;
        }

        Rows ao;

        for (auto& x : accounts()) {
            if (x[1] != cn) ao.push_back(x);
        }

        if (!write(CF, co) || !write(AF, ao)) {
            send(r, err("Could not delete customer and accounts"), 500);
            return;
        }

        send(r, "{\"success\":true}");
    });

    // ==================== ACCOUNTS ====================

    s.Get("/api/accounts", [](const Request&, Response& r) {
        send(r, arr(accounts()));
    });

    // ADD ADDITIONAL ACCOUNT
    s.Post("/api/accounts", [](const Request& req, Response& r) {

        auto c = customers();
        auto a = accounts();

        string cn = p(req, "cnic");
        bool exists = false;

        for (auto& x : c) {
            if (x[1] == cn) exists = true;
        }

        if (!exists) {
            send(r, err("Customer CNIC not found"), 404);
            return;
        }

        string no = accountNo(a);
        string type = p(req, "accountType");
        string bal = p(req, "initialBalance");

        if (type.empty()) type = "Savings";
        if (bal.empty()) bal = "0";

        if (!validBalance(bal)) {
            send(r, err("Invalid initial balance"), 400);
            return;
        }

        Row x = {nextId(a), cn, no, type, "Active", bal};

        if (!valid(x)) {
            send(r, err("Invalid account field"), 400);
            return;
        }

        a.push_back(x);

        if (!write(AF, a)) {
            send(r, err("Cannot save account file"), 500);
            return;
        }

        refreshCustomerStatuses();

        send(r, "{\"success\":true,\"accountNumber\":" + q(no) + "}", 201);
    });

    // EDIT ACCOUNT TYPE AND BALANCE
    s.Put(R"(/api/accounts/(\d+))", [](const Request& req, Response& r) {

        auto a = accounts();

        string id = req.matches[1];
        string type = p(req, "accountType");
        string balance = p(req, "balance");

        bool found = false;
        string customerCNIC;
        string accountNumber;

        if (req.has_param("balance") && !validBalance(balance)) {
            send(r, err("Invalid account balance"), 400);
            return;
        }

        for (auto& x : a) {

            if (x[0] != id) continue;

            found = true;

            customerCNIC = x[1];
            accountNumber = x[2];

            if (req.has_param("accountType")) {
                if (!safe(type) || type.empty()) {
                    send(r, err("Invalid account type"), 400);
                    return;
                }
                x[3] = type;
            }

            if (req.has_param("balance")) {
                x[5] = balance;
            }
        }

        if (!found) {
            send(r, err("Account not found"), 404);
            return;
        }

        if (!write(AF, a)) {
            send(r, err("Cannot save account file"), 500);
            return;
        }

        // Update matching customer's displayed account details
        auto c = customers();

        for (auto& x : c) {

            if (x[1] == customerCNIC && x[11] == accountNumber) {

                if (req.has_param("accountType")) {
                    x[10] = type;
                }

                if (req.has_param("balance")) {
                    x[12] = balance;
                }
            }
        }

        if (!write(CF, c)) {
            send(r, err("Account saved but customer file update failed"), 500);
            return;
        }

        send(r, "{\"success\":true}");
    });

    // ACTIVATE / DEACTIVATE ACCOUNT
    s.Put(R"(/api/accounts/(\d+)/status)", [](const Request& req, Response& r) {

        auto a = accounts();

        string id = req.matches[1];
        string st = p(req, "status");

        if (st != "Active" && st != "Inactive") {
            send(r, err("Invalid status"), 400);
            return;
        }

        bool found = false;

        for (auto& x : a) {

            if (x[0] == id) {
                x[4] = st;
                found = true;
            }
        }

        if (!found) {
            send(r, err("Account not found"), 404);
            return;
        }

        if (!write(AF, a)) {
            send(r, err("Cannot save account file"), 500);
            return;
        }

        refreshCustomerStatuses();

        send(r, "{\"success\":true}");
    });

    // ==================== BRANCHES ====================

    s.Get("/api/branches", [](const Request&, Response& r) {
        send(r, arr(branches()));
    });

    s.Post("/api/branches", [](const Request& req, Response& r) {

        auto a = branches();

        Row x = {
            nextId(a),
            p(req, "name"),
            p(req, "code"),
            p(req, "address"),
            p(req, "manager")
        };

        if (x[1].empty() || !valid(x)) {
            send(r, err("Branch name required"), 400);
            return;
        }

        a.push_back(x);

        if (!write(BF, a)) {
            send(r, err("Cannot save branch file"), 500);
            return;
        }

        send(r, "{\"success\":true}", 201);
    });

    // -------------------- START SERVER --------------------

    cout << "API running on port 8080" << endl;

    s.listen("0.0.0.0", 8080);

    return 0;
}
