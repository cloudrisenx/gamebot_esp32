# Pet: a cat that lives on the screen (v0.9.38)

Open it from **Home → tap R** (right button). Hold R = Animation, tap R 3x fast = Music (a single tap waits ±0.3 s to
tell it apart from 3 taps). The reference for every behaviour and every pixel is the PC simulation
[`tools/pet_sim.py`](../tools/pet_sim.py); the watch code is a 1:1 port of it.

| File | Role |
|---|---|
| `src/pet_core.h/.cpp` | Pet engine: behaviour + drawing, no Arduino dependency (also built on a PC for tests) |
| `src/pet.h/.cpp` | Pet screen: buttons, freeze/timing, copy to the OLED, 30 fps |
| `src/home.cpp` | right button on Home: tap = Pet, hold = Animation, 3x = Music |
| `tools/pet_sim.py` | simulation (pygame window with only the 4 watch buttons) |
| `tools/pet_host_test.cpp`, `tools/pet_parity.py` | PC tests of the C++ engine |

## Buttons (only the 4 watch buttons)

| Button | Tap | Hold (≥ 0.5 s) |
|---|---|---|
| L (UP) | glance left, head tilt | **feed**: a fish falls into the mouth, chewing, crumbs; 3 fish = full, then it washes its face |
| R (DOWN) | glance right, head tilt | **play**: the face morphs smoothly into an 8-bit cat that watches, stalks, wiggles, pounces, catches and chases a toy mouse; release = morphs back |
| OK | pet it: happy + hearts; 3x fast = tickled; 6x fast = runs away annoyed (tap OK while it is gone = call it back) | stroke: purr + kneading; > 3.5 s = falls asleep, release = wakes up yawning |
| BACK | back to Home | |

## On its own (no button)

Face: eyes look around and drift, blinks (sometimes double), slow blink (trust), head tilt, ear twitches, wink, tongue
out (blep), sniffing, washing the face with a paw, watching a bird fly by with chattering jaw.
Full-body 8-bit cat (±30% of the random activities): patrol walk, sitting and looking around, chasing its own tail
(dizzy afterwards), stretching (play bow, arched back), scratching an ear, playing alone with the toy mouse, zoomies,
and suddenly disappearing to peek in from the top / left / right / bottom edge.
Bored (15 s without a button): slower blinks, yawns, more activities, sometimes goes off to play (never annoyed).
Sleep (after 30 s, or sometimes right after a yawn): face sleep or curled-up 8-bit loaf; **max 3 minutes (random
0.5-3 min)**, then it wakes up by itself yawning. Any button wakes it.

Never sudden anger: the annoyed run-away only happens when it is tickled 6x.

## Power

- The screen stays on for **5 minutes** after the last button while Pet is shown, then the normal "Screen off" rule.
- While the screen is off, during a WhatsApp pop-up or a call, the pet is **frozen** (it does not jump ahead) and Pet
  never holds off light sleep. A WhatsApp arriving while Pet is shown makes the cat look surprised afterwards.

## Measured cost (PC estimate, not yet measured on the watch)

| Item | Value |
|---|---|
| Flash | engine ±60 KB code (firmware ±79.7% → ±83% of the 1.75 MB slot) |
| RAM | static ±22 KB (two 8 KB pixel buffers + sprites + state), no heap use while drawing |
| Normal frame | ±1,450 pixels, estimated 4-7 ms drawing + ±12 ms I2C at 800 kHz → 30 fps |
| Face ↔ cat morph (0.8 s) | heaviest frames, estimated 15-30 ms drawing → ±25-30 fps |

## PC tests (no watch needed)

```powershell
pip install ziglang   # once: a portable C++ compiler
python -m ziglang c++ -std=c++17 -O2 -ffp-contract=off -Isrc tools/pet_host_test.cpp src/pet_core.cpp -o tools/pet_host_test.exe
tools\pet_host_test.exe behave     # behaviour (buttons, activities, sleep limit, play, feed, run away, hide & peek)
python tools/pet_parity.py         # pose table + pixel-exact comparison with the simulation (must be 0 different)
python tools/pet_sim.py            # the simulation window (--debug: hidden test keys)
```

`-ffp-contract=off` is needed on the PC: otherwise the compiler fuses multiply-adds and a few pixels round
differently than in the simulation (the ESP32-C3 has no such instruction).
