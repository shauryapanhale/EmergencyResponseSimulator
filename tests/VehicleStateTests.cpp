#include <iostream>
#include <memory>
#include <vector>
#include "ers/common/Strings.h"
#include "ers/models/EmergencyFactory.h"
#include "ers/models/Station.h"
#include "ers/models/VehicleFactory.h"

using namespace ers;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)

template <typename Ex, typename F>
static bool throwsType(F f) {
    try { f(); } catch (const Ex&) { return true; } catch (...) { return false; }
    return false;
}

int main() {
    auto amb = createVehicle(VehicleType::Ambulance, 1, 0, 10, 40.0);
    CHECK(amb->isAvailable() && amb->currentNode() == 10);
    amb->assignIncident(7);
    CHECK(amb->status() == VehicleStatus::EnRoute);
    CHECK(amb->assignedIncident().value() == 7);
    amb->setCurrentNode(3);
    amb->markArrived();
    CHECK(amb->status() == VehicleStatus::OnScene);
    amb->markReturning();
    CHECK(amb->status() == VehicleStatus::Returning);
    amb->markBackAtBase();
    CHECK(amb->isAvailable() && amb->currentNode() == 10 && !amb->assignedIncident().has_value());

    CHECK(throwsType<InvalidTransitionException>([&] { amb->markArrived(); }));
    CHECK(throwsType<InvalidTransitionException>([&] { amb->setStatus(VehicleStatus::Returning); }));
    amb->assignIncident(8);
    CHECK(throwsType<InvalidTransitionException>([&] { amb->assignIncident(9); }));
    CHECK(amb->assignedIncident().value() == 8);

    auto police = createVehicle(VehicleType::Police, 2, 1, 11, 50.0);
    police->markOutOfService();
    CHECK(throwsType<InvalidTransitionException>([&] { police->assignIncident(1); }));
    police->markInService();
    CHECK(police->isAvailable());

    CHECK(throwsType<ValidationException>([] { createVehicle(VehicleType::FireTruck, 3, 0, 1, 0.0); }));
    CHECK(throwsType<ValidationException>([] { createVehicle(VehicleType::FireTruck, -1, 0, 1, 30.0); }));
    CHECK(throwsType<ValidationException>([&] { amb->setCurrentNode(-5); }));

    std::vector<std::unique_ptr<Vehicle>> fleet;
    fleet.push_back(createVehicle(VehicleType::Ambulance, 10, 0, 1, 40.0));
    fleet.push_back(createVehicle(VehicleType::FireTruck, 11, 0, 1, 35.0));
    fleet.push_back(createVehicle(VehicleType::Police, 12, 0, 1, 50.0));

    auto medical = createEmergency(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "");
    auto fire = createEmergency(EmergencyType::Fire, 2, 2, Priority::Medium, 0.0, "");
    auto crime = createEmergency(EmergencyType::Crime, 3, 2, Priority::Low, 0.0, "");
    auto accident = createEmergency(EmergencyType::RoadAccident, 4, 2, Priority::High, 0.0, "");

    CHECK(fleet[0]->canRespondTo(*medical) && !fleet[1]->canRespondTo(*medical) && !fleet[2]->canRespondTo(*medical));
    CHECK(!fleet[0]->canRespondTo(*fire) && fleet[1]->canRespondTo(*fire));
    CHECK(fleet[2]->canRespondTo(*crime) && !fleet[0]->canRespondTo(*crime));
    CHECK(fleet[0]->canRespondTo(*accident) && fleet[2]->canRespondTo(*accident) && !fleet[1]->canRespondTo(*accident));
    accident->changePriority(Priority::Critical);
    CHECK(fleet[1]->canRespondTo(*accident));

    CHECK(fleet[0]->hasEquipment("defibrillator"));
    CHECK(toString(VehicleStatus::EnRoute) == "En Route");

    Station st(0, "Central", 10, StationType::Shared);
    st.addVehicle(1);
    CHECK(throwsType<ValidationException>([&] { st.addVehicle(1); }));
    CHECK(st.removeVehicle(1) && !st.removeVehicle(1));

    if (failures == 0) std::cout << "vehicle tests passed\n";
    return failures == 0 ? 0 : 1;
}