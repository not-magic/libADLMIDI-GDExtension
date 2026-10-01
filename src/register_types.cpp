#include "register_types.h"

#include "audio_stream_midi.h"
#include "audio_stream_midi_base.h"
#include "audio_stream_midi_sequencer.h"
#include "resource_format_loader_midi.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

static Ref<ResourceFormatLoaderMIDI> midi_loader;

void initialize_adlmidi_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_ABSTRACT_CLASS(AudioStreamMIDIBase);
	GDREGISTER_ABSTRACT_CLASS(AudioStreamPlaybackMIDIBase);
	GDREGISTER_CLASS(AudioStreamMIDI);
	GDREGISTER_CLASS(AudioStreamPlaybackMIDI);
	GDREGISTER_CLASS(AudioStreamMIDISequencer);
	GDREGISTER_CLASS(AudioStreamPlaybackMIDISequencer);
	GDREGISTER_CLASS(ResourceFormatLoaderMIDI);

	midi_loader.instantiate();
	ResourceLoader::get_singleton()->add_resource_format_loader(midi_loader);
}

void uninitialize_adlmidi_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ResourceLoader::get_singleton()->remove_resource_format_loader(midi_loader);
	midi_loader.unref();
}

extern "C" {
GDExtensionBool GDE_EXPORT adlmidi_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	const godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_adlmidi_module);
	init_obj.register_terminator(uninitialize_adlmidi_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
