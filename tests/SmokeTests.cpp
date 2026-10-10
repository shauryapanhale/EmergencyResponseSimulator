#include <iostream>
#include <string>
#include "ers/common/Exceptions.h"
#include "ers/common/Result.h"
#include "ers/common/Version.h"

int main() {
    auto r = ers::Result<int>::ok(5);
    if (!r.isOk() || r.value() != 5) return 1;
    auto f = ers::Result<int>::fail("bad");
    if (f.isOk() || f.error() != "bad") return 1;
    try { throw ers::NoRouteException("x"); }
    catch (const ers::ErsException&) { std::cout << "smoke ok\n"; return 0; }
    return 1;
}