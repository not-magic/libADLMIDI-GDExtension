#pragma once

#include <adlmidi.h>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_playback_resampled.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {

Dictionary make_stream_parameter(const String &p_name, Variant::Type p_type, PropertyHint p_hint, const String &p_hint_string, const Variant &p_default_value);

// Abstract base of the playback classes: holds the synth settings and exposes
// them both as AudioStreamPlayer parameters and as regular properties.
class AudioStreamPlaybackMIDIBase : public AudioStreamPlaybackResampled {
	GDCLASS(AudioStreamPlaybackMIDIBase, AudioStreamPlaybackResampled) // NOLINT

	int embedded_bank_index = 0;
	int num_chips = 4;
	int four_op_channel_total = -1;
	int volume_model_id = ADLMIDI_VolumeModel_AUTO;
	int emulator_id = ADLMIDI_EMU_NUKED;
	bool use_full_range_brightness = false;

protected:
	static void _bind_methods();

public:
	virtual void _set_parameter(const StringName &p_name, const Variant &p_value) override;
	virtual Variant _get_parameter(const StringName &p_name) const override;

	void set_embedded_bank(int p_bank_index) { embedded_bank_index = p_bank_index; }
	int get_embedded_bank() const { return embedded_bank_index; }
	void set_num_chips(int p_chip_total) { num_chips = p_chip_total; }
	int get_num_chips() const { return num_chips; }
	void set_four_op_channels(int p_channel_total) { four_op_channel_total = p_channel_total; }
	int get_four_op_channels() const { return four_op_channel_total; }
	void set_volume_model(int p_model_id) { volume_model_id = p_model_id; }
	int get_volume_model() const { return volume_model_id; }
	void set_emulator(int p_emulator_id) { emulator_id = p_emulator_id; }
	int get_emulator() const { return emulator_id; }
	void set_full_range_brightness(bool p_is_enabled) { use_full_range_brightness = p_is_enabled; }
	bool is_full_range_brightness() const { return use_full_range_brightness; }
};

// Abstract base of AudioStreamMIDI and AudioStreamMIDISequencer: custom bank
// data and the shared libADLMIDI player setup.
class AudioStreamMIDIBase : public AudioStream {
	GDCLASS(AudioStreamMIDIBase, AudioStream) // NOLINT

	PackedByteArray bank_data;

protected:
	static void _bind_methods();

	virtual void _on_config_changed() {}

	// Loads no sequence data; returns nullptr (and logs) on failure.
	ADL_MIDIPlayer *create_base_player(long p_sample_rate, const AudioStreamPlaybackMIDIBase &p_config) const;

public:
	virtual bool _is_monophonic() const override;
	virtual TypedArray<Dictionary> _get_parameter_list() const override;

	void set_bank_data(const PackedByteArray &p_data);
	PackedByteArray get_bank_data() const { return bank_data; }
};

} // namespace godot
