#pragma once

namespace ers {

class Database;

// Creates all tables if they do not exist (idempotent). Throws DatabaseException on failure.
void initSchema(Database& db);

// Number of user tables the schema creates (used by tests).
constexpr int kSchemaTableCount = 9;

} // namespace ers