# Graph Report - ESP32  (2026-10-19)

## Corpus Check
- 56 files · ~63,395 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 2 file(s) not represented in the graph (top: (none) 2)

## Summary
- 1283 nodes · 2934 edges · 65 communities (63 shown, 2 thin omitted)
- Extraction: 75% EXTRACTED · 25% INFERRED · 0% AMBIGUOUS · INFERRED: 732 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `47ee82ad`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- ble_link.cpp
- loopBody
- power.cpp
- frames_store.cpp
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
- nav_screen_update
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
- T
- PoseId
- KindDef
- Preset
- ota.cpp
- Play
- CatMode
- startScreen
- anim_update
- pet_core.h
- menu.cpp
- PlaySt
- dlog
- PhoneItem
- Spring
- game_flappy.cpp
- RawNotif
- GameDef
- screen.cpp
- Rng
- buttons.cpp
- ble_connected
- phone.cpp
- BondSet
- carousel_draw
- Kind
- Verdict
- ota_enter
- ota_holdMenu
- debuglog.h
- startAdvertising
- AmsUpdate
- remote_connected
- ble_address

## God Nodes (most connected - your core abstractions)
1. `Engine` - 111 edges
2. `T()` - 54 edges
3. `Face` - 49 edges
4. `loopBody()` - 38 edges
5. `oled_flush()` - 38 edges
6. `activateSetting()` - 35 edges
7. `Fb` - 34 edges
8. `Cat` - 31 edges
9. `Pose` - 29 edges
10. `settings_screen_update()` - 28 edges

## Surprising Connections (you probably didn't know these)
- `resumeAfterOverlay()` --calls--> `anim_resume()`  [INFERRED]
  src/main.cpp → src/anim.cpp
- `nav_screen_update()` --calls--> `ble_navChanged()`  [INFERRED]
  src/menu.cpp → src/ble_link.cpp
- `phone_update()` --calls--> `ble_historyDelete()`  [INFERRED]
  src/phone.cpp → src/ble_link.cpp
- `phone_update()` --calls--> `ble_phoneInfoChanged()`  [INFERRED]
  src/phone.cpp → src/ble_link.cpp
- `setup()` --calls--> `buttons_begin()`  [INFERRED]
  src/main.cpp → src/buttons.cpp

## Import Cycles
- None detected.

## Communities (65 total, 2 thin omitted)

### Community 0 - "ble_link.cpp"
Cohesion: 0.10
Nodes (33): addHistory(), ble_begin(), ble_deletePairings(), ble_historyDelete(), ble_navChanged(), ble_phoneInfoChanged(), ble_pushNotification(), ble_remoteStart() (+25 more)

### Community 1 - "loopBody"
Cohesion: 0.18
Nodes (21): btn_pressed(), buttons_clear(), pong_resume(), games_resume(), games_update(), home_update(), handleCall(), handlePopup() (+13 more)

### Community 2 - "power.cpp"
Cohesion: 0.12
Nodes (17): loop(), loopIdleMs(), enterDeepSleep(), power_activity(), power_applyContrast(), power_buttonRecentlyChanged(), power_lightSleepActive(), power_loopWait() (+9 more)

### Community 3 - "frames_store.cpp"
Cohesion: 0.13
Nodes (24): crc32Update(), dataLen(), dataPtr(), decodeFrame(), findPartition(), frameOffset(), frames_begin(), frames_get() (+16 more)

### Community 4 - "ios_link.cpp"
Cohesion: 0.09
Nodes (37): ble_markPhoneInfoChanged(), ble_setLocalTime(), onConnection(), ams_parsePlaybackInfo(), ancs_parseAttributes(), ancs_parseNotificationSource(), appLabel(), applyTime() (+29 more)

### Community 5 - "game_duel.cpp"
Cohesion: 0.10
Nodes (27): btn_armTiming(), afterResult(), centerLine(), drawCowboy(), duel_enter(), duel_resume(), duel_update(), DuelMode (+19 more)

### Community 6 - "game_hitung.cpp"
Cohesion: 0.06
Nodes (53): launch(), centerLine(), DistDef, dMax, dMin, kinds, draw(), drawHalf() (+45 more)

### Community 7 - "game_pong.cpp"
Cohesion: 0.11
Nodes (29): btn_held(), btn_tapSync(), bounce(), draw(), launch(), loadMode(), onHit(), point2P() (+21 more)

### Community 8 - "game_ttt.cpp"
Cohesion: 0.16
Nodes (21): boardFull(), centerAt(), cpuMove(), cpuTurn(), cursorToEmpty(), draw(), drawMark(), drawPanel() (+13 more)

### Community 9 - "game_breakout.cpp"
Cohesion: 0.38
Nodes (7): attachBall(), breakout_enter(), buildLevel(), hitBrick(), resetRound(), setSpeed(), step()

### Community 10 - "game_dino.cpp"
Cohesion: 0.14
Nodes (17): dino_enter(), drawObstacle(), gapFor(), obsSize(), Obstacle, on, type, x (+9 more)

### Community 11 - "SettingItem"
Cohesion: 0.08
Nodes (24): SettingItem, SI_BOOTANIM, SI_BT, SI_BT_RESTART, SI_CLOCK, SI_CONTRAST, SI_COUNT, SI_DEBUG (+16 more)

### Community 12 - "settings.cpp"
Cohesion: 0.13
Nodes (28): activateSetting(), settingCard(), settingValue(), settings_bootAnim(), settings_bt(), settings_contrastIdx(), settings_hitungLv5Name(), settings_i2cHz() (+20 more)

### Community 13 - "clockstore.cpp"
Cohesion: 0.33
Nodes (10): ble_epoch(), ble_setClock(), ble_setClockParts(), ble_takeTimeSynced(), clock_begin(), clock_loop(), clock_save(), clock_setManual() (+2 more)

### Community 14 - "text_screen.cpp"
Cohesion: 0.20
Nodes (15): handleTextGet(), handleTextSet(), appendUtf8(), charW(), draw(), emojiCode(), load(), rebase() (+7 more)

### Community 15 - "Engine"
Cohesion: 0.02
Nodes (80): Engine, autoPlayUntil, blink2, blinkAt, bodyProb, boredAfter, catDir, catPh (+72 more)

### Community 16 - "remote_hid.cpp"
Cohesion: 0.21
Nodes (13): ble_remoteStop(), clearStore(), inHid(), nvsLoad(), nvsSave(), readStore(), remote_begin(), remote_clearBonds() (+5 more)

### Community 17 - "nav_screen_update"
Cohesion: 0.13
Nodes (23): ble_historyCount(), asciiOnly(), draw_androidOnly(), drawHeader(), drawWrapped(), drawWrappedAscii(), nav_screen_update(), printRight() (+15 more)

### Community 18 - "pet_core.cpp"
Cohesion: 0.13
Nodes (17): applyPreset(), blinkValue(), buildPoses(), clampd(), dmax(), dmin(), drawFace(), ensureInit() (+9 more)

### Community 19 - "setup"
Cohesion: 0.22
Nodes (9): buttons_attachWake(), setup(), power_begin(), power_loopWaitBegin(), power_screenOffNow(), power_wokeByTimer(), power_wokeFromSleep(), oled_begin() (+1 more)

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
Cohesion: 0.11
Nodes (19): Esc, active, back, dir, end, gone, kind, nPeeks (+11 more)

### Community 24 - "games.cpp"
Cohesion: 0.23
Nodes (4): games_drawSidePanels(), games_isPlaying(), panelDots(), printShort()

### Community 25 - "Fb"
Cohesion: 0.13
Nodes (30): birdDraw(), blitScaled(), catSitting(), catSprite(), drawPeek(), dustDraw(), earsDraw(), eyeDraw() (+22 more)

### Community 26 - "ParamId"
Cohesion: 0.08
Nodes (26): ParamId, NPARAM, P_AMP, P_BLUSH, P_CURVE, P_EARL, P_EARR, P_FY (+18 more)

### Community 27 - "Mood"
Cohesion: 0.07
Nodes (28): Choice, id, type, Ev, EV_BIRD, EV_BLEP, EV_GROOM, EV_KEPO (+20 more)

### Community 28 - "Pose"
Cohesion: 0.08
Nodes (24): Pose, arch, bob, crouch, ear, eyesClosed, gaitAmp, gaitLift (+16 more)

### Community 29 - "Cat"
Cohesion: 0.09
Nodes (22): Cat, edge, flip, ground, hasToy, look, mode, nzs (+14 more)

### Community 30 - "draw"
Cohesion: 0.14
Nodes (26): ble_date(), ble_timeStr(), ble_unreadCount(), ble_weatherCacheAgeH(), ble_weatherCity(), ble_weatherCount(), ble_weatherIsCache(), ble_weatherTime() (+18 more)

### Community 31 - "T"
Cohesion: 0.18
Nodes (26): breakout_update(), draw(), gameOver(), dino_update(), draw(), gameOver(), draw(), drawReady() (+18 more)

### Community 32 - "PoseId"
Cohesion: 0.12
Nodes (17): PoseId, NPOSE, PS_ALERT, PS_ARCH, PS_CATCH, PS_DIZZY, PS_FREEZE, PS_LOAF (+9 more)

### Community 33 - "KindDef"
Cohesion: 0.12
Nodes (16): KindDef, awayMax, awayMin, back, backRun, dur, fade, happy (+8 more)

### Community 34 - "Preset"
Cohesion: 0.12
Nodes (16): Preset, PR_BIRD, PR_BOSAN, PR_GELI, PR_GROOM, PR_KENYANG, PR_KESAL, PR_MAIN (+8 more)

### Community 35 - "ota.cpp"
Cohesion: 0.09
Nodes (9): anim_enter(), anim_resume(), logStats(), markAppValid(), drawQr(), ota_holdMenuReset(), pet_keepAwake(), pet_notify() (+1 more)

### Community 36 - "Play"
Cohesion: 0.14
Nodes (14): Play, hidden, mdir, mpause, mspeed, mtarget, mv, mx (+6 more)

### Community 37 - "CatMode"
Cohesion: 0.22
Nodes (9): CatMode, CM_DOTS, CM_FADE, CM_MORPH, CM_NONE, CM_PEEK, CM_PEEKEDGE, CM_PLAY (+1 more)

### Community 38 - "startScreen"
Cohesion: 0.17
Nodes (12): ble_markRead(), camLoadTimer(), phone_enter(), PhoneScreen, PS_CAMERA, PS_LIST, PS_MUSIC, PS_NOTIF_DETAIL (+4 more)

### Community 39 - "anim_update"
Cohesion: 0.12
Nodes (24): anim_isPlaying(), anim_update(), drawFrame(), ble_lastNotifInfo(), ble_setNavScreen(), frames_count(), frames_drawFrame(), frames_fps() (+16 more)

### Community 40 - "pet_core.h"
Cohesion: 0.15
Nodes (11): Edge, EDGE_ATAS, EDGE_BAWAH, EDGE_KANAN, EDGE_KIRI, Peek, dur, edge (+3 more)

### Community 41 - "menu.cpp"
Cohesion: 0.15
Nodes (17): ble_takeSyncDone(), clock_lastSync(), clock_sourceName(), dlog_count(), clockEditorUpdate(), clockStep(), drawClockEditor(), drawFullMessage() (+9 more)

### Community 42 - "PlaySt"
Cohesion: 0.20
Nodes (10): PlaySt, W_CATCH, W_CHASE, W_FREEZE, W_MISS, W_POUNCE, W_REST, W_STALK (+2 more)

### Community 43 - "dlog"
Cohesion: 0.23
Nodes (3): dlog(), IosServerCallbacks, RemoteCallbacks

### Community 44 - "PhoneItem"
Cohesion: 0.29
Nodes (7): PhoneItem, ITEM_COUNT, PI_CAMERA, PI_MODE, PI_MUSIC, PI_NOTIF, PI_WEATHER

### Community 45 - "Spring"
Cohesion: 0.33
Nodes (6): Spring, c, k, t, v, x

### Community 46 - "game_flappy.cpp"
Cohesion: 0.18
Nodes (15): draw(), drawPipe(), flappy_enter(), flappy_update(), gameOver(), gapHeight(), Pipe, gapH (+7 more)

### Community 47 - "RawNotif"
Cohesion: 0.40
Nodes (5): RawNotif, app, icon, msg, title

### Community 48 - "GameDef"
Cohesion: 0.40
Nodes (4): GameDef, id, name, portrait

### Community 49 - "screen.cpp"
Cohesion: 0.20
Nodes (13): drawOledTest(), oled_applyClock(), oled_clockHz(), oled_errors(), oled_frames(), oled_lastUs(), oled_maxUs(), oled_reinit() (+5 more)

### Community 51 - "buttons.cpp"
Cohesion: 0.15
Nodes (9): btn_any_pending(), btn_any_pressed(), btn_chord(), btn_doubleTap(), btn_pressTimeUs(), buttons_armWake(), buttons_begin(), buttons_update() (+1 more)

### Community 52 - "ble_connected"
Cohesion: 0.21
Nodes (14): ble_callAnswer(), ble_connected(), ble_isIphone(), ble_musicControl(), ble_musicProgress(), ble_navActive(), ble_phoneBattery(), ble_requestTimeSync() (+6 more)

### Community 53 - "phone.cpp"
Cohesion: 0.18
Nodes (9): camAnimating(), camSaveTimer(), drawControls(), drawEqualizer(), drawIconGray(), feedback(), musicInput(), phone_requestStart() (+1 more)

### Community 54 - "BondSet"
Cohesion: 0.20
Nodes (8): BondSet, cccd, nCccd, nOur, nPeer, our, peer, version

### Community 55 - "carousel_draw"
Cohesion: 0.31
Nodes (8): card_draw(), carousel_draw(), carousel_frameDue(), carouselFrame(), pet_update(), present(), musicAnimating(), power_screenOn()

### Community 56 - "Kind"
Cohesion: 0.20
Nodes (10): Kind, K_COUNT, K_EKOR, K_GARUK, K_JALAN, K_KABUR, K_LOAF, K_NGUMPET (+2 more)

### Community 57 - "Verdict"
Cohesion: 0.22
Nodes (9): Verdict, kind, us, who, VerdictKind, V_FOUL, V_NONE, V_TIE (+1 more)

### Community 58 - "ota_enter"
Cohesion: 0.32
Nodes (7): handleDeviceSet(), ota_bgStart(), ota_enter(), asciiOk(), settings_otaPassword(), settings_otaSsid(), settings_setNames()

### Community 59 - "ota_holdMenu"
Cohesion: 0.33
Nodes (6): drawHoldMenu(), ota_bgPoll(), ota_bgRunning(), ota_bgStop(), ota_holdMenu(), otaExit()

### Community 60 - "debuglog.h"
Cohesion: 0.33
Nodes (3): dlog_clear(), dlog_line(), dlog_seq()

### Community 61 - "startAdvertising"
Cohesion: 0.40
Nodes (5): remote_update(), remoteAddress(), remoteName(), startAdvertising(), settings_deviceName()

### Community 62 - "AmsUpdate"
Cohesion: 0.50
Nodes (4): AmsUpdate, attr, entity, value

### Community 63 - "remote_connected"
Cohesion: 0.67
Nodes (4): camCapture(), remote_connected(), remote_volumeUp(), sendConsumer()

## Knowledge Gaps
- **438 isolated node(s):** `icon`, `app`, `title`, `msg`, `savedAt` (+433 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 513 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Engine` connect `Engine` to `Play`, `pet_core.h`, `Spring`, `pet_core.cpp`, `Rng`, `Face`, `Esc`, `Mood`, `Pose`?**
  _High betweenness centrality (0.108) - this node is a cross-community bridge._
- **Are the 53 inferred relationships involving `T()` (e.g. with `anim_update()` and `ble_lastNotifInfo()`) actually correct?**
  _`T()` has 53 INFERRED edges - model-reasoned connections that need verification._
- **What connects `icon`, `app`, `title` to the rest of the system?**
  _438 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ble_link.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.0990990990990991 - nodes in this community are weakly interconnected._
- **Why does `ParamId` connect `ParamId` to `pet_core.h`?**
  _High betweenness centrality (0.069) - this node is a cross-community bridge._
- **Are the 31 inferred relationships involving `loopBody()` (e.g. with `anim_isPlaying()` and `anim_update()`) actually correct?**
  _`loopBody()` has 31 INFERRED edges - model-reasoned connections that need verification._
- **Should `power.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.12333333333333334 - nodes in this community are weakly interconnected._