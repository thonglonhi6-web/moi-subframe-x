#include "../src/Journal.hpp"
#include <cassert>
#include <iostream>
using namespace moi;
using us = std::chrono::microseconds;
int main() {
    Journal j; TimePoint t0 = Clock::time_point{} + std::chrono::seconds(1);
    j.frameBegin(t0); j.frameBegin(t0 + us(16667));
    assert(j.onButton(true, 1, true, t0 + us(16667 + 4000)) == Anomaly::None);
    assert(j.onButton(true, 1, true, t0 + us(16667 + 4100)) == Anomaly::DuplicatePush);
    assert(j.heldCount() == 1);
    assert(j.onButton(false, 1, true, t0 + us(16667 + 9000)) == Anomaly::None);
    assert(j.onButton(false, 1, true, t0 + us(16667 + 9100)) == Anomaly::OrphanRelease);
    assert(j.heldCount() == 0);
    assert(j.onButton(true, 1, false, t0 + us(16667 + 100)) == Anomaly::None);
    assert(j.heldCount() == 1); j.clearHeld(); assert(j.heldCount() == 0);
    assert(j.onButton(true, 99, true, t0) == Anomaly::None);
    assert(j.events() == 6 && j.duplicatePushes() == 1 && j.orphanReleases() == 1);
    std::cout << j.summary() << "\n";
    j.reset(); assert(j.events() == 0);
    std::cout << "journal_test OK\n";
}
