# Graph Report - ESP32  (2026-10-14)

## Corpus Check
- 51 files · ~44,516 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 4 file(s) not represented in the graph (top: (none) 2, .S 1, .defaults 1)

## Summary
- 776 nodes · 1934 edges · 29 communities (26 shown, 3 thin omitted)
- Extraction: 74% EXTRACTED · 26% INFERRED · 0% AMBIGUOUS · INFERRED: 510 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `9f81f5c2`
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
- home.cpp
- GameDef
- game_flappy.cpp
- remote_hid.cpp
- phone.cpp
- settings_hiscore
- ble_isIphone
- ota.cpp
- WeatherCache
- SyncState
- ble_update
- IosServerCallbacks
- RawNotif
- AmsUpdate

## God Nodes (most connected - your core abstractions)
1. `oled_flush()` - 35 edges
2. `loopBody()` - 34 edges
3. `activateSetting()` - 32 edges
4. `btn_pressed()` - 25 edges
5. `settings_hiscore()` - 25 edges
6. `hitung_update()` - 24 edges
7. `phone_update()` - 24 edges
8. `settings_screen_update()` - 23 edges
9. `SettingItem` - 22 edges
10. `setup()` - 19 edges

## Surprising Connections (you probably didn't know these)
- `enterState()` --calls--> `ble_setNavScreen()`  [INFERRED]
  src/main.cpp → src/ble_link.cpp
- `applyTime()` --calls--> `ble_setLocalTime()`  [INFERRED]
  src/ios_link.cpp → src/ble_link.cpp
- `clockEditorUpdate()` --calls--> `ble_takeSyncDone()`  [INFERRED]
  src/menu.cpp → src/ble_link.cpp
- `nav_screen_update()` --calls--> `ble_navChanged()`  [INFERRED]
  src/menu.cpp → src/ble_link.cpp
- `phone_update()` --calls--> `ble_phoneInfoChanged()`  [INFERRED]
  src/phone.cpp → src/ble_link.cpp

## Import Cycles
- None detected.

## Communities (29 total, 3 thin omitted)

### Community 0 - "ble_link.cpp"
Cohesion: 0.08
Nodes (35): addHistory(), ble_address(), ble_begin(), ble_deletePairings(), ble_lastNotifInfo(), ble_navChanged(), ble_phoneInfoChanged(), ble_pushNotification() (+27 more)

### Community 1 - "menu.cpp"
Cohesion: 0.07
Nodes (65): anim_enter(), anim_isPlaying(), anim_resume(), anim_update(), logStats(), btn_any_pressed(), btn_pressed(), buttons_clear() (+57 more)

### Community 2 - "settings.cpp"
Cohesion: 0.06
Nodes (55): buttons_attachWake(), setup(), activateSetting(), settingCard(), settingValue(), clockMinutes(), enterDeepSleep(), power_activity() (+47 more)

### Community 3 - "frames_store.cpp"
Cohesion: 0.16
Nodes (24): drawFrame(), crc32Update(), dataLen(), dataPtr(), decodeFrame(), findPartition(), frameOffset(), frames_begin() (+16 more)

### Community 4 - "ios_link.cpp"
Cohesion: 0.13
Nodes (25): ble_markPhoneInfoChanged(), ble_setRinging(), ams_parsePlaybackInfo(), ancs_parseAttributes(), ancs_parseNotificationSource(), appLabel(), applyTime(), copyAttr() (+17 more)

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
Cohesion: 0.16
Nodes (19): centerAt(), cpuMove(), cpuTurn(), cursorToEmpty(), draw(), drawMark(), drawPanel(), findLine() (+11 more)

### Community 9 - "game_breakout.cpp"
Cohesion: 0.32
Nodes (10): attachBall(), breakout_enter(), breakout_update(), buildLevel(), gameOver(), hitBrick(), launch(), resetRound() (+2 more)

### Community 10 - "game_dino.cpp"
Cohesion: 0.16
Nodes (20): dino_enter(), dino_update(), draw(), drawObstacle(), gameOver(), gapFor(), obsSize(), Obstacle (+12 more)

### Community 11 - "SettingItem"
Cohesion: 0.09
Nodes (22): SettingItem, SI_BOOTANIM, SI_BT, SI_BT_RESTART, SI_CLOCK, SI_CONTRAST, SI_COUNT, SI_GUIDE (+14 more)

### Community 12 - "games.cpp"
Cohesion: 0.15
Nodes (4): games_drawSidePanels(), games_isPlaying(), panelDots(), printShort()

### Community 13 - "home.cpp"
Cohesion: 0.11
Nodes (16): ble_epoch(), ble_setClock(), ble_setClockParts(), ble_takeTimeSynced(), clock_begin(), clock_loop(), clock_save(), clock_setManual() (+8 more)

### Community 14 - "GameDef"
Cohesion: 0.40
Nodes (4): GameDef, id, name, portrait

### Community 15 - "game_flappy.cpp"
Cohesion: 0.18
Nodes (15): draw(), drawPipe(), flappy_enter(), flappy_update(), gameOver(), gapHeight(), Pipe, gapH (+7 more)

### Community 16 - "remote_hid.cpp"
Cohesion: 0.09
Nodes (26): ble_remoteStop(), BondSet, cccd, nCccd, nOur, nPeer, our, peer (+18 more)

### Community 17 - "phone.cpp"
Cohesion: 0.07
Nodes (49): ble_connected(), ble_historyCount(), ble_historyDelete(), ble_markRead(), ble_musicControl(), header_signature(), infoSignature(), settingsSignature() (+41 more)

### Community 18 - "settings_hiscore"
Cohesion: 0.22
Nodes (11): draw(), drawReady(), saveHi(), boardFull(), drawReady(), place(), games_centerText(), games_drawOver() (+3 more)

### Community 19 - "ble_isIphone"
Cohesion: 0.14
Nodes (23): ble_callAnswer(), ble_date(), ble_isIphone(), ble_musicProgress(), ble_navActive(), ble_phoneBattery(), ble_setLowPowerLink(), ble_unreadCount() (+15 more)

### Community 20 - "ota.cpp"
Cohesion: 0.10
Nodes (19): draw(), drawQr(), drawStatus(), handleFirmwareDone(), handleFirmwareUpload(), handleFramesDone(), handleTextGet(), handleTextSet() (+11 more)

### Community 21 - "WeatherCache"
Cohesion: 0.29
Nodes (6): WeatherCache, city, count, items, savedAt, time

### Community 22 - "SyncState"
Cohesion: 0.29
Nodes (7): SyncState, SYNC_ASKING, SYNC_FAIL, SYNC_IDLE, SYNC_NOCONN, SYNC_NOTIME, SYNC_OK

### Community 23 - "ble_update"
Cohesion: 0.25
Nodes (9): ble_restartRadio(), ble_running(), ble_update(), saveWeatherCache(), stopAll(), tuneBlePower(), ios_begin(), ios_running() (+1 more)

### Community 25 - "RawNotif"
Cohesion: 0.40
Nodes (5): RawNotif, app, icon, msg, title

### Community 26 - "AmsUpdate"
Cohesion: 0.50
Nodes (4): AmsUpdate, attr, entity, value

## Knowledge Gaps
- **99 isolated node(s):** `icon`, `app`, `title`, `msg`, `savedAt` (+94 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 166 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SettingItem` connect `SettingItem` to `menu.cpp`?**
  _High betweenness centrality (0.053) - this node is a cross-community bridge._
- **Are the 34 inferred relationships involving `oled_flush()` (e.g. with `anim_update()` and `drawFrame()`) actually correct?**
  _`oled_flush()` has 34 INFERRED edges - model-reasoned connections that need verification._
- **What connects `icon`, `app`, `title` to the rest of the system?**
  _99 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ble_link.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.07948717948717948 - nodes in this community are weakly interconnected._
- **Why does `loopBody()` connect `menu.cpp` to `settings.cpp`, `frames_store.cpp`, `game_duel.cpp`, `game_pong.cpp`, `games.cpp`, `home.cpp`, `remote_hid.cpp`, `phone.cpp`, `ble_isIphone`, `ota.cpp`, `ble_update`?**
  _High betweenness centrality (0.036) - this node is a cross-community bridge._
- **Are the 27 inferred relationships involving `loopBody()` (e.g. with `anim_isPlaying()` and `anim_update()`) actually correct?**
  _`loopBody()` has 27 INFERRED edges - model-reasoned connections that need verification._
- **Should `menu.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06609195402298851 - nodes in this community are weakly interconnected._