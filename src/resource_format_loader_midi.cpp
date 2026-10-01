#include "resource_format_loader_midi.h"

#include "audio_stream_midi.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

PackedStringArray ResourceFormatLoaderMIDI::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("mid");
	extensions.push_back("midi");
	extensions.push_back("rmi");
	extensions.push_back("xmi");
	extensions.push_back("kar");
	return extensions;
}

bool ResourceFormatLoaderMIDI::_handles_type(const StringName &p_type) const {
	return ClassDB::is_parent_class(p_type, "AudioStream");
}

String ResourceFormatLoaderMIDI::_get_resource_type(const String &p_path) const {
	const String ext = p_path.get_extension().to_lower();
	if (ext == "mid" || ext == "midi" || ext == "rmi" || ext == "xmi" || ext == "kar") {
		return "AudioStreamMIDI";
	}
	return "";
}

Variant ResourceFormatLoaderMIDI::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	if (file.is_null()) {
		return Variant((int)ERR_FILE_CANT_OPEN);
	}

	const PackedByteArray data = file->get_buffer(file->get_length());

	Ref<AudioStreamMIDI> stream;
	stream.instantiate();
	stream->set_midi_data(data);

	return stream;
}
