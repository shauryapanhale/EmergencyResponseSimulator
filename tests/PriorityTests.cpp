#include <iostream>
#include <vector>
#include "ers/models/EmergencyFactory.h"
#include "ers/models/PriorityKey.h"
#include "ers/services/PendingQueue.h"

using namespace ers;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)

int main() {
    CHECK((PriorityKey{0, 5.0, 9} < PriorityKey{1, 0.0, 1}));
    CHECK((PriorityKey{1, 1.0, 9} < PriorityKey{1, 2.0, 1}));
    CHECK((PriorityKey{1, 1.0, 1} < PriorityKey{1, 1.0, 2}));
    CHECK(!(PriorityKey{1, 1.0, 2} < PriorityKey{1, 1.0, 1}));

    auto e1 = createEmergency(EmergencyType::Medical, 1, 1, Priority::Low, 0.0, "a");
    auto e2 = createEmergency(EmergencyType::Fire, 2, 1, Priority::High, 1.0, "b");
    auto e3 = createEmergency(EmergencyType::Crime, 3, 1, Priority::Critical, 2.0, "c");
    auto e4 = createEmergency(EmergencyType::Medical, 4, 1, Priority::High, 0.5, "d");

    PendingQueue q;
    CHECK(q.empty() && !q.popNext().has_value());
    q.push(*e1); q.push(*e2); q.push(*e3); q.push(*e4);
    CHECK(q.size() == 4);
    CHECK((q.snapshot() == std::vector<EmergencyId>{3, 4, 2, 1}));

    e1->changePriority(Priority::Critical);
    q.push(*e1);
    CHECK(q.size() == 4);
    CHECK((q.snapshot() == std::vector<EmergencyId>{1, 3, 4, 2}));

    CHECK(q.remove(3));
    CHECK(!q.remove(3));
    CHECK(!q.contains(3));

    CHECK(q.popNext().value() == 1);
    CHECK(q.popNext().value() == 4);
    CHECK(q.popNext().value() == 2);
    CHECK(!q.popNext().has_value());
    CHECK(q.empty());

    auto a = createEmergency(EmergencyType::Fire, 10, 1, Priority::High, 1.0, "");
    auto b = createEmergency(EmergencyType::Fire, 11, 1, Priority::High, 1.0, "");
    PendingQueue t;
    t.push(*b); t.push(*a);
    CHECK(t.popNext().value() == 10);

    if (failures == 0) std::cout << "priority tests passed\n";
    return failures == 0 ? 0 : 1;
}