#include "audio_stream_midi_base.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

Dictionary godot::make_stream_parameter(const String &p_name, Variant::Type p_type, PropertyHint p_hint, const String &p_hint_string, const Variant &p_default_value) {
	Dictionary parameter;
	parameter["name"] = p_name;
	parameter["class_name"] = StringName();
	parameter["type"] = static_cast<int>(p_type);
	parameter["hint"] = static_cast<int>(p_hint);
	parameter["hint_string"] = p_hint_string;
	parameter["usage"] = static_cast<int>(PROPERTY_USAGE_DEFAULT);
	parameter["default_value"] = p_default_value;
	return parameter;
}

// ============================== MidiSynthConfig ==============================

void MidiSynthConfig::append_parameters(TypedArray<Dictionary> &r_parameters) {
	const MidiSynthConfig defaults;
	r_parameters.push_back(make_stream_parameter("embedded_bank", Variant::INT, PROPERTY_HINT_RANGE, "0,255,1", defaults.embedded_bank_index));
	r_parameters.push_back(make_stream_parameter("num_chips", Variant::INT, PROPERTY_HINT_RANGE, "1,100,1", defaults.num_chips));
	r_parameters.push_back(make_stream_parameter("four_op_channels", Variant::INT, PROPERTY_HINT_RANGE, "-1,128,1", defaults.four_op_channel_total));
	r_parameters.push_back(make_stream_parameter("volume_model", Variant::INT, PROPERTY_HINT_ENUM, "Auto,Generic,Native OPL3,DMX,Apogee,9X,DMX Fixed,Apogee Fixed,AIL,9X Generic FM,HMI,HMI Old,MS AdLib,IMF Creator,O'Connell", defaults.volume_model_id));
	r_parameters.push_back(make_stream_parameter("emulator", Variant::INT, PROPERTY_HINT_ENUM, "Nuked,Nuked Fast,DosBox,Opal,Java,ESFMu,MAME OPL2,YMFM OPL2,YMFM OPL3,Nuked OPL2 LLE,Nuked OPL3 LLE,Nuked OPL2 Lite,Nuked CQM,DosBox OPL2", defaults.emulator_id));
	r_parameters.push_back(make_stream_parameter("full_range_brightness", Variant::BOOL, PROPERTY_HINT_NONE, "", defaults.is_full_range_brightness));
}

bool MidiSynthConfig::set_parameter(const StringName &p_name, const Variant &p_value) {
	if (p_name == StringName("embedded_bank")) {
		embedded_bank_index = p_value;
	} else if (p_name == StringName("num_chips")) {
		num_chips = p_value;
	} else if (p_name == StringName("four_op_channels")) {
		four_op_channel_total = p_value;
	} else if (p_name == StringName("volume_model")) {
		volume_model_id = p_value;
	} else if (p_name == StringName("emulator")) {
		emulator_id = p_value;
	} else if (p_name == StringName("full_range_brightness")) {
		is_full_range_brightness = p_value;
	} else {
		return false;
	}
	return true;
}

bool MidiSynthConfig::find_parameter(const StringName &p_name, Variant &r_value) const {
	if (p_name == StringName("embedded_bank")) {
		r_value = embedded_bank_index;
	} else if (p_name == StringName("num_chips")) {
		r_value = num_chips;
	} else if (p_name == StringName("four_op_channels")) {
		r_value = four_op_channel_total;
	} else if (p_name == StringName("volume_model")) {
		r_value = volume_model_id;
	} else if (p_name == StringName("emulator")) {
		r_value = emulator_id;
	} else if (p_name == StringName("full_range_brightness")) {
		r_value = is_full_range_brightness;
	} else {
		return false;
	}
	return true;
}

// ============================== AudioStreamMIDIBase ==============================

bool AudioStreamMIDIBase::_is_monophonic() const {
	return false;
}

TypedArray<Dictionary> AudioStreamMIDIBase::_get_parameter_list() const {
	TypedArray<Dictionary> parameters;
	MidiSynthConfig::append_parameters(parameters);
	return parameters;
}

ADL_MIDIPlayer *AudioStreamMIDIBase::create_base_player(long p_sample_rate, const MidiSynthConfig &p_config) const {
	ADL_MIDIPlayer *p = adl_init(p_sample_rate);
	if (!p) {
		UtilityFunctions::push_error("libADLMIDI: failed to initialize: ", String(adl_errorString()));
		return nullptr;
	}

	adl_setNumChips(p, p_config.num_chips);
	adl_setVolumeRangeModel(p, p_config.volume_model_id);
	adl_switchEmulator(p, p_config.emulator_id);
	adl_setNumFourOpsChn(p, p_config.four_op_channel_total);
	adl_setFullRangeBrightness(p, p_config.is_full_range_brightness ? 1 : 0);

	if (!bank_data.is_empty()) {
		if (adl_openBankData(p, bank_data.ptr(), (unsigned long)bank_data.size()) < 0) {
			UtilityFunctions::push_error("libADLMIDI: failed to load custom bank: ", String(adl_errorInfo(p)));
			adl_close(p);
			return nullptr;
		}
	} else {
		adl_setBank(p, p_config.embedded_bank_index);
	}

	return p;
}

void AudioStreamMIDIBase::set_bank_data(const PackedByteArray &p_data) {
	bank_data = p_data;
	_on_config_changed();
	emit_changed();
}

void AudioStreamMIDIBase::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_bank_data", "data"), &AudioStreamMIDIBase::set_bank_data);
	ClassDB::bind_method(D_METHOD("get_bank_data"), &AudioStreamMIDIBase::get_bank_data);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "bank_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_bank_data", "get_bank_data");
}
