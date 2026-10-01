#pragma once

#include <adlmidi.h>

#include <deque>

// Sample-accurate queue of real-time libADLMIDI events. No Godot dependency,
// so tests can use it directly. Does not own the player: call set_player()
// again (nullptr first) whenever the caller recreates it.
class MidiScheduler {
public:
	enum class MessageType {
		NOTE_ON,
		NOTE_OFF,
		NOTE_AFTER_TOUCH,
		CHANNEL_AFTER_TOUCH,
		CONTROLLER_CHANGE,
		PATCH_CHANGE,
		PITCH_BEND,
		PANIC,
		RESET_STATE,
	};

	struct NoteOnParams {
		ADL_UInt8 channel_index;
		ADL_UInt8 note_index;
		ADL_UInt8 velocity;
	};

	struct NoteOffParams {
		ADL_UInt8 channel_index;
		ADL_UInt8 note_index;
	};

	struct NoteAfterTouchParams {
		ADL_UInt8 channel_index;
		ADL_UInt8 note_index;
		ADL_UInt8 value;
	};

	struct ChannelAfterTouchParams {
		ADL_UInt8 channel_index;
		ADL_UInt8 value;
	};

	struct ControllerChangeParams {
		ADL_UInt8 channel_index;
		ADL_UInt8 controller_id;
		ADL_UInt8 value;
	};

	struct PatchChangeParams {
		ADL_UInt8 channel_index;
		ADL_UInt8 patch_index;
	};

	struct PitchBendParams {
		ADL_UInt8 channel_index;
		ADL_UInt16 value;
	};

	// PANIC and RESET_STATE carry no parameters.
	union MessageParams {
		NoteOnParams note_on;
		NoteOffParams note_off;
		NoteAfterTouchParams note_after_touch;
		ChannelAfterTouchParams channel_after_touch;
		ControllerChangeParams controller_change;
		PatchChangeParams patch_change;
		PitchBendParams pitch_bend;
	};

	struct QueuedMessage {
		MessageParams params;
		MessageType type;
		int frame_index;
	};

	struct AudioFrame {
		float left;
		float right;
	};

	// Test hook, called right after a message is dispatched.
	using DispatchCallback = void (*)(void *p_userdata, const QueuedMessage &p_message, int p_frame_index);

private:
	ADL_MIDIPlayer *player = nullptr;
	DispatchCallback dispatch_callback = nullptr;
	void *dispatch_callback_userdata = nullptr;
	std::deque<QueuedMessage> message_queue;
	int current_frame_index = 0;
	bool is_message_queue_dirty = false;

public:
	void set_player(ADL_MIDIPlayer *p_player) { player = p_player; }

	void set_dispatch_callback(DispatchCallback p_callback, void *p_userdata = nullptr) {
		dispatch_callback = p_callback;
		dispatch_callback_userdata = p_userdata;
	}

	void reset();

	int get_current_frame() const { return current_frame_index; }
	size_t get_queue_size() const { return message_queue.size(); }

	// Fills p_dst_buffer, dispatching each due message at its exact frame.
	// Returns false, leaving the buffer untouched, if no player is set.
	bool try_mix(AudioFrame *p_dst_buffer, int p_frame_count);

	// Each returns false if the message was discarded for being in the past.
	bool try_note_on(int p_frame_index, int p_channel_index, int p_note_index, int p_velocity);
	bool try_note_off(int p_frame_index, int p_channel_index, int p_note_index);
	bool try_note_after_touch(int p_frame_index, int p_channel_index, int p_note_index, int p_value);
	bool try_channel_after_touch(int p_frame_index, int p_channel_index, int p_value);
	bool try_controller_change(int p_frame_index, int p_channel_index, int p_controller_id, int p_value);
	bool try_patch_change(int p_frame_index, int p_channel_index, int p_patch_index);
	bool try_pitch_bend(int p_frame_index, int p_channel_index, int p_value);
	bool try_panic(int p_frame_index);
	bool try_reset_state(int p_frame_index);
};
