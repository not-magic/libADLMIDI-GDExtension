#pragma once

#include "audio_stream_midi_base.h"
#include "midi_scheduler.h"

#include <godot_cpp/classes/audio_stream_playback_resampled.hpp>

#include <adlmidi.h>

namespace godot {

class AudioStreamPlaybackMIDISequencer;

// A live OPL3 synth with no fixed song. Chip/bank/emulator settings are
// AudioStreamPlayer parameters inherited from AudioStreamMIDIBase. Each
// playback instance gets its own ADL_MIDIPlayer, driven entirely through
// AudioStreamPlaybackMIDISequencer's note_on/note_off/... methods, for
// procedurally generated music.
class AudioStreamMIDISequencer : public AudioStreamMIDIBase {
	GDCLASS(AudioStreamMIDISequencer, AudioStreamMIDIBase)

protected:
	// Nothing of its own to bind: everything lives on AudioStreamMIDIBase.
	static void _bind_methods() {}

public:
	// Returns nullptr if libADLMIDI failed to initialize or load the bank.
	ADL_MIDIPlayer *create_player(long p_sample_rate, const MidiSynthConfig &p_config) const;

	virtual Ref<AudioStreamPlayback> _instantiate_playback() const override;
	virtual String _get_stream_name() const override;
	virtual double _get_length() const override;
};

// Per-playback instance: owns its own ADL_MIDIPlayer running in real-time
// mode (adl_generateFormat, no loaded sequence), wrapping a MidiScheduler
// (src/midi_scheduler.h) that does the actual queueing/scheduling/dispatch
// work. MidiScheduler has no Godot dependency and is unit tested directly
// (tests/runtime_scheduling_test.cpp); this class is just the GDExtension-
// facing glue around it: player lifecycle, method binding, and turning a
// rejected (too-late) message into a logged error.
class AudioStreamPlaybackMIDISequencer : public AudioStreamPlaybackResampled {
	GDCLASS(AudioStreamPlaybackMIDISequencer, AudioStreamPlaybackResampled)

	friend class AudioStreamMIDISequencer;

	Ref<AudioStreamMIDISequencer> stream;
	ADL_MIDIPlayer *player = nullptr;
	MidiSynthConfig synth_config;
	MidiScheduler scheduler;
	float mix_rate = 44100.0f;
	bool is_active = false;

	void _report_if_discarded(bool p_queued, int p_time) const;

protected:
	static void _bind_methods();

public:
	AudioStreamPlaybackMIDISequencer();
	~AudioStreamPlaybackMIDISequencer();

	virtual void _start(double p_from_pos) override;
	virtual void _stop() override;
	virtual bool _is_playing() const override;
	virtual int32_t _mix_resampled(AudioFrame *p_dst_buffer, int32_t p_frame_count) override;
	virtual float _get_stream_sampling_rate() const override;
	virtual void _set_parameter(const StringName &p_name, const Variant &p_value) override;
	virtual Variant _get_parameter(const StringName &p_name) const override;

	void note_on(int p_time, int p_channel, int p_note, int p_velocity);
	void note_off(int p_time, int p_channel, int p_note);
	void note_after_touch(int p_time, int p_channel, int p_note, int p_value);
	void channel_after_touch(int p_time, int p_channel, int p_value);
	void controller_change(int p_time, int p_channel, int p_controller, int p_value);
	void patch_change(int p_time, int p_channel, int p_patch);
	void pitch_bend(int p_time, int p_channel, int p_value);
	void panic(int p_time);
	void reset_state(int p_time);

	int get_current_time() const;
};

} // namespace godot
