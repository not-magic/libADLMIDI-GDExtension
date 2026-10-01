#!/usr/bin/env python
import os
import sys

# You can find documentation for SCons and SConstruct files at:
# https://scons.org/documentation.html

ADDON_NAME = 'ADLMIDI'


# This lets SCons know that we're using godot-cpp, from the godot-cpp folder.
env = SConscript("godot-cpp/SConstruct")

# Configures the 'src' directory as a source for header files.
env.Append(CPPPATH=["src/", "libADLMIDI/include/"])

# Collects all .cpp files in the 'src' folder as compile targets.
sources = Glob("src/*.cpp")

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

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

# The filename for the dynamic library for this GDExtension.
# $SHLIBPREFIX is a platform specific prefix for the dynamic library ('lib' on Unix, '' on Windows).
# $SHLIBSUFFIX is the platform specific suffix for the dynamic library (for example '.dll' on Windows).
# env["suffix"] includes the build's feature tags (e.g. '.windows.template_debug.x86_64')
# (see https://docs.godotengine.org/en/stable/tutorials/export/feature_tags.html).
# The final path should match a path in the '.gdextension' file.
lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), ADDON_NAME, env["suffix"], env.subst('$SHLIBSUFFIX'))

# Creates a SCons target for the path with our sources.
library = env.SharedLibrary(
    "demo/addons/{}/bin/{}".format(ADDON_NAME, lib_filename),
    source=sources,
)

# Selects the shared library as the default target.
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

# --- Formatting and linting (.clang-format, .clang-tidy) ---
# `scons format` rewrites src/ and tests/ in place with clang-format;
# `scons tidy` runs clang-tidy over every src/*.cpp (plus headers under src/) with
# the same include paths/defines the real build uses. Point CLANG_FORMAT /
# CLANG_TIDY at a specific binary to override the one found on PATH. The
# .clang-format/.clang-tidy files need a recent LLVM (distro clang 14 can't
# parse them); `pip install clang-format clang-tidy` provides one.
import subprocess

godot_env = env
lint_sources = sorted(str(f) for f in Glob("src/*.cpp") + Glob("src/*.h") + Glob("tests/*.cpp"))


def run_format(target, source, env):
    tool = os.environ.get("CLANG_FORMAT", "clang-format")
    return subprocess.call([tool, "-i", "--style=file"] + lint_sources)


def run_tidy(target, source, env):
    tool = os.environ.get("CLANG_TIDY", "clang-tidy")
    status = 0
    for path in lint_sources:
        if not path.endswith(".cpp") or path.startswith("tests"):
            continue
        compile_args = ["-I" + str(d) for d in godot_env["CPPPATH"]] + ["-std=c++17"]
        for define in godot_env["CPPDEFINES"]:
            if isinstance(define, (tuple, list)):
                compile_args.append("-D{}={}".format(*define))
            else:
                compile_args.append("-D" + str(define))
        status |= subprocess.call([tool, "--quiet", "--header-filter=.*/src/.*", path, "--"] + compile_args)
    return status


format_sources = Command("format", None, run_format)
AlwaysBuild(format_sources)

tidy_sources = Command("tidy", None, run_tidy)
AlwaysBuild(tidy_sources)

# --- Docs update (doc_classes/) ---
# Regenerates doc_classes/*.xml from the classes' _bind_methods() by loading
# the built extension into Godot's --doctool. Requires a template_debug build
# (with doc data compiled in, see the GodotCPPDocData block above) and a
# flatpak install of the Godot editor (org.godotengine.Godot). Run with
# `scons docs`. Runs from demo/ since --doctool needs a Godot project
# (project.godot) to load the extension into.
update_docs = Command(
    "update_docs",
    None,
    "flatpak run org.godotengine.Godot --doctool ../ --gdextension-docs",
    chdir="demo",
)
AlwaysBuild(update_docs)

# --- Wiki docs (docs/) ---
# Regenerates docs/*.md (GitHub wiki pages) from doc_classes/*.xml via
# tools/generate_docs.py. Not part of the default build -- run explicitly
# with `scons update_wiki` (against whatever doc_classes/*.xml is currently
# on disk), or via `scons docs`, which also regenerates that XML first so
# the wiki pages never drift from it. Split into two Command nodes so a
# plain build/`scons update_wiki` never pulls in the flatpak --doctool step
# above.
wiki_action = "{} tools/generate_docs.py --src doc_classes --out docs".format(sys.executable)

update_wiki = Command("update_wiki", None, wiki_action)
AlwaysBuild(update_wiki)

update_wiki_after_docs = Command("update_wiki_after_docs", None, wiki_action)
AlwaysBuild(update_wiki_after_docs)
Requires(update_wiki_after_docs, update_docs)

docs_alias = Alias("docs", [update_docs, update_wiki_after_docs])
