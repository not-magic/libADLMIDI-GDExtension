#!/usr/bin/env python
import os
import sys

# Godot API version we build against
# ARGUMENTS.setdefault("api_version", "4.5")

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

env.Append(CPPPATH=["src/", "libADLMIDI/include/"])
sources = Glob("src/*.cpp")

# Embeds doc_classes/*.xml into the extension so AudioStreamMIDI and friends
# get real entries (descriptions, etc.) in the editor's Help panel, not just
# auto-generated stubs. Only editor/template_debug builds need it -- release
# export templates never show the Help panel, matching Godot's own convention.
if env["target"] in ["editor", "template_debug"]:
    doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
    sources.append(doc_data)

# libADLMIDI doesn't ship its own SCons build, so its sources are compiled
# directly here instead. This list (and the ENABLE_END_SILENCE_SKIPPING
# define below) mirrors libADLMIDI's default CMake configuration
# (WITH_MIDI_SEQUENCER, WITH_EMBEDDED_BANKS, WITH_XMI_SUPPORT, and every
# emulator backend it enables by default) and was extracted from a working
# `cmake --build` of libADLMIDI/CMakeLists.txt, so it should be kept in sync
# by hand if libADLMIDI adds or removes default source files upstream.
adlmidi_env = env.Clone()
adlmidi_env.Append(CPPDEFINES=["ENABLE_END_SILENCE_SKIPPING"])

# godot-cpp builds with exceptions disabled by default, but libADLMIDI's
# internal containers use them (e.g. pl_list's std::bad_alloc on OOM).
if "-fno-exceptions" in adlmidi_env["CXXFLAGS"]:
    adlmidi_env["CXXFLAGS"].remove("-fno-exceptions")
    adlmidi_env.Append(CXXFLAGS=["-fexceptions"])

# libADLMIDI is a git submodule (a separate upstream repo); build its object
# files into build/libADLMIDI/ instead of alongside its sources so `scons`
# never leaves untracked build artifacts inside the submodule's working tree.
libadlmidi_src_dir = "libADLMIDI/src"
libadlmidi_build_dir = "build/libADLMIDI"
VariantDir(libadlmidi_build_dir, libadlmidi_src_dir, duplicate=0)

adlmidi_relative_sources = [
    "adlmidi.cpp",
    "adlmidi_load.cpp",
    "adlmidi_midiplay.cpp",
    "adlmidi_opl3.cpp",
    "adlmidi_private.cpp",
    "adlmidi_sequencer.cpp",
    "inst_db.cpp",
    "wopl/wopl_file.c",
    "models/model_ail.c",
    "models/model_apogee.c",
    "models/model_dmx.c",
    "models/model_generic.c",
    "models/model_hmi_sos.c",
    "models/model_msadlib.c",
    "models/model_oconnell.c",
    "models/model_win9x.c",
    "chips/dosbox_opl2.cpp",
    "chips/dosbox_opl3.cpp",
    "chips/dosbox/dbopl.cpp",
    "chips/nuked_opl2.cpp",
    "chips/nuked_opl3.cpp",
    "chips/nuked_opl3_fast.cpp",
    "chips/nuked_cqm.cpp",
    "chips/nuked/nukedopl2.c",
    "chips/nuked/nukedopl3.c",
    "chips/nuked_fast/nukedopl3_fast.c",
    "chips/nuked_cqm/cqm.c",
    "chips/opal_opl3.cpp",
    "chips/opal/opal.c",
    "chips/java_opl3.cpp",
    "chips/esfmu_opl3.cpp",
    "chips/esfmu/esfm.c",
    "chips/esfmu/esfm_registers.c",
    "chips/mame_opl2.cpp",
    "chips/mame/mame_fmopl.cpp",
    "chips/ymfm_opl2.cpp",
    "chips/ymfm_opl3.cpp",
    "chips/ymfm/ymfm_adpcm.cpp",
    "chips/ymfm/ymfm_misc.cpp",
    "chips/ymfm/ymfm_opl.cpp",
    "chips/ymfm/ymfm_pcm.cpp",
    "chips/ymfm/ymfm_ssg.cpp",
]

adlmidi_objects = [adlmidi_env.SharedObject(os.path.join(libadlmidi_build_dir, f)) for f in adlmidi_relative_sources]
sources += adlmidi_objects

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/addons/ADLMIDI/bin/libadlmidi.{}.{}.framework/libadlmidi.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/addons/ADLMIDI/bin/libadlmidi.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/addons/ADLMIDI/bin/libadlmidi.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "demo/addons/ADLMIDI/bin/libadlmidi{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)

# --- Standalone libADLMIDI-level tests (tests/) ---
#
# These link straight against the same libADLMIDI objects compiled above
# plus MidiScheduler (src/midi_scheduler.*, no godot-cpp dependency) -- no
# GDExtension layer involved.
#
# Opt-in and off by default:
#   scons tests=yes              build tests/*.cpp into tests/bin/
#   scons tests=yes run_tests=yes   ...and run each one, failing the build
#                                    if any test exits non-zero
build_tests = ARGUMENTS.get("tests", "no").lower() in ("1", "true", "yes")
run_tests = ARGUMENTS.get("run_tests", "no").lower() in ("1", "true", "yes")

if build_tests:
    test_env = adlmidi_env.Clone()
    if env["platform"] != "windows":
        test_env.Append(LIBS=["pthread"])

    # Built separately (under build/tests/) from the main extension's own
    # copy of the same source, so the two differently-configured
    # environments (this one has no godot-cpp) don't fight over one output
    # file.
    midi_scheduler_test_object = test_env.Object("build/tests/midi_scheduler", "src/midi_scheduler.cpp")

    def _run_test(target, source, env):
        import subprocess

        program = str(source[0])
        print("Running {} ...".format(program))
        result = subprocess.run([program])
        if result.returncode != 0:
            print("FAILED: {} (exit code {})".format(program, result.returncode))
            return 1
        print("PASSED: {}".format(program))
        return 0

    for test_source in Glob("tests/*.cpp"):
        test_name = os.path.splitext(os.path.basename(str(test_source)))[0]
        test_sources = [test_source, midi_scheduler_test_object] + adlmidi_objects

        test_program = test_env.Program("tests/bin/{}".format(test_name), source=test_sources)
        Default(test_program)

        if run_tests:
            test_stamp = test_env.Command(
                "tests/bin/{}.ran".format(test_name),
                test_program,
                Action(_run_test, "Running $SOURCE"),
            )
            test_env.AlwaysBuild(test_stamp)
            Default(test_stamp)

# `scons docs` regenerates doc_classes/*.xml via Godot's --doctool (needs a
# template_debug build and the flatpak editor, org.godotengine.Godot) and then
# docs/*.md for the GitHub wiki; `scons update_wiki` only does the latter.
update_docs = Command(
    "update_docs",
    None,
    "flatpak run org.godotengine.Godot --doctool ../ --gdextension-docs",
    chdir="demo",
)
AlwaysBuild(update_docs)

wiki_action = "{} tools/generate_docs.py --src doc_classes --out docs".format(sys.executable)

update_wiki = Command("update_wiki", None, wiki_action)
AlwaysBuild(update_wiki)

update_wiki_after_docs = Command("update_wiki_after_docs", None, wiki_action)
AlwaysBuild(update_wiki_after_docs)
Requires(update_wiki_after_docs, update_docs)

Alias("docs", [update_docs, update_wiki_after_docs])
