#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ers/models/EmergencyFactory.h"

using namespace ers;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)

static bool has(const std::vector<VehicleType>& v, VehicleType t) {
    for (auto x : v) if (x == t) return true;
    return false;
}

int main() {
    int before = Emergency::totalCreated();

    auto med = createEmergency(EmergencyType::Medical, 1, 3, Priority::High, 0.0, "chest pain");
    auto fire = createEmergency(EmergencyType::Fire, 2, 4, Priority::Medium, 1.0, "kitchen fire");
    auto crime = createEmergency(EmergencyType::Crime, 3, 5, Priority::Low, 2.0, "robbery");
    auto acc = createEmergency(EmergencyType::RoadAccident, 4, 6, Priority::High, 3.0, "collision");

    CHECK(Emergency::totalCreated() == before + 4);

    CHECK(med->type() == EmergencyType::Medical && med->typeName() == "Medical");
    CHECK(med->requiredResources() == std::vector<VehicleType>{VehicleType::Ambulance});
    CHECK(crime->requiredResources() == std::vector<VehicleType>{VehicleType::Police});
    CHECK(has(acc->requiredResources(), VehicleType::Ambulance));
    CHECK(has(acc->requiredResources(), VehicleType::Police));
    CHECK(!has(acc->requiredResources(), VehicleType::FireTruck));

    CHECK(!has(fire->requiredResources(), VehicleType::Ambulance));
    fire->changePriority(Priority::Critical);
    CHECK(has(fire->requiredResources(), VehicleType::Ambulance));
    acc->changePriority(Priority::Critical);
    CHECK(has(acc->requiredResources(), VehicleType::FireTruck));

    CHECK(med->status() == EmergencyStatus::Pending);
    med->markDispatched();
    med->markOnScene();
    med->markResolved();
    CHECK(med->status() == EmergencyStatus::Resolved);

    bool badTransition = false;
    try { med->markDispatched(); } catch (const InvalidTransitionException&) { badTransition = true; }
    CHECK(badTransition);

    bool closedPriority = false;
    try { med->changePriority(Priority::Low); } catch (const InvalidTransitionException&) { closedPriority = true; }
    CHECK(closedPriority);

    bool skipStep = false;
    try { crime->markOnScene(); } catch (const InvalidTransitionException&) { skipStep = true; }
    CHECK(skipStep);

    crime->markUnserved();
    CHECK(crime->status() == EmergencyStatus::Unserved);

    bool badNode = false;
    try { createEmergency(EmergencyType::Fire, 9, -1, Priority::Low, 0.0, "x"); }
    catch (const InvalidIncidentException&) { badNode = true; }
    CHECK(badNode);

    bool badDesc = false;
    try { createEmergency(EmergencyType::Fire, 9, 1, Priority::Low, 0.0, std::string(201, 'a')); }
    catch (const InvalidIncidentException&) { badDesc = true; }
    CHECK(badDesc);

    if (failures == 0) std::cout << "emergency tests passed\n";
    return failures == 0 ? 0 : 1;
}