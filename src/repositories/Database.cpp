#include "ers/repositories/Database.h"
#include "ers/common/Exceptions.h"
#include <sqlite3.h>
#include <utility>

namespace ers {

Database::Database(const std::string& path) {
    int rc = sqlite3_open_v2(path.c_str(), &db_,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        close();
        throw DatabaseException("Cannot open database '" + path + "': " + msg);
    }
    exec("PRAGMA foreign_keys = ON;");
}

Database::~Database() { close(); }

Database::Database(Database&& other) noexcept : db_(other.db_) { other.db_ = nullptr; }

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        close();
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

void Database::close() noexcept {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void Database::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw DatabaseException("SQL error: " + msg);
    }
}

void Database::begin() { exec("BEGIN;"); }
void Database::commit() { exec("COMMIT;"); }
void Database::rollback() { exec("ROLLBACK;"); }

long long Database::lastInsertRowId() const {
    return static_cast<long long>(sqlite3_last_insert_rowid(db_));
}

Transaction::Transaction(Database& db) : db_(db) { db_.begin(); }

Transaction::~Transaction() {
    if (!done_) {
        try { db_.rollback(); } catch (...) {}
    }
}

void Transaction::commit() {
    db_.commit();
    done_ = true;
}

Statement::Statement(Database& db, const std::string& sql) : db_(db.handle()) {
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db_);
        throw DatabaseException("Cannot prepare statement: " + msg);
    }
}

Statement::~Statement() { finalize(); }

Statement::Statement(Statement&& other) noexcept : db_(other.db_), stmt_(other.stmt_) {
    other.db_ = nullptr;
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        finalize();
        db_ = other.db_;
        stmt_ = other.stmt_;
        other.db_ = nullptr;
        other.stmt_ = nullptr;
    }
    return *this;
}

void Statement::finalize() noexcept {
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

void Statement::check(int rc) const {
    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {
        throw DatabaseException(std::string("SQLite error: ") + sqlite3_errmsg(db_));
    }
}

void Statement::bind(int index, int value) { check(sqlite3_bind_int(stmt_, index, value)); }
void Statement::bind(int index, long long value) {
    check(sqlite3_bind_int64(stmt_, index, static_cast<sqlite3_int64>(value)));
}
void Statement::bind(int index, double value) { check(sqlite3_bind_double(stmt_, index, value)); }
void Statement::bind(int index, const std::string& value) {
    check(sqlite3_bind_text(stmt_, index, value.c_str(), static_cast<int>(value.size()),
                            SQLITE_TRANSIENT));
}
void Statement::bindNull(int index) { check(sqlite3_bind_null(stmt_, index)); }

bool Statement::step() {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;
    throw DatabaseException(std::string("SQLite step failed: ") + sqlite3_errmsg(db_));
}

void Statement::reset() {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

int Statement::columnInt(int col) const { return sqlite3_column_int(stmt_, col); }
long long Statement::columnInt64(int col) const {
    return static_cast<long long>(sqlite3_column_int64(stmt_, col));
}
double Statement::columnDouble(int col) const { return sqlite3_column_double(stmt_, col); }
std::string Statement::columnText(int col) const {
    const unsigned char* t = sqlite3_column_text(stmt_, col);
    return t ? std::string(reinterpret_cast<const char*>(t)) : std::string();
}
bool Statement::columnIsNull(int col) const {
    return sqlite3_column_type(stmt_, col) == SQLITE_NULL;
}

} // namespace ers