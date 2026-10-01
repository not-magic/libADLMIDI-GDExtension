#pragma once

#include "audio_stream_midi_base.h"

#include <godot_cpp/variant/packed_byte_array.hpp>

struct ADL_MIDIPlayer;

namespace godot {

class AudioStreamPlaybackMIDI;

// A MIDI song rendered through libADLMIDI. Read-only: editable settings are
// AudioStreamPlayer parameters (see _get_parameter_list).
class AudioStreamMIDI : public AudioStreamMIDIBase {
	GDCLASS(AudioStreamMIDI, AudioStreamMIDIBase) // NOLINT

	friend class AudioStreamPlaybackMIDI;

	PackedByteArray midi_data;

	mutable ADL_MIDIPlayer *info_player = nullptr;

protected:
	static void _bind_methods();
	virtual void _on_config_changed() override;

public:
	AudioStreamMIDI();
	~AudioStreamMIDI();

	ADL_MIDIPlayer *create_player(long p_sample_rate, const AudioStreamPlaybackMIDIBase &p_config, int p_song_number, bool p_use_loop, int p_loop_count) const;

	virtual Ref<AudioStreamPlayback> _instantiate_playback() const override;
	virtual String _get_stream_name() const override;
	virtual double _get_length() const override;
	virtual bool _has_loop() const override;
	virtual TypedArray<Dictionary> _get_parameter_list() const override;

	void set_midi_data(const PackedByteArray &p_data);
	PackedByteArray get_midi_data() const { return midi_data; }

	String get_title() const;
	String get_copyright() const;
	int get_track_count() const;
	String get_track_title(int p_index) const;
	int get_songs_count() const;
	double get_loop_start_time() const;
	double get_loop_end_time() const;

	static int get_bank_count();
	static String get_bank_name(int p_index);
};

// Owns its own ADL_MIDIPlayer, so one AudioStreamMIDI can play concurrently.
class AudioStreamPlaybackMIDI : public AudioStreamPlaybackMIDIBase {
	GDCLASS(AudioStreamPlaybackMIDI, AudioStreamPlaybackMIDIBase) // NOLINT

	friend class AudioStreamMIDI;

	Ref<AudioStreamMIDI> stream;
	ADL_MIDIPlayer *player = nullptr;
	float mix_rate = 44100.0f;
	int song_number = -1;
	int loop_count = -1;
	bool use_loop = true;
	bool is_active = false;

protected:
	static void _bind_methods();

public:
	AudioStreamPlaybackMIDI();
	~AudioStreamPlaybackMIDI();

	virtual void _start(double p_from_pos) override;
	virtual void _stop() override;
	virtual bool _is_playing() const override;
	virtual double _get_playback_position() const override;
	virtual void _seek(double p_position) override;
	virtual int32_t _mix_resampled(AudioFrame *p_dst_buffer, int32_t p_frame_count) override;
	virtual float _get_stream_sampling_rate() const override;
	virtual void _set_parameter(const StringName &p_name, const Variant &p_value) override;
	virtual Variant _get_parameter(const StringName &p_name) const override;

	void set_song_number(int p_song_index) { song_number = p_song_index; }
	int get_song_number() const { return song_number; }
	void set_loop_enabled(bool p_is_enabled) { use_loop = p_is_enabled; }
	bool is_loop_enabled() const { return use_loop; }
	void set_loop_count(int p_loop_total) { loop_count = p_loop_total; }
	int get_loop_count() const { return loop_count; }

	double get_song_length() const;
	bool is_at_end() const;
};

} // namespace godot
