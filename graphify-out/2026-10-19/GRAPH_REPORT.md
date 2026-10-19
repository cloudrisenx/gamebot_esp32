# Graph Report - ESP32  (2026-10-15)

## Corpus Check
- 48 files · ~47,080 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 2 file(s) not represented in the graph (top: (none) 2)

## Summary
- 806 nodes · 2041 edges · 28 communities
- Extraction: 73% EXTRACTED · 27% INFERRED · 0% AMBIGUOUS · INFERRED: 547 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `1937b4c2`
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
- main.cpp
- settings_hiscore
- remote_hid.cpp
- phone.cpp
- anim_update
- handleInfo
- ota.cpp
- WeatherCache
- SyncState
- ota_holdMenu
- power.h
- Verdict
- Pipe
- RoundState

## God Nodes (most connected - your core abstractions)
1. `oled_flush()` - 36 edges
2. `loopBody()` - 35 edges
3. `activateSetting()` - 31 edges
4. `phone_update()` - 27 edges
5. `btn_pressed()` - 26 edges
6. `settings_hiscore()` - 25 edges
7. `hitung_update()` - 24 edges
8. `settings_screen_update()` - 23 edges
9. `SettingItem` - 22 edges
10. `btn_held()` - 18 edges

## Surprising Connections (you probably didn't know these)
- `clockEditorUpdate()` --calls--> `ble_takeSyncDone()`  [INFERRED]
  src/menu.cpp → src/ble_link.cpp
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

## Communities (28 total, 0 thin omitted)

### Community 0 - "ble_link.cpp"
Cohesion: 0.05
Nodes (74): addHistory(), ble_address(), ble_begin(), ble_callAnswer(), ble_connected(), ble_date(), ble_deletePairings(), ble_isIphone() (+66 more)

### Community 1 - "menu.cpp"
Cohesion: 0.08
Nodes (60): anim_resume(), btn_pressed(), buttons_clear(), clock_lastSync(), drawList(), games_isPlaying(), games_update(), home_update() (+52 more)

### Community 2 - "settings.cpp"
Cohesion: 0.06
Nodes (51): buttons_attachWake(), loop(), loopIdleMs(), setup(), activateSetting(), settingCard(), settingValue(), power_applyContrast() (+43 more)

### Community 3 - "frames_store.cpp"
Cohesion: 0.24
Nodes (16): crc32Update(), dataLen(), dataPtr(), decodeFrame(), findPartition(), frameOffset(), frames_begin(), frames_writeBegin() (+8 more)

### Community 4 - "ios_link.cpp"
Cohesion: 0.07
Nodes (38): ble_markPhoneInfoChanged(), ble_setLocalTime(), ble_setRinging(), ams_parsePlaybackInfo(), AmsUpdate, attr, entity, value (+30 more)

### Community 5 - "game_duel.cpp"
Cohesion: 0.13
Nodes (23): btn_armTiming(), afterResult(), centerLine(), draw(), drawCowboy(), duel_enter(), duel_resume(), duel_update() (+15 more)

### Community 6 - "game_hitung.cpp"
Cohesion: 0.06
Nodes (54): centerLine(), DistDef, dMax, dMin, kinds, draw(), drawHalf(), drawReady() (+46 more)

### Community 7 - "game_pong.cpp"
Cohesion: 0.05
Nodes (59): btn_any_pending(), btn_chord(), btn_doubleTap(), btn_held(), btn_pressTimeUs(), btn_tapSync(), buttons_armWake(), buttons_begin() (+51 more)

### Community 8 - "game_ttt.cpp"
Cohesion: 0.15
Nodes (22): boardFull(), centerAt(), cpuMove(), cpuTurn(), cursorToEmpty(), draw(), drawMark(), drawPanel() (+14 more)

### Community 9 - "game_breakout.cpp"
Cohesion: 0.29
Nodes (8): attachBall(), breakout_enter(), buildLevel(), hitBrick(), launch(), resetRound(), setSpeed(), step()

### Community 10 - "game_dino.cpp"
Cohesion: 0.16
Nodes (20): dino_enter(), dino_update(), draw(), drawObstacle(), gameOver(), gapFor(), obsSize(), Obstacle (+12 more)

### Community 11 - "SettingItem"
Cohesion: 0.09
Nodes (22): SettingItem, SI_BOOTANIM, SI_BT, SI_BT_RESTART, SI_CLOCK, SI_CONTRAST, SI_COUNT, SI_FLASH_MODE (+14 more)

### Community 12 - "games.cpp"
Cohesion: 0.18
Nodes (7): GameDef, id, name, portrait, games_drawSidePanels(), panelDots(), printShort()

### Community 13 - "clockstore.cpp"
Cohesion: 0.32
Nodes (11): ble_epoch(), ble_setClock(), ble_setClockParts(), ble_takeTimeSynced(), clock_begin(), clock_loop(), clock_save(), clock_setManual() (+3 more)

### Community 14 - "main.cpp"
Cohesion: 0.12
Nodes (3): colonOn(), drawClock(), markAppValid()

### Community 15 - "settings_hiscore"
Cohesion: 0.16
Nodes (17): breakout_update(), draw(), gameOver(), drawReady(), draw(), flappy_enter(), flappy_update(), gameOver() (+9 more)

### Community 16 - "remote_hid.cpp"
Cohesion: 0.08
Nodes (32): ble_remoteStop(), camCapture(), drawHidStatus(), BondSet, cccd, nCccd, nOur, nPeer (+24 more)

### Community 17 - "phone.cpp"
Cohesion: 0.07
Nodes (45): ble_historyCount(), ble_historyDelete(), ble_markRead(), ble_phoneInfoChanged(), beginScreen(), camAnimating(), camLoadTimer(), camSaveTimer() (+37 more)

### Community 18 - "anim_update"
Cohesion: 0.15
Nodes (18): anim_enter(), anim_update(), drawFrame(), logStats(), ble_setNavScreen(), btn_any_pressed(), frames_count(), frames_drawFrame() (+10 more)

### Community 19 - "handleInfo"
Cohesion: 0.19
Nodes (12): anim_isPlaying(), frames_height(), frames_valid(), frames_width(), handleInfo(), ota_bgStart(), ota_enter(), asciiOk() (+4 more)

### Community 20 - "ota.cpp"
Cohesion: 0.16
Nodes (10): draw(), drawQr(), drawStatus(), handleDeviceSet(), handleFirmwareDone(), handleFirmwareUpload(), jsonStr(), onChunk() (+2 more)

### Community 21 - "WeatherCache"
Cohesion: 0.29
Nodes (6): WeatherCache, city, count, items, savedAt, time

### Community 22 - "SyncState"
Cohesion: 0.29
Nodes (7): SyncState, SYNC_ASKING, SYNC_FAIL, SYNC_IDLE, SYNC_NOCONN, SYNC_NOTIME, SYNC_OK

### Community 23 - "ota_holdMenu"
Cohesion: 0.24
Nodes (8): drawHoldMenu(), ota_bgPoll(), ota_bgRunning(), ota_bgStop(), ota_holdMenu(), otaExit(), draw(), text_update()

### Community 24 - "power.h"
Cohesion: 0.31
Nodes (6): handleTextGet(), handleTextSet(), load(), rebase(), text_get(), text_set()

### Community 25 - "Verdict"
Cohesion: 0.22
Nodes (9): Verdict, kind, us, who, VerdictKind, V_FOUL, V_NONE, V_TIE (+1 more)

### Community 26 - "Pipe"
Cohesion: 0.29
Nodes (7): drawPipe(), Pipe, gapH, gapY, on, scored, x

### Community 27 - "RoundState"
Cohesion: 0.50
Nodes (4): RoundState, RS_FIRE, RS_RESULT, RS_WAIT

## Knowledge Gaps
- **105 isolated node(s):** `icon`, `app`, `title`, `msg`, `savedAt` (+100 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 172 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SettingItem` connect `SettingItem` to `menu.cpp`?**
  _High betweenness centrality (0.052) - this node is a cross-community bridge._
- **Are the 35 inferred relationships involving `oled_flush()` (e.g. with `anim_update()` and `drawFrame()`) actually correct?**
  _`oled_flush()` has 35 INFERRED edges - model-reasoned connections that need verification._
- **What connects `icon`, `app`, `title` to the rest of the system?**
  _105 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ble_link.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.05401234567901234 - nodes in this community are weakly interconnected._
- **Why does `oled_flush()` connect `menu.cpp` to `ble_link.cpp`, `settings.cpp`, `game_duel.cpp`, `game_hitung.cpp`, `game_pong.cpp`, `game_ttt.cpp`, `game_dino.cpp`, `settings_hiscore`, `phone.cpp`, `anim_update`, `ota.cpp`, `ota_holdMenu`?**
  _High betweenness centrality (0.034) - this node is a cross-community bridge._
- **Are the 28 inferred relationships involving `loopBody()` (e.g. with `anim_isPlaying()` and `anim_update()`) actually correct?**
  _`loopBody()` has 28 INFERRED edges - model-reasoned connections that need verification._
- **Should `menu.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.0777489818585709 - nodes in this community are weakly interconnected._