cmake_minimum_required (VERSION 3.31)

set (target_name phyllotaxis)
set (target_version "1.0.0")

project (${target_name} VERSION ${target_version} LANGUAGES C CXX OBJC OBJCXX)

# ==== YUP, vendored next to this file
set (YUP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (YUP_BUILD_PLUGINS OFF CACHE BOOL "" FORCE)
set (YUP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory (yup)

# ==== Identity. The AU codes are OSTypes: exactly four characters, and the
# manufacturer code needs at least one upper case letter.
set (plugin_name "Phyllotaxis")
set (plugin_description "A rotating line plays the points of a phyllotaxis spiral as MIDI notes.")
set (plugin_vendor "sousastep")
set (plugin_bundle_namespace "quest.sousastep")
set (plugin_email "")
set (plugin_url "https://sousastep.quest")
set (plugin_au_manufacturer "Sous")
set (plugin_au_subtype "Phyl")
set (plugin_copyright "The author disclaims copyright to this source code. May you do good and not evil.")

# A MIDI generator: MIDI in (ignored) and out, plus a silent stereo output that
# the AU instrument base class and most hosts expect.
yup_audio_plugin (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_APP_ID "${plugin_bundle_namespace}.${plugin_name}"
    TARGET_APP_NAMESPACE "${plugin_bundle_namespace}"
    TARGET_CXX_STANDARD 20
    TARGET_IDE_GROUP "Phyllotaxis"
    PLUGIN_ID "${plugin_bundle_namespace}.${plugin_name}"
    PLUGIN_NAME "${plugin_name}"
    PLUGIN_VENDOR "${plugin_vendor}"
    PLUGIN_EMAIL "${plugin_email}"
    PLUGIN_VERSION "${target_version}"
    PLUGIN_DESCRIPTION "${plugin_description}"
    PLUGIN_URL "${plugin_url}"
    PLUGIN_IS_SYNTH ON
    PLUGIN_IS_MONO OFF
    PLUGIN_CREATE_CLAP ON
    PLUGIN_CREATE_VST3 ON
    PLUGIN_CREATE_AU ON
    PLUGIN_CREATE_AUv3 OFF
    PLUGIN_CREATE_LV2 OFF
    PLUGIN_CREATE_AAX OFF
    PLUGIN_CREATE_STANDALONE ON
    PLUGIN_CLAP_FEATURES note-effect utility
    PLUGIN_VST3_CATEGORIES Instrument Generator
    PLUGIN_AU_MANUFACTURER "${plugin_au_manufacturer}"
    PLUGIN_AU_SUBTYPE "${plugin_au_subtype}"
    PLUGIN_COPY_AFTER_BUILD ON
    DEFINITIONS
        YUP_EMBED_DEFAULT_THEME_TEXT_SERIF_FONT=1
        YUP_EMBED_DEFAULT_THEME_TEXT_MONOSPACE_FONT=1
    MODULES
        yup::yup_gui
        yup::yup_audio_processors)

# ==== Register the AU as a MIDI processor ('aumi') so Logic lists it under
# MIDI FX. YUP only writes 'aumu' or 'aufx', so the bundle gets its own plist
# made from YUP's template.
if (TARGET ${target_name}_au_plugin)
    set (PLUGIN_AU_TYPE "aumi")
    set (PLUGIN_AU_SUBTYPE "${plugin_au_subtype}")
    set (PLUGIN_AU_MANUFACTURER "${plugin_au_manufacturer}")
    set (PLUGIN_AU_NAME "${plugin_name}")
    set (PLUGIN_AU_VERSION "${target_version}")
    set (PLUGIN_AU_SANDBOX_SAFE "<false/>")
    set (PLUGIN_AU_TAG "MIDI")
    set (PLUGIN_AU_COMPONENT_NAME "${plugin_vendor}: ${plugin_name}")
    string (REGEX MATCHALL "[0-9]+" version_parts "${target_version}")
    list (GET version_parts 0 version_major)
    list (GET version_parts 1 version_minor)
    list (GET version_parts 2 version_patch)
    math (EXPR PLUGIN_AU_COMPONENT_VERSION "(${version_major} << 16) | (${version_minor} << 8) | ${version_patch}")

    # YUP's template carries YUP's own copyright line; ours goes in its place
    file (READ "${CMAKE_CURRENT_LIST_DIR}/yup/cmake/platforms/mac/AudioUnitInfo.plist.in" au_plist_template)
    string (REGEX REPLACE "<key>NSHumanReadableCopyright</key>[ \t\r\n]*<string>[^<]*</string>"
        "<key>NSHumanReadableCopyright</key>\n\t<string>${plugin_copyright}</string>" au_plist_template "${au_plist_template}")
    set (au_midi_plist_in "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_au_midi_processor.plist.in")
    file (WRITE "${au_midi_plist_in}" "${au_plist_template}")

    set (au_midi_plist "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_au_midi_processor.plist")
    configure_file ("${au_midi_plist_in}" "${au_midi_plist}" @ONLY)
    set_target_properties (${target_name}_au_plugin PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${au_midi_plist}")
endif()

if (TARGET ${target_name}_standalone_plugin)
    set_target_properties (${target_name}_standalone_plugin PROPERTIES MACOSX_BUNDLE_COPYRIGHT "${plugin_copyright}")
endif()

# ==== Install real copies of the VST3 and CLAP bundles. YUP symlinks them into
# ~/Library/Audio/Plug-Ins, and a link into a privacy-protected folder such as
# ~/Desktop is silently skipped by hosts without access to it (Ableton Live).
# The copy is signed again, ad hoc, since the bundle changes after linking.
if (APPLE)
    foreach (format vst3 clap)
        set (format_target ${target_name}_${format}_plugin)
        if (NOT TARGET ${format_target})
            continue()
        endif()

        string (TOUPPER "${format}" format_folder)
        set (install_dir "$ENV{HOME}/Library/Audio/Plug-Ins/${format_folder}")
        add_custom_command (TARGET ${format_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${install_dir}/${format_target}.${format}" "${install_dir}/${plugin_name}.${format}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${format_target}>" "${install_dir}/${plugin_name}.${format}"
            COMMAND codesign --force --sign - "${install_dir}/${plugin_name}.${format}"
            COMMENT "Copying ${format_folder} plugin to ${install_dir}/${plugin_name}.${format}"
            VERBATIM)
    endforeach()
elseif (WIN32)
    # YUP installs nothing on Windows. The per-user folders from the VST3 and
    # CLAP specs need no administrator rights. A VST3 bundle's inner binary
    # must carry the bundle's name, so both are renamed.
    set (user_plugins "$ENV{LOCALAPPDATA}/Programs/Common")
    if (TARGET ${target_name}_vst3_plugin)
        set (vst3_bundle "${user_plugins}/VST3/${plugin_name}.vst3")
        add_custom_command (TARGET ${target_name}_vst3_plugin POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${vst3_bundle}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${target_name}_vst3_plugin>/../.." "${vst3_bundle}"
            COMMAND ${CMAKE_COMMAND} -E rename "${vst3_bundle}/Contents/x86_64-win/$<TARGET_FILE_NAME:${target_name}_vst3_plugin>" "${vst3_bundle}/Contents/x86_64-win/${plugin_name}.vst3"
            COMMENT "Copying VST3 plugin to ${vst3_bundle}"
            VERBATIM)
    endif()
    if (TARGET ${target_name}_clap_plugin)
        add_custom_command (TARGET ${target_name}_clap_plugin POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${user_plugins}/CLAP"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${target_name}_clap_plugin>" "${user_plugins}/CLAP/${plugin_name}.clap"
            COMMENT "Copying CLAP plugin to ${user_plugins}/CLAP/${plugin_name}.clap"
            VERBATIM)
    endif()
endif()

# ==== Sources, compiled once into the core every format links
file (GLOB sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/*.h")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/source FILES ${sources})
target_sources (${target_name}_shared PRIVATE ${sources})

# Strict warnings on our own files only. YUP and its externals are read as
# system headers here (quoted includes inside them inherit that).
if (MSVC)
    set (strict_warnings "/W4;/external:anglebrackets;/external:W0")
else()
    set (strict_warnings "--system-header-prefix=yup_;--system-header-prefix=xsimd/;-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion;-Wsign-conversion;-Wfloat-conversion;-Wdouble-promotion")
endif()
set_source_files_properties (${sources} PROPERTIES COMPILE_OPTIONS "${strict_warnings}")
