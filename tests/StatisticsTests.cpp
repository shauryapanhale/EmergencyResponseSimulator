#include <cmath>
#include <iostream>
#include "ers/services/StatisticsService.h"

using namespace ers;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)
static bool nearEq(double a, double b) { return std::fabs(a - b) < 1e-9; }

template <typename Ex, typename F>
static bool throwsType(F f) {
    try { f(); } catch (const Ex&) { return true; } catch (...) { return false; }
    return false;
}

int main() {
    StatisticsService st;
    st.incidentCreated(1, 0.0);
    st.incidentCreated(2, 1.0);
    st.incidentCreated(3, 2.0);
    st.incidentDispatched(1, 0.0);
    st.incidentDispatched(2, 3.0);
    st.incidentDispatched(1, 5.0);  // ignored: only the first dispatch counts
    st.vehicleArrived(1, 0.0, 5.0);
    st.vehicleArrived(2, 3.0, 9.0);
    st.incidentResolved(1, 20.0);
    for (int i = 0; i < 3; ++i) st.trackVehicle(1, true, 1.0);
    st.trackVehicle(1, false, 1.0);
    st.trackVehicle(2, false, 1.0);

    StatsSnapshot s = st.snapshot();
    CHECK(s.created == 3 && s.resolved == 1 && s.active == 1 && s.pending == 1 && s.unserved == 1);
    CHECK(s.responseCount == 2);
    CHECK(nearEq(s.avgResponse, 5.5) && nearEq(s.minResponse, 5.0) && nearEq(s.maxResponse, 6.0));
    CHECK(nearEq(s.avgWaiting, 1.0));
    CHECK(nearEq(s.avgResolution, 20.0));
    CHECK(nearEq(s.utilization.at(1), 0.75) && nearEq(s.utilization.at(2), 0.0));

    CHECK(throwsType<ValidationException>([&] { st.incidentDispatched(99, 1.0); }));
    CHECK(throwsType<ValidationException>([&] { st.vehicleArrived(1, 5.0, 4.0); }));
    CHECK(throwsType<ValidationException>([&] { st.incidentResolved(1, 25.0); }));
    CHECK(throwsType<ValidationException>([&] { st.incidentCreated(1, 0.0); }));
    CHECK(throwsType<ValidationException>([&] { st.trackVehicle(1, true, 0.0); }));

    st.reset();
    CHECK(st.snapshot().created == 0 && st.snapshot().responseCount == 0);

    if (failures == 0) std::cout << "statistics tests passed\n";
    return failures == 0 ? 0 : 1;
}