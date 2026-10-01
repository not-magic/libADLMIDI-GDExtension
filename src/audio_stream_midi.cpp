#include "audio_stream_midi.h"

#include <adlmidi.h>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstring>

using namespace godot;

namespace {

MidiSynthConfig make_info_config() {
	MidiSynthConfig config;
	config.num_chips = 1;
	return config;
}

void close_info_player(ADL_MIDIPlayer *&r_player) {
	if (r_player) {
		adl_close(r_player);
		r_player = nullptr;
	}
}

ADL_MIDIPlayer *ensure_info_player(const AudioStreamMIDI &p_stream, ADL_MIDIPlayer *&r_player) {
	if (!r_player) {
		r_player = p_stream.create_player(ADL_CHIP_SAMPLE_RATE, make_info_config(), -1, true, -1);
	}
	return r_player;
}

} // namespace

// ============================== AudioStreamMIDI ==============================

AudioStreamMIDI::AudioStreamMIDI() {
}

AudioStreamMIDI::~AudioStreamMIDI() {
	close_info_player(info_player);
}

ADL_MIDIPlayer *AudioStreamMIDI::create_player(long p_sample_rate, const MidiSynthConfig &p_config, int p_song_number, bool p_use_loop, int p_loop_count) const {
	ADL_MIDIPlayer *const p = create_base_player(p_sample_rate, p_config);
	if (!p) {
		return nullptr;
	}

	adl_setLoopEnabled(p, p_use_loop ? 1 : 0);
	adl_setLoopCount(p, p_loop_count);

	if (!midi_data.is_empty()) {
		if (p_song_number >= 0) {
			adl_selectSongNum(p, p_song_number);
		}

		if (adl_openData(p, midi_data.ptr(), (unsigned long)midi_data.size()) < 0) {
			UtilityFunctions::push_error("AudioStreamMIDI: failed to load MIDI data: ", String(adl_errorInfo(p)));
			adl_close(p);
			return nullptr;
		}
	}

	return p;
}

Ref<AudioStreamPlayback> AudioStreamMIDI::_instantiate_playback() const {
	Ref<AudioStreamPlaybackMIDI> playback;
	playback.instantiate();
	playback->stream = Ref<AudioStreamMIDI>(const_cast<AudioStreamMIDI *>(this));
	return playback;
}

String AudioStreamMIDI::_get_stream_name() const {
	return "MIDI";
}

double AudioStreamMIDI::_get_length() const {
	ADL_MIDIPlayer *const p = ensure_info_player(*this, info_player);
	return p ? adl_totalTimeLength(p) : 0.0;
}

bool AudioStreamMIDI::_has_loop() const {
	return true;
}

TypedArray<Dictionary> AudioStreamMIDI::_get_parameter_list() const {
	TypedArray<Dictionary> parameters = AudioStreamMIDIBase::_get_parameter_list();
	parameters.push_back(make_stream_parameter("song_number", Variant::INT, PROPERTY_HINT_RANGE, "-1,255,1", -1));
	parameters.push_back(make_stream_parameter("loop_enabled", Variant::BOOL, PROPERTY_HINT_NONE, "", true));
	parameters.push_back(make_stream_parameter("loop_count", Variant::INT, PROPERTY_HINT_RANGE, "-1,100,1", -1));
	return parameters;
}

void AudioStreamMIDI::_on_config_changed() {
	close_info_player(info_player);
}

void AudioStreamMIDI::set_midi_data(const PackedByteArray &p_data) {
	midi_data = p_data;
	close_info_player(info_player);
	emit_changed();
}

String AudioStreamMIDI::get_title() const {
	ADL_MIDIPlayer *const p = ensure_info_player(*this, info_player);
	if (!p) {
		return String();
	}
	const char *title = adl_metaMusicTitle(p);
	return title ? String(title) : String();
}

String AudioStreamMIDI::get_copyright() const {

	// adl seems to crash here if midi_data is empty
	if (midi_data.is_empty()) {
		return String();
	}

	ADL_MIDIPlayer *const p = ensure_info_player(*this, info_player);
	if (!p) {
		return String();
	}
	const char *copyright = adl_metaMusicCopyright(p);
	return copyright ? String(copyright) : String();
}

int AudioStreamMIDI::get_track_count() const {
	ADL_MIDIPlayer *const p = ensure_info_player(*this, info_player);
	return p ? (int)adl_metaTrackTitleCount(p) : 0;
}

String AudioStreamMIDI::get_track_title(int p_index) const {
	ADL_MIDIPlayer *const p = ensure_info_player(*this, info_player);
	if (!p) {
		return String();
	}
	const char *title = adl_metaTrackTitle(p, (size_t)p_index);
	return title ? String(title) : String();
}

int AudioStreamMIDI::get_songs_count() const {
	ADL_MIDIPlayer *const p = ensure_info_player(*this, info_player);
	return p ? adl_getSongsCount(p) : 0;
}

double AudioStreamMIDI::get_loop_start_time() const {
	ADL_MIDIPlayer *const p = ensure_info_player(*this, info_player);
	return p ? adl_loopStartTime(p) : -1.0;
}

double AudioStreamMIDI::get_loop_end_time() const {
	ADL_MIDIPlayer *const p = ensure_info_player(*this, info_player);
	return p ? adl_loopEndTime(p) : -1.0;
}

int AudioStreamMIDI::get_bank_count() {
	return adl_getBanksCount();
}

String AudioStreamMIDI::get_bank_name(int p_index) {
	const char *const *names = adl_getBankNames();
	const int count = adl_getBanksCount();
	if (!names || p_index < 0 || p_index >= count) {
		return String();
	}
	return String(names[p_index]);
}

void AudioStreamMIDI::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_midi_data", "data"), &AudioStreamMIDI::set_midi_data);
	ClassDB::bind_method(D_METHOD("get_midi_data"), &AudioStreamMIDI::get_midi_data);

	ClassDB::bind_method(D_METHOD("get_title"), &AudioStreamMIDI::get_title);
	ClassDB::bind_method(D_METHOD("get_copyright"), &AudioStreamMIDI::get_copyright);
	ClassDB::bind_method(D_METHOD("get_track_count"), &AudioStreamMIDI::get_track_count);
	ClassDB::bind_method(D_METHOD("get_track_title", "index"), &AudioStreamMIDI::get_track_title);
	ClassDB::bind_method(D_METHOD("get_songs_count"), &AudioStreamMIDI::get_songs_count);
	ClassDB::bind_method(D_METHOD("get_loop_start_time"), &AudioStreamMIDI::get_loop_start_time);
	ClassDB::bind_method(D_METHOD("get_loop_end_time"), &AudioStreamMIDI::get_loop_end_time);

	ClassDB::bind_static_method("AudioStreamMIDI", D_METHOD("get_bank_count"), &AudioStreamMIDI::get_bank_count);
	ClassDB::bind_static_method("AudioStreamMIDI", D_METHOD("get_bank_name", "index"), &AudioStreamMIDI::get_bank_name);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "midi_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_midi_data", "get_midi_data");

	ADD_GROUP("Info", "");
	constexpr PropertyUsageFlags read_only_usage = static_cast<PropertyUsageFlags>(PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "title", PROPERTY_HINT_NONE, "", read_only_usage), "", "get_title");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "copyright", PROPERTY_HINT_NONE, "", read_only_usage), "", "get_copyright");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "track_count", PROPERTY_HINT_NONE, "", read_only_usage), "", "get_track_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "songs_count", PROPERTY_HINT_NONE, "", read_only_usage), "", "get_songs_count");
}

// =========================== AudioStreamPlaybackMIDI ===========================

AudioStreamPlaybackMIDI::AudioStreamPlaybackMIDI() {
}

AudioStreamPlaybackMIDI::~AudioStreamPlaybackMIDI() {
	if (player) {
		adl_close(player);
	}
}

void AudioStreamPlaybackMIDI::_start(double p_from_pos) {
	if (player) {
		adl_close(player);
		player = nullptr;
	}

	is_active = false;

	if (stream.is_valid()) {
		mix_rate = AudioServer::get_singleton()->get_mix_rate();
		player = stream->create_player((long)mix_rate, synth_config, song_number, use_loop, loop_count);
	}

	if (player) {
		is_active = true;
		if (p_from_pos > 0.0) {
			adl_positionSeek(player, p_from_pos);
		}
	}

	begin_resample();
}

void AudioStreamPlaybackMIDI::_stop() {
	if (player) {
		adl_close(player);
		player = nullptr;
	}
	is_active = false;
}

bool AudioStreamPlaybackMIDI::_is_playing() const {
	return is_active;
}

double AudioStreamPlaybackMIDI::_get_playback_position() const {
	return player ? adl_positionTell(player) : 0.0;
}

void AudioStreamPlaybackMIDI::_seek(double p_position) {
	if (player) {
		adl_positionSeek(player, p_position);
	}
}

int32_t AudioStreamPlaybackMIDI::_mix_resampled(AudioFrame *p_dst_buffer, int32_t p_frame_count) {
	if (!is_active || !player) {
		std::memset(p_dst_buffer, 0, sizeof(AudioFrame) * (size_t)p_frame_count);
		return p_frame_count;
	}

	ADLMIDI_AudioFormat format;
	format.type = ADLMIDI_SampleType_F32;
	format.containerSize = sizeof(float);
	format.sampleOffset = sizeof(AudioFrame);

	ADL_UInt8 *left = reinterpret_cast<ADL_UInt8 *>(&p_dst_buffer[0].left);
	ADL_UInt8 *right = reinterpret_cast<ADL_UInt8 *>(&p_dst_buffer[0].right);
	const int sample_count = p_frame_count * 2;

	const int got = adl_playFormat(player, sample_count, left, right, &format);

	const int frames_got = got / 2;
	if (frames_got < p_frame_count) {
		std::memset(p_dst_buffer + frames_got, 0, sizeof(AudioFrame) * (size_t)(p_frame_count - frames_got));
		is_active = false;
	}

	return p_frame_count;
}

float AudioStreamPlaybackMIDI::_get_stream_sampling_rate() const {
	return mix_rate;
}

void AudioStreamPlaybackMIDI::_set_parameter(const StringName &p_name, const Variant &p_value) {
	if (p_name == StringName("song_number")) {
		song_number = p_value;
	} else if (p_name == StringName("loop_enabled")) {
		use_loop = p_value;
	} else if (p_name == StringName("loop_count")) {
		loop_count = p_value;
	} else {
		synth_config.try_set_parameter(p_name, p_value);
	}
}

Variant AudioStreamPlaybackMIDI::_get_parameter(const StringName &p_name) const {
	if (p_name == StringName("song_number")) {
		return song_number;
	}
	if (p_name == StringName("loop_enabled")) {
		return use_loop;
	}
	if (p_name == StringName("loop_count")) {
		return loop_count;
	}
	Variant value;
	synth_config.try_get_parameter(p_name, value);
	return value;
}

double AudioStreamPlaybackMIDI::get_song_length() const {
	return player ? adl_totalTimeLength(player) : 0.0;
}

bool AudioStreamPlaybackMIDI::is_at_end() const {
	return player ? adl_atEnd(player) == 1 : false;
}

void AudioStreamPlaybackMIDI::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_song_length"), &AudioStreamPlaybackMIDI::get_song_length);
	ClassDB::bind_method(D_METHOD("is_at_end"), &AudioStreamPlaybackMIDI::is_at_end);
}
