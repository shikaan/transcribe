---
title: Untitled
author: manuel
date: 2026-03-08T15:43:50Z
---

# Music Transcription Tool — Requirements & User Stories

## Vision

A native desktop application (C99) for musicians who transcribe music by ear. The tool makes the *listening and deciphering* phase faster and more comfortable — it is not an auto-transcriber. Think "Transcribe! with modern UX and deeper workflow features."

**Primary persona**: A musician practicing and learning licks, solos, or parts from recordings.
**Secondary personas**: Gigging musician learning covers; composer/arranger dissecting recordings for orchestration.

**Platform**: Native desktop, C99, fully local, no server dependency. macOS first, then Linux and Windows.

---

## Decisions Log

| Decision | Choice | Rationale |
|---|---|---|
| Language | C99 | Compiles everywhere, fast builds, no toolchain hassle |
| Platform | macOS first, then Linux, Windows | Developer has a Mac; macOS is the first target |
| Architecture | Core/platform split (see below) | Keeps engine portable; allows native shells per OS later |
| UI strategy | Portable fallback shell first (TBD: Raylib, sokol+cimgui, SDL2+cimgui); native shells (AppKit, GTK4) later | Ship fast with one codebase, then add platform-native UI |
| Network | Fully offline | Privacy, simplicity, no subscription model |
| YouTube import | Deferred (v2+) | Legal gray area, yt-dlp dependency |
| Track isolation | EQ/frequency filtering for v1 | Demucs bundling is heavy (~1GB model, Python/ONNX); revisit for v2 |
| Notation output | Out of scope | Piano roll + MIDI export; delegate notation to MuseScore etc. |
| Spectral note hints | Pitch detection overlay (show likely notes) | Not full polyphonic transcription; a helper, not an oracle |
| Piano roll | Nice-to-have for v1 | Core value is in the listening/looping workflow |

---

## Architecture

### Core / Platform Split

The codebase is split into three parts, enforced at compile time:

```
src/
  core/          Pure C99. Audio engine, DSP, FFT, spectrogram computation,
                 loop logic, pitch detection, project save/load.
                 ZERO platform or UI includes. Compiles as a static lib
                 without linking any UI or windowing library.

  platform/
    fallback/    Portable shell (Raylib, sokol+cimgui, or SDL — TBD).
                 Used for prototyping and as the default on unsupported platforms.
    macos/       Native macOS shell (Objective-C, AppKit). Added later.
    gtk/         Native Linux shell (GTK4, C). Added later.
    win32/       Native Windows shell (Win32 or similar). Added later.

  bridge.h       The API contract between core and platform. Core never
                 includes platform headers; platform never touches core internals.
```

### Build-Level Enforcement

`core/` is compiled as a static library (`core.a`) with NO UI/platform headers on the include path. If a core file accidentally includes `raylib.h` or `<gtk/gtk.h>`, the build breaks. This is the primary mechanism for keeping the boundary clean — not a linter, not discipline, but a compiler error.

```makefile
core.a:
    $(CC) -std=c99 -Wall -Werror -Icore/ -c core/*.c
    # Deliberately no -I for any platform library
    ar rcs core.a *.o
```

### Bridge API Design Principles

The bridge (`bridge.h`) defines how the platform shell talks to the core engine:

- **Commands in, state out.** The platform shell sends commands (`engine_play()`, `engine_set_loop(start, end)`, `engine_set_speed(0.5f)`). Core exposes read-only state for the UI to render (`engine_get_playhead_pos()`, `engine_get_spectrogram_data()`).
- **Input abstraction.** The platform shell translates raw input (key presses, mouse events, MIDI CC) into a command enum. Core never knows what key was pressed:

```c
// bridge.h
typedef enum {
    CMD_PLAY_PAUSE,
    CMD_LOOP_TOGGLE,
    CMD_LOOP_SET_START,
    CMD_LOOP_SET_END,
    CMD_LOOP_POP,          // exit sub-loop
    CMD_SPEED_UP,
    CMD_SPEED_DOWN,
    CMD_PITCH_UP,
    CMD_PITCH_DOWN,
    CMD_FILTER_HP_TOGGLE,
    CMD_FILTER_LP_TOGGLE,
    // ...
} AppCommand;

void engine_handle_command(Engine *e, AppCommand cmd);
```

- **Spectrogram as pixel buffer.** Core produces a flat pixel buffer (`uint32_t *pixels, int w, int h`). The platform shell uploads it to whatever texture system it uses (OpenGL texture, `CGBitmapContext`, `GdkTexture`, etc.). Core never touches GPU APIs.
- **Audio output via miniaudio.** The audio pipeline lives in core, using miniaudio directly. The platform shell does not touch audio — this avoids having to rewrite the audio engine per platform.

### Migration Path: Fallback → Native

1. Build the entire app using the fallback shell (portable, one codebase).
2. When the app is feature-complete and stable, add a macOS-native shell (`platform/macos/`) that replaces only the windowing, menu bar, file dialogs, toolbar, and widget chrome. The spectrogram view, piano roll, and EQ visualizations are custom-drawn in all shells anyway.
3. Add GTK4 shell for Linux.
4. The fallback shell remains available as a compile option for unsupported platforms or for developers who prefer it.

The "native" surface area is small: menu bar, toolbar buttons, file picker dialogs, window chrome, and perhaps a sidebar list. All the heavy custom views (spectrogram, piano roll, EQ curve) are rendered the same way regardless of shell.

---

## Tech Stack

### Decided

| Component | Library | Notes |
|---|---|---|
| Language | C99 | `-std=c99 -Wall -Werror` |
| Audio I/O | miniaudio | Single-header C lib; abstracts CoreAudio, ALSA, WASAPI |
| Time-stretch / pitch-shift | rubberband (C API) | Link via `librubberband`; C API: `rubberband_new()`, `rubberband_process()` |
| FFT | pffft or KissFFT | Single-file C libs; needed for spectrogram + pitch detection |
| Audio decoding | dr_libs (dr_mp3, dr_wav, dr_flac) or libsndfile + minimp3 | Single-header C decoders for MP3, WAV, FLAC |
| Project file format | JSON (cJSON) or SQLite | cJSON is single-file C; SQLite is single-file C |

### To Be Decided

| Component | Candidates | Decision criteria |
|---|---|---|
| Fallback UI shell | Raylib+raygui, sokol_app+cimgui, SDL2+cimgui | Evaluate based on: widget richness needed, spectrogram rendering ease, macOS support, dependency weight |
| Build system | Make, CMake, or plain shell scripts | Keep it simple; avoid heavy build tooling |

---

## Epics & User Stories

Priority key: **P0** = launch blocker, **P1** = strong v1 feature, **P2** = nice-to-have / v2 candidate.

---

### Epic 1: File Import

**Goal**: Get audio into the app with minimal friction.

#### US-1.1 — Open local audio files [P0]

> As a musician, I want to open MP3 and WAV files from disk so I can start working immediately.

Acceptance criteria:
- File picker or drag-and-drop onto the app window.
- Supports at minimum: MP3, WAV, FLAC, OGG, M4A/AAC.
- Decodes to an internal PCM representation on open.
- Shows an error message for unsupported or corrupted files.

#### US-1.2 — Open from URL [P2]

> As a musician, I want to paste a direct URL to an audio file so I can work with remotely hosted recordings without downloading them manually.

Acceptance criteria:
- Accepts a direct HTTP(S) link to a supported audio format.
- Downloads to a temp location, then opens normally.
- Shows progress for large files.

#### US-1.3 — Open from YouTube [P2, deferred]

> As a musician, I want to paste a YouTube URL and have the app extract the audio.

Acceptance criteria:
- Extracts audio via yt-dlp or equivalent.
- User sees extraction progress.
- Deferred to v2+.

---

### Epic 2: Spectrogram Display & Navigation

**Goal**: Provide a visual representation of the audio that helps the user identify what to transcribe and where.

#### US-2.1 — Spectrogram rendering [P0]

> As a musician, I want to see a spectrogram of the loaded audio so I can visually identify melodic contours, rhythmic hits, and frequency content.

Acceptance criteria:
- Renders a frequency-vs-time spectrogram with amplitude mapped to color intensity.
- Configurable FFT size / window for resolution trade-off (sensible default, advanced setting).
- Renders smoothly for files up to at least 20 minutes.

#### US-2.2 — Scroll and zoom [P0]

> As a musician, I want to scroll through the spectrogram and zoom in/out so I can navigate from a bird's-eye view down to individual notes.

Acceptance criteria:
- Mouse wheel scrolls horizontally through time.
- Ctrl+Wheel (or Cmd+Wheel on macOS) zooms in/out on the time axis.
- Zoom centers on the cursor position.
- At maximum zoom, individual note onsets should be clearly distinguishable.
- Consider vertical zoom on frequency axis (P1).

#### US-2.3 — Timecode display [P0]

> As a musician, I want to see the current time position so I can reference specific moments ("the lick at 2:34").

Acceptance criteria:
- A persistent timecode readout showing current playhead position (MM:SS.ms or MM:SS:frames).
- Time ruler along the spectrogram.
- At higher zoom levels, the time ruler shows finer subdivisions automatically.

#### US-2.4 — Playhead [P0]

> As a musician, I want a visible playhead on the spectrogram that tracks playback and that I can reposition.

Acceptance criteria:
- Vertical line moves across the spectrogram during playback.
- Click anywhere on the spectrogram to move the playhead there.
- Drag the playhead to scrub.
- Moving the playhead while playing does **not** pause — playback continues from the new position.
- Spectrogram auto-scrolls to follow the playhead during playback.

---

### Epic 3: Playback Controls

**Goal**: Give the musician full control over how the audio sounds during transcription.

#### US-3.1 — Play / Pause [P0]

> As a musician, I want to play and pause audio with a single key so I can quickly toggle between listening and working.

Acceptance criteria:
- Space plays/pauses (toggle).
- Playback starts from the current playhead position.
- Pause keeps the playhead where it stopped (does not reset).

#### US-3.2 — Playback speed (time-stretch) [P0]

> As a musician, I want to slow down or speed up playback without changing pitch so I can hear fast passages clearly and gradually work up to full tempo.

Acceptance criteria:
- Speed control expressed as a percentage (e.g., 25%, 50%, 75%, 100%, 125%, 150%).
- Preserves pitch (time-stretching algorithm — e.g., rubberband, soundtouch, or WSOLA).
- Minimum 25%, maximum 150% (at least).
- Keyboard shortcuts to increment/decrement speed in steps (e.g., 5% or 10%).
- Current speed always visible in the UI.

#### US-3.3 — Pitch shift [P0]

> As a musician, I want to shift the pitch of the audio in semitone steps so I can match my instrument's tuning or transpose to a more comfortable key.

Acceptance criteria:
- Pitch shift in semitone increments from -12 to +12.
- Does not change tempo.
- Keyboard shortcuts for pitch up/down.
- Current transposition always visible in the UI.

---

### Epic 4: Looping

**Goal**: Let the musician define, recall, and nest regions of audio to loop, which is the core transcription workflow.

#### US-4.1 — Select a loop region [P0]

> As a musician, I want to select a range on the spectrogram to define a loop, the way I'd select text in an editor.

Acceptance criteria:
- Click-and-drag on the spectrogram to define a range (start → end).
- The selected region is visually highlighted.
- Selection snaps to zero-crossings or beat grid if available (P1), but also allows free selection.

#### US-4.2 — Play selection [P0]

> As a musician, I want to press Space to play the selected range once so I can hear a phrase.

Acceptance criteria:
- When a selection exists, Space plays from selection start to selection end, then stops.
- If no selection, Space plays from playhead as usual (US-3.1).

q#### US-4.3 — Loop selection [P0]

> As a musician, I want to press Shift+Space to loop the selected range continuously so I can listen to a phrase repeatedly while I work it out.

Acceptance criteria:
- Shift+Space starts looping the selected range.
- Playback repeats seamlessly (minimal gap/click at the loop boundary).
- Space pauses the loop; Shift+Space or Space resumes.

#### US-4.4 — Set loop boundaries during playback [P0]

> As a musician, I want keyboard shortcuts to mark loop-start and loop-end while audio is playing so I can define a loop on the fly without stopping.

Acceptance criteria:
- A hotkey (e.g., `[`) sets loop start to current playhead position.
- A hotkey (e.g., `]`) sets loop end to current playhead position.
- If both are set, the region is highlighted and looping can be engaged immediately.
- Adjusting boundaries during active looping updates the loop in real time.

#### US-4.5 — Save and recall loop regions [P1]

> As a musician, I want to save named loop regions so I can return to specific phrases across sessions.

Acceptance criteria:
- User can save the current selection as a named region (e.g., "Chorus lick", "Bridge chord").
- Saved regions are listed in a sidebar or panel.
- Clicking a saved region restores the selection and optionally starts playback.
- Regions persist with the project file (see Epic 8).

#### US-4.6 — Sub-loops (nested loops) [P1]

> As a musician transcribing a phrase, I want to zoom into a smaller portion within my current loop to focus on a few notes I can't figure out, then pop back out to the full phrase.

Acceptance criteria:
- While a loop is active, the user can select a sub-range within it and loop that instead.
- A shortcut (e.g., Escape) pops back to the parent loop.
- At least 2 levels of nesting (parent → child). Deeper nesting is a bonus.
- Visual indication of parent loop while child loop is active (e.g., dimmed highlight).

---

### Epic 5: Track Isolation / EQ

**Goal**: Help the musician focus on specific instruments or frequency ranges.

#### US-5.1 — Parametric EQ [P1]

> As a musician, I want to apply EQ to the audio so I can boost or cut frequency ranges to isolate the instrument I'm transcribing.

Acceptance criteria:
- At least a 3-band parametric EQ (low, mid, high) with adjustable frequency, gain, and Q.
- Bypass toggle to quickly A/B the EQ'd vs. original signal.
- Presets for common use cases (e.g., "isolate bass", "cut drums", "vocal focus").
- EQ settings persist per project.

#### US-5.2 — High-pass / Low-pass quick filters [P0]

> As a musician, I want a quick way to filter out everything below or above a certain frequency so I can focus on a specific register.

Acceptance criteria:
- High-pass filter with adjustable cutoff frequency.
- Low-pass filter with adjustable cutoff frequency.
- Keyboard shortcuts or easy UI toggles.
- Visual overlay on the spectrogram showing the active filter range.

#### US-5.3 — AI-based stem separation [P2, deferred]

> As a musician, I want to isolate individual instruments (vocals, bass, drums, other) from the mix.

Acceptance criteria:
- Integrates Demucs (ONNX or bundled Python runtime) for offline stem separation.
- User triggers separation; progress indicator shown.
- Separated stems available as togglable layers (solo/mute per stem).
- Deferred to v2.

---

### Epic 6: Spectral Pitch Hints

**Goal**: Give the musician a nudge toward the right notes by analyzing the spectrum.

#### US-6.1 — Pitch detection overlay [P1]

> As a musician, I want the app to highlight which notes (pitch classes) are likely present at the current playhead position so I can confirm or narrow down what I'm hearing.

Acceptance criteria:
- At the current playhead (or within a selected region), analyze the frequency spectrum and identify the strongest fundamental frequencies.
- Display the results as note names (e.g., "G4, D5, B5") with a confidence indicator (e.g., bar or percentage).
- Updates in near-real-time as the playhead moves (or on demand for a selection).
- Not expected to be perfect — this is a hint, not a transcription. Label it accordingly in the UI.

#### US-6.2 — Note overlay on spectrogram [P2]

> As a musician, I want to see horizontal reference lines on the spectrogram corresponding to musical note frequencies so I can visually map spectral energy to pitches.

Acceptance criteria:
- Optional overlay showing horizontal lines at note frequencies (at least across the piano range, A0–C8).
- Lines labeled with note names.
- Togglable on/off.
- Adjustable reference pitch (A4 = 440Hz default, but configurable for non-standard tunings).

---

### Epic 7: Piano Roll & MIDI

**Goal**: Let the musician jot down what they hear without needing an external instrument or notation software.

#### US-7.1 — Piano roll note entry [P2]

> As a musician away from my instrument, I want a piano roll where I can enter notes I've transcribed so I can sketch out what I'm hearing.

Acceptance criteria:
- Piano roll view aligned to the spectrogram timeline.
- Click to place a note; drag to set duration.
- Notes are quantizable to a grid (configurable subdivision) but also allow free placement.
- Playback of entered notes using a basic synthesizer (sine/piano sound).

#### US-7.2 — Keyboard-as-instrument [P2]

> As a musician, I want to play notes using my computer keyboard so I can quickly test pitches without a MIDI controller.

Acceptance criteria:
- One-octave mapping: Z→C, S→C#, X→D, D→D#, C→E, V→F, G→F#, B→G, H→G#, N→A, J→A#, M→B.
- Octave shift keys: Q shifts down one octave, W shifts up one octave (or similar).
- Played notes sound immediately (low latency).
- Optionally record played notes into the piano roll.

#### US-7.3 — MIDI controller input [P2]

> As a musician, I want to connect a MIDI controller to play notes into the piano roll.

Acceptance criteria:
- Detects connected MIDI devices.
- MIDI config screen to select input device and channel.
- Note-on/note-off maps to piano roll input.
- Velocity-sensitive.

#### US-7.4 — MIDI export [P2]

> As a musician, I want to export the piano roll contents as a MIDI file so I can open it in MuseScore, a DAW, or any other tool.

Acceptance criteria:
- Export to Standard MIDI File (Type 0 or Type 1).
- Preserves note pitches, durations, and velocities.
- Tempo metadata matches the project's detected or user-set BPM.

---

### Epic 8: Project Management

**Goal**: Let the musician save and resume work across sessions.

#### US-8.1 — Save / Load project [P0]

> As a musician, I want to save my work (loop regions, EQ settings, playback position, speed, pitch, notes) and resume later.

Acceptance criteria:
- Project file stores: reference to audio file (path or embedded), all saved loop regions, EQ settings, playback speed, pitch shift, playhead position, piano roll notes.
- Custom file format (e.g., JSON or binary) or SQLite.
- "Save" / "Save As" / "Open Project" standard workflow.
- Prompt to save on close if there are unsaved changes.

#### US-8.2 — Recent files [P1]

> As a musician, I want to see recently opened files/projects so I can quickly resume work.

Acceptance criteria:
- Recent files list in File menu or on a start screen.
- Shows file name, path, and last-opened date.
- Configurable max count (default 10).

---

### Epic 9: Keyboard Shortcuts & Accessibility

**Goal**: The tool should be drivable almost entirely from the keyboard since the musician's hands may be on their instrument.

#### US-9.1 — Comprehensive keyboard shortcuts [P0]

> As a musician with my hands on my instrument, I want keyboard shortcuts for all core actions so I can control the app without reaching for the mouse.

Minimum shortcut set:
- Space: play/pause
- Shift+Space: loop selection
- `[` / `]`: set loop start / end
- Escape: exit sub-loop (pop to parent)
- `+` / `-`: speed up / slow down (in steps)
- Up/Down arrows: pitch shift up/down by semitone
- Ctrl+S: save project
- Ctrl+O: open file
- Ctrl+Z / Ctrl+Y: undo / redo (where applicable)

Acceptance criteria:
- All shortcuts listed above are functional.
- Shortcuts are discoverable (tooltip, help panel, or cheat sheet).
- Shortcuts are user-configurable (P1).

#### US-9.2 — MIDI pedal / controller mapping for transport [P2]

> As a musician, I want to map MIDI foot pedal or controller buttons to play, pause, loop, and speed controls so I can keep both hands on my instrument.

Acceptance criteria:
- Configurable MIDI CC / note mappings to transport actions.
- At minimum: play/pause, loop toggle, speed increment/decrement.

---

## Non-Functional Requirements

| Requirement | Target |
|---|---|
| Audio latency | < 20ms from user action (play, scrub) to audible output |
| Spectrogram render | Smooth scrolling and zoom at 60fps for files up to 20 min |
| Startup time | < 2 seconds to an empty workspace |
| File open time | < 3 seconds for a typical 5-minute MP3 |
| Memory usage | < 500MB for a 20-minute file with spectrogram cached |
| Build time | < 5 seconds for a full rebuild (excluding vendored libs) |
| Platforms | macOS first (dev machine), then Linux and Windows |
| Language standard | C99 (`-std=c99 -Wall -Werror`) |
| Dependencies | Minimal; prefer single-header/single-file C libraries |
| Architecture | Core engine must compile with zero UI/platform dependencies |

---

## Suggested Build Order

A rough phasing based on priorities:

### Phase 1 — Core loop (P0s)

Get to a usable "listen and loop" tool as fast as possible.

1. File import (US-1.1)
2. Audio playback engine with play/pause (US-3.1)
3. Spectrogram rendering (US-2.1) + scroll/zoom (US-2.2) + timecode (US-2.3) + playhead (US-2.4)
4. Loop selection + play + loop (US-4.1, US-4.2, US-4.3, US-4.4)
5. Time-stretch (US-3.2) + pitch shift (US-3.3)
6. HP/LP quick filters (US-5.2)
7. Project save/load (US-8.1)
8. Keyboard shortcuts (US-9.1)

At the end of Phase 1 you have a functional competitor to Transcribe! with better UX.

### Phase 2 — Depth

9. Saved loop regions (US-4.5) + sub-loops (US-4.6)
10. Parametric EQ (US-5.1)
11. Pitch detection overlay (US-6.1)
12. Recent files (US-8.2)
13. Configurable shortcuts

### Phase 3 — Nice-to-haves

14. Piano roll (US-7.1) + keyboard input (US-7.2) + MIDI input (US-7.3) + MIDI export (US-7.4)
15. Note frequency overlay on spectrogram (US-6.2)
16. URL import (US-1.2)
17. MIDI pedal mapping (US-9.2)

### Phase 4 — AI & beyond

18. AI stem separation via Demucs (US-5.3)
19. YouTube import (US-1.3)
20. Beat detection / grid snapping
21. Polyphonic pitch detection improvements
