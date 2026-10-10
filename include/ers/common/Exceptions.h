#pragma once
#include <stdexcept>
#include <string>

namespace ers {

class ErsException : public std::runtime_error {
public:
    explicit ErsException(const std::string& msg) : std::runtime_error(msg) {}
};
class ValidationException : public ErsException {
public:
    explicit ValidationException(const std::string& m) : ErsException(m) {}
};
class InvalidIncidentException : public ErsException {
public:
    explicit InvalidIncidentException(const std::string& m) : ErsException(m) {}
};
class InvalidTransitionException : public ErsException {
public:
    explicit InvalidTransitionException(const std::string& m) : ErsException(m) {}
};
class NoRouteException : public ErsException {
public:
    explicit NoRouteException(const std::string& m) : ErsException(m) {}
};
class NoEligibleVehicleException : public ErsException {
public:
    explicit NoEligibleVehicleException(const std::string& m) : ErsException(m) {}
};
class DatabaseException : public ErsException {
public:
    explicit DatabaseException(const std::string& m) : ErsException(m) {}
};

} // namespace ers