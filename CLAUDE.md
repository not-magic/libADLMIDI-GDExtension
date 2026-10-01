# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

A Godot 4 GDExtension that adds `.mid`/`.midi` playback and real-time MIDI-driven music generation to Godot, rendered as OPL3 FM synthesis audio via **libADLMIDI**. It exposes:

- `AudioStreamMIDIBase` (`src/audio_stream_midi_base.h/.cpp`) — abstract base shared by the two stream classes below: the read-only `bank_data` custom bank, `_get_parameter_list()` for the shared synth parameters (`embedded_bank`, `num_chips`, `emulator`, `volume_model`, `four_op_channels`, `full_range_brightness`) and `create_base_player()`, which does the common `adl_init` + chip/bank/emulator setup from an `AudioStreamPlaybackMIDIBase` and returns an unopened `ADL_MIDIPlayer*`. Registered via `GDREGISTER_ABSTRACT_CLASS` so it shows up in the class hierarchy and its bound properties/methods are inherited by both derived classes, but it can't be instantiated directly from GDScript.
- `AudioStreamMIDI` (`src/audio_stream_midi.h/.cpp`) — a `Resource`/`AudioStream` holding raw MIDI bytes (`midi_data`) (streams are read-only; looping/song number are playback parameters). `create_player()` calls `create_base_player()` and then applies loop settings and opens `midi_data`. Metadata (title, length, track list, loop points) is exposed as read-only properties and queried lazily via a cached "info" `ADL_MIDIPlayer` (default config, 1 chip), invalidated through the `_on_config_changed()` hook (overridden from the base; called by every base-class setter) as well as directly from `set_midi_data()`.
- `AudioStreamPlaybackMIDI` (same files) — one `ADL_MIDIPlayer` per playback instance, so the same `AudioStreamMIDI` resource can be played concurrently by multiple `AudioStreamPlayer`s, auto-playing through libADLMIDI's built-in sequencer (`adl_play`/`adl_playFormat`).
- `AudioStreamMIDISequencer` / `AudioStreamPlaybackMIDISequencer` (`src/audio_stream_midi_sequencer.h/.cpp`) — the "generate MIDI music" path: a stream with no fixed song, adding nothing beyond `AudioStreamMIDIBase` except `_instantiate_playback()`/`_get_stream_name()`/`_get_length()`. Its playback runs libADLMIDI in real-time mode and is driven entirely by the bound `note_on`/`note_off`/`controller_change`/`pitch_bend`/`panic`/... methods, each forwarding straight to a `MidiScheduler` member and logging an error (via `report_if_discarded()`) if it rejects the call.
- `MidiScheduler` (`src/midi_scheduler.h/.cpp`) — the actual sample-accurate scheduling engine behind `AudioStreamPlaybackMIDISequencer`: queues note_on/note_off/... calls by target frame (a `MessageType` enum (bool-returning methods are `try_*`) + a `MessageParams` union of small per-type param structs, in a `std::deque<QueuedMessage>`), rejects anything scheduled before `current_frame_index`, and `try_mix()` sorts the queue, generates audio up to each due message via `adl_generateFormat`, and dispatches it via the matching `adl_rt_*` call, so messages land at the exact right frame within the buffer. **Deliberately has zero Godot/godot-cpp dependency** (only `<adlmidi.h>` and `<deque>`) specifically so it can be unit tested directly — see `tests/runtime_scheduling_test.cpp`, which exercises the real class (via an optional `DispatchCallback` hook meant for exactly this) rather than a hand-mirrored copy. `AudioStreamPlaybackMIDISequencer` owns the `ADL_MIDIPlayer*` and must call `scheduler.set_player()` every time it changes (create in `_start()`, `nullptr` in `_stop()`/before recreating) — the scheduler never creates or closes the player itself.
- `ResourceFormatLoaderMIDI` (`src/resource_format_loader_midi.h/.cpp`) — recognizes `.mid`/`.midi`/`.rmi`/`.xmi`/`.kar`, reads the file via Godot's `FileAccess`, and wraps the bytes in an `AudioStreamMIDI`.

Audio is generated directly in Godot's native float `AudioFrame` layout: libADLMIDI's `ADLMIDI_AudioFormat` (F32, `containerSize=4`, `sampleOffset=sizeof(AudioFrame)=8`) writes straight into the interleaved `left`/`right` buffer with zero conversion — see `AudioStreamPlaybackMIDI::_mix_resampled` and `MidiScheduler::try_mix`. `MidiScheduler` has its own `AudioFrame` struct (identical layout, no Godot dependency); `AudioStreamPlaybackMIDISequencer::_mix_resampled` just `reinterpret_cast`s godot-cpp's `AudioFrame*` to `MidiScheduler::AudioFrame*` when calling it. This mapping (and the `sampleCount = frame_count * 2` convention libADLMIDI uses) was verified against libADLMIDI's own source (`libADLMIDI/src/adlmidi.cpp`), not just its header docs, since the docs are ambiguous about whether "samples" means per-channel or interleaved-total.

Playback classes derive from godot-cpp's **`AudioStreamPlaybackResampled`** (not raw `AudioStreamPlayback`), matching the pattern in `~/Documents/gh/obfuscated-godot/modules/fluidsynth` (a Fluidsynth-based Godot audio module used as the reference implementation for this project) — libADLMIDI is initialized at the engine's actual mix rate, so resampling is a no-op in practice but this still gets `AudioStreamPlayer.pitch_scale` support for free.

### IMPORTANT: libADLMIDI contribution policy

`libADLMIDI/AGENTS.md` states that upstream **does not accept AI-generated code contributions** (license-provenance concerns). This repo only *consumes* libADLMIDI as a submodule/dependency, so writing GDExtension binding code in `src/` is fine — but never author, edit, or "fix" code *inside* the `libADLMIDI/` submodule itself; if a change there is genuinely needed, flag it for the user to handle upstream rather than committing it here.

## Repository layout

- `src/` — the GDExtension C++ source (`register_types.*`, `audio_stream_midi_base.*`, `audio_stream_midi.*`, `audio_stream_midi_sequencer.*`, `resource_format_loader_midi.*`) plus `midi_scheduler.*`, the one file in here with no Godot dependency (see below).
- `godot-cpp/` — git submodule (branch `4.5`), the official C++ bindings for Godot's GDExtension API. Bindings are generated from its bundled `godot-cpp/gdextension/extension_api.json` (Godot 4.5), so the target Godot API version is set by which godot-cpp commit the submodule points at; `compatibility_minimum` in `project/addons/adlmidi/adlmidi.gdextension` should match it.
- `libADLMIDI/` — git submodule, the MIDI/OPL3 synth library being wrapped. Public C API: `libADLMIDI/include/adlmidi.h` (`extern "C"`, `adl_*`/`ADL_*`).
- `project/` — a minimal Godot project for manually exercising the extension, with the addon already installed at `project/addons/adlmidi/` (`adlmidi.gdextension` is the extension descriptor, which is all Godot needs to load it — there's deliberately no `plugin.cfg`/`EditorPlugin`, since an empty one only adds a Plugins-list toggle that can't actually unload the extension; `project/addons/adlmidi/bin/*.so|*.dll|*.dylib` are build output, gitignored). This is also the layout to copy into `res://addons/` in another project, and the shape a Godot Asset Library submission for this extension would use.
- `tests/` — standalone libADLMIDI-level regression tests, opt-in via `scons tests=yes` (see "Standalone tests" below). `tests/bin/` is build output (gitignored).
- `build/libADLMIDI/` — libADLMIDI's compiled object files (gitignored). The root `SConstruct` builds them here via `VariantDir` instead of in-place, specifically so the `libADLMIDI/` submodule's working tree never picks up untracked build artifacts.

## Building

SCons, following the [official GDExtension C++ example](https://docs.godotengine.org/en/4.4/tutorials/scripting/gdextension/gdextension_cpp_example.html)'s directory/`SConstruct` layout. libADLMIDI has no SCons build of its own, so the root `SConstruct` compiles its sources directly (see "libADLMIDI source list" below) instead of shelling out to its `CMakeLists.txt`.

```bash
git submodule update --init --recursive   # first time only
scons platform=linux target=template_debug -j$(nproc)
```

Swap `platform=linux` for `windows`/`macos` as needed, and `target=template_debug` for `target=template_release` for a release/export build. Output lands in `project/addons/adlmidi/bin/libadlmidi<suffix><SHLIBSUFFIX>` (e.g. `project/addons/adlmidi/bin/libadlmidi.linux.template_debug.x86_64.so`), matching the filenames already referenced in `project/addons/adlmidi/adlmidi.gdextension`. The first build compiles godot-cpp's full binding set from scratch (~2100 files) plus libADLMIDI with every default OPL3 emulator backend — this takes a while but is a one-time cost; re-running `scons` after touching only `src/` is fast and incremental (SCons correctly no-ops on files it hasn't seen change).

On `target=editor`/`template_debug` builds (not `template_release`), `SConstruct` also compiles `doc_classes/*.xml` into `src/gen/doc_data.gen.cpp` (via godot-cpp's `GodotCPPDocData` builder, gitignored, zlib-compressed and embedded in the binary) so classes like `AudioStreamMIDI` get real entries in the editor's Help panel instead of nothing. Keep `doc_classes/*.xml` in sync by hand when adding/renaming bound methods or properties — nothing regenerates the stub XML automatically; a Godot editor's "Update Docs" pass (Project menu) can scaffold new empty `<method>`/`<member>` entries for anything missing, but won't fill in prose.

### libADLMIDI source list

`SConstruct` hardcodes the list of libADLMIDI `.c`/`.cpp` files to compile (`adlmidi_relative_sources`) plus one required define (`ENABLE_END_SILENCE_SKIPPING`) and a `-fexceptions` override (godot-cpp disables exceptions by default; libADLMIDI's `pl_list` container throws `std::bad_alloc`). This list was extracted from a real `cmake --build` of `libADLMIDI/CMakeLists.txt` with default options (`WITH_MIDI_SEQUENCER`, `WITH_EMBEDDED_BANKS`, `WITH_XMI_SUPPORT`, and all non-restricted-platform emulator backends ON) — i.e. it mirrors what libADLMIDI itself considers its default, full-featured build. If libADLMIDI is updated upstream and adds/removes/renames source files, this list needs to be updated by hand; there's no automated way to keep it in sync short of re-diffing against libADLMIDI's `CMakeLists.txt`.

There is no in-repo Godot editor binary, so loading the extension in an actual Godot project has only been verified manually (see `project/`) — the standalone tests below are the closest thing to automated coverage available in this environment.

### Standalone tests (tests/)

`tests/*.cpp` are plain executables that link directly against the same libADLMIDI objects SConstruct builds above, plus `src/midi_scheduler.cpp` — no godot-cpp involved, so they build fast and can run in any environment without a Godot editor. Currently just `runtime_scheduling_test.cpp`, which exercises the real `MidiScheduler` (`src/midi_scheduler.h`) — it's the reason that class has no Godot dependency in the first place; `AudioStreamPlaybackMIDISequencer` itself can't be instantiated outside a live Godot engine, but `MidiScheduler` can, so all the scheduling logic worth testing lives there instead.

Opt-in, off by default (a plain `scons` build doesn't touch `tests/`):

```bash
scons platform=linux target=template_debug tests=yes              # build tests/*.cpp -> tests/bin/
scons platform=linux target=template_debug tests=yes run_tests=yes   # ...and run each one
```

`run_tests=yes` fails the `scons` invocation (non-zero exit) if any test exits non-zero. godot-cpp's own option parser will print `WARNING: Unknown SCons variables were passed and will be ignored` for `tests`/`run_tests` — harmless; they're read directly from `ARGUMENTS` in the root `SConstruct`, not through godot-cpp's `Variables` object.

### CI / releases (.github/workflows/)

- `build.yml` — on every push to `main` and on PRs, builds `template_debug` and runs `tests=yes run_tests=yes` on Linux, Windows, and macOS runners. Compile-only sanity check; doesn't publish anything.
- `release.yml` — on pushing a `v*` tag (or manual dispatch with a tag input), builds `template_debug` + `template_release` for all three platforms, then zips `project/addons/adlmidi/` (source-free, with every platform's binaries) into a GitHub Release asset whose internal layout is `addons/adlmidi/...`, so it extracts directly into a project root. Deliberately does **not** commit binaries into the repo — the Asset Library's GitHub-commit-based download can't see them either way, so a submission should use the release zip as a "Custom" download URL rather than pointing at a commit.

Both cache SCons build objects via `actions/cache` keyed on `src/`, `SConstruct`, and the pinned submodule commits (using SCons's built-in `SCONS_CACHE` support, already wired into godot-cpp's `SConstruct`) to avoid recompiling godot-cpp's ~2100-file binding set from scratch on every run.

## Architecture notes for future work

- godot-cpp virtual method overrides in extension classes are **underscore-prefixed** (`_start`, `_mix_resampled`, `_instantiate_playback`, ...) — this differs from the non-prefixed names used by core-engine C++ modules (like the fluidsynth reference module), so don't copy method names verbatim from a core module; check the relevant `godot-cpp/gen/include/godot_cpp/classes/*.hpp` for the exact GDExtension virtual signature instead.
- **AudioStream resources must be read-only; editable settings are `AudioStreamPlayer` parameters** (same pattern as `../GameMusicEmu-GDExtension`): declare them in the stream's `_get_parameter_list()`, store them in the playback instance, and handle them in the playback's `_set_parameter`/`_get_parameter`. They are read in `_start()` when the player is created (Godot sets parameters after instantiating the playback, before starting). Only content (`midi_data`, `bank_data`) is a stored stream property.
- Shared *synth* parameters are members of `AudioStreamPlaybackMIDIBase` (`audio_stream_midi_base.*`), bound both as regular properties and via `_set_parameter`/`_get_parameter`, and the playback is passed to `create_base_player()`; add new shared ones there and in `AudioStreamMIDIBase::_get_parameter_list()`. Parameters specific to one class (loop config, song number on `AudioStreamMIDI`) belong in that class's `_get_parameter_list()` and playback. `_on_config_changed()` remains the hook for invalidating caches when stream content changes.
- Godot-cpp inherits bound methods/properties across a *registered* C++ class chain automatically — `AudioStreamMIDIBase::_bind_methods()` only needs to run once; `AudioStreamMIDI`/`AudioStreamMIDISequencer` don't need to (and shouldn't) rebind inherited properties themselves. `GDREGISTER_ABSTRACT_CLASS` (not `GDREGISTER_CLASS`) is what registers a shared base like this without making it directly instantiable — every `GDCLASS`-declared class still needs its own `_bind_methods()` defined (even if empty) or the `GDCLASS` macro's `static_assert` fails at compile time.
- Enum-like settings (`volume_model`, `emulator`) are exposed as plain `int` properties with `PROPERTY_HINT_ENUM` hint strings rather than registered C++ enum types — simpler to wire up in godot-cpp and still gives an Inspector dropdown.
