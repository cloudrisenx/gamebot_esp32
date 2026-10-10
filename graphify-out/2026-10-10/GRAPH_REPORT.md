# Graph Report - ESP32  (2026-10-10)

## Corpus Check
- 46 files · ~41,956 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 2 file(s) not represented in the graph (top: (none) 2)

## Summary
- 736 nodes · 1838 edges · 22 communities (21 shown, 1 thin omitted)
- Extraction: 74% EXTRACTED · 26% INFERRED · 0% AMBIGUOUS · INFERRED: 476 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

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
- settings_hiscore
- remote_hid.cpp
- Adafruit_SSD1306
- main.cpp
- WeatherCache
- SyncState
- Verdict

## God Nodes (most connected - your core abstractions)
1. `loopBody()` - 32 edges
2. `oled_flush()` - 31 edges
3. `activateSetting()` - 25 edges
4. `settings_hiscore()` - 25 edges
5. `hitung_update()` - 24 edges
6. `phone_update()` - 24 edges
7. `btn_pressed()` - 23 edges
8. `settings_screen_update()` - 21 edges
9. `setup()` - 18 edges
10. `SettingItem` - 18 edges

## Surprising Connections (you probably didn't know these)
- `resumeAfterOverlay()` --calls--> `anim_resume()`  [INFERRED]
  src/main.cpp → src/anim.cpp
- `applyTime()` --calls--> `ble_setLocalTime()`  [INFERRED]
  src/ios_link.cpp → src/ble_link.cpp
- `nav_screen_update()` --calls--> `ble_navChanged()`  [INFERRED]
  src/menu.cpp → src/ble_link.cpp
- `phone_update()` --calls--> `ble_historyDelete()`  [INFERRED]
  src/phone.cpp → src/ble_link.cpp
- `phone_update()` --calls--> `ble_phoneInfoChanged()`  [INFERRED]
  src/phone.cpp → src/ble_link.cpp

## Import Cycles
- None detected.

## Communities (22 total, 1 thin omitted)

### Community 0 - "ble_link.cpp"
Cohesion: 0.06
Nodes (68): addHistory(), ble_address(), ble_begin(), ble_connected(), ble_date(), ble_historyDelete(), ble_isIphone(), ble_lastNotifInfo() (+60 more)

### Community 1 - "menu.cpp"
Cohesion: 0.07
Nodes (72): ble_historyCount(), ble_takeSyncDone(), btn_pressed(), buttons_clear(), clock_lastSync(), drawList(), games_update(), home_update() (+64 more)

### Community 2 - "settings.cpp"
Cohesion: 0.06
Nodes (54): buttons_attachWake(), loop(), loopIdleMs(), oledResponds(), setup(), activateSetting(), settingCard(), settingValue() (+46 more)

### Community 3 - "frames_store.cpp"
Cohesion: 0.12
Nodes (31): anim_isPlaying(), anim_update(), drawFrame(), logStats(), btn_any_pressed(), crc32Update(), dataLen(), dataPtr() (+23 more)

### Community 4 - "ios_link.cpp"
Cohesion: 0.07
Nodes (40): ble_callAnswer(), ble_deletePairings(), ble_markPhoneInfoChanged(), ble_musicProgress(), ble_setRinging(), ams_parsePlaybackInfo(), AmsUpdate, attr (+32 more)

### Community 5 - "game_duel.cpp"
Cohesion: 0.11
Nodes (26): btn_armTiming(), afterResult(), centerLine(), draw(), drawCowboy(), drawReady(), duel_enter(), duel_resume() (+18 more)

### Community 6 - "game_hitung.cpp"
Cohesion: 0.06
Nodes (54): centerLine(), DistDef, dMax, dMin, kinds, draw(), drawHalf(), drawReady() (+46 more)

### Community 7 - "game_pong.cpp"
Cohesion: 0.07
Nodes (47): btn_any_pending(), btn_chord(), btn_doubleTap(), btn_held(), btn_pressTimeUs(), btn_tapSync(), buttons_begin(), buttons_update() (+39 more)

### Community 8 - "game_ttt.cpp"
Cohesion: 0.15
Nodes (22): boardFull(), centerAt(), cpuMove(), cpuTurn(), cursorToEmpty(), draw(), drawMark(), drawPanel() (+14 more)

### Community 9 - "game_breakout.cpp"
Cohesion: 0.22
Nodes (14): attachBall(), breakout_enter(), breakout_update(), buildLevel(), draw(), gameOver(), hitBrick(), launch() (+6 more)

### Community 10 - "game_dino.cpp"
Cohesion: 0.16
Nodes (20): dino_enter(), dino_update(), draw(), drawObstacle(), gameOver(), gapFor(), obsSize(), Obstacle (+12 more)

### Community 11 - "SettingItem"
Cohesion: 0.11
Nodes (18): SettingItem, SI_BOOTANIM, SI_BT, SI_BT_RESTART, SI_CLOCK, SI_CONTRAST, SI_COUNT, SI_GUIDE (+10 more)

### Community 12 - "games.cpp"
Cohesion: 0.17
Nodes (5): anim_resume(), games_drawSidePanels(), games_isPlaying(), panelDots(), printShort()

### Community 13 - "clockstore.cpp"
Cohesion: 0.28
Nodes (11): ble_epoch(), ble_setClock(), ble_setClockParts(), ble_takeTimeSynced(), clock_begin(), clock_loop(), clock_save(), clock_setManual() (+3 more)

### Community 14 - "GameDef"
Cohesion: 0.40
Nodes (4): GameDef, id, name, portrait

### Community 15 - "settings_hiscore"
Cohesion: 0.15
Nodes (18): draw(), drawPipe(), flappy_enter(), flappy_update(), gameOver(), gapHeight(), Pipe, gapH (+10 more)

### Community 16 - "remote_hid.cpp"
Cohesion: 0.05
Nodes (44): ble_markRead(), ble_remoteStart(), ble_remoteStop(), ble_setSuspended(), otaExit(), camCapture(), camLoadTimer(), drawHidStatus() (+36 more)

### Community 20 - "main.cpp"
Cohesion: 0.07
Nodes (19): anim_enter(), ble_setNavScreen(), games_enter(), colonOn(), drawClock(), drawInfoText(), enterState(), markAppValid() (+11 more)

### Community 21 - "WeatherCache"
Cohesion: 0.29
Nodes (6): WeatherCache, city, count, items, savedAt, time

### Community 22 - "SyncState"
Cohesion: 0.29
Nodes (7): SyncState, SYNC_ASKING, SYNC_FAIL, SYNC_IDLE, SYNC_NOCONN, SYNC_NOTIME, SYNC_OK

### Community 23 - "Verdict"
Cohesion: 0.22
Nodes (9): Verdict, kind, us, who, VerdictKind, V_FOUL, V_NONE, V_TIE (+1 more)

## Knowledge Gaps
- **95 isolated node(s):** `icon`, `app`, `title`, `msg`, `savedAt` (+90 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 157 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SettingItem` connect `SettingItem` to `menu.cpp`?**
  _High betweenness centrality (0.046) - this node is a cross-community bridge._
- **Are the 25 inferred relationships involving `loopBody()` (e.g. with `anim_isPlaying()` and `anim_update()`) actually correct?**
  _`loopBody()` has 25 INFERRED edges - model-reasoned connections that need verification._
- **What connects `icon`, `app`, `title` to the rest of the system?**
  _95 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ble_link.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.056576576576576575 - nodes in this community are weakly interconnected._
- **Why does `loopBody()` connect `menu.cpp` to `ble_link.cpp`, `settings.cpp`, `frames_store.cpp`, `game_pong.cpp`, `games.cpp`, `clockstore.cpp`, `main.cpp`?**
  _High betweenness centrality (0.032) - this node is a cross-community bridge._
- **Are the 30 inferred relationships involving `oled_flush()` (e.g. with `anim_update()` and `drawFrame()`) actually correct?**
  _`oled_flush()` has 30 INFERRED edges - model-reasoned connections that need verification._
- **Should `menu.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.07030205827318899 - nodes in this community are weakly interconnected._