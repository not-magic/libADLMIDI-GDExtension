# libADLMIDI-gdextension

A Godot 4 GDExtension that adds MIDI playback and real-time MIDI music generation, rendered as OPL3 FM synthesis audio via [libADLMIDI](https://github.com/Wohlstand/libADLMIDI).

It adds two new `AudioStream` types:

- `AudioStreamMIDI` — loads and plays `.mid`/`.midi` files through the normal Godot audio pipeline (`AudioStreamPlayer`, `AudioStreamPlayer2D`/`3D`) just like a `.wav` or `.ogg`.
- `AudioStreamMIDISequencer` — a live OPL3 synth with no fixed song, driven directly from code (note on/off, controller changes, pitch bend, ...) for procedurally generated music.

## Features

- Loads `.mid`, `.midi`, `.rmi`, `.xmi`, and `.kar` files directly as an `AudioStream` resource (`res://song.mid`).
- Plays through libADLMIDI's built-in MIDI sequencer, including loop point support (`loopStart`/`loopEnd` tags).
- `AudioStreamMIDISequencer` needs no file at all — drive its playback directly with `note_on`/`note_off`/`controller_change`/`pitch_bend`/... for generative/procedural music.
- Configurable synth: emulated OPL3 chip count, emulator core (Nuked, DOSBox, Opal, YMFM, ESFMu, MAME, ...), embedded or custom (WOPL) instrument bank, volume model, four-op channel count.
- Song metadata (on `AudioStreamMIDI`): title, copyright, track titles, track/song count, loop start/end time, total length.
- Multiple concurrent playbacks of the same stream resource each get their own independent synth state.

## Requirements

- Godot 4.5 or newer (bindings are generated against the Godot 4.5 API bundled with the `godot-cpp` submodule).
- [SCons](https://scons.org/) and a C++17-capable compiler to build the extension from source.
- Git (for submodules).

## Getting the code

```bash
git clone --recurse-submodules <this repo's URL>
# or, if you already cloned without submodules:
git submodule update --init --recursive
```

This pulls in the two dependencies as submodules:

- [`godot-cpp`](https://github.com/godotengine/godot-cpp) — the official C++ bindings for Godot's GDExtension API.
- [`libADLMIDI`](https://github.com/Wohlstand/libADLMIDI) — the OPL3 MIDI synthesizer library doing the actual sound generation.

## Building

```bash
scons platform=linux target=template_debug -j$(nproc)
```

Swap `platform=linux` for `windows` or `macos`, and use `target=template_release` for a release/export build. The first build compiles godot-cpp's full binding set and libADLMIDI from source, so it takes a while; rebuilds after that are incremental.

The result is written to `demo/addons/ADLMIDI/bin/`, matching the paths already referenced by `demo/addons/ADLMIDI/adlmidi.gdextension` (e.g. `demo/addons/ADLMIDI/bin/libADLMIDI.linux.template_debug.x86_64.so`).

See `CLAUDE.md` for more build/architecture detail.

## Using it in a Godot project

To use this in your own Godot project, copy `demo/addons/ADLMIDI/` into your project's `res://addons/` folder, then reload the project. `AudioStreamMIDI` and `AudioStreamMIDISequencer` will show up as normal resource types; the addon shows up (and can be enabled/disabled) under Project Settings > Plugins.

The `demo/` folder is itself a minimal Godot project with the addon already installed and enabled, useful as a starting point or for manually testing changes — open `demo/project.godot` in the editor after building.

### Playing a MIDI file

```gdscript
var player := AudioStreamPlayer.new()
add_child(player)
player.stream = load("res://song.mid")
player.play()
```

Or build the stream programmatically:

```gdscript
var stream := AudioStreamMIDI.new()
stream.midi_data = FileAccess.get_file_as_bytes("res://song.mid")
player.stream = stream
player.set("parameters/num_chips", 6)
player.set("parameters/emulator", 0) # Nuked
player.play()
```

Streams are read-only resources; synth and playback settings (`embedded_bank`, `num_chips`, `four_op_channels`, `volume_model`, `emulator`, `full_range_brightness`, and for `AudioStreamMIDI` also `song_number`, `loop_enabled`, `loop_count`) are `AudioStreamPlayer` parameters (`parameters/<name>`), read when playback starts.

### Generating music in real time

Use `AudioStreamMIDISequencer` and drive its playback directly — it behaves like a live OPL3 synth with no fixed song:

```gdscript
var stream := AudioStreamMIDISequencer.new()
player.stream = stream
player.play()

var playback := player.get_stream_playback() as AudioStreamPlaybackMIDISequencer
playback.note_on(0, 60, 100)   # channel 0, middle C, velocity 100
playback.note_off(0, 60)
```

### Inspecting a loaded song

```gdscript
var stream: AudioStreamMIDI = load("res://song.mid")
print(stream.get_title(), " - ", stream.get_length(), "s")
for i in stream.get_track_count():
    print(stream.get_track_title(i))
```

See `src/audio_stream_midi.h` for the full set of properties and methods on `AudioStreamMIDI`/`AudioStreamPlaybackMIDI`, and `src/audio_stream_midi_sequencer.h` for `AudioStreamMIDISequencer`/`AudioStreamPlaybackMIDISequencer`.

## Status

This extension builds and has been manually tested end-to-end in the Godot editor on Linux (see `demo/`). CI (`.github/workflows/build.yml`) builds and runs the standalone test suite (`tests/`) on Linux, Windows, and macOS on every push/PR, but the Windows/macOS builds haven't been manually exercised inside the Godot editor.

## Releases

Pushing a `v*` tag (or running the "Release" workflow manually with a tag) builds `template_debug` and `template_release` for Linux, Windows, and macOS, then publishes a GitHub Release with a zip of the ready-to-install `demo/addons/ADLMIDI/` folder (binaries for every platform included) attached. The zip contains `addons/ADLMIDI/`, so extract it into your project's root folder.

## License

This repository's own extension code is MIT licensed (see `LICENSE`). Its dependencies carry their own terms:

- `godot-cpp` is MIT licensed.
- `libADLMIDI` is licensed under a mix of LGPL 2.1+/2+, GPL v2+/v3+, MIT, BSD 3-Clause, and the Boost Software License, varying by component (embedded FM banks carry their own terms too) — see [libADLMIDI's README](https://github.com/Wohlstand/libADLMIDI#license) for the full breakdown before redistributing a build of this extension.
