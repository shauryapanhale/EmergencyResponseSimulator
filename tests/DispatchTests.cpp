#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ers/models/EmergencyFactory.h"
#include "ers/models/VehicleFactory.h"
#include "ers/services/DispatchEngine.h"

using namespace ers;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)
static bool nearEq(double a, double b) { return std::fabs(a - b) < 1e-9; }

template <typename Ex, typename F>
static bool throwsType(F f) {
    try { f(); } catch (const Ex&) { return true; } catch (...) { return false; }
    return false;
}

using Fleet = std::vector<std::unique_ptr<Vehicle>>;

static CityMap makeCity() {
    CityMap m;
    for (int i = 0; i <= 5; ++i) m.addNode(CityNode(i, "N" + std::to_string(i), i * 10.0, 0));
    m.addRoad(Road(1, 0, 1, 2.0, 4.0));
    m.addRoad(Road(2, 1, 2, 3.0, 6.0));
    m.addRoad(Road(3, 1, 3, 2.0, 4.0));
    m.addRoad(Road(4, 3, 2, 1.0, 2.0));
    m.addRoad(Road(5, 0, 4, 3.0, 6.0));
    m.addRoad(Road(6, 4, 3, 2.0, 5.0));
    m.addRoad(Road(7, 5, 1, 2.0, 3.0));
    return m;
}

static Fleet fullFleet() {
    Fleet f;
    f.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 40.0));
    f.push_back(createVehicle(VehicleType::Ambulance, 2, 0, 5, 40.0));
    f.push_back(createVehicle(VehicleType::FireTruck, 3, 1, 4, 35.0));
    f.push_back(createVehicle(VehicleType::Police, 4, 2, 5, 50.0));
    return f;
}

int main() {
    // 1. Closest eligible ambulance by ETA
    {
        CityMap city = makeCity(); RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet = fullFleet();
        auto e = createEmergency(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "");
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        CHECK(r.fullyAllocated() && r.records.size() == 1);
        CHECK(r.records[0].vehicleId() == 2);
        CHECK(nearEq(r.records[0].etaMinutes(), 9.0));
        CHECK(nearEq(r.records[0].etaMinutes(), r.records[0].route().totalMinutes()));
        CHECK(fleet[1]->status() == VehicleStatus::EnRoute);
        CHECK(fleet[0]->isAvailable());
        CHECK(e->status() == EmergencyStatus::Dispatched);
        CHECK(!r.records[0].reason().empty());
    }
    // 2. Blocked road makes the closer ambulance unreachable
    {
        CityMap city = makeCity(); city.setBlocked(7, true);
        RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet = fullFleet();
        auto e = createEmergency(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "");
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        CHECK(r.records.size() == 1 && r.records[0].vehicleId() == 1);
        CHECK(nearEq(r.records[0].etaMinutes(), 10.0));
    }
    // 3. Incompatible vehicle never chosen: only a fire truck for a medical incident
    {
        CityMap city = makeCity(); RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet;
        fleet.push_back(createVehicle(VehicleType::FireTruck, 3, 1, 4, 35.0));
        auto e = createEmergency(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "");
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        CHECK(!r.anyAllocated());
        CHECK(r.unmet == std::vector<VehicleType>{VehicleType::Ambulance});
        CHECK(e->status() == EmergencyStatus::Pending);
        CHECK(fleet[0]->isAvailable());
        CHECK(r.explanation.find("Ambulance") != std::string::npos);
    }
    // 4. No double booking
    {
        CityMap city = makeCity(); RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet;
        fleet.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 40.0));
        auto e1 = createEmergency(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "");
        auto e2 = createEmergency(EmergencyType::Medical, 2, 1, Priority::High, 1.0, "");
        CHECK(de.dispatch(*e1, fleet, 0.0).fullyAllocated());
        DispatchResult r2 = de.dispatch(*e2, fleet, 1.0);
        CHECK(!r2.anyAllocated());
        CHECK(e2->status() == EmergencyStatus::Pending);
        CHECK(fleet[0]->assignedIncident().value() == 1);
        CHECK(r2.explanation.find("unavailable") != std::string::npos);
    }
    // 5. Partial allocation for a critical road accident (no fire truck in fleet)
    {
        CityMap city = makeCity(); RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet;
        fleet.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 40.0));
        fleet.push_back(createVehicle(VehicleType::Police, 4, 2, 5, 50.0));
        auto e = createEmergency(EmergencyType::RoadAccident, 1, 3, Priority::Critical, 0.0, "");
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        CHECK(r.records.size() == 2);
        CHECK(r.unmet == std::vector<VehicleType>{VehicleType::FireTruck});
        CHECK(r.anyAllocated() && !r.fullyAllocated());
        CHECK(e->status() == EmergencyStatus::Dispatched);
    }
    // 6. Tie-break: lowest vehicle id
    {
        CityMap city = makeCity(); RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet;
        fleet.push_back(createVehicle(VehicleType::Ambulance, 7, 0, 0, 40.0));
        fleet.push_back(createVehicle(VehicleType::Ambulance, 5, 0, 0, 40.0));
        auto e = createEmergency(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "");
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        CHECK(r.records[0].vehicleId() == 5);
    }
    // 7. Only pending incidents can be dispatched; record ids increase
    {
        CityMap city = makeCity(); RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet = fullFleet();
        auto e = createEmergency(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "");
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        CHECK(throwsType<InvalidTransitionException>([&] { de.dispatch(*e, fleet, 1.0); }));
        auto e2 = createEmergency(EmergencyType::Crime, 2, 3, Priority::Low, 1.0, "");
        DispatchResult r2 = de.dispatch(*e2, fleet, 1.0);
        CHECK(r.records[0].id() == 1 && r2.records[0].id() == 2);
        CHECK(de.dispatchesMade() == 2);
    }
    // 8. Unreachable only vehicle stays available
    {
        CityMap city = makeCity(); city.setBlocked(7, true);
        RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet;
        fleet.push_back(createVehicle(VehicleType::Ambulance, 2, 0, 5, 40.0));
        auto e = createEmergency(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "");
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        CHECK(!r.anyAllocated() && fleet[0]->isAvailable());
        CHECK(r.explanation.find("1 unreachable") != std::string::npos);
    }
    // 9. DispatchRecord timestamps and friend audit
    {
        DispatchRecord rec(1, 1, 2, Route({0, 1}, {1}, 4.0), 5.0, 4.0, "test reason");
        CHECK(throwsType<ValidationException>([&] { rec.markArrived(3.0); }));
        CHECK(throwsType<ValidationException>([&] { rec.responseTime(); }));
        rec.markArrived(9.0);
        CHECK(nearEq(rec.responseTime(), 4.0));
        CHECK(throwsType<ValidationException>([&] { rec.markArrived(10.0); }));
        CHECK(throwsType<ValidationException>([&] { rec.markCompleted(8.0); }));
        rec.markCompleted(12.0);
        CHECK(rec.hasCompleted());
        CHECK(dispatchAudit(rec).find("Emergency 1") != std::string::npos);
    }

    if (failures == 0) std::cout << "dispatch tests passed\n";
    return failures == 0 ? 0 : 1;
}