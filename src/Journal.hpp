#pragma once
// Pure C++ (no Geode dependency) so the logic can be unit-tested off-device.
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>

namespace moi {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

enum class Anomaly { None, DuplicatePush, OrphanRelease };

class Journal {
public:
    static constexpr int kBuckets = 8;

    void frameBegin(TimePoint now) {
        if (m_haveFrame) {
            m_lastFrameUs = std::chrono::duration_cast<std::chrono::microseconds>(now - m_frameStart).count();
        }
        m_frameStart = now;
        m_haveFrame = true;
        m_eventsThisFrame = 0;
        ++m_frames;
    }

    // Records one button event exactly as the game applies it. Never alters it.
    Anomaly onButton(bool down, int button, bool isPlayer1, TimePoint now) {
        ++m_events;
        down ? ++m_pushes : ++m_releases;
        if (++m_eventsThisFrame == 2) ++m_multiEventFrames;

        if (m_haveFrame) {
            auto off = std::chrono::duration_cast<std::chrono::microseconds>(now - m_frameStart).count();
            m_sumOffsetUs += off;
            ++m_offsetSamples;
            if (off < m_minOffsetUs) m_minOffsetUs = off;
            if (off > m_maxOffsetUs) m_maxOffsetUs = off;
            if (m_lastFrameUs > 0) {
                int b = static_cast<int>(off * kBuckets / m_lastFrameUs);
                if (b < 0) b = 0;
                if (b >= kBuckets) b = kBuckets - 1;
                ++m_hist[b];
            }
        }

        if (button < 0 || button >= kButtons) return Anomaly::None;
        bool& held = m_held[isPlayer1 ? 0 : 1][button];
        if (down) {
            if (held) { ++m_dupPush; return Anomaly::DuplicatePush; }
            held = true;
        } else {
            if (!held) { ++m_orphanRelease; return Anomaly::OrphanRelease; }
            held = false;
        }
        return Anomaly::None;
    }

    int heldCount() const {
        int n = 0;
        for (auto& p : m_held) for (bool h : p) n += h;
        return n;
    }
    void clearHeld() { for (auto& p : m_held) p.fill(false); }

    void reset() { *this = Journal{}; }

    uint64_t events() const { return m_events; }
    uint64_t duplicatePushes() const { return m_dupPush; }
    uint64_t orphanReleases() const { return m_orphanRelease; }

    std::string summary() const {
        char buf[512];
        double mean = m_offsetSamples ? (double)m_sumOffsetUs / m_offsetSamples / 1000.0 : 0.0;
        double mn = m_offsetSamples ? m_minOffsetUs / 1000.0 : 0.0;
        double mx = m_offsetSamples ? m_maxOffsetUs / 1000.0 : 0.0;
        std::snprintf(buf, sizeof buf,
            "[MOI SUBFRAME] frames=%llu events=%llu (push=%llu release=%llu) multiEventFrames=%llu "
            "dupPush=%llu orphanRelease=%llu stillHeld=%d | offset-in-frame ms mean=%.3f min=%.3f max=%.3f | "
            "hist(1/8 frame)=%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu",
            (unsigned long long)m_frames, (unsigned long long)m_events,
            (unsigned long long)m_pushes, (unsigned long long)m_releases,
            (unsigned long long)m_multiEventFrames, (unsigned long long)m_dupPush,
            (unsigned long long)m_orphanRelease, heldCount(), mean, mn, mx,
            (unsigned long long)m_hist[0], (unsigned long long)m_hist[1], (unsigned long long)m_hist[2],
            (unsigned long long)m_hist[3], (unsigned long long)m_hist[4], (unsigned long long)m_hist[5],
            (unsigned long long)m_hist[6], (unsigned long long)m_hist[7]);
        return buf;
    }

private:
    static constexpr int kButtons = 8;
    std::array<std::array<bool, kButtons>, 2> m_held{};
    std::array<uint64_t, kBuckets> m_hist{};
    TimePoint m_frameStart{};
    bool m_haveFrame = false;
    int64_t m_lastFrameUs = 0;
    int64_t m_sumOffsetUs = 0, m_offsetSamples = 0;
    int64_t m_minOffsetUs = std::numeric_limits<int64_t>::max();
    int64_t m_maxOffsetUs = 0;
    uint64_t m_frames = 0, m_events = 0, m_pushes = 0, m_releases = 0;
    uint64_t m_multiEventFrames = 0, m_dupPush = 0, m_orphanRelease = 0;
    uint32_t m_eventsThisFrame = 0;
};

} // namespace moi
