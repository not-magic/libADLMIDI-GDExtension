// Exercises MidiScheduler against a real libADLMIDI player: queue sorting,
// fully filled buffers, no frame drift, exact-frame chronological dispatch.
#include "midi_scheduler.h"

#include <adlmidi.h>

#include <cstdio>
#include <vector>

struct DispatchRecord {
	int frame_index;
	int note_index;
};

static std::vector<DispatchRecord> dispatch_log;

static void on_dispatch(void *p_userdata, const MidiScheduler::QueuedMessage &p_message, int p_frame_index) {
	int note_index = -1;
	if (p_message.type == MidiScheduler::MessageType::NOTE_ON) {
		note_index = p_message.params.note_on.note_index;
	} else if (p_message.type == MidiScheduler::MessageType::NOTE_OFF) {
		note_index = p_message.params.note_off.note_index;
	}
	dispatch_log.push_back({ p_frame_index, note_index });
}

int main() {
	ADL_MIDIPlayer *player = adl_init(44100);
	if (!player) {
		printf("FAIL: adl_init failed: %s\n", adl_errorString());
		return 1;
	}
	adl_setNumChips(player, 4);
	adl_setBank(player, 0);

	MidiScheduler scheduler;
	scheduler.set_player(player);
	scheduler.set_dispatch_callback(on_dispatch);

	scheduler.try_note_on(50000, 0, 64, 100);
	scheduler.try_note_on(0, 0, 60, 100);
	scheduler.try_note_off(2049, 0, 60);
	scheduler.try_note_on(1024, 0, 62, 100);

	const int frame_count = 1024;
	std::vector<MidiScheduler::AudioFrame> buffer(frame_count);
	bool is_ok = true;
	int frame_total = 0;

	for (int i = 0; i < 130 && is_ok; ++i) {
		for (auto &frame : buffer) {
			frame.left = -999.0f;
			frame.right = -999.0f;
		}

		is_ok = scheduler.try_mix(buffer.data(), frame_count);

		if (is_ok) {
			for (auto &frame : buffer) {
				if (frame.left == -999.0f && frame.right == -999.0f) {
					printf("FAIL: under-filled buffer at call %d\n", i);
					is_ok = false;
					break;
				}
			}
		}

		frame_total += frame_count;
	}

	adl_close(player);

	printf("is_ok=%d frame_total=%d current_frame_index=%d queue_remaining=%zu\n",
			is_ok, frame_total, scheduler.get_current_frame(), scheduler.get_queue_size());

	if (scheduler.get_current_frame() != frame_total) {
		printf("FAIL: current_frame (%d) drifted from total frames generated (%d)\n", scheduler.get_current_frame(), frame_total);
		is_ok = false;
	}

	if (scheduler.get_queue_size() != 0) {
		printf("FAIL: %zu message(s) never dispatched\n", scheduler.get_queue_size());
		is_ok = false;
	}

	bool is_ordered = true;
	int last_frame_index = -1;
	printf("dispatch order:\n");
	for (auto &d : dispatch_log) {
		printf("  at frame=%d note=%d\n", d.frame_index, d.note_index);
		if (d.frame_index < last_frame_index) {
			is_ordered = false;
		}
		last_frame_index = d.frame_index;
	}
	if (!is_ordered) {
		printf("FAIL: messages dispatched out of chronological order\n");
		is_ok = false;
	}
	if (dispatch_log.size() != 4) {
		printf("FAIL: expected 4 dispatched messages, got %zu\n", dispatch_log.size());
		is_ok = false;
	}

	const int expected_frame_indices[4] = { 0, 1024, 2049, 50000 };
	for (int i = 0; i < (int)dispatch_log.size() && i < 4; ++i) {
		if (dispatch_log[i].frame_index != expected_frame_indices[i]) {
			printf("FAIL: message %d dispatched at frame %d, expected %d\n", i, dispatch_log[i].frame_index, expected_frame_indices[i]);
			is_ok = false;
		}
	}

	printf(is_ok ? "PASS\n" : "FAIL\n");
	return is_ok ? 0 : 1;
}
