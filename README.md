# MOI SUBFRAME

Geode mod for Geometry Dash (Android ARM64). **Status: v1.0.0 is a pass-through input diagnostic, not a
sub-frame click system.** Read "Why not true sub-frame" before expecting CBF behavior.

## What it does
- Hooks `GJBaseGameLayer::handleButton` and `PlayLayer::update` and **always calls the original**.
  Inputs are never delayed, reordered, injected or dropped.
- Records, with a monotonic clock, where inside a frame each button event is applied
  (mean/min/max + an 8-bucket histogram). This tells you whether events are quantized to frame/step starts.
- Flags duplicate pushes, releases without a push, and buttons still held at level exit (log only).
- ON/OFF: Geode mod settings, or the **SF: ON/OFF** button on the pause menu. Extra settings: log every event,
  report on level exit. Output goes to the Geode log.

## Why not true sub-frame (what I could and could not establish)
Real CBF needs the *time the finger touched the screen*, earlier than the frame in which the game sees it.
- Touches on Android are delivered by the Java layer to native code. In standard cocos2d-x 2.x the native touch
  entry points carry no timestamp, and by the time the game thread runs, the event has been queued to the frame
  boundary. A native Geode mod therefore sees "arrival ~ frame start", which contains no sub-frame information.
- A real timestamp would be `MotionEvent.getEventTime()` on the Java side. Geode mods are native code; I know of no
  documented Geode API to hook the Android Java view / touch listener.
- Polling faster or timestamping inside the game thread would only *look* like sub-frame (explicitly out of scope).
- These points are from knowledge of cocos2d-x/Geode, **not verified against GD's actual binary** (see Limitations).
  If you find a documented way to obtain Java-side event times, the diagnostic data from this mod is the baseline
  to compare against.

## Build
GitHub: push to `main` (or run workflow manually). Artifact "Build Output" contains the `.geode` file.
Local (needs Geode CLI + SDK): `export GEODE_SDK=...; geode build --platform android64`.
Install: copy `moi.subframe.geode` into the game's `Android/media/com.geode.launcher/game/geode/mods/`.

## Before first build: check `mod.json`
`"geode"` (loader version) and `"gd"."android"` (game version) are set to values I could not verify offline.
Set them to match `geode sdk version` and your installed GD, or the mod will not load.

## Upload with Termux
```
pkg install git unzip
unzip moi-subframe.zip && cd moi-subframe
git init -b main && git add . && git commit -m "MOI SUBFRAME"
git remote add origin https://github.com/<you>/moi-subframe.git
git push -u origin main   # use a Personal Access Token as the password
```

## Limitations / honest test report
- Verified: `src/Journal.hpp` logic via `tests/journal_test.cpp` (duplicate/orphan/held/reset), compiled with g++.
- **Not verified:** Geode build, hook signatures (`handleButton` return type, `PlayLayer::update/resetLevel/onQuit`,
  `PauseLayer::customSetup`) against current bindings, workflow action inputs, on-device behavior.
  The build environment used to write this had no network, so no SDK was available.
- Pause-menu button position is a guess; it may overlap other UI on some devices.
