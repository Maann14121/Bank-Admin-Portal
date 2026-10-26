#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>

using namespace std;

// Educational console project only. Do not use real customer data or real banking.
struct Staff {
    int id{};
    string firstName, lastName, title, phone, email, branch;
    bool canViewCustomers{}, canManageAccounts{}, canViewReports{}, canManageStaff{};
};

struct Customer {
    string cnic, name, fatherName, motherName, address, accountType, branch;
    long long accountNumber{};
    double balance{};
    bool active{true};
};

struct Transaction {
    long long accountNumber{};
    string type;
    double amount{};
    string note;
};

vector<Staff> staff;
vector<Customer> customers;
vector<Transaction> transactions;
int nextStaffId = 1001;
long long nextAccountNumber = 1000000001LL;

const string STAFF_FILE = "staff.txt";
const string CUSTOMER_FILE = "customers.txt";
const string TRANSACTION_FILE = "transactions.txt";

string readLine(const string& prompt) {
    cout << prompt;
    string value;
    getline(cin, value);
    // The pipe is used as a file separator.
    replace(value.begin(), value.end(), '|', '/');
    return value;
}

int readInt(const string& prompt, int minValue, int maxValue) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        stringstream ss(line);
        int value;
        char extra;
        if (ss >> value && !(ss >> extra) && value >= minValue && value <= maxValue)
            return value;
        cout << "Invalid choice. Please enter a number from "
             << minValue << " to " << maxValue << ".\n";
    }
}

double readMoney(const string& prompt, double minValue = 0.0) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        stringstream ss(line);
        double value;
        char extra;
        if (ss >> value && !(ss >> extra) && value >= minValue)
            return value;
        cout << "Invalid amount. Enter a valid amount >= " << minValue << ".\n";
    }
}

bool askYesNo(const string& prompt) {
    while (true) {
        string answer = readLine(prompt + " (y/n): ");
        transform(answer.begin(), answer.end(), answer.begin(),
                  [](unsigned char c) { return static_cast<char>(tolower(c)); });
        if (answer == "y" || answer == "yes") return true;
        if (answer == "n" || answer == "no") return false;
        cout << "Please enter y or n.\n";
    }
}

string money(double amount) {
    ostringstream out;
    out << fixed << setprecision(2) << amount;
    return out.str();
}

string statusText(bool active) { return active ? "ACTIVE" : "DEACTIVATED"; }

void saveData() {
    {
        ofstream out(STAFF_FILE);
        for (const auto& s : staff) {
            out << s.id << '|' << s.firstName << '|' << s.lastName << '|'
                << s.title << '|' << s.phone << '|' << s.email << '|' << s.branch << '|'
                << s.canViewCustomers << '|' << s.canManageAccounts << '|'
                << s.canViewReports << '|' << s.canManageStaff << '\n';
        }
    }
    {
        ofstream out(CUSTOMER_FILE);
        for (const auto& c : customers) {
            out << c.cnic << '|' << c.name << '|' << c.fatherName << '|'
                << c.motherName << '|' << c.address << '|' << c.accountType << '|'
                << c.branch << '|' << c.accountNumber << '|' << setprecision(15)
                << c.balance << '|' << c.active << '\n';
        }
    }
    {
        ofstream out(TRANSACTION_FILE);
        for (const auto& t : transactions)
            out << t.accountNumber << '|' << t.type << '|' << setprecision(15)
                << t.amount << '|' << t.note << '\n';
    }
}

vector<string> split(const string& line, char delimiter = '|') {
    vector<string> parts;
    string part;
    stringstream ss(line);
    while (getline(ss, part, delimiter)) parts.push_back(part);
    return parts;
}

void loadData() {
    string line;
    {
        ifstream in(STAFF_FILE);
        while (getline(in, line)) {
            auto p = split(line);
            if (p.size() != 11) continue;
            try {
                Staff s;
                s.id = stoi(p[0]); s.firstName = p[1]; s.lastName = p[2];
                s.title = p[3]; s.phone = p[4]; s.email = p[5]; s.branch = p[6];
                s.canViewCustomers = stoi(p[7]) != 0;
                s.canManageAccounts = stoi(p[8]) != 0;
                s.canViewReports = stoi(p[9]) != 0;
                s.canManageStaff = stoi(p[10]) != 0;
                staff.push_back(s);
                nextStaffId = max(nextStaffId, s.id + 1);
            } catch (...) {}
        }
    }
    {
        ifstream in(CUSTOMER_FILE);
        while (getline(in, line)) {
            auto p = split(line);
            if (p.size() != 10) continue;
            try {
                Customer c;
                c.cnic = p[0]; c.name = p[1]; c.fatherName = p[2];
                c.motherName = p[3]; c.address = p[4]; c.accountType = p[5];
                c.branch = p[6]; c.accountNumber = stoll(p[7]);
                c.balance = stod(p[8]); c.active = stoi(p[9]) != 0;
                customers.push_back(c);
                nextAccountNumber = max(nextAccountNumber, c.accountNumber + 1);
            } catch (...) {}
        }
    }
    {
        ifstream in(TRANSACTION_FILE);
        while (getline(in, line)) {
            auto p = split(line);
            if (p.size() != 4) continue;
            try {
                transactions.push_back({stoll(p[0]), p[1], stod(p[2]), p[3]});
            } catch (...) {}
        }
    }
}

int findCustomerByCNIC(const string& cnic) {
    for (int i = 0; i < static_cast<int>(customers.size()); ++i)
        if (customers[i].cnic == cnic) return i;
    return -1;
}

int findCustomerByAccount(long long accountNumber) {
    for (int i = 0; i < static_cast<int>(customers.size()); ++i)
        if (customers[i].accountNumber == accountNumber) return i;
    return -1;
}

int findStaffById(int id) {
    for (int i = 0; i < static_cast<int>(staff.size()); ++i)
        if (staff[i].id == id) return i;
    return -1;
}

void printCustomer(const Customer& c) {
    cout << "\n--- CUSTOMER / ACCOUNT DETAILS ---\n"
         << "Name:             " << c.name << '\n'
         << "Father's name:    " << c.fatherName << '\n'
         << "Mother's name:    " << c.motherName << '\n'
         << "CNIC:             " << c.cnic << '\n'
         << "Address:          " << c.address << '\n'
         << "Account type:     " << c.accountType << '\n'
         << "Account number:   " << c.accountNumber << '\n'
         << "Branch:           " << c.branch << '\n'
         << "Balance:          Rs. " << money(c.balance) << '\n'
         << "Status:           " << statusText(c.active) << "\n";
}

void printStaff(const Staff& s) {
    cout << "\nStaff ID: " << s.id << "\nName: " << s.firstName << ' ' << s.lastName
         << "\nWorking title: " << s.title << "\nPhone: " << s.phone
         << "\nEmail: " << s.email << "\nBranch/location: " << s.branch << '\n';
}

void addStaff() {
    Staff s;
    s.id = nextStaffId++;
    cout << "\n--- ADD STAFF MEMBER ---\n";
    s.firstName = readLine("First name: ");
    s.lastName = readLine("Last name: ");
    s.title = readLine("Working title: ");
    s.phone = readLine("Contact number: ");
    s.email = readLine("Email address: ");
    s.branch = readLine("Branch/location: ");
    cout << "\nSet permissions (y = allow, n = deny):\n";
    s.canViewCustomers = askYesNo("Can view customer records?");
    s.canManageAccounts = askYesNo("Can activate/deactivate accounts?");
    s.canViewReports = askYesNo("Can view reports?");
    s.canManageStaff = askYesNo("Can manage staff directory?");
    staff.push_back(s);
    saveData();
    cout << "Staff member added. Staff ID: " << s.id << '\n';
}

void removeStaff() {
    int id = readInt("Enter staff ID to remove: ", 1, numeric_limits<int>::max());
    int index = findStaffById(id);
    if (index < 0) { cout << "Staff ID not found.\n"; return; }
    printStaff(staff[index]);
    if (!askYesNo("Remove this staff member")) { cout << "Cancelled.\n"; return; }
    staff.erase(staff.begin() + index);
    saveData();
    cout << "Staff member removed.\n";
}

void staffDirectory() {
    while (true) {
        cout << "\n========== STAFF DIRECTORY ==========\n"
             << "1. Show all staff\n2. Search by first name\n3. Search by last name\n"
             << "4. Search by branch/location\n5. Add staff\n6. Remove staff\n"
             << "0. Back\n";
        int choice = readInt("Select: ", 0, 6);
        if (choice == 0) return;
        if (choice == 5) { addStaff(); continue; }
        if (choice == 6) { removeStaff(); continue; }
        string query;
        if (choice == 2) query = readLine("First name: ");
        if (choice == 3) query = readLine("Last name: ");
        if (choice == 4) query = readLine("Branch/location: ");
        string q = query;
        transform(q.begin(), q.end(), q.begin(), [](unsigned char c) {
            return static_cast<char>(tolower(c));
        });
        bool found = false;
        for (const auto& s : staff) {
            string field;
            if (choice == 2) field = s.firstName;
            else if (choice == 3) field = s.lastName;
            else if (choice == 4) field = s.branch;
            string f = field;
            transform(f.begin(), f.end(), f.begin(), [](unsigned char c) {
                return static_cast<char>(tolower(c));
            });
            if (choice == 1 || f.find(q) != string::npos) {
                printStaff(s); found = true;
            }
        }
        if (!found) cout << "No matching staff found.\n";
    }
}

void staffPermissions() {
    int id = readInt("Enter staff ID to view permissions: ", 1, numeric_limits<int>::max());
    int i = findStaffById(id);
    if (i < 0) { cout << "Staff ID not found.\n"; return; }
    const Staff& s = staff[i];
    printStaff(s);
    cout << "\n--- ACCESS PERMISSIONS ---\n"
         << "View customer records:       " << (s.canViewCustomers ? "ALLOWED" : "DENIED") << '\n'
         << "Manage account status:       " << (s.canManageAccounts ? "ALLOWED" : "DENIED") << '\n'
         << "View reports:                " << (s.canViewReports ? "ALLOWED" : "DENIED") << '\n'
         << "Manage staff directory:      " << (s.canManageStaff ? "ALLOWED" : "DENIED") << '\n';
    if (askYesNo("Edit these permissions")) {
        staff[i].canViewCustomers = askYesNo("Allow viewing customer records?");
        staff[i].canManageAccounts = askYesNo("Allow account activation/deactivation?");
        staff[i].canViewReports = askYesNo("Allow viewing reports?");
        staff[i].canManageStaff = askYesNo("Allow managing staff directory?");
        saveData();
        cout << "Permissions updated.\n";
    }
}

void addCustomer() {
    Customer c;
    cout << "\n--- ADD CUSTOMER / OPEN ACCOUNT ---\n";
    c.name = readLine("Customer full name: ");
    c.fatherName = readLine("Father's name: ");
    c.motherName = readLine("Mother's name: ");
    c.cnic = readLine("CNIC (example format 12345-1234567-1): ");
    if (c.cnic.empty()) { cout << "CNIC cannot be empty.\n"; return; }
    if (findCustomerByCNIC(c.cnic) >= 0) {
        cout << "A customer with this CNIC already exists. Use customer search instead.\n";
        return;
    }
    c.address = readLine("Home address: ");
    cout << "Account types:\n1. Savings\n2. Current\n3. Basic\n";
    int type = readInt("Select account type: ", 1, 3);
    c.accountType = (type == 1 ? "Savings" : type == 2 ? "Current" : "Basic");
    c.branch = readLine("Branch/location: ");
    c.balance = readMoney("Initial deposit (Rs.): ");
    c.active = true;
    c.accountNumber = nextAccountNumber++;
    customers.push_back(c);
    if (c.balance > 0)
        transactions.push_back({c.accountNumber, "OPENING DEPOSIT", c.balance, "Initial account balance"});
    saveData();
    cout << "\nCustomer added successfully. Generated account number: " << c.accountNumber << '\n';
}

void removeCustomer() {
    string cnic = readLine("Enter customer's CNIC to remove: ");
    int i = findCustomerByCNIC(cnic);
    if (i < 0) { cout << "Customer not found.\n"; return; }
    printCustomer(customers[i]);
    cout << "For an actual bank, accounts should normally be closed/archived, not erased.\n";
    if (!askYesNo("Permanently remove this DEMO record")) { cout << "Cancelled.\n"; return; }
    long long acc = customers[i].accountNumber;
    customers.erase(customers.begin() + i);
    transactions.erase(remove_if(transactions.begin(), transactions.end(),
        [acc](const Transaction& t) { return t.accountNumber == acc; }), transactions.end());
    saveData();
    cout << "Demo customer record removed.\n";
}

void searchCustomer() {
    string cnic = readLine("Enter CNIC: ");
    int i = findCustomerByCNIC(cnic);
    if (i < 0) { cout << "No customer found for that CNIC.\n"; return; }
    printCustomer(customers[i]);
    cout << "\n1. Activate account\n2. Deactivate account\n0. Back\n";
    int choice = readInt("Select: ", 0, 2);
    if (choice == 0) return;
    customers[i].active = (choice == 1);
    saveData();
    cout << "Account status updated to " << statusText(customers[i].active) << ".\n";
}

void listCustomers() {
    if (customers.empty()) { cout << "No customer records.\n"; return; }
    for (const auto& c : customers) printCustomer(c);
}

void accountManagement() {
    long long acc = stoll(to_string(readInt("Enter account number (last 9 digits may not fit): ", 1, numeric_limits<int>::max())));
    int i = findCustomerByAccount(acc);
    if (i < 0) { cout << "Account not found. Use CNIC search for full account numbers.\n"; return; }
    printCustomer(customers[i]);
    cout << "1. Activate\n2. Deactivate\n0. Back\n";
    int choice = readInt("Select: ", 0, 2);
    if (choice == 0) return;
    customers[i].active = (choice == 1);
    saveData();
    cout << "Account status updated.\n";
}

void recordTransaction() {
    long long acc;
    cout << "Account number: ";
    string line; getline(cin, line);
    try { acc = stoll(line); } catch (...) { cout << "Invalid account number.\n"; return; }
    int i = findCustomerByAccount(acc);
    if (i < 0) { cout << "Account not found.\n"; return; }
    if (!customers[i].active) { cout << "This account is deactivated; transaction denied.\n"; return; }
    cout << "1. Deposit\n2. Withdraw\n";
    int choice = readInt("Select: ", 1, 2);
    double amount = readMoney("Amount (Rs.): ", 0.01);
    if (choice == 2 && amount > customers[i].balance) {
        cout << "Insufficient demo balance. Transaction cancelled.\n"; return;
    }
    if (choice == 1) {
        customers[i].balance += amount;
        transactions.push_back({acc, "DEPOSIT", amount, "Admin demo transaction"});
    } else {
        customers[i].balance -= amount;
        transactions.push_back({acc, "WITHDRAWAL", amount, "Admin demo transaction"});
    }
    saveData();
    cout << "Transaction recorded. New balance: Rs. " << money(customers[i].balance) << '\n';
}

void transactionHistory() {
    long long acc;
    cout << "Enter account number: ";
    string line; getline(cin, line);
    try { acc = stoll(line); } catch (...) { cout << "Invalid account number.\n"; return; }
    bool found = false;
    cout << "\n--- TRANSACTION HISTORY ---\n";
    for (const auto& t : transactions) {
        if (t.accountNumber == acc) {
            cout << t.type << " | Rs. " << money(t.amount) << " | " << t.note << '\n';
            found = true;
        }
    }
    if (!found) cout << "No transactions found for this account.\n";
}

void dashboard() {
    int active = 0, inactive = 0;
    double totalBalances = 0;
    for (const auto& c : customers) {
        if (c.active) ++active; else ++inactive;
        totalBalances += c.balance;
    }
    cout << "\n========== BANK ADMIN DASHBOARD ==========\n"
         << "Total staff:             " << staff.size() << '\n'
         << "Total customers:         " << customers.size() << '\n'
         << "Active accounts:         " << active << '\n'
         << "Deactivated accounts:    " << inactive << '\n'
         << "Total demo balances:     Rs. " << money(totalBalances) << '\n'
         << "Recorded transactions:   " << transactions.size() << '\n';
}

void reports() {
    cout << "\n========== REPORTS ==========\n";
    cout << "STAFF BY BRANCH\n";
    for (const auto& s : staff)
        cout << s.firstName << ' ' << s.lastName << " | " << s.title << " | " << s.branch << '\n';
    cout << "\nCUSTOMER ACCOUNTS\n";
    for (const auto& c : customers)
        cout << c.accountNumber << " | " << c.name << " | " << c.branch
             << " | " << c.accountType << " | " << statusText(c.active)
             << " | Rs. " << money(c.balance) << '\n';
}

void seedDemoDataIfEmpty() {
    if (!staff.empty() || !customers.empty()) return;
    staff.push_back({nextStaffId++, "Ayesha", "Khan", "Branch Manager",
                     "0300-0000000", "ayesha.demo@example.com", "Lahore Main Branch",
                     true, true, true, true});
    staff.push_back({nextStaffId++, "Hamza", "Ali", "Customer Service Officer",
                     "0300-0000001", "hamza.demo@example.com", "Karachi Branch",
                     true, false, false, false});
    customers.push_back({"12345-1234567-1", "Demo Customer", "Demo Father",
                         "Demo Mother", "Demo Address", "Savings", "Lahore Main Branch",
                         nextAccountNumber++, 25000.00, true});
    saveData();
}

int main() {
    loadData();
    seedDemoDataIfEmpty();

    cout << "============================================\n"
         << "       BANK MANAGEMENT ADMIN PORTAL\n"
         << "============================================\n"
         << "Educational demo only. Do not enter real CNICs or financial data.\n";

    while (true) {
        cout << "\n=============== MAIN MENU ===============\n"
             << "1. Admin dashboard\n"
             << "2. Staff directory (search/add/remove)\n"
             << "3. Staff access permissions\n"
             << "4. Search customer by CNIC / account status\n"
             << "5. Add customer / open account\n"
             << "6. Remove demo customer\n"
             << "7. List all customers\n"
             << "8. Account management (activate/deactivate)\n"
             << "9. Deposit / withdrawal (demo)\n"
             << "10. Transaction history\n"
             << "11. Reports\n"
             << "0. Save and exit\n";
        int choice = readInt("Choose a page: ", 0, 11);
        switch (choice) {
            case 1: dashboard(); break;
            case 2: staffDirectory(); break;
            case 3: staffPermissions(); break;
            case 4: searchCustomer(); break;
            case 5: addCustomer(); break;
            case 6: removeCustomer(); break;
            case 7: listCustomers(); break;
            case 8: accountManagement(); break;
            case 9: recordTransaction(); break;
            case 10: transactionHistory(); break;
            case 11: reports(); break;
            case 0:
                saveData();
                cout << "Data saved. Goodbye!\n";
                return 0;
        }
    }
}
