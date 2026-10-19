# Graph Report - ESP32  (2026-10-19)

## Corpus Check
- 50 files · ~49,332 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 2 file(s) not represented in the graph (top: (none) 2)

## Summary
- 823 nodes · 2146 edges · 29 communities
- Extraction: 72% EXTRACTED · 28% INFERRED · 0% AMBIGUOUS · INFERRED: 606 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `788cfd92`
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
- main.cpp
- game_flappy.cpp
- remote_hid.cpp
- phone.cpp
- buttons.cpp
- setup
- settings_hiscore
- WeatherCache
- SyncState
- power_lightSleep
- games_drawOver
- Verdict
- Pipe
- RoundState
- DuelMode

## God Nodes (most connected - your core abstractions)
1. `T()` - 53 edges
2. `loopBody()` - 36 edges
3. `oled_flush()` - 36 edges
4. `activateSetting()` - 33 edges
5. `phone_update()` - 27 edges
6. `btn_pressed()` - 26 edges
7. `settings_hiscore()` - 25 edges
8. `hitung_update()` - 24 edges
9. `settings_screen_update()` - 24 edges
10. `SettingItem` - 23 edges

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

## Communities (29 total, 0 thin omitted)

### Community 0 - "ble_link.cpp"
Cohesion: 0.05
Nodes (76): addHistory(), ble_address(), ble_begin(), ble_callAnswer(), ble_connected(), ble_date(), ble_deletePairings(), ble_historyDelete() (+68 more)

### Community 1 - "menu.cpp"
Cohesion: 0.08
Nodes (63): ble_lastNotifInfo(), ble_takeSyncDone(), btn_held(), btn_pressed(), buttons_clear(), clock_lastSync(), drawList(), games_update() (+55 more)

### Community 2 - "power.cpp"
Cohesion: 0.12
Nodes (22): loop(), loopIdleMs(), enterDeepSleep(), power_activity(), power_applyContrast(), power_buttonRecentlyChanged(), power_dimmed(), power_loopWait() (+14 more)

### Community 3 - "ota.cpp"
Cohesion: 0.05
Nodes (56): anim_enter(), anim_isPlaying(), anim_resume(), anim_update(), drawFrame(), logStats(), ble_setNavScreen(), btn_any_pressed() (+48 more)

### Community 4 - "ios_link.cpp"
Cohesion: 0.06
Nodes (41): ble_markPhoneInfoChanged(), ble_setLocalTime(), ble_setRinging(), ams_parsePlaybackInfo(), AmsUpdate, attr, entity, value (+33 more)

### Community 5 - "game_duel.cpp"
Cohesion: 0.18
Nodes (17): btn_armTiming(), centerLine(), draw(), drawCowboy(), duel_enter(), duel_resume(), duel_update(), endRound() (+9 more)

### Community 6 - "game_hitung.cpp"
Cohesion: 0.06
Nodes (54): centerLine(), DistDef, dMax, dMin, kinds, draw(), drawHalf(), drawReady() (+46 more)

### Community 7 - "game_pong.cpp"
Cohesion: 0.06
Nodes (51): btn_chord(), btn_tapSync(), bounce(), centerText(), draw(), draw1P(), draw2P(), drawOver2P() (+43 more)

### Community 8 - "game_ttt.cpp"
Cohesion: 0.15
Nodes (22): boardFull(), centerAt(), cpuMove(), cpuTurn(), cursorToEmpty(), draw(), drawMark(), drawPanel() (+14 more)

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
Cohesion: 0.19
Nodes (20): activateSetting(), settingCard(), settingValue(), settings_bt(), settings_contrastIdx(), settings_i2cHz(), settings_i2cIdx(), settings_lang() (+12 more)

### Community 13 - "clockstore.cpp"
Cohesion: 0.32
Nodes (11): ble_epoch(), ble_setClock(), ble_setClockParts(), ble_takeTimeSynced(), clock_begin(), clock_loop(), clock_save(), clock_setManual() (+3 more)

### Community 14 - "main.cpp"
Cohesion: 0.05
Nodes (28): GameDef, id, name, portrait, games_drawSidePanels(), games_isPlaying(), panelDots(), printShort() (+20 more)

### Community 15 - "game_flappy.cpp"
Cohesion: 0.30
Nodes (9): draw(), drawPipe(), flappy_enter(), flappy_update(), gameOver(), gapHeight(), resetRound(), spawnPipe() (+1 more)

### Community 16 - "remote_hid.cpp"
Cohesion: 0.08
Nodes (30): ble_remoteStop(), camCapture(), BondSet, cccd, nCccd, nOur, nPeer, our (+22 more)

### Community 17 - "phone.cpp"
Cohesion: 0.06
Nodes (44): ble_historyCount(), ble_markRead(), beginScreen(), camAnimating(), camLoadTimer(), camSaveTimer(), centerLine(), draw() (+36 more)

### Community 18 - "buttons.cpp"
Cohesion: 0.20
Nodes (5): btn_any_pending(), btn_doubleTap(), btn_pressTimeUs(), buttons_begin(), buttons_update()

### Community 19 - "setup"
Cohesion: 0.22
Nodes (9): buttons_attachWake(), setup(), power_begin(), power_loopWaitBegin(), power_wokeByTimer(), power_wokeFromSleep(), oled_begin(), settings_begin() (+1 more)

### Community 20 - "settings_hiscore"
Cohesion: 0.33
Nodes (8): afterResult(), drawReady(), saveHi(), drawReady(), gameOver(), games_drawReady(), settings_hiscore(), settings_setHiscore()

### Community 21 - "WeatherCache"
Cohesion: 0.29
Nodes (6): WeatherCache, city, count, items, savedAt, time

### Community 22 - "SyncState"
Cohesion: 0.29
Nodes (7): SyncState, SYNC_ASKING, SYNC_FAIL, SYNC_IDLE, SYNC_NOCONN, SYNC_NOTIME, SYNC_OK

### Community 23 - "power_lightSleep"
Cohesion: 0.50
Nodes (4): buttons_armWake(), power_lightSleep(), settings_pmMode(), settings_pmModeName()

### Community 24 - "games_drawOver"
Cohesion: 0.67
Nodes (3): draw(), games_centerText(), games_drawOver()

### Community 25 - "Verdict"
Cohesion: 0.22
Nodes (9): Verdict, kind, us, who, VerdictKind, V_FOUL, V_NONE, V_TIE (+1 more)

### Community 26 - "Pipe"
Cohesion: 0.33
Nodes (6): Pipe, gapH, gapY, on, scored, x

### Community 27 - "RoundState"
Cohesion: 0.50
Nodes (4): RoundState, RS_FIRE, RS_RESULT, RS_WAIT

### Community 28 - "DuelMode"
Cohesion: 0.67
Nodes (3): DuelMode, MODE_2P, MODE_CPU

## Knowledge Gaps
- **106 isolated node(s):** `icon`, `app`, `title`, `msg`, `savedAt` (+101 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 174 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SettingItem` connect `SettingItem` to `menu.cpp`?**
  _High betweenness centrality (0.053) - this node is a cross-community bridge._
- **Are the 52 inferred relationships involving `T()` (e.g. with `anim_update()` and `ble_lastNotifInfo()`) actually correct?**
  _`T()` has 52 INFERRED edges - model-reasoned connections that need verification._
- **What connects `icon`, `app`, `title` to the rest of the system?**
  _106 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ble_link.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.051131354687040845 - nodes in this community are weakly interconnected._
- **Why does `T()` connect `menu.cpp` to `ble_link.cpp`, `ota.cpp`, `game_duel.cpp`, `game_hitung.cpp`, `game_pong.cpp`, `game_ttt.cpp`, `settings.cpp`, `clockstore.cpp`, `main.cpp`, `phone.cpp`, `settings_hiscore`, `games_drawOver`?**
  _High betweenness centrality (0.049) - this node is a cross-community bridge._
- **Are the 29 inferred relationships involving `loopBody()` (e.g. with `anim_isPlaying()` and `anim_update()`) actually correct?**
  _`loopBody()` has 29 INFERRED edges - model-reasoned connections that need verification._
- **Should `menu.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.08065618591934381 - nodes in this community are weakly interconnected._