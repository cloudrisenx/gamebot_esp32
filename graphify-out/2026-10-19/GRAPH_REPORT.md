# Graph Report - ESP32  (2026-10-19)

## Corpus Check
- 52 files · ~62,504 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 2 file(s) not represented in the graph (top: (none) 2)

## Summary
- 1261 nodes · 2863 edges · 51 communities (48 shown, 3 thin omitted)
- Extraction: 75% EXTRACTED · 25% INFERRED · 0% AMBIGUOUS · INFERRED: 705 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `47ee82ad`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- ble_link.cpp
- menu.cpp
- power.cpp
- ota.cpp
- ios_link.cpp
- game_duel.cpp
- game_hitung.cpp
- game_pong.cpp
- game_ttt.cpp
- game_breakout.cpp
- game_dino.cpp
- SettingItem
- settings.cpp
- clockstore.cpp
- text_screen.cpp
- Engine
- remote_hid.cpp
- phone.cpp
- pet_core.cpp
- setup
- Face
- WeatherCache
- SyncState
- Esc
- games.cpp
- Fb
- ParamId
- Mood
- Pose
- Cat
- draw
- catSprite
- PoseId
- KindDef
- Preset
- main.cpp
- Play
- lang.h
- startScreen
- power_update
- Edge
- buttons.h
- PlaySt
- .onDisconnect
- PhoneItem
- Spring
- drawWeather
- RawNotif
- GameDef
- Stat
- Rng

## God Nodes (most connected - your core abstractions)
1. `Engine` - 111 edges
2. `T()` - 53 edges
3. `Face` - 49 edges
4. `loopBody()` - 36 edges
5. `oled_flush()` - 36 edges
6. `Fb` - 34 edges
7. `activateSetting()` - 33 edges
8. `Cat` - 31 edges
9. `Pose` - 29 edges
10. `phone_update()` - 27 edges

## Surprising Connections (you probably didn't know these)
- `resumeAfterOverlay()` --calls--> `anim_resume()`  [INFERRED]
  src/main.cpp → src/anim.cpp
- `enterState()` --calls--> `ble_setNavScreen()`  [INFERRED]
  src/main.cpp → src/ble_link.cpp
- `clockEditorUpdate()` --calls--> `ble_takeSyncDone()`  [INFERRED]
  src/menu.cpp → src/ble_link.cpp
- `nav_screen_update()` --calls--> `ble_navChanged()`  [INFERRED]
  src/menu.cpp → src/ble_link.cpp
- `loopBody()` --calls--> `buttons_update()`  [INFERRED]
  src/main.cpp → src/buttons.cpp

## Import Cycles
- None detected.

## Communities (51 total, 3 thin omitted)

### Community 0 - "ble_link.cpp"
Cohesion: 0.12
Nodes (29): addHistory(), ble_address(), ble_begin(), ble_callAnswer(), ble_connected(), ble_isIphone(), ble_lastNotifInfo(), ble_musicControl() (+21 more)

### Community 1 - "menu.cpp"
Cohesion: 0.08
Nodes (63): btn_pressed(), buttons_clear(), clock_lastSync(), drawReady(), drawReady(), drawReady(), drawReady(), drawList() (+55 more)

### Community 2 - "power.cpp"
Cohesion: 0.16
Nodes (12): loop(), loopIdleMs(), power_applyContrast(), power_buttonRecentlyChanged(), power_dimmed(), power_lightSleepActive(), power_loopWait(), power_sleepPending() (+4 more)

### Community 3 - "ota.cpp"
Cohesion: 0.05
Nodes (56): anim_enter(), anim_isPlaying(), anim_resume(), anim_update(), drawFrame(), logStats(), btn_any_pressed(), crc32Update() (+48 more)

### Community 4 - "ios_link.cpp"
Cohesion: 0.06
Nodes (54): ble_markPhoneInfoChanged(), ble_pushNotification(), ble_setLocalTime(), ble_setRinging(), ble_update(), copyCStr(), onConnection(), onNotification() (+46 more)

### Community 5 - "game_duel.cpp"
Cohesion: 0.05
Nodes (52): btn_armTiming(), afterResult(), centerLine(), draw(), drawCowboy(), duel_enter(), duel_resume(), duel_update() (+44 more)

### Community 6 - "game_hitung.cpp"
Cohesion: 0.06
Nodes (53): centerLine(), DistDef, dMax, dMin, kinds, draw(), drawHalf(), drawSprite() (+45 more)

### Community 7 - "game_pong.cpp"
Cohesion: 0.05
Nodes (58): btn_any_pending(), btn_chord(), btn_doubleTap(), btn_held(), btn_pressTimeUs(), btn_tapSync(), buttons_armWake(), buttons_update() (+50 more)

### Community 8 - "game_ttt.cpp"
Cohesion: 0.16
Nodes (21): boardFull(), centerAt(), cpuMove(), cpuTurn(), cursorToEmpty(), draw(), drawMark(), drawPanel() (+13 more)

### Community 9 - "game_breakout.cpp"
Cohesion: 0.32
Nodes (10): attachBall(), breakout_enter(), breakout_update(), buildLevel(), gameOver(), hitBrick(), launch(), resetRound() (+2 more)

### Community 10 - "game_dino.cpp"
Cohesion: 0.16
Nodes (20): dino_enter(), dino_update(), draw(), drawObstacle(), gameOver(), gapFor(), obsSize(), Obstacle (+12 more)

### Community 11 - "SettingItem"
Cohesion: 0.09
Nodes (23): SettingItem, SI_BOOTANIM, SI_BT, SI_BT_RESTART, SI_CLOCK, SI_CONTRAST, SI_COUNT, SI_FLASH_MODE (+15 more)

### Community 12 - "settings.cpp"
Cohesion: 0.13
Nodes (27): ble_deletePairings(), ble_restartRadio(), activateSetting(), settingCard(), settingValue(), power_lightSleep(), power_requestSleep(), settings_bootAnim() (+19 more)

### Community 13 - "clockstore.cpp"
Cohesion: 0.32
Nodes (11): ble_epoch(), ble_setClock(), ble_setClockParts(), ble_takeTimeSynced(), clock_begin(), clock_loop(), clock_save(), clock_setManual() (+3 more)

### Community 14 - "text_screen.cpp"
Cohesion: 0.22
Nodes (15): handleTextGet(), handleTextSet(), appendUtf8(), charW(), draw(), emojiCode(), load(), rebase() (+7 more)

### Community 15 - "Engine"
Cohesion: 0.02
Nodes (79): Engine, autoPlayUntil, blink2, blinkAt, bodyProb, boredAfter, catDir, catPh (+71 more)

### Community 16 - "remote_hid.cpp"
Cohesion: 0.08
Nodes (31): ble_remoteStart(), ble_remoteStop(), ble_setSuspended(), BondSet, cccd, nCccd, nOur, nPeer (+23 more)

### Community 17 - "phone.cpp"
Cohesion: 0.11
Nodes (30): ble_historyCount(), ble_historyDelete(), ble_phoneInfoChanged(), beginScreen(), camAnimating(), camCapture(), camSaveTimer(), centerLine() (+22 more)

### Community 18 - "pet_core.cpp"
Cohesion: 0.13
Nodes (16): applyPreset(), blinkValue(), buildPoses(), Choice, id, type, clampd(), dmax() (+8 more)

### Community 19 - "setup"
Cohesion: 0.18
Nodes (11): buttons_attachWake(), buttons_begin(), setup(), F, power_begin(), power_loopWaitBegin(), power_wokeByTimer(), power_wokeFromSleep() (+3 more)

### Community 20 - "Face"
Cohesion: 0.05
Nodes (37): Face, birdDir, birdFlap, birdX, birdY, bL, bR, breath (+29 more)

### Community 21 - "WeatherCache"
Cohesion: 0.29
Nodes (6): WeatherCache, city, count, items, savedAt, time

### Community 22 - "SyncState"
Cohesion: 0.29
Nodes (7): SyncState, SYNC_ASKING, SYNC_FAIL, SYNC_IDLE, SYNC_NOCONN, SYNC_NOTIME, SYNC_OK

### Community 23 - "Esc"
Cohesion: 0.07
Nodes (29): Esc, active, back, dir, end, gone, kind, nPeeks (+21 more)

### Community 24 - "games.cpp"
Cohesion: 0.15
Nodes (7): draw(), games_centerText(), games_drawOver(), games_drawSidePanels(), games_isPlaying(), panelDots(), printShort()

### Community 25 - "Fb"
Cohesion: 0.18
Nodes (25): birdDraw(), blitScaled(), drawFace(), drawPeek(), dustDraw(), earsDraw(), eyeDraw(), faceDraw() (+17 more)

### Community 26 - "ParamId"
Cohesion: 0.08
Nodes (26): ParamId, NPARAM, P_AMP, P_BLUSH, P_CURVE, P_EARL, P_EARR, P_FY (+18 more)

### Community 27 - "Mood"
Cohesion: 0.08
Nodes (24): Ev, EV_BIRD, EV_BLEP, EV_GROOM, EV_KEPO, EV_NONE, EV_SLOWBLINK, EV_SNIFF (+16 more)

### Community 28 - "Pose"
Cohesion: 0.08
Nodes (24): Pose, arch, bob, crouch, ear, eyesClosed, gaitAmp, gaitLift (+16 more)

### Community 29 - "Cat"
Cohesion: 0.09
Nodes (22): Cat, edge, flip, ground, hasToy, look, mode, nzs (+14 more)

### Community 30 - "draw"
Cohesion: 0.20
Nodes (18): ble_date(), ble_timeStr(), ble_unreadCount(), ble_weatherCacheAgeH(), clock_source(), draw(), homeSignature(), header_signature() (+10 more)

### Community 31 - "catSprite"
Cohesion: 0.27
Nodes (5): catSitting(), catSprite(), iabs(), mouseSprite(), peekSprite()

### Community 32 - "PoseId"
Cohesion: 0.12
Nodes (17): PoseId, NPOSE, PS_ALERT, PS_ARCH, PS_CATCH, PS_DIZZY, PS_FREEZE, PS_LOAF (+9 more)

### Community 33 - "KindDef"
Cohesion: 0.12
Nodes (16): KindDef, awayMax, awayMin, back, backRun, dur, fade, happy (+8 more)

### Community 34 - "Preset"
Cohesion: 0.12
Nodes (16): Preset, PR_BIRD, PR_BOSAN, PR_GELI, PR_GROOM, PR_KENYANG, PR_KESAL, PR_MAIN (+8 more)

### Community 36 - "Play"
Cohesion: 0.14
Nodes (14): Play, hidden, mdir, mpause, mspeed, mtarget, mv, mx (+6 more)

### Community 37 - "lang.h"
Cohesion: 0.15
Nodes (9): CatMode, CM_DOTS, CM_FADE, CM_MORPH, CM_NONE, CM_PEEK, CM_PEEKEDGE, CM_PLAY (+1 more)

### Community 38 - "startScreen"
Cohesion: 0.17
Nodes (12): ble_markRead(), camLoadTimer(), phone_enter(), PhoneScreen, PS_CAMERA, PS_LIST, PS_MUSIC, PS_NOTIF_DETAIL (+4 more)

### Community 39 - "power_update"
Cohesion: 0.22
Nodes (10): ble_shutdown(), enterDeepSleep(), power_nightMinutesLeft(), power_screenOffNow(), power_update(), setScreen(), settings_nightEndMin(), settings_nightOn() (+2 more)

### Community 40 - "Edge"
Cohesion: 0.18
Nodes (11): Edge, EDGE_ATAS, EDGE_BAWAH, EDGE_KANAN, EDGE_KIRI, Peek, dur, edge (+3 more)

### Community 41 - "buttons.h"
Cohesion: 0.22
Nodes (3): colonOn(), drawClock(), drawInfoText()

### Community 42 - "PlaySt"
Cohesion: 0.20
Nodes (10): PlaySt, W_CATCH, W_CHASE, W_FREEZE, W_MISS, W_POUNCE, W_REST, W_STALK (+2 more)

### Community 44 - "PhoneItem"
Cohesion: 0.29
Nodes (7): PhoneItem, ITEM_COUNT, PI_CAMERA, PI_MODE, PI_MUSIC, PI_NOTIF, PI_WEATHER

### Community 45 - "Spring"
Cohesion: 0.33
Nodes (6): Spring, c, k, t, v, x

### Community 46 - "drawWeather"
Cohesion: 0.70
Nodes (5): ble_weatherCity(), ble_weatherCount(), ble_weatherIsCache(), ble_weatherTime(), drawWeather()

### Community 47 - "RawNotif"
Cohesion: 0.40
Nodes (5): RawNotif, app, icon, msg, title

### Community 48 - "GameDef"
Cohesion: 0.40
Nodes (4): GameDef, id, name, portrait

### Community 49 - "Stat"
Cohesion: 0.50
Nodes (4): Stat, ST_BLINK, ST_GAZE, ST_MAINSENDIRI

## Knowledge Gaps
- **436 isolated node(s):** `icon`, `app`, `title`, `msg`, `savedAt` (+431 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 508 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Engine` connect `Engine` to `Play`, `lang.h`, `Spring`, `pet_core.cpp`, `setup`, `Face`, `Rng`, `Esc`, `Mood`, `Pose`?**
  _High betweenness centrality (0.146) - this node is a cross-community bridge._
- **Are the 52 inferred relationships involving `T()` (e.g. with `anim_update()` and `ble_lastNotifInfo()`) actually correct?**
  _`T()` has 52 INFERRED edges - model-reasoned connections that need verification._
- **What connects `icon`, `app`, `title` to the rest of the system?**
  _436 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ble_link.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.125 - nodes in this community are weakly interconnected._
- **Why does `setup()` connect `setup` to `ble_link.cpp`, `menu.cpp`, `power.cpp`, `ota.cpp`, `main.cpp`, `power_update`, `game_pong.cpp`, `settings.cpp`, `clockstore.cpp`?**
  _High betweenness centrality (0.069) - this node is a cross-community bridge._
- **Are the 29 inferred relationships involving `loopBody()` (e.g. with `anim_isPlaying()` and `anim_update()`) actually correct?**
  _`loopBody()` has 29 INFERRED edges - model-reasoned connections that need verification._
- **Should `menu.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.08105263157894736 - nodes in this community are weakly interconnected._