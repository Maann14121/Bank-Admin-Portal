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
using Row=vector<string>; using Rows=vector<Row>;
const string SF="staff.txt", CF="customers.txt", AF="accounts.txt", BF="branches.txt";
string esc(const string&s){string o;for(char c:s){if(c=='"')o+="\\\"";else if(c=='\\')o+="\\\\";else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else if((unsigned char)c>=32)o+=c;}return o;}
string q(const string&s){return "\""+esc(s)+"\"";}
string p(const Request&r,const string&k){return r.has_param(k)?r.get_param_value(k):"";}
bool safe(const string&s){return s.find('|')==string::npos&&s.find('\n')==string::npos&&s.find('\r')==string::npos;}
Rows read(const string&f){Rows a;ifstream in(f);string line;while(getline(in,line)){Row r;stringstream ss(line);string v;while(getline(ss,v,'|'))r.push_back(v);if(!r.empty())a.push_back(r);}return a;}
bool write(const string&f,const Rows&a){string t=f+".tmp";{ofstream o(t);if(!o)return false;for(auto&r:a){for(size_t i=0;i<r.size();i++){if(i)o<<'|';o<<r[i];}o<<'\n';}if(!o.good())return false;}if(rename(t.c_str(),f.c_str())){remove(t.c_str());return false;}return true;}
string nextId(const Rows&a){long long n=0;for(auto&r:a)try{n=max(n,stoll(r[0]));}catch(...){}return to_string(n+1);}
string bt(string s){return(s=="1"||s=="true"||s=="TRUE"||s=="yes"||s=="on")?"true":"false";}
Rows staff(){Rows out;for(auto r:read(SF)){if(r.size()==11){r.push_back("0");r.push_back("0");}if(r.size()==13){if(r[4].find('@')==string::npos&&r[5].find('@')!=string::npos)swap(r[4],r[5]);for(int i=7;i<13;i++)r[i]=bt(r[i]);out.push_back(r);}}write(SF,out);return out;}
Rows customers(){Rows out;for(auto r:read(CF)){if(r.size()==7)r={r[0],r[1],r[2],"","","","","","","",r[3],r[4],r[5],r[6]=="1"?"Active":r[6]};if(r.size()==13)r.insert(r.begin()+6,"");if(r.size()==14){if(r[13]=="1")r[13]="Active";if(r[13]=="0")r[13]="Inactive";out.push_back(r);}}write(CF,out);return out;}
Rows accounts(){Rows out;for(auto r:read(AF)){if(r.size()==5){string bal="0";for(auto&c:customers())if(c[1]==r[1]&&c[11]==r[2])bal=c[12];r.push_back(bal);r[4]=r[4]=="1"?"Active":r[4]=="0"?"Inactive":r[4];}if(r.size()==6)out.push_back(r);}write(AF,out);return out;}
Rows branches(){Rows out;for(auto r:read(BF))if(r.size()==5)out.push_back(r);return out;}
string arr(const Rows&a){string s="[";for(size_t i=0;i<a.size();i++){if(i)s+=",";s+="[";for(size_t j=0;j<a[i].size();j++){if(j)s+=",";s+=q(a[i][j]);}s+="]";}return s+"]";}
void send(Response&r,string s,int code=200){r.status=code;r.set_content(s,"application/json; charset=utf-8");}
string err(string s){return "{\"error\":"+q(s)+"}";}
bool valid(const Row&r){for(auto&s:r)if(!safe(s))return false;return true;}
string accountNo(const Rows&a){unsigned long long n=100000000000ULL;for(auto&r:a)try{n=max(n,stoull(r[2]));}catch(...){}return to_string(n+1);}
int main(){Server s;s.set_default_headers({{"Access-Control-Allow-Origin","*"},{"Access-Control-Allow-Methods","GET,POST,PUT,DELETE,OPTIONS"},{"Access-Control-Allow-Headers","Content-Type"}});s.Options(R"(.*)",[](const Request&,Response&r){r.status=204;});
s.Get("/api/status",[](const Request&,Response&r){send(r,"{\"status\":\"ok\"}");});
s.Get("/api/staff",[](const Request&,Response&r){send(r,arr(staff()));});
s.Post("/api/staff",[](const Request&req,Response&r){auto a=staff();Row x={nextId(a),p(req,"firstName"),p(req,"lastName"),p(req,"jobTitle"),p(req,"email"),p(req,"phone"),p(req,"branch"),bt(p(req,"canViewCustomers")),bt(p(req,"canManageAccounts")),bt(p(req,"canViewReports")),bt(p(req,"canManageStaff")),bt(p(req,"canAddEmployees")),bt(p(req,"canDeleteEmployees"))};if(x[1].empty()||x[2].empty()||!valid(x)){send(r,err("First and last name are required; fields cannot contain |"),400);return;}a.push_back(x);if(!write(SF,a)){send(r,err("Cannot save staff file"),500);return;}send(r,"{\"success\":true,\"id\":"+q(x[0])+"}",201);});
s.Delete(R"(/api/staff/(\d+))",[](const Request&req,Response&r){auto a=staff();Rows b;string id=req.matches[1];for(auto&x:a)if(x[0]!=id)b.push_back(x);if(a.size()==b.size()){send(r,err("Employee not found"),404);return;}write(SF,b);send(r,"{\"success\":true}");});
s.Get("/api/customers",[](const Request&,Response&r){send(r,arr(customers()));});
s.Post("/api/customers",[](const Request&req,Response&r){auto c=customers();auto a=accounts();string cn=p(req,"cnic"),name=p(req,"fullName");if(cn.size()!=13||!all_of(cn.begin(),cn.end(),[](unsigned char z){return isdigit(z);})){send(r,err("New CNIC must be exactly 13 digits"),400);return;}for(auto&x:c)if(x[1]==cn){send(r,err("CNIC already exists. Use Additional Account."),409);return;}string no=accountNo(a),type=p(req,"accountType"),bal=p(req,"initialBalance");if(type.empty())type="Savings";if(bal.empty())bal="0";Row x={nextId(c),cn,name,p(req,"fatherName"),p(req,"motherName"),p(req,"dob"),p(req,"email"),p(req,"phone"),p(req,"address"),p(req,"branch"),type,no,bal,"Active"};if(name.empty()||!valid(x)){send(r,err("Full name required or invalid field"),400);return;}c.push_back(x);a.push_back({nextId(a),cn,no,type,"Active",bal});if(!write(CF,c)||!write(AF,a)){send(r,err("Could not save customer/account"),500);return;}send(r,"{\"success\":true,\"accountNumber\":"+q(no)+"}",201);});
s.Put(R"(/api/customers/(\d+))",[](const Request&req,Response&r){auto c=customers();string id=req.matches[1];bool found=false;vector<pair<int,string>> f={{2,"fullName"},{3,"fatherName"},{4,"motherName"},{5,"dob"},{6,"email"},{7,"phone"},{8,"address"},{9,"branch"},{10,"accountType"},{12,"balance"}};for(auto&x:c)if(x[0]==id){found=true;for(auto&z:f)if(req.has_param(z.second))x[z.first]=p(req,z.second);}if(!found){send(r,err("Customer not found"),404);return;}write(CF,c);auto a=accounts();for(auto&x:c)if(x[0]==id)for(auto&y:a)if(y[1]==x[1]&&y[2]==x[11]){y[3]=x[10];y[5]=x[12];}write(AF,a);send(r,"{\"success\":true}");});
s.Put(R"(/api/customers/(\d+)/status)",[](const Request&req,Response&r){auto c=customers();string id=req.matches[1],st=p(req,"status");if(st!="Active"&&st!="Inactive"){send(r,err("Status must be Active or Inactive"),400);return;}bool ok=false;for(auto&x:c)if(x[0]==id){x[13]=st;ok=true;}if(!ok){send(r,err("Customer not found"),404);return;}write(CF,c);send(r,"{\"success\":true}");});
s.Delete(R"(/api/customers/(\d+))",[](const Request&req,Response&r){auto c=customers();Rows co;string id=req.matches[1],cn;for(auto&x:c)if(x[0]==id)cn=x[1];else co.push_back(x);if(cn.empty()){send(r,err("Customer not found"),404);return;}Rows ao;for(auto&x:accounts())if(x[1]!=cn)ao.push_back(x);write(CF,co);write(AF,ao);send(r,"{\"success\":true}");});
s.Get("/api/accounts",[](const Request&,Response&r){send(r,arr(accounts()));});
s.Post("/api/accounts",[](const Request&req,Response&r){auto c=customers();auto a=accounts();string cn=p(req,"cnic");bool exists=false;for(auto&x:c)if(x[1]==cn)exists=true;if(!exists){send(r,err("Customer CNIC not found"),404);return;}string no=accountNo(a),type=p(req,"accountType"),bal=p(req,"initialBalance");if(type.empty())type="Savings";if(bal.empty())bal="0";a.push_back({nextId(a),cn,no,type,"Active",bal});write(AF,a);send(r,"{\"success\":true,\"accountNumber\":"+q(no)+"}",201);});
s.Put(R"(/api/accounts/(\d+)/status)",[](const Request&req,Response&r){auto a=accounts();string id=req.matches[1],st=p(req,"status");if(st!="Active"&&st!="Inactive"){send(r,err("Invalid status"),400);return;}bool ok=false;for(auto&x:a)if(x[0]==id){x[4]=st;ok=true;}if(!ok){send(r,err("Account not found"),404);return;}write(AF,a);send(r,"{\"success\":true}");});
s.Get("/api/branches",[](const Request&,Response&r){send(r,arr(branches()));});
s.Post("/api/branches",[](const Request&req,Response&r){auto a=branches();Row x={nextId(a),p(req,"name"),p(req,"code"),p(req,"address"),p(req,"manager")};if(x[1].empty()||!valid(x)){send(r,err("Branch name required"),400);return;}a.push_back(x);write(BF,a);send(r,"{\"success\":true}",201);});
cout<<"API running on port 8080"<<endl;s.listen("0.0.0.0",8080);}
