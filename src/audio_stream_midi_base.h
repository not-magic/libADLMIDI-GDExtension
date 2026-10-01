#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <adlmidi.h>

namespace godot {

Dictionary make_stream_parameter(const String &p_name, Variant::Type p_type, PropertyHint p_hint, const String &p_hint_string, const Variant &p_default_value);

// Chip/bank/emulator settings. Exposed as AudioStreamPlayer parameters and
// held by each playback instance; the streams themselves stay read-only.
struct MidiSynthConfig {
	int embedded_bank_index = 0;
	int num_chips = 4;
	int four_op_channel_total = -1;
	int volume_model_id = ADLMIDI_VolumeModel_AUTO;
	int emulator_id = ADLMIDI_EMU_NUKED;
	bool is_full_range_brightness = false;

	static void append_parameters(TypedArray<Dictionary> &r_parameters);
	bool set_parameter(const StringName &p_name, const Variant &p_value);
	bool find_parameter(const StringName &p_name, Variant &r_value) const;
};

// Shared functionality between AudioStreamMIDI (plays a fixed song) and
// AudioStreamMIDISequencer (a live synth driven with no song loaded): the
// custom bank data and the libADLMIDI player setup. Registered as an
// abstract class — not meant to be instantiated directly.
class AudioStreamMIDIBase : public AudioStream {
	GDCLASS(AudioStreamMIDIBase, AudioStream)

	PackedByteArray bank_data;

protected:
	static void _bind_methods();

	virtual void _on_config_changed() {}

	// Returns nullptr (and logs an error) if libADLMIDI failed to initialize
	// or load the custom bank. Does not load any MIDI sequence data.
	ADL_MIDIPlayer *create_base_player(long p_sample_rate, const MidiSynthConfig &p_config) const;

public:
	virtual bool _is_monophonic() const override;
	virtual TypedArray<Dictionary> _get_parameter_list() const override;

	void set_bank_data(const PackedByteArray &p_data);
	PackedByteArray get_bank_data() const { return bank_data; }
};

} // namespace godot
