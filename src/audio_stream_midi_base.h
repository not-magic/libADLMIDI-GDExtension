#pragma once

#include <adlmidi.h>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {

Dictionary make_stream_parameter(const String &p_name, Variant::Type p_type, PropertyHint p_hint, const String &p_hint_string, const Variant &p_default_value);

// Synth settings, exposed as AudioStreamPlayer parameters and held per playback.
struct MidiSynthConfig {
	int embedded_bank_index = 0;
	int num_chips = 4;
	int four_op_channel_total = -1;
	int volume_model_id = ADLMIDI_VolumeModel_AUTO;
	int emulator_id = ADLMIDI_EMU_NUKED;
	bool use_full_range_brightness = false;

	static void append_parameters(TypedArray<Dictionary> &r_parameters);
	bool try_set_parameter(const StringName &p_name, const Variant &p_value);
	bool try_get_parameter(const StringName &p_name, Variant &r_value) const;
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
	ADL_MIDIPlayer *create_base_player(long p_sample_rate, const MidiSynthConfig &p_config) const;

public:
	virtual bool _is_monophonic() const override;
	virtual TypedArray<Dictionary> _get_parameter_list() const override;

	void set_bank_data(const PackedByteArray &p_data);
	PackedByteArray get_bank_data() const { return bank_data; }
};

} // namespace godot
