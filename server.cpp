#include "httplib.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using namespace httplib;

const string STAFF_FILE="staff.txt", CUSTOMER_FILE="customers.txt", ACCOUNT_FILE="accounts.txt", BRANCH_FILE="branches.txt";

string clean(string s){ for(char &c:s) if(c=='|'||c=='\n'||c=='\r') c=' '; return s; }
string esc(const string&s){string o;for(unsigned char c:s){switch(c){case '"':o+="\\\"";break;case '\\':o+="\\\\";break;case '\n':o+="\\n";break;case '\r':o+="\\r";break;case '\t':o+="\\t";break;default:if(c<32){char b[8];snprintf(b,sizeof(b),"\\u%04x",c);o+=b;}else o+=(char)c;}}return o;}
string js(const string&s){return "\""+esc(s)+"\"";}
vector<string> split(const string&s){vector<string>v;stringstream ss(s);string x;while(getline(ss,x,'|'))v.push_back(x);if(!s.empty()&&s.back()=='|')v.push_back("");return v;}
vector<vector<string>> readFile(const string&f){vector<vector<string>>v;ifstream in(f);string l;while(getline(in,l))if(!l.empty())v.push_back(split(l));return v;}
bool saveFile(const string&f,const vector<vector<string>>&v){string t=f+".tmp";ofstream o(t);if(!o)return false;for(auto&r:v){for(size_t i=0;i<r.size();i++){if(i)o<<'|';o<<clean(r[i]);}o<<'\n';}o.close();if(!o)return false;remove(f.c_str());return rename(t.c_str(),f.c_str())==0;}
string p(const Request&r,const string&k){return r.has_param(k)?clean(r.get_param_value(k)):"";}
string idFor(const vector<vector<string>>&v,const string&prefix){int n=0;for(auto&r:v)if(!r.empty()){string d;for(char c:r[0])if(isdigit((unsigned char)c))d+=c;try{if(!d.empty())n=max(n,stoi(d));}catch(...){}}return prefix+to_string(n+1);}
string rowsJson(const vector<vector<string>>&v){string o="[";for(size_t i=0;i<v.size();i++){if(i)o+=",";o+="[";for(size_t j=0;j<v[i].size();j++){if(j)o+=",";o+=js(v[i][j]);}o+="]";}return o+"]";}
void json(Response&r,const string&s,int status=200){r.status=status;r.set_content(s,"application/json; charset=utf-8");}
bool hasCnic(const vector<vector<string>>&v,const string&c){for(auto&r:v)if(r.size()>1&&r[1]==c)return true;return false;}
bool hasAccount(const vector<vector<string>>&v,const string&n){for(auto&r:v)if(r.size()>2&&r[2]==n)return true;return false;}
string newAccountNo(const vector<vector<string>>&v){static mt19937_64 g((uint64_t)chrono::steady_clock::now().time_since_epoch().count());uniform_int_distribution<unsigned long long>d(100000000000ULL,999999999999ULL);for(int i=0;i<10000;i++){string n=to_string(d(g));if(!hasAccount(v,n))return n;}return "";}
bool validCnic(const string&c){string d;for(char x:c)if(isdigit((unsigned char)x))d+=x;return d.size()==13;}
void cors(Server&s){
 s.set_pre_routing_handler([](const Request&,Response&r){r.set_header("Access-Control-Allow-Origin","*");r.set_header("Access-Control-Allow-Methods","GET, POST, PUT, DELETE, OPTIONS");r.set_header("Access-Control-Allow-Headers","Content-Type");return Server::HandlerResponse::Unhandled;});
 s.Options(R"(.*)",[](const Request&,Response&r){r.status=204;});
}
int main(){
 Server s;cors(s);
 s.Get("/api/status",[](const Request&,Response&r){json(r,R"({"status":"ok","message":"API running"})");});

 // STAFF: [id, first, last, title, email, phone, branch, six permission flags]
 s.Get("/api/staff",[](const Request&,Response&r){json(r,rowsJson(readFile(STAFF_FILE)));});
 s.Post("/api/staff",[](const Request&q,Response&r){auto v=readFile(STAFF_FILE);if(p(q,"firstName").empty()||p(q,"lastName").empty())return json(r,R"({"error":"First and last name are required"})",400);
 v.push_back({idFor(v,"S"),p(q,"firstName"),p(q,"lastName"),p(q,"jobTitle"),p(q,"email"),p(q,"phone"),p(q,"branch"),p(q,"canViewCustomers"),p(q,"canManageAccounts"),p(q,"canViewReports"),p(q,"canManageStaff"),p(q,"canAddEmployees"),p(q,"canDeleteEmployees")});
 if(!saveFile(STAFF_FILE,v))return json(r,R"({"error":"Unable to save staff"})",500);json(r,R"({"success":true})",201);});
 s.Delete(R"(/api/staff/([A-Za-z0-9_-]+))",[](const Request&q,Response&r){string id=q.matches[1];auto v=readFile(STAFF_FILE);auto n=v.size();v.erase(remove_if(v.begin(),v.end(),[&](auto&a){return !a.empty()&&a[0]==id;}),v.end());if(n==v.size())return json(r,R"({"error":"Staff not found"})",404);if(!saveFile(STAFF_FILE,v))return json(r,R"({"error":"Save failed"})",500);json(r,R"({"success":true})");});

 // CUSTOMER: [id, CNIC, name, father, mother, DOB, email, phone, address, branch, type, primary account no, balance, status]
 s.Get("/api/customers",[](const Request&,Response&r){json(r,rowsJson(readFile(CUSTOMER_FILE)));});
 s.Post("/api/customers",[](const Request&q,Response&r){auto c=readFile(CUSTOMER_FILE);auto a=readFile(ACCOUNT_FILE);string cn=p(q,"cnic"),name=p(q,"fullName");
 if(!validCnic(cn))return json(r,R"({"error":"CNIC must be exactly 13 digits"})",400);
 if(hasCnic(c,cn))return json(r,R"({"error":"CNIC already exists. Use Open New Account for this customer."})",409);
 if(name.empty())return json(r,R"({"error":"Customer name is required"})",400);
 string bal=p(q,"initialBalance");if(bal.empty())bal="0";try{if(stod(bal)<0)throw 1;}catch(...){return json(r,R"({"error":"Initial balance must be zero or greater"})",400);}
 string no=newAccountNo(a);if(no.empty())return json(r,R"({"error":"Could not generate account number"})",500);
 string cid=idFor(c,"C"),aid=idFor(a,"A"),type=p(q,"accountType");if(type.empty())type="Savings";
 c.push_back({cid,cn,name,p(q,"fatherName"),p(q,"motherName"),p(q,"dob"),p(q,"email"),p(q,"phone"),p(q,"address"),p(q,"branch"),type,no,bal,"Active"});
 a.push_back({aid,cn,no,type,"Active",bal});
 if(!saveFile(CUSTOMER_FILE,c)||!saveFile(ACCOUNT_FILE,a))return json(r,R"({"error":"Could not save customer/account"})",500);
 json(r,"{\"success\":true,\"customerId\":"+js(cid)+",\"accountNumber\":"+js(no)+"}",201);});
 s.Put(R"(/api/customers/([A-Za-z0-9_-]+))",[](const Request&q,Response&r){string id=q.matches[1];auto v=readFile(CUSTOMER_FILE);bool ok=false;for(auto&x:v)if(!x.empty()&&x[0]==id){x.resize(max<size_t>(14,x.size()));x[2]=p(q,"fullName");x[3]=p(q,"fatherName");x[4]=p(q,"motherName");x[5]=p(q,"dob");x[6]=p(q,"email");x[7]=p(q,"phone");x[8]=p(q,"address");x[9]=p(q,"branch");x[10]=p(q,"accountType");x[12]=p(q,"balance");ok=true;}if(!ok)return json(r,R"({"error":"Customer not found"})",404);if(!saveFile(CUSTOMER_FILE,v))return json(r,R"({"error":"Save failed"})",500);json(r,R"({"success":true})");});
 s.Put(R"(/api/customers/([A-Za-z0-9_-]+)/status)",[](const Request&q,Response&r){string id=q.matches[1],st=p(q,"status");if(st!="Active"&&st!="Inactive")return json(r,R"({"error":"Invalid status"})",400);auto v=readFile(CUSTOMER_FILE);bool ok=false;for(auto&x:v)if(!x.empty()&&x[0]==id){x.resize(max<size_t>(14,x.size()));x[13]=st;ok=true;}if(!ok)return json(r,R"({"error":"Customer not found"})",404);if(!saveFile(CUSTOMER_FILE,v))return json(r,R"({"error":"Save failed"})",500);json(r,R"({"success":true})");});
 s.Delete(R"(/api/customers/([A-Za-z0-9_-]+))",[](const Request&q,Response&r){string id=q.matches[1],cn;auto c=readFile(CUSTOMER_FILE);for(auto&x:c)if(!x.empty()&&x[0]==id&&x.size()>1)cn=x[1];if(cn.empty())return json(r,R"({"error":"Customer not found"})",404);c.erase(remove_if(c.begin(),c.end(),[&](auto&x){return !x.empty()&&x[0]==id;}),c.end());auto a=readFile(ACCOUNT_FILE);a.erase(remove_if(a.begin(),a.end(),[&](auto&x){return x.size()>1&&x[1]==cn;}),a.end());if(!saveFile(CUSTOMER_FILE,c)||!saveFile(ACCOUNT_FILE,a))return json(r,R"({"error":"Delete failed"})",500);json(r,R"({"success":true})");});

 // ACCOUNTS: [id, customer CNIC, generated number, type, status, balance]
 s.Get("/api/accounts",[](const Request&,Response&r){json(r,rowsJson(readFile(ACCOUNT_FILE)));});
 s.Post("/api/accounts",[](const Request&q,Response&r){auto c=readFile(CUSTOMER_FILE);auto a=readFile(ACCOUNT_FILE);string cn=p(q,"cnic");if(!hasCnic(c,cn))return json(r,R"({"error":"Customer CNIC not found; create customer first"})",404);string bal=p(q,"initialBalance");if(bal.empty())bal="0";try{if(stod(bal)<0)throw 1;}catch(...){return json(r,R"({"error":"Initial balance must be zero or greater"})",400);}string no=newAccountNo(a);if(no.empty())return json(r,R"({"error":"Could not generate account number"})",500);string type=p(q,"accountType");if(type.empty())return json(r,R"({"error":"Account type required"})",400);a.push_back({idFor(a,"A"),cn,no,type,"Active",bal});if(!saveFile(ACCOUNT_FILE,a))return json(r,R"({"error":"Could not save account"})",500);json(r,"{\"success\":true,\"accountNumber\":"+js(no)+"}",201);});
 s.Put(R"(/api/accounts/([A-Za-z0-9_-]+)/status)",[](const Request&q,Response&r){string id=q.matches[1],st=p(q,"status");if(st!="Active"&&st!="Inactive")return json(r,R"({"error":"Invalid status"})",400);auto a=readFile(ACCOUNT_FILE);bool ok=false;for(auto&x:a)if(!x.empty()&&x[0]==id){x.resize(max<size_t>(6,x.size()));x[4]=st;ok=true;}if(!ok)return json(r,R"({"error":"Account not found"})",404);if(!saveFile(ACCOUNT_FILE,a))return json(r,R"({"error":"Save failed"})",500);json(r,R"({"success":true})");});

 // BRANCHES: [id, name, code, address, manager]
 s.Get("/api/branches",[](const Request&,Response&r){json(r,rowsJson(readFile(BRANCH_FILE)));});
 s.Post("/api/branches",[](const Request&q,Response&r){auto v=readFile(BRANCH_FILE);if(p(q,"name").empty())return json(r,R"({"error":"Branch name required"})",400);v.push_back({idFor(v,"B"),p(q,"name"),p(q,"code"),p(q,"address"),p(q,"manager")});if(!saveFile(BRANCH_FILE,v))return json(r,R"({"error":"Save failed"})",500);json(r,R"({"success":true})",201);});

 cout<<"Bank Admin Portal API listening on port 8080"<<endl;s.listen("0.0.0.0",8080);
}
