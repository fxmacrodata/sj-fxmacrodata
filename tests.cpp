#include "fxmacrodata_sj.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <type_traits>
using namespace fxmd_sj;
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}assert(rejected);}
int main(){
 static_assert(!std::is_copy_constructible<NativeSnapshot>::value,"Native pointers require a single storage owner");
 const std::string original=R"({"currency":"USD","number":12345678901234567890.00100e-17,"missing":null,"clock":"2026-11-05T00:00:00Z","unknown":{"a/b":{"~key":[true,false,{"future":-0.00}]}}})";
 Response response{429,{{"Content-Type","application/json"},{"X-Test","one"},{"X-Test","two"}},original};
 auto doc=response.json();assert(response.status==429&&response.headers.size()==3&&response.body==original&&doc.bytes()==original);
 assert(doc.at("/number").raw()=="12345678901234567890.00100e-17");assert(doc.at("/missing").kind()==Kind::null_value);
 assert(doc.at("/unknown/a~1b/~0key/2/future").raw()=="-0.00");assert(doc.at("/clock").text()=="2026-11-05T00:00:00Z");
 assert(doc.at("/unknown/a~1b/~0key/0").raw()=="true");assert(doc.at().raw()==original);
 auto owned=Document(R"({"x":[1,{"nested":null}]})").at("/x");
 auto snapshot=owned.native();auto root=snapshot->root();assert(root.type==SJ_ARRAY&&root.depth==1);
 sj_Value value;assert(sj_iter_array(&snapshot->reader(),root,&value)&&value.type==SJ_NUMBER);
 assert(sj_iter_array(&snapshot->reader(),root,&value)&&value.type==SJ_OBJECT);
 assert(!sj_iter_array(&snapshot->reader(),root,&value));assert(!snapshot->reader().error);
 auto scalar=Document("-0.00100e+12").at().native();assert(scalar->root().type==SJ_NUMBER&&scalar->root().depth==0);
 auto text=Document(R"({"\u0000":"nul","a":"\uD834\uDD1E","":"empty","utf8":"é"})");
 assert(text.at(std::string("/\0",2)).text()=="nul");assert(text.at("/").text()=="empty");
 assert(text.at("/a").text()==std::string("\xf0\x9d\x84\x9e",4));assert(text.at("/utf8").text()=="é");
 for(const auto& bad:{""," ","01","-01","+1","1.","1e","1e+","--1","NaN","Infinity","[1,]","{\"a\":1,}","{\"a\" 1}","[1 2]","[}","{]","truefalse","null 1","\"\\x20\"","\"\\uD800\"","\"\\uDC00\"","\"\\uD800\\u0000\"","{\"a\":1,\"\\u0061\":2}"}) rejects([&]{Document d(bad);});
 for(const std::string& bad:{std::string("\"\xc0\x80\"",4),std::string("\"\xed\xa0\x80\"",5),std::string("\"\xf4\x90\x80\x80\"",6),std::string("\"a\0b\"",5),std::string("\"\n\"",3)})rejects([&]{Document d(bad);});
 for(const char* good:{"0","-0","1e+2","0.0","true","false","null","[]","{}","\"\"","[{},[],1,\"a\",null,false]"}){Document d(good);auto n=d.at().native();assert(n->root().type!=SJ_ERROR);}
 rejects([&]{doc.at("missing");});rejects([&]{doc.at("/unknown/a~1b/~0key/01");});rejects([&]{doc.at("/unknown/a~1b/~0key/-");});rejects([&]{doc.at("/unknown/a~1b/~0key/9999999999999999999999999999999");});
 rejects([&]{doc.at("/~2");});rejects([&]{doc.at("/~");});rejects([&]{doc.at("/number/child");});rejects([&]{doc.at("/number").text();});
 rejects([]{NativeSnapshot n("{]");});rejects([]{Document d(std::string(4*1024*1024+1,' '));});
 std::string boundary(128,'[');boundary+="0";boundary+=std::string(128,']');assert(Document(boundary).at().native()->root().type==SJ_ARRAY);
 std::string nested(129,'[');nested+="0";nested+=std::string(129,']');rejects([&]{Document d(nested);});
 Response binary{500,{{"Content-Type","application/octet-stream"}},std::string("\0\xff",2)};rejects([&]{binary.json();});assert(binary.body==std::string("\0\xff",2)&&binary.status==500);
 const std::string complete=R"({"a":[1,"x",true,null,{"b":2}]})";
 for(std::size_t i=0;i<complete.size();i++)rejects([&]{Document d(complete.substr(0,i));});
 std::string many="[";for(int i=0;i<100000;i++){if(i)many+=',';many+='0';}many+=']';rejects([&]{Document d(many);});
 assert(Document("  [1,2] \n").at().raw()=="[1,2]");
 std::cout<<"PASS: native nested iteration, response retention, ownership, exact lexemes, UTF-8/surrogates, RFC6901, invalid JSON, truncation and resource limits\n";
}
