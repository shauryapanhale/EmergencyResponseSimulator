#pragma once
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace ers {

// RAII wrapper over a SQLite connection. All failures throw DatabaseException.
class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;

    void exec(const std::string& sql);
    void begin();
    void commit();
    void rollback();
    long long lastInsertRowId() const;
    sqlite3* handle() const { return db_; }

private:
    void close() noexcept;
    sqlite3* db_ = nullptr;
};

// Rolls back automatically unless commit() was called.
class Transaction {
public:
    explicit Transaction(Database& db);
    ~Transaction();
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    void commit();

private:
    Database& db_;
    bool done_ = false;
};

// RAII prepared statement. Parameter and column indexes: binds are 1-based, columns 0-based.
class Statement {
public:
    Statement(Database& db, const std::string& sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    void bind(int index, int value);
    void bind(int index, long long value);
    void bind(int index, double value);
    void bind(int index, const std::string& value);
    void bindNull(int index);

    bool step();   // true = a row is available, false = finished
    void reset();

    int columnInt(int col) const;
    long long columnInt64(int col) const;
    double columnDouble(int col) const;
    std::string columnText(int col) const;
    bool columnIsNull(int col) const;

private:
    void finalize() noexcept;
    void check(int rc) const;
    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
};

} // namespace ers