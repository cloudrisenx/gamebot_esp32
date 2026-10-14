# Graph Report - ESP32  (2026-10-10)

## Corpus Check
- 46 files · ~42,981 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 2 file(s) not represented in the graph (top: (none) 2)

## Summary
- 754 nodes · 1886 edges · 23 communities
- Extraction: 74% EXTRACTED · 26% INFERRED · 0% AMBIGUOUS · INFERRED: 499 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `1c3dbdef`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- ble_link.cpp
- menu.cpp
- settings.cpp
- frames_store.cpp
- ios_link.cpp
- game_duel.cpp
- game_hitung.cpp
- game_pong.cpp
- game_ttt.cpp
- game_breakout.cpp
- game_dino.cpp
- SettingItem
- games.cpp
- clockstore.cpp
- GameDef
- game_flappy.cpp
- remote_hid.cpp
- phone.cpp
- settings_hiscore
- TttMode
- ota.cpp
- WeatherCache
- SyncState

## God Nodes (most connected - your core abstractions)
1. `oled_flush()` - 33 edges
2. `loopBody()` - 32 edges
3. `activateSetting()` - 29 edges
4. `settings_hiscore()` - 25 edges
5. `btn_pressed()` - 24 edges
6. `hitung_update()` - 24 edges
7. `phone_update()` - 24 edges
8. `settings_screen_update()` - 23 edges
9. `SettingItem` - 20 edges
10. `setup()` - 19 edges

## Surprising Connections (you probably didn't know these)
- `resumeAfterOverlay()` --calls--> `anim_resume()`  [INFERRED]
  src/main.cpp → src/anim.cpp
- `nav_screen_update()` --calls--> `ble_navChanged()`  [INFERRED]
  src/menu.cpp → src/ble_link.cpp
- `setup()` --calls--> `buttons_begin()`  [INFERRED]
  src/main.cpp → src/buttons.cpp
- `loopBody()` --calls--> `buttons_update()`  [INFERRED]
  src/main.cpp → src/buttons.cpp
- `loopBody()` --calls--> `btn_any_pending()`  [INFERRED]
  src/main.cpp → src/buttons.cpp

## Import Cycles
- None detected.

## Communities (23 total, 0 thin omitted)

### Community 0 - "ble_link.cpp"
Cohesion: 0.06
Nodes (67): addHistory(), ble_address(), ble_begin(), ble_callAnswer(), ble_connected(), ble_date(), ble_isIphone(), ble_lastNotifInfo() (+59 more)

### Community 1 - "menu.cpp"
Cohesion: 0.11
Nodes (49): ble_takeSyncDone(), btn_pressed(), buttons_clear(), clock_lastSync(), drawList(), games_update(), home_update(), handleCall() (+41 more)

### Community 2 - "settings.cpp"
Cohesion: 0.06
Nodes (57): buttons_attachWake(), loop(), loopIdleMs(), setup(), activateSetting(), settingCard(), settingValue(), clockMinutes() (+49 more)

### Community 3 - "frames_store.cpp"
Cohesion: 0.09
Nodes (37): anim_enter(), anim_isPlaying(), anim_resume(), anim_update(), drawFrame(), logStats(), ble_setNavScreen(), btn_any_pressed() (+29 more)

### Community 4 - "ios_link.cpp"
Cohesion: 0.07
Nodes (40): ble_deletePairings(), ble_markPhoneInfoChanged(), ble_setLocalTime(), ble_setRinging(), ams_parsePlaybackInfo(), AmsUpdate, attr, entity (+32 more)

### Community 5 - "game_duel.cpp"
Cohesion: 0.06
Nodes (41): btn_any_pending(), btn_armTiming(), btn_chord(), btn_doubleTap(), btn_pressTimeUs(), btn_tapSync(), buttons_begin(), buttons_update() (+33 more)

### Community 6 - "game_hitung.cpp"
Cohesion: 0.06
Nodes (54): centerLine(), DistDef, dMax, dMin, kinds, draw(), drawHalf(), drawReady() (+46 more)

### Community 7 - "game_pong.cpp"
Cohesion: 0.07
Nodes (52): btn_held(), bounce(), centerText(), draw(), draw1P(), draw2P(), drawOver2P(), drawPaddle() (+44 more)

### Community 8 - "game_ttt.cpp"
Cohesion: 0.20
Nodes (16): centerAt(), cpuMove(), cpuTurn(), cursorToEmpty(), draw(), drawMark(), drawPanel(), findLine() (+8 more)

### Community 9 - "game_breakout.cpp"
Cohesion: 0.32
Nodes (10): attachBall(), breakout_enter(), breakout_update(), buildLevel(), gameOver(), hitBrick(), launch(), resetRound() (+2 more)

### Community 10 - "game_dino.cpp"
Cohesion: 0.16
Nodes (20): dino_enter(), dino_update(), draw(), drawObstacle(), gameOver(), gapFor(), obsSize(), Obstacle (+12 more)

### Community 11 - "SettingItem"
Cohesion: 0.10
Nodes (20): SettingItem, SI_BOOTANIM, SI_BT, SI_BT_RESTART, SI_CLOCK, SI_CONTRAST, SI_COUNT, SI_GUIDE (+12 more)

### Community 12 - "games.cpp"
Cohesion: 0.19
Nodes (4): games_drawSidePanels(), games_isPlaying(), panelDots(), printShort()

### Community 13 - "clockstore.cpp"
Cohesion: 0.32
Nodes (11): ble_epoch(), ble_setClock(), ble_setClockParts(), ble_takeTimeSynced(), clock_begin(), clock_loop(), clock_save(), clock_setManual() (+3 more)

### Community 14 - "GameDef"
Cohesion: 0.40
Nodes (4): GameDef, id, name, portrait

### Community 15 - "game_flappy.cpp"
Cohesion: 0.18
Nodes (15): draw(), drawPipe(), flappy_enter(), flappy_update(), gameOver(), gapHeight(), Pipe, gapH (+7 more)

### Community 16 - "remote_hid.cpp"
Cohesion: 0.09
Nodes (24): ble_remoteStop(), BondSet, cccd, nCccd, nOur, nPeer, our, peer (+16 more)

### Community 17 - "phone.cpp"
Cohesion: 0.07
Nodes (43): ble_historyCount(), ble_historyDelete(), ble_markRead(), ble_phoneInfoChanged(), beginScreen(), camAnimating(), camCapture(), camLoadTimer() (+35 more)

### Community 18 - "settings_hiscore"
Cohesion: 0.22
Nodes (11): draw(), drawReady(), saveHi(), boardFull(), drawReady(), place(), games_centerText(), games_drawOver() (+3 more)

### Community 19 - "TttMode"
Cohesion: 0.67
Nodes (3): TttMode, MODE_2P, MODE_CPU

### Community 20 - "ota.cpp"
Cohesion: 0.07
Nodes (17): colonOn(), drawClock(), drawInfoText(), markAppValid(), draw(), drawQr(), drawStatus(), handleFirmwareDone() (+9 more)

### Community 21 - "WeatherCache"
Cohesion: 0.29
Nodes (6): WeatherCache, city, count, items, savedAt, time

### Community 22 - "SyncState"
Cohesion: 0.29
Nodes (7): SyncState, SYNC_ASKING, SYNC_FAIL, SYNC_IDLE, SYNC_NOCONN, SYNC_NOTIME, SYNC_OK

## Knowledge Gaps
- **97 isolated node(s):** `icon`, `app`, `title`, `msg`, `savedAt` (+92 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 158 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SettingItem` connect `SettingItem` to `menu.cpp`?**
  _High betweenness centrality (0.050) - this node is a cross-community bridge._
- **Are the 32 inferred relationships involving `oled_flush()` (e.g. with `anim_update()` and `drawFrame()`) actually correct?**
  _`oled_flush()` has 32 INFERRED edges - model-reasoned connections that need verification._
- **What connects `icon`, `app`, `title` to the rest of the system?**
  _97 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ble_link.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06164383561643835 - nodes in this community are weakly interconnected._
- **Why does `oled_flush()` connect `menu.cpp` to `ble_link.cpp`, `settings.cpp`, `frames_store.cpp`, `game_duel.cpp`, `game_hitung.cpp`, `game_pong.cpp`, `game_ttt.cpp`, `game_dino.cpp`, `game_flappy.cpp`, `phone.cpp`, `settings_hiscore`, `ota.cpp`?**
  _High betweenness centrality (0.034) - this node is a cross-community bridge._
- **Are the 25 inferred relationships involving `loopBody()` (e.g. with `anim_isPlaying()` and `anim_update()`) actually correct?**
  _`loopBody()` has 25 INFERRED edges - model-reasoned connections that need verification._
- **Should `menu.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.10828796128251664 - nodes in this community are weakly interconnected._