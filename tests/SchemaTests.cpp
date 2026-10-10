#include "ers/repositories/Database.h"
#include "ers/repositories/Schema.h"
#include "ers/common/Exceptions.h"
#include <filesystem>
#include <iostream>
#include <string>

static int g_failures = 0;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        std::cerr << "CHECK failed: " #cond " (line " << __LINE__ << ")\n"; } } while (0)

using namespace ers;

static int scalarInt(Database& db, const std::string& sql) {
    Statement q(db, sql);
    return q.step() ? q.columnInt(0) : -1;
}

static bool throwsDb(Database& db, const std::string& sql) {
    try { db.exec(sql); } catch (const DatabaseException&) { return true; }
    return false;
}

int main() {
    auto p = std::filesystem::temp_directory_path() / "ers_schema_test.db";
    std::filesystem::remove(p);
    const std::string path = p.string();

    {
        Database db(path);
        initSchema(db);
        initSchema(db);  // idempotent

        CHECK(scalarInt(db,
            "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';")
            == kSchemaTableCount);
        CHECK(scalarInt(db, "SELECT COUNT(*) FROM schema_version;") == 1);
        CHECK(scalarInt(db, "SELECT version FROM schema_version;") == 1);
        CHECK(scalarInt(db, "PRAGMA foreign_keys;") == 1);

        CHECK(throwsDb(db,
            "INSERT INTO road (road_id, from_node, to_node, distance_km, base_minutes) "
            "VALUES (1, 10, 11, 1.0, 2.0);"));

        db.exec("INSERT INTO city_node VALUES (1, 'A', 0, 0, 'Intersection');");
        db.exec("INSERT INTO city_node VALUES (2, 'B', 1, 1, 'Intersection');");
        CHECK(throwsDb(db,
            "INSERT INTO road (road_id, from_node, to_node, distance_km, base_minutes) "
            "VALUES (1, 1, 2, -1.0, 2.0);"));
        CHECK(throwsDb(db,
            "INSERT INTO road (road_id, from_node, to_node, distance_km, base_minutes) "
            "VALUES (1, 1, 2, 1.0, 0);"));
        db.exec("INSERT INTO road (road_id, from_node, to_node, distance_km, base_minutes) "
                "VALUES (1, 1, 2, 1.0, 2.0);");
        CHECK(scalarInt(db, "SELECT COUNT(*) FROM road;") == 1);
        CHECK(throwsDb(db, "DELETE FROM city_node WHERE node_id = 1;"));
    }

    std::filesystem::remove(p);
    if (g_failures == 0) std::cout << "SchemaTests: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}