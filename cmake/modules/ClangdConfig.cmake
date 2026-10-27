#
# Write a '.clangd' configuration file at the top of the source tree, pointing clangd at *this*
# build tree's 'compile_commands.json'.
#
# Editors and IDEs that use clangd (see the "Code intelligence" section of the BUILD-*.md guides)
# need to be told which build tree to read the compilation database from. This source tree builds
# either GPlates or pyGPlates (see GPLATES_BUILD_GPLATES), from separate build directories with
# different compile commands, so a single committed '.clangd' would always be wrong for one of them.
# Generating it here means the tree you configured last is the tree clangd uses - so switching what
# you're working on is just re-running the configure step you'd run anyway.
#
# The generated '.clangd' is ignored by git (the template '.clangd.in' is what's committed).
#
option(GPLATES_WRITE_CLANGD_CONFIG
    "Write a '.clangd' file (from '.clangd.in') pointing clangd at this build tree's compilation database."
    true)

if (NOT GPLATES_WRITE_CLANGD_CONFIG)
    return()
endif()

# Nothing to point at if the compilation database isn't being generated. CMAKE_EXPORT_COMPILE_COMMANDS
# defaults to ON (see the root 'CMakeLists.txt') but the user can turn it off...
if (NOT CMAKE_EXPORT_COMPILE_COMMANDS)
    return()
endif()

# ...and it is silently ignored by the generators that don't support it (Visual Studio, Xcode), which
# would otherwise have us point clangd at a build tree containing no database at all.
if (NOT CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    return()
endif()

#
# Only write into the source tree for a build tree that is *inside* the source tree - ie, the
# 'build-gplates'/'build-pygplates' sub-directories that the build guides use.
#
# This deliberately excludes the automated builds that configure into a scratch directory elsewhere:
# a "pip install ." (scikit-build-core) or "conda build ..." of this source tree would otherwise
# leave '.clangd' pointing into a temporary build tree that is deleted when the build finishes -
# clobbering the developer's working configuration as a side effect of packaging.
#
file(RELATIVE_PATH _clangd_compilation_database "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
if (_clangd_compilation_database STREQUAL "" OR                # In-source build.
    _clangd_compilation_database MATCHES "^[.][.]" OR          # Build tree outside the source tree.
    IS_ABSOLUTE "${_clangd_compilation_database}")             # Build tree on another Windows drive.
    return()
endif()

# Recorded in a comment in the generated file, so it's obvious at a glance which target it describes.
if (GPLATES_BUILD_GPLATES)
    set(_clangd_target "GPlates (GPLATES_BUILD_GPLATES=TRUE)")
else()
    set(_clangd_target "pyGPlates (GPLATES_BUILD_GPLATES=FALSE)")
endif()

set(GPLATES_CLANGD_COMPILATION_DATABASE "${_clangd_compilation_database}")
set(GPLATES_CLANGD_TARGET "${_clangd_target}")

# Note: 'configure_file' only rewrites the output when its contents change, so re-configuring the
# same tree leaves the file's timestamp alone (and hence doesn't make clangd re-index needlessly).
configure_file(
    "${CMAKE_SOURCE_DIR}/.clangd.in"
    "${CMAKE_SOURCE_DIR}/.clangd"
    @ONLY)
