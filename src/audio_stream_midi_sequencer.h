#pragma once

#include "audio_stream_midi_base.h"
#include "midi_scheduler.h"

#include <godot_cpp/classes/audio_stream_playback_resampled.hpp>

struct ADL_MIDIPlayer;

namespace godot {

class AudioStreamPlaybackMIDISequencer;

// A live OPL3 synth with no fixed song, driven through its playback's
// note_on/note_off/... methods.
class AudioStreamMIDISequencer : public AudioStreamMIDIBase {
	GDCLASS(AudioStreamMIDISequencer, AudioStreamMIDIBase) // NOLINT

protected:
	static void _bind_methods() {}

public:
	ADL_MIDIPlayer *create_player(long p_sample_rate, const MidiSynthConfig &p_config) const;

	virtual Ref<AudioStreamPlayback> _instantiate_playback() const override;
	virtual String _get_stream_name() const override;
	virtual double _get_length() const override;
};

// Owns its own ADL_MIDIPlayer in real-time mode, wrapped by a MidiScheduler.
class AudioStreamPlaybackMIDISequencer : public AudioStreamPlaybackResampled {
	GDCLASS(AudioStreamPlaybackMIDISequencer, AudioStreamPlaybackResampled) // NOLINT

	friend class AudioStreamMIDISequencer;

	Ref<AudioStreamMIDISequencer> stream;
	ADL_MIDIPlayer *player = nullptr;
	MidiSynthConfig synth_config;
	MidiScheduler scheduler;
	float mix_rate = 44100.0f;
	bool is_active = false;

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

	void note_on(int p_frame_index, int p_channel_index, int p_note_index, int p_velocity);
	void note_off(int p_frame_index, int p_channel_index, int p_note_index);
	void note_after_touch(int p_frame_index, int p_channel_index, int p_note_index, int p_value);
	void channel_after_touch(int p_frame_index, int p_channel_index, int p_value);
	void controller_change(int p_frame_index, int p_channel_index, int p_controller_id, int p_value);
	void patch_change(int p_frame_index, int p_channel_index, int p_patch_index);
	void pitch_bend(int p_frame_index, int p_channel_index, int p_value);
	void panic(int p_frame_index);
	void reset_state(int p_frame_index);

	int get_current_time() const;
};

} // namespace godot
