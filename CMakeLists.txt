# RESEARCH(Krey): Consider rewritting in meson
# RESEARCH(Krey): Consider rewritting in cargo-make (requires adaptation)

# TRACKING-CARGOMAKE(Krey): Tracking for https://github.com/sagiegurari/cargo-make/issues/486
## NOTE(Krey): Target here is to run client `cave` that is a frontend for the package manager with ultimate goal to compile any package on exheres-0 eapi -> Expected to then allow work on custom eapi
## - [ ] We need method to create static and dynamic libraries depending on file type on demand
## - [ ] Parsing options inside the source code prior to compilation
## - [ ] Specifying executables
## - [ ] Method to run tests
## - [ ] Method to run benchmarks

# NOTICE(Krey): Project-provided libraries are expected to be versioned as:
## - {NAME}{PROJECT_NAME}_{MAJOR}.{MINOR}.{EXT}._<{MAJOR} * 100) + {MINOR}>
### - {} -> Variable
### - <> -> Arithmetics
## - BRAINSTORM(Krey)

# FIXME-DOCS(Krey): Cave depends on following libraries
# - NOTE(Krey): There are also SE (Shell Executable [bash]) and NN (NiceNames [bash]) files that are being compiled with the source.. Seems as malpractice -> Not included in the list, needs investigation
# - libpaludis_3.0.so.300
## - libs/about_metadata.cc
## - libs/action.cc
## - libs/action_names.cc
## - libs/additional_package_dep_spec_requirement.cc
## - libs/always_enabled_dependency_label.cc
## - libs/broken_linkage_configuration.cc
## - libs/broken_linkage_finder.cc
## - libs/buffer_output_manager.cc
## - libs/call_pretty_printer.cc
## - libs/changed_choices.cc
## - libs/choice.cc
## - libs/comma_separated_dep_parser.cc
## - libs/comma_separated_dep_pretty_printer.cc
## - libs/command_output_manager.cc
## - libs/common_sets.cc
## - libs/contents.cc
## - libs/create_output_manager_info.cc
## - libs/dep_label.cc
## - libs/dep_spec.cc
## - libs/dep_spec_annotations.cc
## - libs/dep_spec_data.cc
## - libs/dep_spec_flattener.cc
## - libs/distribution.cc
## - libs/elf_linkage_checker.cc
## - libs/elike_blocker.cc
## - libs/elike_choices.cc
## - libs/elike_dep_parser.cc
## - libs/elike_conditional_dep_spec.cc
## - libs/elike_package_dep_spec.cc
## - libs/elike_slot_requirement.cc
## - libs/elike_use_requirement.cc
## - libs/environment.cc
## - libs/environment_factory.cc
## - libs/environment_implementation.cc
## - libs/file_output_manager.cc
## - libs/filter.cc
## - libs/filter_handler.cc
## - libs/filtered_generator.cc
## - libs/format_messages_output_manager.cc
## - libs/formatted_pretty_printer.cc
## - libs/forward_at_finish_output_manager.cc
## - libs/fs_merger.cc
## - libs/fuzzy_finder.cc
## - libs/generator.cc
## - libs/generator_handler.cc
## - libs/hook.cc
## - libs/hooker.cc
## - libs/ipc_output_manager.cc
## - libs/libtool_linkage_checker.cc
## - libs/linkage_checker.cc
## - libs/literal_metadata_key.cc
## - libs/maintainer.cc
## - libs/mask.cc
## - libs/mask_utils.cc
## - libs/match_package.cc
## - libs/merger.cc
## - libs/merger_entry_type.cc
## - libs/metadata_key.cc
## - libs/metadata_key_holder.cc
## - libs/name.cc
## - libs/ndbam.cc
## - libs/ndbam_merger.cc
## - libs/ndbam_unmerger.cc
## - libs/notifier_callback.cc
## - libs/output_manager.cc
## - libs/output_manager_factory.cc
## - libs/output_manager_from_environment.cc
## - libs/package_dep_spec_collection.cc
## - libs/package_dep_spec_properties.cc
## - libs/package_id.cc
## - libs/paludislike_options_conf.cc
## - libs/partially_made_package_dep_spec.cc
## - libs/partitioning.cc
## - libs/permitted_choice_value_parameter_values.cc
## - libs/pretty_print_options.cc
## - libs/pretty_printer.cc
## - libs/repository.cc
## - libs/repository_factory.cc
## - libs/repository_name_cache.cc
## - libs/selection.cc
## - libs/selection_handler.cc
## - libs/serialise.cc
## - libs/set_file.cc
## - libs/slot.cc
## - libs/slot_requirement.cc
## - libs/spec_tree.cc
## - libs/standard_output_manager.cc
## - libs/stripper.cc
## - libs/syncer.cc
## - libs/tar_merger.cc
## - libs/tee_output_manager.cc
## - libs/unchoices_key.cc
## - libs/unformatted_pretty_printer.cc
## - libs/unmerger.cc
## - libs/user_dep_spec.cc
## - libs/version_operator.cc
## - libs/version_requirements.cc
## - version_spec.cc
# - libpaludisargs_3.0.so.300
## - args/args.cc
## - args/args_error.cc
## - args/args_group.cc
## - args/args_handler.cc
## - args/args_option.cc
## - args/args_section.cc
## - args/bad_argument.cc
## - args/args_visitor.cc
## - args/args_dumper.cc
## - args/escape.cc
## - args/log_level_arg.cc
## - args/man.cc
# - libpaludisutil_3.0.so.300
## - utils/active_object_ptr.cc
## - utils/buffer_output_stream.cc
## - utils/channel.cc
## - utils/config_file.cc
## - utils/cookie.cc
## - utils/damerau_levenshtein.cc
## - utils/destringify.cc
## - utils/deferred_construction_ptr.cc
## - utils/digest_registry.cc
## - utils/discard_output_stream.cc
## - utils/elf.cc
## - utils/elf_dynamic_section.cc
## - utils/elf_relocation_section.cc
## - utils/elf_sections.cc
## - utils/elf_symbol_section.cc
## - utils/enum_iterator.cc
## - utils/env_var_names.cc
## - utils/exception.cc
## - utils/executor.cc
## - utils/extract_host_from_url.cc
## - utils/fs_iterator.cc
## - utils/fs_error.cc
## - utils/fs_path.cc
## - utils/fs_stat.cc
## - utils/graph.cc
## - utils/hashes.cc
## - utils/is_file_with_extension.cc
## - utils/log.cc
## - utils/make_named_values.cc
## - utils/map.cc
## - utils/md5.cc
## - utils/named_value.cc
## - utils/options.cc
## - utils/persona.cc
## - utils/pipe.cc
## - utils/pool.cc
## - utils/pretty_print.cc
## - utils/process.cc
## - utils/pty.cc
## - utils/realpath.cc
## - utils/return_literal_function.cc
## - utils/rmd160.cc
## - utils/safe_ifstream.cc
## - utils/safe_ofstream.cc
## - utils/sequence.cc
## - utils/set.cc
## - utils/sha1.cc
## - utils/sha256.cc
## - utils/sha512.cc
## - utils/simple_parser.cc
## - utils/string_list_stream.cc
## - utils/strip.cc
## - utils/system.cc
## - utils/tail_output_stream.cc
## - utils/tee_output_stream.cc
## - utils/thread_pool.cc
## - utils/timestamp.cc
## - utils/tokeniser.cc
## - utils/tribool.cc
## - utils/type_list.cc
## - utils/upper_lower.cc
## - utils/visitor.cc
## - utils/visitor_cast.cc
## - utils/whirlpool.cc
## - utils/wildcard_expander.cc
# linux-vdso.so.1
# libdl.so.2
# libstdc++.so.6
# libpthread.so.0
# libc.so.6
# ld-linux-x86-64.so.2
# libm.so.6
# libgcc_s.so.1

# FIXME(Krey): Apply logic
# NOTE(Krey): Comments '^#\s{1}.*' are stripped, '^###!\s{1}.*' are converted in markdown format with code below formatted in README.md

# NOTE(Krey): Set version handling for cmake
# FIXME-CI(Krey): Set CI to handled cmake compatibility
cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

# Metadata
project(PALUDIS
	VERSION 3.0.0
	DESCRIPTION Source-based package manager with options
	HOMEPAGE_URL https://example.tld
	LANGUAGES CXX)
set(PROJECT_MAINTAINER kreyren@rixotstudio.cz)

# Project configuration
set(CMAKE_CXX_STANDARD 11)
list(APPEND CMAKE_MODULE_PATH ## - cmake/modules)

# Includes
## CMake libraries
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
### Required for installation in GNU userland
include(GNUInstallDirs)
include(CMakeDependentOption)
# Project specific
## FIXME-QA(Krey): Use PROJECT_NAME here to allow renames
include(PaludisList)
include(PaludisAddLibrary)
include(PaludisAddTest)
include(PaludisCheckFunctionExists)
include(PaludisCompileFlags)

## File hierarchy
set(${PROJECT_NAME}_REPOROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(${PROJECT_NAME}_CMAKEDIR ${PROJECT_NAME}_REPOROOT/cmake)
### Components
set(${PROJECT_NAME}_EDITORDIR ${PROJECT_NAME}_REPOROOT/paludis/editors)
set(${PROJECT_NAME}_BINDINGDIR ${PROJECT_NAME}_REPOROOT/paludis/binding)

## Configuration
include(${${PROJECT_NAME}_REPOROOT}/cmake/config/distribution.cmake)
# WTF(Krey): Why do we prefer PTHREAD?
find_package(Threads REQUIRED)
	set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
	set(THREADS_PREFER_PTHREAD_FLAG TRUE)

# Options
## Binding options
include(## - cmake/config/binding.cmake)
## Completion optiosn
include(## - cmake/config/completion.cmake)
## Component options
include(## - cmake/config/component.cmake)
## Color options
include(## - cmake/config/color.cmake)
## Component options
include(## - cmake/config/component.cmake)
## Editor support
include(## - cmake/config/editor.cmake)
## Client support e.g. cave
include(## - cmake/config/client.cmake)

## Build specific options
# FIXME(Krey): Needs processing
# FIXME-CI(Krey): Add build for static
option(BUILD_SHARED_LIBS build shared (dynamically-linked) libraries ON)
option(ENABLE_DOXYGEN enable doxygen based documentation OFF)
option(ENABLE_DOXYGEN_TAGS use 'wget' to fetch external doxygen tags OFF)
# FIXME-CI(Krey): Add build for GTest
option(ENABLE_GTEST enable GTest based tests OFF)
option(ENABLE_SEARCH_INDEX enable search index (requires sqlite3) OFF)

# WTF(Krey)
if(CMAKE_CROSSCOMPILING)
  option(USE_PREBUILT_DOCUMENTATION use prebuilt documentation ON)
else()
  option(USE_PREBUILT_DOCUMENTATION use prebuilt documentation OFF)
endif()

# Add subdirs
# add_subdirectory(args)
# add_subdirectory(bindings)
# add_subdirectory(completions)
add_subdirectory(clients)
# add_subdirectory(distributions)
# add_subdirectory(editors)
# add_subdirectory(environments)
# add_subdirectory(fetchers)
# add_subdirectory(hooks)
# add_subdirectory(misc)
# add_subdirectory(repositories)
# add_subdirectory(resolvers)
# add_subdirectory(selinux)
# add_subdirectory(syncers)
# add_subdirectory(utils)

# Additional code