#include "audio_stream_midi_sequencer.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstring>

using namespace godot;

// =========================== AudioStreamMIDISequencer ===========================

ADL_MIDIPlayer *AudioStreamMIDISequencer::create_player(long p_sample_rate, const MidiSynthConfig &p_config) const {
	return create_base_player(p_sample_rate, p_config);
}

Ref<AudioStreamPlayback> AudioStreamMIDISequencer::_instantiate_playback() const {
	Ref<AudioStreamPlaybackMIDISequencer> playback;
	playback.instantiate();
	playback->stream = Ref<AudioStreamMIDISequencer>(const_cast<AudioStreamMIDISequencer *>(this));
	return playback;
}

String AudioStreamMIDISequencer::_get_stream_name() const {
	return "MIDI Sequencer";
}

double AudioStreamMIDISequencer::_get_length() const {
	return 0.0;
}

// ======================= AudioStreamPlaybackMIDISequencer =======================

AudioStreamPlaybackMIDISequencer::AudioStreamPlaybackMIDISequencer() {
}

AudioStreamPlaybackMIDISequencer::~AudioStreamPlaybackMIDISequencer() {
	if (player) {
		adl_close(player);
	}
}

void AudioStreamPlaybackMIDISequencer::_start(double p_from_pos) {
	if (player) {
		adl_close(player);
		player = nullptr;
	}

	is_active = false;
	scheduler.set_player(nullptr);
	scheduler.reset();

	if (stream.is_valid()) {
		mix_rate = AudioServer::get_singleton()->get_mix_rate();
		player = stream->create_player((long)mix_rate, synth_config);
	}

	scheduler.set_player(player);
	is_active = player != nullptr;

	begin_resample();
}

void AudioStreamPlaybackMIDISequencer::_stop() {
	if (player) {
		adl_close(player);
		player = nullptr;
	}
	scheduler.set_player(nullptr);
	is_active = false;
	scheduler.reset();
}

bool AudioStreamPlaybackMIDISequencer::_is_playing() const {
	return is_active;
}

int32_t AudioStreamPlaybackMIDISequencer::_mix_resampled(AudioFrame *p_dst_buffer, int32_t p_frame_count) {
	if (!is_active || !player) {
		std::memset(p_dst_buffer, 0, sizeof(AudioFrame) * (size_t)p_frame_count);
		return p_frame_count;
	}

	scheduler.mix(reinterpret_cast<MidiScheduler::AudioFrame *>(p_dst_buffer), p_frame_count);

	return p_frame_count;
}

float AudioStreamPlaybackMIDISequencer::_get_stream_sampling_rate() const {
	return mix_rate;
}

void AudioStreamPlaybackMIDISequencer::_set_parameter(const StringName &p_name, const Variant &p_value) {
	synth_config.set_parameter(p_name, p_value);
}

Variant AudioStreamPlaybackMIDISequencer::_get_parameter(const StringName &p_name) const {
	Variant value;
	synth_config.find_parameter(p_name, value);
	return value;
}

void AudioStreamPlaybackMIDISequencer::_report_if_discarded(bool p_queued, int p_time) const {
	if (!p_queued) {
		UtilityFunctions::push_error(
				"AudioStreamPlaybackMIDISequencer: discarding message scheduled at frame ", p_time,
				", which is before the current frame ", scheduler.get_current_frame(), ".");
	}
}

void AudioStreamPlaybackMIDISequencer::note_on(int p_time, int p_channel, int p_note, int p_velocity) {
	_report_if_discarded(scheduler.note_on(p_time, p_channel, p_note, p_velocity), p_time);
}

void AudioStreamPlaybackMIDISequencer::note_off(int p_time, int p_channel, int p_note) {
	_report_if_discarded(scheduler.note_off(p_time, p_channel, p_note), p_time);
}

void AudioStreamPlaybackMIDISequencer::note_after_touch(int p_time, int p_channel, int p_note, int p_value) {
	_report_if_discarded(scheduler.note_after_touch(p_time, p_channel, p_note, p_value), p_time);
}

void AudioStreamPlaybackMIDISequencer::channel_after_touch(int p_time, int p_channel, int p_value) {
	_report_if_discarded(scheduler.channel_after_touch(p_time, p_channel, p_value), p_time);
}

void AudioStreamPlaybackMIDISequencer::controller_change(int p_time, int p_channel, int p_controller, int p_value) {
	_report_if_discarded(scheduler.controller_change(p_time, p_channel, p_controller, p_value), p_time);
}

void AudioStreamPlaybackMIDISequencer::patch_change(int p_time, int p_channel, int p_patch) {
	_report_if_discarded(scheduler.patch_change(p_time, p_channel, p_patch), p_time);
}

void AudioStreamPlaybackMIDISequencer::pitch_bend(int p_time, int p_channel, int p_value) {
	_report_if_discarded(scheduler.pitch_bend(p_time, p_channel, p_value), p_time);
}

void AudioStreamPlaybackMIDISequencer::panic(int p_time) {
	_report_if_discarded(scheduler.panic(p_time), p_time);
}

void AudioStreamPlaybackMIDISequencer::reset_state(int p_time) {
	_report_if_discarded(scheduler.reset_state(p_time), p_time);
}

int AudioStreamPlaybackMIDISequencer::get_current_time() const {
	return scheduler.get_current_frame();
}

void AudioStreamPlaybackMIDISequencer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("note_on", "time", "channel", "note", "velocity"), &AudioStreamPlaybackMIDISequencer::note_on);
	ClassDB::bind_method(D_METHOD("note_off", "time", "channel", "note"), &AudioStreamPlaybackMIDISequencer::note_off);
	ClassDB::bind_method(D_METHOD("note_after_touch", "time", "channel", "note", "value"), &AudioStreamPlaybackMIDISequencer::note_after_touch);
	ClassDB::bind_method(D_METHOD("channel_after_touch", "time", "channel", "value"), &AudioStreamPlaybackMIDISequencer::channel_after_touch);
	ClassDB::bind_method(D_METHOD("controller_change", "time", "channel", "controller", "value"), &AudioStreamPlaybackMIDISequencer::controller_change);
	ClassDB::bind_method(D_METHOD("patch_change", "time", "channel", "patch"), &AudioStreamPlaybackMIDISequencer::patch_change);
	ClassDB::bind_method(D_METHOD("pitch_bend", "time", "channel", "value"), &AudioStreamPlaybackMIDISequencer::pitch_bend);
	ClassDB::bind_method(D_METHOD("panic", "time"), &AudioStreamPlaybackMIDISequencer::panic);
	ClassDB::bind_method(D_METHOD("reset_state", "time"), &AudioStreamPlaybackMIDISequencer::reset_state);

	ClassDB::bind_method(D_METHOD("get_current_time"), &AudioStreamPlaybackMIDISequencer::get_current_time);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "current_time"), "", "get_current_time");
}
