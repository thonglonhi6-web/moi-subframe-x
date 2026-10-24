// MOI SUBFRAME - input timing diagnostics (pass-through).
// Every hook below calls the original function unchanged. Nothing is delayed,
// reordered, injected or dropped.
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "Journal.hpp"

using namespace geode::prelude;

namespace {
    moi::Journal& journal() {
        static moi::Journal j;
        return j;
    }
    bool diagOn() { return Mod::get()->getSettingValue<bool>("diagnostics"); }
    char const* label(bool on) { return on ? "SF: ON" : "SF: OFF"; }
}

class $modify(MoiBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (diagOn()) {
            auto a = journal().onButton(down, button, isPlayer1, moi::Clock::now());
            if (a == moi::Anomaly::DuplicatePush) {
                log::warn("[MOI SUBFRAME] duplicate push (button {}, p1={})", button, isPlayer1);
            } else if (a == moi::Anomaly::OrphanRelease) {
                log::warn("[MOI SUBFRAME] release without push (button {}, p1={})", button, isPlayer1);
            }
            if (Mod::get()->getSettingValue<bool>("log-each-event")) {
                log::info("[MOI SUBFRAME] {} button={} p1={}", down ? "push" : "release", button, isPlayer1);
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

class $modify(MoiPlayLayer, PlayLayer) {
    void update(float dt) {
        if (diagOn()) journal().frameBegin(moi::Clock::now());
        PlayLayer::update(dt);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        journal().clearHeld();
    }

    void onQuit() {
        if (diagOn() && Mod::get()->getSettingValue<bool>("report-on-quit")) {
            log::info("{}", journal().summary());
        }
        journal().reset();
        PlayLayer::onQuit();
    }
};

class $modify(MoiPauseLayer, PauseLayer) {
    struct Fields { ButtonSprite* spr = nullptr; };

    void customSetup() {
        PauseLayer::customSetup();
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        auto spr = ButtonSprite::create(label(diagOn()));
        m_fields->spr = spr;
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MoiPauseLayer::onMoiToggle));
        btn->setPosition({win.width - 55.f, 25.f});
        menu->addChild(btn);
        this->addChild(menu, 10);
    }

    void onMoiToggle(CCObject*) {
        bool v = !diagOn();
        Mod::get()->setSettingValue<bool>("diagnostics", v);
        if (m_fields->spr) m_fields->spr->setString(label(v));
        if (!v) journal().reset();
    }
};
