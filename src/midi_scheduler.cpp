#include "midi_scheduler.h"

#include <algorithm>

namespace {

bool try_queue_message(std::deque<MidiScheduler::QueuedMessage> &r_queue, bool &r_is_dirty, int p_current_frame_index, int p_frame_index, MidiScheduler::MessageType p_type, const MidiScheduler::MessageParams &p_params) {
	if (p_frame_index < p_current_frame_index) {
		return false;
	}

	r_queue.push_back({ p_params, p_type, p_frame_index });
	r_is_dirty = true;
	return true;
}

void dispatch_message(ADL_MIDIPlayer *p_player, const MidiScheduler::QueuedMessage &p_message) {
	switch (p_message.type) {
		case MidiScheduler::MessageType::NOTE_ON: {
			const MidiScheduler::NoteOnParams &p = p_message.params.note_on;
			adl_rt_noteOn(p_player, p.channel_index, p.note_index, p.velocity);
		} break;
		case MidiScheduler::MessageType::NOTE_OFF: {
			const MidiScheduler::NoteOffParams &p = p_message.params.note_off;
			adl_rt_noteOff(p_player, p.channel_index, p.note_index);
		} break;
		case MidiScheduler::MessageType::NOTE_AFTER_TOUCH: {
			const MidiScheduler::NoteAfterTouchParams &p = p_message.params.note_after_touch;
			adl_rt_noteAfterTouch(p_player, p.channel_index, p.note_index, p.value);
		} break;
		case MidiScheduler::MessageType::CHANNEL_AFTER_TOUCH: {
			const MidiScheduler::ChannelAfterTouchParams &p = p_message.params.channel_after_touch;
			adl_rt_channelAfterTouch(p_player, p.channel_index, p.value);
		} break;
		case MidiScheduler::MessageType::CONTROLLER_CHANGE: {
			const MidiScheduler::ControllerChangeParams &p = p_message.params.controller_change;
			adl_rt_controllerChange(p_player, p.channel_index, p.controller_id, p.value);
		} break;
		case MidiScheduler::MessageType::PATCH_CHANGE: {
			const MidiScheduler::PatchChangeParams &p = p_message.params.patch_change;
			adl_rt_patchChange(p_player, p.channel_index, p.patch_index);
		} break;
		case MidiScheduler::MessageType::PITCH_BEND: {
			const MidiScheduler::PitchBendParams &p = p_message.params.pitch_bend;
			adl_rt_pitchBend(p_player, p.channel_index, p.value);
		} break;
		case MidiScheduler::MessageType::PANIC:
			adl_panic(p_player);
			break;
		case MidiScheduler::MessageType::RESET_STATE:
			adl_rt_resetState(p_player);
			break;
	}
}

} // namespace

void MidiScheduler::reset() {
	current_frame_index = 0;
	message_queue.clear();
	is_message_queue_dirty = false;
}

bool MidiScheduler::try_mix(AudioFrame *p_dst_buffer, int p_frame_count) {
	if (!player) {
		return false;
	}

	if (is_message_queue_dirty) {
		std::sort(message_queue.begin(), message_queue.end(), [](const QueuedMessage &p_a, const QueuedMessage &p_b) {
			return p_a.frame_index < p_b.frame_index;
		});
		is_message_queue_dirty = false;
	}

	ADLMIDI_AudioFormat format;
	format.type = ADLMIDI_SampleType_F32;
	format.containerSize = sizeof(float);
	format.sampleOffset = sizeof(AudioFrame);

	const int batch_end_frame_index = current_frame_index + p_frame_count;
	int filled_frame_total = 0;

	auto process_until = [&](int p_end_frame_index) {
		const int process_frame_total = p_end_frame_index - current_frame_index;
		if (process_frame_total <= 0) {
			return;
		}

		AudioFrame *segment_dst = p_dst_buffer + filled_frame_total;
		ADL_UInt8 *left = reinterpret_cast<ADL_UInt8 *>(&segment_dst[0].left);
		ADL_UInt8 *right = reinterpret_cast<ADL_UInt8 *>(&segment_dst[0].right);

		const int generated_frame_total = adl_generateFormat(player, process_frame_total * 2, left, right, &format) / 2;
		filled_frame_total += generated_frame_total;
		current_frame_index += generated_frame_total;
	};

	while (!message_queue.empty() && message_queue.front().frame_index < batch_end_frame_index) {
		const QueuedMessage message = message_queue.front();
		process_until(message.frame_index);
		dispatch_message(player, message);
		if (dispatch_callback) {
			dispatch_callback(dispatch_callback_userdata, message, current_frame_index);
		}
		message_queue.pop_front();
	}

	process_until(batch_end_frame_index);

	return true;
}

bool MidiScheduler::try_note_on(int p_frame_index, int p_channel_index, int p_note_index, int p_velocity) {
	MessageParams params{};
	params.note_on = { (ADL_UInt8)p_channel_index, (ADL_UInt8)p_note_index, (ADL_UInt8)p_velocity };
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::NOTE_ON, params);
}

bool MidiScheduler::try_note_off(int p_frame_index, int p_channel_index, int p_note_index) {
	MessageParams params{};
	params.note_off = { (ADL_UInt8)p_channel_index, (ADL_UInt8)p_note_index };
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::NOTE_OFF, params);
}

bool MidiScheduler::try_note_after_touch(int p_frame_index, int p_channel_index, int p_note_index, int p_value) {
	MessageParams params{};
	params.note_after_touch = { (ADL_UInt8)p_channel_index, (ADL_UInt8)p_note_index, (ADL_UInt8)p_value };
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::NOTE_AFTER_TOUCH, params);
}

bool MidiScheduler::try_channel_after_touch(int p_frame_index, int p_channel_index, int p_value) {
	MessageParams params{};
	params.channel_after_touch = { (ADL_UInt8)p_channel_index, (ADL_UInt8)p_value };
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::CHANNEL_AFTER_TOUCH, params);
}

bool MidiScheduler::try_controller_change(int p_frame_index, int p_channel_index, int p_controller_id, int p_value) {
	MessageParams params{};
	params.controller_change = { (ADL_UInt8)p_channel_index, (ADL_UInt8)p_controller_id, (ADL_UInt8)p_value };
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::CONTROLLER_CHANGE, params);
}

bool MidiScheduler::try_patch_change(int p_frame_index, int p_channel_index, int p_patch_index) {
	MessageParams params{};
	params.patch_change = { (ADL_UInt8)p_channel_index, (ADL_UInt8)p_patch_index };
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::PATCH_CHANGE, params);
}

bool MidiScheduler::try_pitch_bend(int p_frame_index, int p_channel_index, int p_value) {
	MessageParams params{};
	params.pitch_bend = { (ADL_UInt8)p_channel_index, (ADL_UInt16)p_value };
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::PITCH_BEND, params);
}

bool MidiScheduler::try_panic(int p_frame_index) {
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::PANIC, {});
}

bool MidiScheduler::try_reset_state(int p_frame_index) {
	return try_queue_message(message_queue, is_message_queue_dirty, current_frame_index, p_frame_index, MessageType::RESET_STATE, {});
}
