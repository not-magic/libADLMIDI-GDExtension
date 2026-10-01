#include "audio_stream_midi_base.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace {

constexpr const char *volume_model_hint = "Auto,Generic,Native OPL3,DMX,Apogee,9X,DMX Fixed,Apogee Fixed,AIL,9X Generic FM,HMI,HMI Old,MS AdLib,IMF Creator,O'Connell";
constexpr const char *emulator_hint = "Nuked,Nuked Fast,DosBox,Opal,Java,ESFMu,MAME OPL2,YMFM OPL2,YMFM OPL3,Nuked OPL2 LLE,Nuked OPL3 LLE,Nuked OPL2 Lite,Nuked CQM,DosBox OPL2";

} // namespace

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

// ========================== AudioStreamPlaybackMIDIBase ==========================

void AudioStreamPlaybackMIDIBase::_set_parameter(const StringName &p_name, const Variant &p_value) {
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
		use_full_range_brightness = p_value;
	}
}

Variant AudioStreamPlaybackMIDIBase::_get_parameter(const StringName &p_name) const {
	if (p_name == StringName("embedded_bank")) {
		return embedded_bank_index;
	}
	if (p_name == StringName("num_chips")) {
		return num_chips;
	}
	if (p_name == StringName("four_op_channels")) {
		return four_op_channel_total;
	}
	if (p_name == StringName("volume_model")) {
		return volume_model_id;
	}
	if (p_name == StringName("emulator")) {
		return emulator_id;
	}
	if (p_name == StringName("full_range_brightness")) {
		return use_full_range_brightness;
	}
	return Variant();
}

void AudioStreamPlaybackMIDIBase::_bind_methods() {
	using Self = AudioStreamPlaybackMIDIBase;
	ClassDB::bind_method(D_METHOD("set_embedded_bank", "bank_index"), &Self::set_embedded_bank);
	ClassDB::bind_method(D_METHOD("get_embedded_bank"), &Self::get_embedded_bank);
	ClassDB::bind_method(D_METHOD("set_num_chips", "chip_total"), &Self::set_num_chips);
	ClassDB::bind_method(D_METHOD("get_num_chips"), &Self::get_num_chips);
	ClassDB::bind_method(D_METHOD("set_four_op_channels", "channel_total"), &Self::set_four_op_channels);
	ClassDB::bind_method(D_METHOD("get_four_op_channels"), &Self::get_four_op_channels);
	ClassDB::bind_method(D_METHOD("set_volume_model", "model_id"), &Self::set_volume_model);
	ClassDB::bind_method(D_METHOD("get_volume_model"), &Self::get_volume_model);
	ClassDB::bind_method(D_METHOD("set_emulator", "emulator_id"), &Self::set_emulator);
	ClassDB::bind_method(D_METHOD("get_emulator"), &Self::get_emulator);
	ClassDB::bind_method(D_METHOD("set_full_range_brightness", "is_enabled"), &Self::set_full_range_brightness);
	ClassDB::bind_method(D_METHOD("is_full_range_brightness"), &Self::is_full_range_brightness);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "embedded_bank", PROPERTY_HINT_RANGE, "0,255,1"), "set_embedded_bank", "get_embedded_bank");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_chips", PROPERTY_HINT_RANGE, "1,100,1"), "set_num_chips", "get_num_chips");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "four_op_channels", PROPERTY_HINT_RANGE, "-1,128,1"), "set_four_op_channels", "get_four_op_channels");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "volume_model", PROPERTY_HINT_ENUM, volume_model_hint), "set_volume_model", "get_volume_model");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emulator", PROPERTY_HINT_ENUM, emulator_hint), "set_emulator", "get_emulator");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "full_range_brightness"), "set_full_range_brightness", "is_full_range_brightness");
}

// ============================== AudioStreamMIDIBase ==============================

bool AudioStreamMIDIBase::_is_monophonic() const {
	return false;
}

TypedArray<Dictionary> AudioStreamMIDIBase::_get_parameter_list() const {
	TypedArray<Dictionary> parameters;
	parameters.push_back(make_stream_parameter("embedded_bank", Variant::INT, PROPERTY_HINT_RANGE, "0,255,1", 0));
	parameters.push_back(make_stream_parameter("num_chips", Variant::INT, PROPERTY_HINT_RANGE, "1,100,1", 4));
	parameters.push_back(make_stream_parameter("four_op_channels", Variant::INT, PROPERTY_HINT_RANGE, "-1,128,1", -1));
	parameters.push_back(make_stream_parameter("volume_model", Variant::INT, PROPERTY_HINT_ENUM, volume_model_hint, ADLMIDI_VolumeModel_AUTO));
	parameters.push_back(make_stream_parameter("emulator", Variant::INT, PROPERTY_HINT_ENUM, emulator_hint, ADLMIDI_EMU_NUKED));
	parameters.push_back(make_stream_parameter("full_range_brightness", Variant::BOOL, PROPERTY_HINT_NONE, "", false));
	return parameters;
}

ADL_MIDIPlayer *AudioStreamMIDIBase::create_base_player(long p_sample_rate, const AudioStreamPlaybackMIDIBase &p_config) const {
	ADL_MIDIPlayer *const p = adl_init(p_sample_rate);
	if (!p) {
		UtilityFunctions::push_error("libADLMIDI: failed to initialize: ", String(adl_errorString()));
		return nullptr;
	}

	adl_setNumChips(p, p_config.get_num_chips());
	adl_setVolumeRangeModel(p, p_config.get_volume_model());
	adl_switchEmulator(p, p_config.get_emulator());
	adl_setNumFourOpsChn(p, p_config.get_four_op_channels());
	adl_setFullRangeBrightness(p, p_config.is_full_range_brightness() ? 1 : 0);

	if (!bank_data.is_empty()) {
		if (adl_openBankData(p, bank_data.ptr(), (unsigned long)bank_data.size()) < 0) {
			UtilityFunctions::push_error("libADLMIDI: failed to load custom bank: ", String(adl_errorInfo(p)));
			adl_close(p);
			return nullptr;
		}
	} else {
		adl_setBank(p, p_config.get_embedded_bank());
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
