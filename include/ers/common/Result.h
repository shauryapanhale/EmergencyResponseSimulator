#pragma once
#include <string>
#include <utility>

namespace ers {

template <typename T>
class Result {
public:
    static Result ok(T value) { return Result(std::move(value), true, ""); }
    static Result fail(std::string error) { return Result(T{}, false, std::move(error)); }

    bool isOk() const { return ok_; }
    const T& value() const { return value_; }
    const std::string& error() const { return error_; }

private:
    Result(T v, bool ok, std::string e) : value_(std::move(v)), ok_(ok), error_(std::move(e)) {}
    T value_;
    bool ok_;
    std::string error_;
};

} // namespace ers