#include "fxmacrodata_sj.hpp"
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
namespace fxmd_sj {
namespace {
struct Node {
    Kind kind; std::size_t start,end;
    std::string decoded;
    std::vector<std::pair<std::string,std::size_t>> children;
};
[[noreturn]] void invalid() { throw std::invalid_argument("Invalid or oversized JSON document"); }
void append_utf8(std::string& out,std::uint32_t cp) {
    if(cp<=0x7f) out.push_back(static_cast<char>(cp));
    else if(cp<=0x7ff) { out.push_back(static_cast<char>(0xc0|(cp>>6))); out.push_back(static_cast<char>(0x80|(cp&63))); }
    else if(cp<=0xffff) { out.push_back(static_cast<char>(0xe0|(cp>>12))); out.push_back(static_cast<char>(0x80|((cp>>6)&63))); out.push_back(static_cast<char>(0x80|(cp&63))); }
    else { out.push_back(static_cast<char>(0xf0|(cp>>18))); out.push_back(static_cast<char>(0x80|((cp>>12)&63))); out.push_back(static_cast<char>(0x80|((cp>>6)&63))); out.push_back(static_cast<char>(0x80|(cp&63))); }
}
}
struct State { std::string bytes; std::vector<Node> nodes; };
namespace {
class Parser {
    State& s; std::size_t pos=0;
    bool take(char c) { if(pos<s.bytes.size() && s.bytes[pos]==c){++pos;return true;}return false; }
    void need(char c) { if(!take(c)) invalid(); }
    void space() { while(pos<s.bytes.size() && (s.bytes[pos]==' '||s.bytes[pos]=='\t'||s.bytes[pos]=='\r'||s.bytes[pos]=='\n')) ++pos; }
    bool digit() const { return pos<s.bytes.size() && s.bytes[pos]>='0' && s.bytes[pos]<='9'; }
    std::uint32_t hex4() {
        std::uint32_t n=0;
        for(int i=0;i<4;i++) { if(pos==s.bytes.size()) { invalid(); }
            unsigned char c=s.bytes[pos++]; n<<=4;
            if(c>='0'&&c<='9') n+=c-'0'; else if(c>='a'&&c<='f') n+=c-'a'+10; else if(c>='A'&&c<='F') n+=c-'A'+10; else invalid(); }
        return n;
    }
    std::string string() {
        need('"'); std::string out;
        for(;;) {
            if(pos==s.bytes.size()) { invalid(); }
            unsigned char c=s.bytes[pos++];
            if(c=='"') return out;
            if(c<32) invalid();
            if(c=='\\') {
                if(pos==s.bytes.size()) { invalid(); }
                char esc=s.bytes[pos++];
                switch(esc) {
                case '"':case '\\':case '/':out.push_back(esc);break;
                case 'b':out.push_back('\b');break;case 'f':out.push_back('\f');break;
                case 'n':out.push_back('\n');break;case 'r':out.push_back('\r');break;case 't':out.push_back('\t');break;
                case 'u': { auto cp=hex4(); if(cp>=0xd800 && cp<=0xdbff){need('\\');need('u');auto low=hex4();if(low<0xdc00||low>0xdfff) invalid();cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);} else if(cp>=0xdc00&&cp<=0xdfff) invalid();append_utf8(out,cp);break; }
                default:invalid();
                }
            } else if(c<128) out.push_back(static_cast<char>(c));
            else {
                std::size_t start=pos-1; unsigned count; std::uint32_t cp,min;
                if(c>=0xc2&&c<=0xdf){count=1;cp=c&31;min=0x80;}
                else if(c>=0xe0&&c<=0xef){count=2;cp=c&15;min=0x800;}
                else if(c>=0xf0&&c<=0xf4){count=3;cp=c&7;min=0x10000;}
                else invalid();
                for(unsigned i=0;i<count;i++){if(pos==s.bytes.size())invalid();unsigned char b=s.bytes[pos++];if((b&0xc0)!=0x80)invalid();cp=(cp<<6)|(b&63);}
                if(cp<min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff)) { invalid(); }
                out.append(s.bytes,start,pos-start);
            }
        }
    }
    void number() {
        take('-'); if(!take('0')) {if(!digit()||s.bytes[pos]=='0')invalid();while(digit())++pos;}
        if(take('.')){if(!digit())invalid();while(digit())++pos;}
        if(take('e')||take('E')){if(!take('+'))take('-');if(!digit())invalid();while(digit())++pos;}
    }
    std::size_t value(unsigned depth) {
        space(); if(depth>128||pos==s.bytes.size()||s.nodes.size()>=100000)invalid();
        std::size_t start=pos, id=s.nodes.size();s.nodes.push_back({Kind::null_value,start,start,{}, {}});
        char c=s.bytes[pos]; if(depth==128&&(c=='{'||c=='['))invalid();
        if(c=='{'||c=='[') {
            ++pos; bool object=c=='{';s.nodes[id].kind=object?Kind::object:Kind::array;char end=object?'}':']';space();
            if(!take(end)) {std::set<std::string> keys;for(;;){
                std::string key;if(object){key=string();if(!keys.insert(key).second)invalid();space();need(':');}
                auto child=value(depth+1);s.nodes[id].children.emplace_back(std::move(key),child);space();
                if(take(end)) { break; }
                need(',');space();
            }}
        } else if(c=='"'){s.nodes[id].kind=Kind::string;s.nodes[id].decoded=string();}
        else if(c=='-'||(c>='0'&&c<='9')){s.nodes[id].kind=Kind::number;number();}
        else {
            const char* word=nullptr;
            if(c=='t'){word="true";s.nodes[id].kind=Kind::boolean;}else if(c=='f'){word="false";s.nodes[id].kind=Kind::boolean;}else if(c=='n')word="null";else invalid();
            for(;*word;word++){if(pos==s.bytes.size()||s.bytes[pos++]!=*word)invalid();}
        }
        s.nodes[id].end=pos;return id;
    }
public:
    explicit Parser(State& state):s(state){}
    void parse(){value(0);space();if(pos!=s.bytes.size())invalid();}
};
std::string segment(const std::string& p,std::size_t start,std::size_t end) {
    std::string out;
    for(auto i=start;i<end;i++){if(p[i]=='~'){if(++i==end)invalid();if(p[i]=='0')out+='~';else if(p[i]=='1')out+='/';else invalid();}else out+=p[i];}return out;
}
}
Document::Document(std::string bytes) {
    if(bytes.size()>4*1024*1024) { invalid(); }
    auto state=std::make_shared<State>();state->bytes=std::move(bytes);Parser(*state).parse();state_=std::move(state);
}
const std::string& Document::bytes() const noexcept{return state_->bytes;}
Value Document::at(const std::string& pointer) const {
    std::size_t id=0;if(pointer.empty())return Value(state_,id);if(pointer[0]!='/')invalid();
    std::size_t start=1;
    for(;;){auto end=pointer.find('/',start);if(end==std::string::npos)end=pointer.size();auto key=segment(pointer,start,end);const auto& n=state_->nodes[id];bool found=false;
        if(n.kind==Kind::object){for(const auto& child:n.children)if(child.first==key){id=child.second;found=true;break;}}
        else if(n.kind==Kind::array){if(key.empty()||(key.size()>1&&key[0]=='0'))throw std::out_of_range("JSON array index");std::size_t index=0;
            for(unsigned char c:key){if(c<'0'||c>'9'||index>(std::numeric_limits<std::size_t>::max()-(c-'0'))/10)throw std::out_of_range("JSON array index");index=index*10+c-'0';}
            if(index<n.children.size()){id=n.children[index].second;found=true;}}
        if(!found) { throw std::out_of_range("JSON pointer not found"); }
        if(end==pointer.size()) { return Value(state_,id); }
        start=end+1;
    }
}
Value::Value(std::shared_ptr<const State> state,std::size_t id):state_(std::move(state)),index_(id){}
Kind Value::kind()const{return state_->nodes[index_].kind;}
std::string Value::raw()const{const auto& n=state_->nodes[index_];return state_->bytes.substr(n.start,n.end-n.start);}
std::string Value::text()const{if(kind()!=Kind::string)throw std::invalid_argument("JSON value is not a string");return state_->nodes[index_].decoded;}
std::unique_ptr<NativeSnapshot> Value::native()const{return std::make_unique<NativeSnapshot>(raw());}
NativeSnapshot::NativeSnapshot(const std::string& bytes):bytes_{},reader_{},root_{} {
    // Direct construction has the same strict bounds/grammar requirement as Value::native().
    Document validated(bytes); bytes_.assign(bytes.begin(),bytes.end());
    reader_=sj_reader(bytes_.data(),bytes_.size());auto token=sj_read(&reader_);
    root_.type=token.type;root_.start=token.start;root_.end=token.end;
    // Upstream only initializes depth for container openings.
    if(token.type==SJ_OBJECT||token.type==SJ_ARRAY)root_.depth=token.depth;
    if(token.type==SJ_ERROR)throw std::invalid_argument("Native tokenization failed");
}
}
