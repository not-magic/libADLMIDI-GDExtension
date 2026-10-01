extends Control

@onready var audio_stream_player: AudioStreamPlayer = $AudioStreamPlayer
@onready var sequencer_player: AudioStreamPlayer = $SequencerPlayer

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass


func _on_play_button_pressed() -> void:
	audio_stream_player.play()


func _on_stop_button_pressed() -> void:
	audio_stream_player.stop()
	


func _on_note_button_button_down() -> void:
	var seq := sequencer_player.get_stream_playback() as AudioStreamPlaybackMIDISequencer
	seq.note_on(seq.current_time, 5, 60, 127)

func _on_note_button_button_up() -> void:
	var seq := sequencer_player.get_stream_playback() as AudioStreamPlaybackMIDISequencer
	seq.note_off(seq.current_time, 5, 60)
