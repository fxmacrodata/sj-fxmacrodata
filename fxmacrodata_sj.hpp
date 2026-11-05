#ifndef FXMACRODATA_SJ_HPP
#define FXMACRODATA_SJ_HPP
#include <memory>
#include <string>
#include <utility>
#include <vector>
extern "C" {
#include "sj.h"
}
namespace fxmd_sj {
enum class Kind { object, array, string, number, boolean, null_value };
struct State;
class NativeSnapshot {
    std::vector<char> bytes_;
    sj_Reader reader_;
    sj_Value root_;
public:
    explicit NativeSnapshot(const std::string&);
    NativeSnapshot(const NativeSnapshot&)=delete;
    NativeSnapshot& operator=(const NativeSnapshot&)=delete;
    NativeSnapshot(NativeSnapshot&&)=delete;
    sj_Reader& reader() noexcept { return reader_; }
    sj_Value root() const noexcept { return root_; }
};
class Value {
    std::shared_ptr<const State> state_;
    std::size_t index_;
    Value(std::shared_ptr<const State>,std::size_t);
    friend class Document;
public:
    Kind kind() const;
    std::string raw() const;
    std::string text() const;
    std::unique_ptr<NativeSnapshot> native() const;
};
class Document {
    std::shared_ptr<const State> state_;
public:
    explicit Document(std::string bytes);
    Value at(const std::string& pointer="") const;
    const std::string& bytes() const noexcept;
};
struct Response {
    int status;
    std::vector<std::pair<std::string,std::string>> headers;
    std::string body;
    Document json() const { return Document(body); }
};
}
#endif
