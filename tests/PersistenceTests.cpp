#include "ers/repositories/Database.h"
#include "ers/common/Exceptions.h"
#include <filesystem>
#include <iostream>
#include <string>

static int g_failures = 0;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        std::cerr << "CHECK failed: " #cond " (line " << __LINE__ << ")\n"; } } while (0)

using namespace ers;

static std::string tempDbPath() {
    auto p = std::filesystem::temp_directory_path() / "ers_persistence_test.db";
    std::filesystem::remove(p);
    return p.string();
}

int main() {
    const std::string path = tempDbPath();

    {
        Database db(path);
        db.exec("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT NOT NULL, val REAL, note TEXT);");

        Transaction tx(db);
        Statement ins(db, "INSERT INTO t (id, name, val, note) VALUES (?, ?, ?, ?);");
        ins.bind(1, 1);
        ins.bind(2, std::string("alpha"));
        ins.bind(3, 2.5);
        ins.bindNull(4);
        ins.step();
        ins.reset();
        ins.bind(1, 2);
        ins.bind(2, std::string("beta"));
        ins.bind(3, 7.25);
        ins.bind(4, std::string("hello"));
        ins.step();
        tx.commit();
        CHECK(db.lastInsertRowId() == 2);
    }

    {
        Database db(path);
        Statement q(db, "SELECT id, name, val, note FROM t ORDER BY id;");
        CHECK(q.step());
        CHECK(q.columnInt(0) == 1);
        CHECK(q.columnText(1) == "alpha");
        CHECK(q.columnDouble(2) == 2.5);
        CHECK(q.columnIsNull(3));
        CHECK(q.step());
        CHECK(q.columnInt64(0) == 2);
        CHECK(q.columnText(1) == "beta");
        CHECK(q.columnText(3) == "hello");
        CHECK(!q.step());
    }

    {
        Database db(path);
        {
            Transaction tx(db);
            db.exec("INSERT INTO t (id, name) VALUES (99, 'ghost');");
        }
        Statement q(db, "SELECT COUNT(*) FROM t;");
        CHECK(q.step());
        CHECK(q.columnInt(0) == 2);
    }

    {
        Database db(path);
        bool badSql = false;
        try { db.exec("THIS IS NOT SQL;"); } catch (const DatabaseException&) { badSql = true; }
        CHECK(badSql);

        bool badPrepare = false;
        try { Statement s(db, "SELECT * FROM missing_table;"); }
        catch (const DatabaseException&) { badPrepare = true; }
        CHECK(badPrepare);

        bool constraint = false;
        try { db.exec("INSERT INTO t (id, name) VALUES (1, 'dup');"); }
        catch (const DatabaseException&) { constraint = true; }
        CHECK(constraint);
    }

    bool badPath = false;
    try { Database d("Z:/no/such/folder/x.db"); } catch (const DatabaseException&) { badPath = true; }
    CHECK(badPath);

    std::filesystem::remove(path);
    if (g_failures == 0) std::cout << "PersistenceTests: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}