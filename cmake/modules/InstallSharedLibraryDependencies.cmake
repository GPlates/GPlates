###################################################
###################################################
# Install dynamically linked dependency libraries #
###################################################
###################################################


##################################
# Enable Windows DLL redirection #
##################################
#
# Ensure our installed dependency DLLs get loaded, not any DLL with the same module name that is already loaded into memory
# (due to the DLL search order including something called "Loaded-module list").
# See https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order
#
# We avoid this by using DLL redirection (which is higher in the DLL search order).
# This involves installing an empty file called "gplate.exe.local" in the same folder as "gplate.exe".
# See https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-redirection
#
# Note: It appears that simply copying "gplate.exe.local" to an existing GPlates installation *after* the installed "gplates.exe"
#       has been run will not work (according to https://stackoverflow.com/a/52756644). This was verified by a user on the
#       GPlates forum (see https://discourse.gplates.org/t/mouse-cursor-offset-in-gplates/735/9).
#
# Note: If this DLL redirection is found to not work then an alternative is to investigate name mangling of DLLs.
#       For example, the delvewheel tool for creating Python wheels on Windows will copy dependency DLLs into the wheel
#       and name-mangle them (see https://github.com/adang1345/delvewheel?tab=readme-ov-file#name-mangling).
#       We currently use that to generate pyGPlates wheels - so pyGPlates is already taken care of.
#
if (WIN32)
    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        # Create an empty "gplates.exe.local" file.
        set(GPLATES_EXE_LOCAL_FILE "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:${BUILD_TARGET}>.local")
        # Note: All configurations generate the same content (empty file) so can have the same output filename.
        file(GENERATE OUTPUT "${GPLATES_EXE_LOCAL_FILE}" CONTENT "")

        # Install "gplates.exe.local" into same directory as "gplates.exe".
        install(FILES "${GPLATES_EXE_LOCAL_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR})
    endif()
endif()

###############################################
# Install the Visual Studio runtime libraries #
###############################################
#
# Note that Qt5 should also be using the same runtime libraries because it should be using the same compiler
# since, when installing Qt, we selected the pre-built components appropriate for our compiler.
# For example, "MSVC 2015 64-bit" when compiling 64-bit using Visual Studio 2015.
#
if (MSVC)
    # CMake tells us for Visual Studio 2015 (and higher) this will: "install the Windows Universal CRT libraries for app-local deployment (e.g. to Windows XP)".
    #
    # I've verified that this copies all the DLLs in the "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt" directory of the Windows SDK
    # (see https://devblogs.microsoft.com/cppblog/introducing-the-universal-crt/).
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)

    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    # Install the runtime libraries in same location as gplates.exe (or pygplates.pyd) so they can be found when executing gplates (or importing pygplates).
    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${STANDALONE_BASE_INSTALL_DIR})
endif()


#######################################################
# Install all dynamically linked dependency libraries #
#######################################################

# List platform-specific parameters to pass to 'file(GET_RUNTIME_DEPENDENCIES ...)'.
#
# Examples include...
# List of regular expressions to exclude when searching runtime dependencies.
# List of directories when searching.
if (WIN32)
    # On Windows exclude 'api-ms-', 'System32' and 'SysWOW64'.
    # The necessary 'api-ms-win-*' get installed when installing the Windows Universal CRT libraries (using InstallRequiredSystemLibraries).
    list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*api-ms-win-.*]])
    list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\][Ss]ystem32[/\\].*]])
    list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\][Ss]ys[Ww][Oo][Ww]64[/\\].*]])
    # Exclude the Python DLL when installing pyGPlates.
    # The Python interpreter (that will 'import pygplates' on the user's system) will load the Python DLL.
    if (NOT GPLATES_BUILD_GPLATES)  # pyGPlates ...
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[^[Pp]ython[0-9]+\.[Dd][Ll][Ll]$]])
    endif()
    # On Windows search for DLLs using the PATH environment variable.
    set(GET_RUNTIME_DEPENDENCIES_DIRECTORIES $ENV{PATH})
elseif (APPLE)
    # On macOS exclude '/usr/lib' and '/System/Library'.
    # These should only contain system libraries (ie, should not contain any of our dependency libraries).
    list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[/usr/lib.*]])
    list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[/System/Library.*]])
else() # Linux
    # On Linux don't exclude the standard library locations (eg, '/lib[64]' or '/usr/lib').
    # Our dependency libraries get installed there (by the binary package manager).
endif()

#
# Find the dependency libraries.
#
# Note: file(GET_RUNTIME_DEPENDENCIES) requires CMake 3.16.
#

# The *build* target: executable (for gplates) or module library (for pygplates).
install(CODE "set(_target_file \"$<TARGET_FILE:${BUILD_TARGET}>\")")

install(
        CODE "set(GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES}]])"
        CODE "set(GET_RUNTIME_DEPENDENCIES_DIRECTORIES [[${GET_RUNTIME_DEPENDENCIES_DIRECTORIES}]])"
        # Note: Using \"${QT_PLUGINS_INSTALLED}\"" instead of [[${QT_PLUGINS_INSTALLED}]] because install code needs to evaluate
        #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_INSTALLED). And a side note, it does this at install time...
        CODE "set(QT_PLUGINS_INSTALLED \"${QT_PLUGINS_INSTALLED}\")"
        CODE "set(GDAL_PLUGINS_INSTALLED \"${GDAL_PLUGINS_INSTALLED}\")"
        CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
        CODE [[
            unset(ARGUMENT_EXECUTABLES)
            unset(ARGUMENT_BUNDLE_EXECUTABLE)
            # Search the Qt/GDAL plugins regardless of whether installing gplates or pygplates.
            set(ARGUMENT_MODULES MODULES ${QT_PLUGINS_INSTALLED} ${GDAL_PLUGINS_INSTALLED})
            # Target 'gplates' is an executable and target 'pygplates' is a module.
            if (GPLATES_BUILD_GPLATES)  # GPlates ...
                # Add gplates to the list of executables to search.
                set(ARGUMENT_BUNDLE_EXECUTABLE BUNDLE_EXECUTABLE "${_target_file}")  # gplates
                set(ARGUMENT_EXECUTABLES EXECUTABLES "${_target_file}")  # gplates
            else()  # pyGPlates ...
                # Add pygplates to the list of modules to search.
                set(ARGUMENT_MODULES ${ARGUMENT_MODULES} "${_target_file}")  # pygplates
            endif()

            # Only specify arguments to file(GET_RUNTIME_DEPENDENCIES) if we have them.
            # The arguments that might be empty are DIRECTORIES, PRE_EXCLUDE_REGEXES and POST_EXCLUDE_REGEXES.
            unset(ARGUMENT_DIRECTORIES)
            unset(ARGUMENT_PRE_EXCLUDE_REGEXES)
            unset(ARGUMENT_POST_EXCLUDE_REGEXES)
            if (GET_RUNTIME_DEPENDENCIES_DIRECTORIES)
                set(ARGUMENT_DIRECTORIES DIRECTORIES ${GET_RUNTIME_DEPENDENCIES_DIRECTORIES})
            endif()
            if (GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES)
                set(ARGUMENT_PRE_EXCLUDE_REGEXES PRE_EXCLUDE_REGEXES ${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES})
                set(ARGUMENT_POST_EXCLUDE_REGEXES POST_EXCLUDE_REGEXES ${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES})
            endif()

            #
            # Search the *build* target, but we'll later install its dependencies into the *install* location.
            #
            file(GET_RUNTIME_DEPENDENCIES
                ${ARGUMENT_EXECUTABLES}  # Evaluates to empty for pygplates.
                # Also search the Qt/GDAL plugins (since they're not discoverable because not dynamically linked)...
                ${ARGUMENT_MODULES}  # Also includes pygplates (when installing pygplates).
                ${ARGUMENT_BUNDLE_EXECUTABLE}  # Ignored on non-Apple platforms, and evaluates to empty for pygplates.
                RESOLVED_DEPENDENCIES_VAR _resolved_dependencies
                UNRESOLVED_DEPENDENCIES_VAR _unresolved_dependencies
                CONFLICTING_DEPENDENCIES_PREFIX _conflicting_dependencies
                ${ARGUMENT_DIRECTORIES}  # Can evaluate to empty.
                ${ARGUMENT_PRE_EXCLUDE_REGEXES}  # Can evaluate to empty.
                ${ARGUMENT_POST_EXCLUDE_REGEXES})  # Can evaluate to empty.

            # Fail if any unresolved/conflicting dependencies.
            if (_unresolved_dependencies)
                message(FATAL_ERROR "There were unresolved dependencies of \"${_target_file}\":
                    ${_unresolved_dependencies}")
            endif()
            if (_conflicting_dependencies)
                message(FATAL_ERROR "There were conflicting dependencies of \"${_target_file}\":
                    ${_conflicting_dependencies}")
            endif()
        ]]
)


#
# Install the dependency libraries.
#
if (WIN32)

    # On Windows we simply copy the dependency DLLs to the install prefix location (where 'gplates.exe', or 'pygplates.pyd', is)
    # so that they will get found at runtime by virtue of being in the same directory.
    install(
            CODE "set(STANDALONE_BASE_INSTALL_DIR [[${STANDALONE_BASE_INSTALL_DIR}]])"
            CODE [[
                # Install the dependency libraries in the *install* location.
                foreach(_resolved_dependency ${_resolved_dependencies})
                    file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}")
                endforeach()
            ]]
    )

elseif (APPLE)

    #
    # On macOS we need to:
    #   1 - Copy each resolved direct and indirect dependency of GPlates (or pyGPlates) and its Qt/GDAL plugins into the appropriate location inside the
    #       installed GPlates application bundle (or base install directory of pyGPlates) depending of whether a regular '.dylib' or a framework.
    #   2 - Fix up the path to each *direct* dependency of GPlates (or pyGPlates), its Qt/GDAL plugins and their resolved dependencies
    #       (ie, each dependency will depend on other dependencies in turn and must point to their location within the installation).
    #   3 - Code sign GPlates (or pyGPlates), its Qt/GDAL plugins and their resolved dependencies with a valid Developer ID certificate.
    #       For GPlates we also then code sign the entire application *bundle* (for pyGPlates, the 'pygplates.so' library has already been signed).
    #

    # Find the 'codesign' command.
    find_program(CODESIGN "codesign")
    if (NOT CODESIGN)
        message(FATAL_ERROR "Unable to find 'codesign' command - cannot sign installed bundle with a Developer ID cerficate")
    endif()

    # Create an entitlements file for code signing.
    #
    # Apple notarization of Developer ID signed app bundles requires hardened runtime to succeed.
    # However that also seems to prevent using relative paths inside our bundle (such as "@executable_path/../Frameworks/...").
    # Currently we get around that by enabling the 'disable-library-validation' entitlement - although it would be better if we
    # didn't (because that enables unsigned libraries inside bundle to be loaded, although we do sign all ours).
    # Still, it's a bit of a security loophole.
    set(ENTITLEMENTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/gplates.entitlements")
    file(WRITE "${ENTITLEMENTS_FILE}" [[
            <?xml version="1.0" encoding="UTF-8"?>
            <!DOCTYPE plist PUBLIC "-//Apple Computer/DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
            <plist version="1.0">
            <dict>
                <key>com.apple.security.cs.disable-library-validation</key>
                <true/>
            </dict>
            </plist>
        ]])

    #
    # Function to code sign an installed file/directory using a code signing identity (typically a Developer ID).
    install(
        CODE "set(CODESIGN [[${CODESIGN}]])"
        CODE "set(ENTITLEMENTS_FILE [[${ENTITLEMENTS_FILE}]])"
        # The CMake cache variable configured by the user to specify their code signing identity on macOS...
        CODE "set(CODE_SIGN_IDENTITY [[${GPLATES_APPLE_CODE_SIGN_IDENTITY}]])"
        CODE [[
            function(codesign installed_file)
                # Only sign if a signing identity was provided.
                if (CODE_SIGN_IDENTITY)
                    # Run 'codesign' to sign installed file/directory with a Developer ID certificate.
                    # Note that we need "--timestamp" to provide a secure timestamp, otherwise notarization will fail.
                    execute_process(
                        COMMAND ${CODESIGN} --timestamp --force --verify --options runtime --sign ${CODE_SIGN_IDENTITY}
                                --entitlements ${ENTITLEMENTS_FILE} ${installed_file}
                        RESULT_VARIABLE _codesign_result
                        OUTPUT_VARIABLE _codesign_output
                        ERROR_VARIABLE _codesign_error)
                    if (_codesign_result)
                        message(FATAL_ERROR "${CODESIGN} failed: ${_codesign_error}")
                    endif()
                endif()
            endfunction()
        ]]
    )

    # Copy each resolved dependency of GPlates (or pyGPlates) and its Qt/GDAL plugins into the appropriate location inside the installed GPlates application bundle
    # (or base install directory of pygplates) depending of whether a regular '.dylib' or a framework.
    install(
            #
            # Function to install/copy the *framework* of a resolved dependency library into the installed bundle.
            #
            # A framework should look like the following:
            #
            #   Dependency.framework/
            #                        Dependency -> Versions/Current/Dependency
            #                        Resources  -> Versions/Current/Resources
            #                        Versions/
            #                                 Current -> 2
            #                                 2/
            #                                         Dependency
            #                                         Resources/
            #                                                       Info.plist
            #
            # We only copy the resolved dependency library itself (eg, 'Dependency.framework/Versions/2/Dependency') and the framework 'Resources' directory
            # (eg, 'Dependency.framework/Versions/2/Resources/'), while also setting up the symbolic links shown above if the framework is versioned
            # (eg, 'Dependency.framework/Dependency', 'Dependency.framework/Resources' and 'Dependency.framework/Versions/Current').
            #
            # The code in this function was inspired by the CMake 3.19 implementation of BundleUtilities.
            #
            CODE [[
                function(install_framework resolved_dependency install_framework_prefix installed_dependency)
                    # Get the *install* dependency directory (within the install framework) to copy the resolved dependency library to.
                    # For example, convert '/.../Dependency.framework/Versions/2/Dependency' to '${install_framework_prefix}/Dependency.framework/Versions/2'.
                    string(REGEX REPLACE "^.*/([^/]+\\.framework/.+)/[^/]+$" "${install_framework_prefix}/\\1" _install_dependency_dir "${resolved_dependency}")

                    # Copy the resolved dependency library into the install dependency directory.
                    # For example, copy '/.../Dependency.framework/Versions/2/Dependency' to '${install_framework_prefix}/Dependency.framework/Versions/2'.
                    file(INSTALL "${resolved_dependency}" DESTINATION "${_install_dependency_dir}" FOLLOW_SYMLINK_CHAIN)

                    # Get the directory of the resolved dependency library.
                    # For example, convert '/.../Dependency.framework/Versions/2/Dependency' to '/.../Dependency.framework/Versions/2'.
                    string(REGEX REPLACE "^(.*)/[^/]+$" "\\1" _resolved_dependency_dir "${resolved_dependency}")

                    # If there's a 'Resources/' directory (in same directory as resolved dependency library) then
                    # copy it to the equivalent directory in the installed framework.
                    # For example, if there's a '/.../Dependency.framework/Versions/2/Resources' directory.
                    if (EXISTS "${_resolved_dependency_dir}/Resources")
                        # For example, copy '/.../Dependency.framework/Versions/2/Resources' to '${install_framework_prefix}/Dependency.framework/Versions/2'.
                        file(INSTALL "${_resolved_dependency_dir}/Resources" DESTINATION "${_install_dependency_dir}" FOLLOW_SYMLINK_CHAIN
                                # Exclude any app bundles inside the 'Resources/' directory since otherwise these would also need to be code signed and
                                # have a hardened runtime (for notarization) and have a secure timestamp (also for notarization).
                                REGEX "[^/]+\\.app" EXCLUDE)
                    endif()

                    # See if there's a "Versions" directory in the framework.
                    # For example, convert '/.../Dependency.framework/Versions/2/Dependency' to 'Versions'.
                    string(REGEX REPLACE "^.*/([^/]+)/[^/]+/[^/]+$" "\\1" _versions_dir_basename "${resolved_dependency}")
                    if (_versions_dir_basename STREQUAL "Versions")
                        # _install_versions_dir = '${install_framework_prefix}/Dependency.framework/Versions'
                        string(REGEX REPLACE "^(.*)/[^/]+$" "\\1" _install_versions_dir "${_install_dependency_dir}")

                        # Create symbolic link (eg, '${install_framework_prefix}/Dependency.framework/Versions/Current' -> '2').
                        if (NOT EXISTS "${_install_versions_dir}/Current")
                            # Get the version directory (eg, "2" from '${install_framework_prefix}/Dependency.framework/Versions/2').
                            string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _install_version_dir_basename "${_install_dependency_dir}")

                            # Create symbolic link.
                            # Note: file(CREATE_LINK) requires CMake 3.14.
                            file(CREATE_LINK "${_install_version_dir_basename}" "${_install_versions_dir}/Current" SYMBOLIC)
                        endif()

                        # Get '${install_framework_prefix}/Dependency.framework' from '${install_framework_prefix}/Dependency.framework/Versions/2'.
                        string(REGEX REPLACE "^(.*)/[^/]+/[^/]+$" "\\1" _install_framework_dir "${_install_dependency_dir}")

                        # Create symbolic link (eg, '${install_framework_prefix}/Dependency.framework/Resources' -> 'Versions/Current/Resources').
                        if (NOT EXISTS "${_install_framework_dir}/Resources")
                            # Create symbolic link.
                            # Note: file(CREATE_LINK) requires CMake 3.14.
                            file(CREATE_LINK "Versions/Current/Resources" "${_install_framework_dir}/Resources" SYMBOLIC)
                        endif()

                        # Get 'Dependency' from '/.../Dependency.framework/Versions/2/Dependency'.
                        string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _dependency_basename "${resolved_dependency}")

                        # Create symbolic link (eg, '${install_framework_prefix}/Dependency.framework/Dependency' -> 'Versions/Current/Dependency').
                        if (NOT EXISTS "${_install_framework_dir}/${_dependency_basename}")
                            # Create symbolic link.
                            # Note: file(CREATE_LINK) requires CMake 3.14.
                            file(CREATE_LINK "Versions/Current/${_dependency_basename}" "${_install_framework_dir}/${_dependency_basename}" SYMBOLIC)
                        endif()
                    endif()

                    # Get '${install_framework_prefix}/Dependency.framework/Versions/2/Dependency' from '/.../Dependency.framework/Versions/2/Dependency'.
                    string(REGEX REPLACE "^.*/([^/]+\\.framework/.*)$" "${install_framework_prefix}/\\1" _installed_dependency "${resolved_dependency}")
                    # Set caller's 'installed_dependency'.
                    set(${installed_dependency} "${_installed_dependency}" PARENT_SCOPE)
                endfunction()
            ]]
            #
            # Copy each resolved dependency of GPlates (or pyGPlates) and its Qt/GDAL plugins into the appropriate location inside the installed GPlates application bundle
            # (or base install directory of pygplates) depending of whether a regular '.dylib' or a framework.
            #
            CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR})"
            CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
            CODE [[
                set(_installed_dependencies)
                foreach(_resolved_dependency ${_resolved_dependencies})
                    # If the resolved dependency is inside a framework then copy the framework into the GPlates bundle or pyGPlates install location
                    # (but only copy the resolved dependency library and the framework 'Resources/' directory).
                    if (_resolved_dependency MATCHES "[^/]+\\.framework/")
                        if (GPLATES_BUILD_GPLATES)  # GPlates ...
                            # Install in the 'Contents/Frameworks/' sub-directory of the 'gplates' app bundle.
                            install_framework(${_resolved_dependency} "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/Frameworks" _installed_dependency)
                        else()  # pyGPlates ...
                            # Install in the 'Frameworks/' sub-directory of base install directory of pygplates.
                            install_framework(${_resolved_dependency} "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/Frameworks" _installed_dependency)
                        endif()
                    else()  # regular '.dylib' ...
                        # Ensure we copy symlinks (using FOLLOW_SYMLINK_CHAIN). For example, with 'libCGAL.13.dylib -> libCGAL.13.0.3.dylib' both the symlink
                        # 'libCGAL.13.dylib' and the dereferenced library 'libCGAL.13.0.3.dylib' are copied, otherwise just the symlink would be copied.
                        if (GPLATES_BUILD_GPLATES)  # GPlates ...
                            # Install in the 'Contents/MacOS/' sub-directory of 'gplates' app bundle.
                            file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS" FOLLOW_SYMLINK_CHAIN)
                        else()  # pyGPlates ...
                            # Install in the 'lib/' sub-directory of base install directory of pygplates.
                            file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib" FOLLOW_SYMLINK_CHAIN)
                        endif()

                        if (GPLATES_BUILD_GPLATES)  # GPlates ...
                            # Get '${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/dependency.dylib' from resolved dependency.
                            string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/\\1" _installed_dependency "${_resolved_dependency}")
                        else()  # pyGPlates ...
                            # Get '${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/dependency.dylib' from resolved dependency.
                            string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/\\1" _installed_dependency "${_resolved_dependency}")
                        endif()
                    endif()

                    # Add installed dependency to the list.
                    list(APPEND _installed_dependencies "${_installed_dependency}")
                endforeach()
            ]]
    )

    # Find the 'otool' command.
    find_program(OTOOL "otool")
    if (NOT OTOOL)
        message(FATAL_ERROR "Unable to find 'otool' command - cannot fix dependency paths to reference inside installation")
    endif()

    # Find the 'install_name_tool' command.
    find_program(INSTALL_NAME_TOOL "install_name_tool")
    if (NOT INSTALL_NAME_TOOL)
        message(FATAL_ERROR "Unable to find 'install_name_tool' command - cannot fix dependency paths to reference inside installation")
    endif()

    # Fix up the path to each *direct* dependency of GPlates (or pyGPlates), its Qt/GDAL plugins and their installed dependencies.
    # Each dependency will depend on other dependencies in turn and must point to their location within the installation.
    # At the same time code sign GPlates (or pyGPlates), its Qt/GDAL plugins and their installed dependencies with a valid Developer ID certificate.
    install(
            #
            # Function to find the relative path from the directory of the installed file to the specified installed dependency library.
            #
            # This is only needed for installing pyGPlates because it uses @loader_path which is different between the pygplates library,
            # its Qt/GDAL plugins and dependency frameworks/libraries (whereas GPlates uses @executable_path which is fixed for all).
            # The returned relative path can then be used as '@loader_path/<relative_path>'.
            #
            CODE [[
                function(get_relative_path_to_installed_dependency installed_file installed_dependency installed_dependency_relative_path)

                    get_filename_component(_installed_file_dir ${installed_file} DIRECTORY)

                    # Need to optionally convert relative paths to absolute paths (required by file(RELATIVE_PATH)) because it's possible that
                    # CMAKE_INSTALL_PREFIX (embedded in install paths) is a relative path (eg, 'staging' if installing with
                    # 'cmake --install . --prefix staging').
                    #
                    # Note that both the installed file and installed dependency will have paths starting with CMAKE_INSTALL_PREFIX so the
                    # relative path will be unaffected by whatever absolute prefix we use, so we don't need to specify BASE_DIR
                    # (it will default to 'CMAKE_CURRENT_SOURCE_DIR' which defaults to the current working directory when this
                    # install code is finally run in cmake script mode '-P' but, as mentioned, it doesn't matter what this is).
                    get_filename_component(_installed_file_dir ${_installed_file_dir} ABSOLUTE)
                    get_filename_component(installed_dependency ${installed_dependency} ABSOLUTE)

                    # Get the relative path.
                    file(RELATIVE_PATH _installed_dependency_relative_path ${_installed_file_dir} ${installed_dependency})

                    # Set caller's relative path.
                    set(${installed_dependency_relative_path} ${_installed_dependency_relative_path} PARENT_SCOPE)
                endfunction()
            ]]

            CODE "set(OTOOL [[${OTOOL}]])"
            CODE "set(INSTALL_NAME_TOOL [[${INSTALL_NAME_TOOL}]])"
            CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR})"
            CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
            #
            # Function to change the dependency install names in the specified install file so that it references the
            # installed dependency file's location relative to the GPlates executable (or pyGPlates library).
            #
            CODE [[
                function(fix_dependency_install_names installed_file)
                    # Run 'otool -L <installed-file>' to get a list of dependencies.
                    execute_process(
                        COMMAND ${OTOOL} -L ${installed_file}
                        RESULT_VARIABLE _otool_result
                        OUTPUT_VARIABLE _otool_output
                        ERROR_VARIABLE _otool_error)
                    if (_otool_result)
                        message(FATAL_ERROR "${OTOOL} failed: ${_otool_error}")
                    endif()

                    # Convert 'otool' output to a list of lines.
                    # We do this by converting newlines to the list separator character ';' but
                    # only after escaping any existing ';' characters in the output.
                    string(REPLACE ";" "\\;" _otool_output_lines "${_otool_output}")
                    string(REPLACE "\n" ";" _otool_output_lines "${_otool_output_lines}")

                    # Extract a dependency from each line in the output.
                    set(_dependency_file_install_names)
                    foreach(_otool_output_line ${_otool_output_lines})
                        # Dependency lines follow the format " <install-name> (<versioning-info>)".
                        # Extract the <install-name> part.
                        # Lines containing dependencies look like:
                        #   dependency.dylib (compatibility version 0.0.0, current version 0.0.0)
                        # ...so we pattern match using the parentheses.
                        if (_otool_output_line MATCHES ".*\\(.*\\)")
                            string(REGEX REPLACE "^(.*)\\(.*\\).*$" "\\1" _dependency_file_install_name "${_otool_output_line}")
                            string(STRIP ${_dependency_file_install_name} _dependency_file_install_name)
                            if (_dependency_file_install_name)  # Might be that the last line is empty for some reason
                                list(APPEND _dependency_file_install_names "${_dependency_file_install_name}")
                            endif()
                        endif()
                    endforeach()

                    # Accumulate 'install_name_tool' options '-change <old> <new>' for each dependency.
                    set(_change_installed_dependency_file_install_names_options)
                    foreach(_dependency_file_install_name ${_dependency_file_install_names})

                        # Skip system libraries since these are also on the deployed system and
                        # hence their install name will still apply (on the deployed system).
                        if ((_dependency_file_install_name MATCHES "^/usr/lib") OR
                            (_dependency_file_install_name MATCHES "^/System"))
                            continue()
                        endif()

                        # Get the install name for the installed dependency (ie, the dependency location in the installed bundle).
                        #
                        # See if it's a framework.
                        if (_dependency_file_install_name MATCHES "[^/]+\\.framework/")
                            if (GPLATES_BUILD_GPLATES)  # GPlates ...
                                # For example, "@executable_path/../Frameworks/Dependency.framework/Versions/2/Dependency".
                                string(REGEX REPLACE "^.*/([^/]+\\.framework/.*)$" "@executable_path/../Frameworks/\\1"
                                        _installed_dependency_file_install_name "${_dependency_file_install_name}")
                            else()  # pyGPlates ...
                                # For example, "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/Frameworks/Dependency.framework/Versions/2/Dependency".
                                string(REGEX REPLACE "^.*/([^/]+\\.framework/.*)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/Frameworks/\\1"
                                        _installed_dependency_file_name "${_dependency_file_install_name}")
                                # Get the relative path from the installed file to its installed dependency.
                                if (_installed_dependency_file_name STREQUAL _dependency_file_install_name)
                                    # string(REGEX REPLACE) did not find a match.
                                    # This can happen when the installed file references itself as a dependency.
                                    # In this case just use "@loader_path".
                                    set(_installed_dependency_file_install_name "@loader_path")
                                else()
                                    # Get the path of installed dependency relative to the referencing installed file.
                                    get_relative_path_to_installed_dependency(
                                            ${installed_file} ${_installed_dependency_file_name} _relative_path_to_installed_dependency)
                                    set(_installed_dependency_file_install_name "@loader_path/${_relative_path_to_installed_dependency}")
                                endif()
                            endif()
                        else()  # it's a regular shared library...
                            if (GPLATES_BUILD_GPLATES)  # GPlates ...
                                # Non-framework librares are installed in same directory as executable.
                                # For example, "@executable_path/../MacOS/dependency.dylib".
                                string(REGEX REPLACE "^.*/([^/]+)$" "@executable_path/../MacOS/\\1"
                                        _installed_dependency_file_install_name "${_dependency_file_install_name}")
                            else()  # pyGPlates ...
                                # Non-framework librares are installed in 'lib/' sub-directory of directory containing pygplates library.
                                # For example, "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/dependency.dylib".
                                string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/\\1"
                                        _installed_dependency_file_name "${_dependency_file_install_name}")
                                # Get the relative path from the installed file to its installed dependency.
                                if (_installed_dependency_file_name STREQUAL _dependency_file_install_name)
                                    # string(REGEX REPLACE) did not find a match.
                                    # This can happen when the installed file references itself as a dependency.
                                    # In this case just use "@loader_path".
                                    set(_installed_dependency_file_install_name "@loader_path")
                                else()
                                    # Get the path of installed dependency relative to the referencing installed file.
                                    get_relative_path_to_installed_dependency(
                                            ${installed_file} ${_installed_dependency_file_name} _relative_path_to_installed_dependency)
                                    set(_installed_dependency_file_install_name "@loader_path/${_relative_path_to_installed_dependency}")
                                endif()
                            endif()
                        endif()

                        # Add '-change <old> <new>' to the list of 'install_name_tool' options.
                        set(_change_installed_dependency_file_install_names_options ${_change_installed_dependency_file_install_names_options}
                                -change "${_dependency_file_install_name}" "${_installed_dependency_file_install_name}")
                    endforeach()

                    # Run 'install_name_tool -change <installed-dependency-file-install-name> <installed-dependency-file> ... <installed-file>' .
                    execute_process(
                        COMMAND ${INSTALL_NAME_TOOL} ${_change_installed_dependency_file_install_names_options} ${installed_file}
                        RESULT_VARIABLE _install_name_tool_result
                        ERROR_VARIABLE _install_name_tool_error)
                    if (_install_name_tool_result)
                        message(FATAL_ERROR "${INSTALL_NAME_TOOL} failed: ${_install_name_tool_error}")
                    endif()

                    # Get the install name for the installed file itself (as opposed to its dependencies).
                    # For this we'll just use the basename since it's not needed at runtime to find dependency libraries.
                    string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _installed_file_install_name "${installed_file}")

                    # Run 'install_name_tool -id <installed-file-install-name> <installed-file>'.
                    #
                    # Note: This does nothing for executables (eg, the 'gplates' executable).
                    #       For example, 'install_name_tool -id gplates ${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/gplates'.
                    execute_process(
                        COMMAND ${INSTALL_NAME_TOOL} -id ${_installed_file_install_name} ${installed_file}
                        RESULT_VARIABLE _install_name_tool_result
                        ERROR_VARIABLE _install_name_tool_error)
                    if (_install_name_tool_result)
                        message(FATAL_ERROR "${INSTALL_NAME_TOOL} failed: ${_install_name_tool_error}")
                    endif()
                endfunction()
            ]]

            # Note: Using \"${QT_PLUGINS_INSTALLED}\"" instead of [[${QT_PLUGINS_INSTALLED}]] because install code needs to evaluate
            #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_INSTALLED). And a side note, it does this at install time...
            CODE "set(QT_PLUGINS_INSTALLED \"${QT_PLUGINS_INSTALLED}\")"
            CODE "set(GDAL_PLUGINS_INSTALLED \"${GDAL_PLUGINS_INSTALLED}\")"
            CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
            CODE "set(STANDALONE_BASE_INSTALL_DIR [[${STANDALONE_BASE_INSTALL_DIR}]])"
            CODE "set(GPLATES_PYTHON_STDLIB_INSTALL_PREFIX [[${GPLATES_PYTHON_STDLIB_INSTALL_PREFIX}]])"
            # The *build* target filename: executable (for gplates) or module library (for pygplates).
            CODE "set(_target_file_name \"$<TARGET_FILE_NAME:${BUILD_TARGET}>\")"
            #
            # Fix up the path to each *direct* dependency of GPlates (or pyGPlates), its Qt/GDAL plugins and their installed dependencies.
            #
            # At the same time code sign GPlates (or pyGPlates), its Qt/GDAL plugins and their installed dependencies with a valid Developer ID certificate (if available).
            #
            CODE [[
                # Fix the dependency install names in each installed dependency.
                foreach(_installed_dependency ${_installed_dependencies})
                    fix_dependency_install_names(${_installed_dependency})
                    # Sign *after* fixing dependencies (since we cannot modify after signing).
                    codesign(${_installed_dependency})
                endforeach()

                if (GPLATES_BUILD_GPLATES)  # GPlates ...
                    #
                    # There are some shared '.so' libraries in the Python framework that need code signing.
                    #
                    # For example, there's a directory called, 'Python.framework/Versions/3.8/lib/python3.8/lib-dynload/' that is in 'sys.path' and contains '.so' libraries.
                    # There's also site packages (eg, in 'Python.framework/Versions/3.8/lib/python3.8/site-packages/' like NumPy that contain '.so' libraries.
                    # We need to codesign and secure timestamp these (otherwise Apple notarization fails).
                    #
                    # Note: The Python standard library is only installed for the 'gplates' target which has an embedded Python interpreter
                    #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
            
                    # Recursively search for '.so' files within the Python standard library.
                    file(GLOB_RECURSE _python_shared_libs "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${GPLATES_PYTHON_STDLIB_INSTALL_PREFIX}/*.so")
                    foreach(_python_shared_lib ${_python_shared_libs})
                        codesign(${_python_shared_lib})
                    endforeach()
                endif()

                # Fix the dependency install names in each installed plugin (Qt and GDAL).
                foreach(_plugin ${QT_PLUGINS_INSTALLED} ${GDAL_PLUGINS_INSTALLED})
                    fix_dependency_install_names(${_plugin})
                    # Sign *after* fixing dependencies (since we cannot modify after signing).
                    codesign(${_plugin})
                endforeach()

                # And finally fix dependencies and code sign the GPlates application bundle (or the pyGPlates library).
                #
                if (GPLATES_BUILD_GPLATES)  # GPlates ...
                    # Fix the dependency install names in the installed gplates executable.
                    fix_dependency_install_names(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/gplates)
                    # Sign *after* fixing dependencies (since we cannot modify after signing).
                    #
                    # NOTE: We sign the entire installed bundle (not just the 'gplates.app/Contents/MacOS/gplates' executable).
                    #       And this must be done as the last step.
                    codesign(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app)
                else()  # pyGPlates ...
                    # Fix the dependency install names in the installed pygplates library.
                    fix_dependency_install_names(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_target_file_name})
                    # Sign *after* fixing dependencies (since we cannot modify after signing).
                    codesign(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_target_file_name})
                endif()
            ]]
    )

else()  # Linux

    #
    # On Linux we need to:
    #   1 - copy each resolved direct and indirect dependency library of GPlates (or pyGPlates) and its Qt/GDAL plugins into the 'lib/' sub-directory of base install directory,
    #   2 - specify an appropriate RPATH for GPlates (or pyGPlates), its Qt/GDAL plugins and their resolved dependencies
    #       (ie, each dependency will depend on other dependencies in turn and must point to their location, ie, in 'lib/').
    #

    install(
            #
            # Copy each resolved dependency of GPlates (or pyGPlates) and its Qt/GDAL plugins into the 'lib/' sub-directory of base install directory.
            #
            # On Linux we simply copy the dependency shared libraries to the 'lib/' sub-directory of the
            # install prefix location so that they will get found at runtime from an RPATH of '$ORIGIN/lib' where $ORIGIN is
            # the location of the gplates executable (or pyGPlates library) in the base install directory.
            #
            CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR})"
            CODE "set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})"
            CODE [[
                set(_installed_dependencies)
                foreach(_resolved_dependency ${_resolved_dependencies})
                    # Install into the 'lib/' sub-directory of base install directory.
                    # Ensure we copy symlinks (using FOLLOW_SYMLINK_CHAIN). For example, with 'libCGAL.13.so -> libCGAL.13.0.3.so' both the symlink
                    # 'libCGAL.13.so' and the dereferenced library 'libCGAL.13.0.3.so' are copied, otherwise just the symlink would be copied.
                    file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/" FOLLOW_SYMLINK_CHAIN)

                    # Get '${CMAKE_INSTALL_PREFIX}/<base-install-dir>/lib/dependency.so' from resolved dependency.
                    string(REGEX REPLACE
                        "^.*/([^/]+)$"
                        "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/\\1"
                        _installed_dependency
                        "${_resolved_dependency}")

                    # Add installed dependency to the list.
                    list(APPEND _installed_dependencies "${_installed_dependency}")
                endforeach()
            ]]
    )

    # Find the 'patchelf' command.
    find_program(PATCHELF "patchelf")
    if (NOT PATCHELF)
        message(FATAL_ERROR "Unable to find 'patchelf' command - cannot set RPATH - please install 'patchelf', for example 'sudo apt install patchelf' on Ubuntu")
    endif()

    install(
            CODE "set(PATCHELF [[${PATCHELF}]])"
            CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR})"
            CODE "set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})"
            #
            # Function to set the RPATH of the specified installed file to '$ORIGIN/<relative-path-to-libs-dir>' so that it can
            # find its direct dependency libraries (in the 'lib/' sub-directory of the base install directory).
            #
            CODE [[
                function(set_rpath installed_file)
                    #
                    # Find the relative path from the directory of the installed file to the directory where all the dependency libraries are installed.
                    #

                    get_filename_component(_installed_file_dir ${installed_file} DIRECTORY)

                    # Need to optionally convert relative paths to absolute paths (required by file(RELATIVE_PATH)) because it's possible that
                    # CMAKE_INSTALL_PREFIX is a relative path (eg, 'staging' if installing with 'cmake --install . --prefix staging').
                    #
                    # Note that both the installed file and installed libs will have paths starting with "${CMAKE_INSTALL_PREFIX}/<base-install-dir>"
                    # so the relative path will be unaffected by whatever absolute prefix we use, so we don't need to specify BASE_DIR
                    # (it will default to 'CMAKE_CURRENT_SOURCE_DIR' which defaults to the current working directory when this
                    # install code is finally run in cmake script mode '-P' but, as mentioned, it doesn't matter what this is).
                    get_filename_component(_installed_file_dir ${_installed_file_dir} ABSOLUTE)
                    get_filename_component(_installed_libs_dir "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}" ABSOLUTE)

                    # Get the relative path to the 'libs' sub-directory of base install directory.
                    file(RELATIVE_PATH _relative_path_to_libs_dir "${_installed_file_dir}" "${_installed_libs_dir}")

                    #
                    # Run 'patchelf --set-rpath <rpath> <installed-file>' to set the required RPATH.
                    #
                    execute_process(
                        COMMAND ${PATCHELF} --set-rpath $ORIGIN/${_relative_path_to_libs_dir} ${installed_file}
                        RESULT_VARIABLE _patchelf_result
                        ERROR_VARIABLE _patchelf_error)
                    if (_patchelf_result)
                        message(FATAL_ERROR "${PATCHELF} failed: ${_patchelf_error}")
                    endif()
                endfunction()
            ]]

            # Note: Using \"${QT_PLUGINS_INSTALLED}\"" instead of [[${QT_PLUGINS_INSTALLED}]] because install code needs to evaluate
            #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_INSTALLED). And a side note, it does this at install time...
            CODE "set(QT_PLUGINS_INSTALLED \"${QT_PLUGINS_INSTALLED}\")"
            CODE "set(GDAL_PLUGINS_INSTALLED \"${GDAL_PLUGINS_INSTALLED}\")"
            # The *build* target filename: executable (for gplates) or module library (for pygplates).
            CODE "set(_target_file_name \"$<TARGET_FILE_NAME:${BUILD_TARGET}>\")"
            #
            # Set the RPATH of gplates (or pygplates), its Qt/GDAL plugins and their installed dependencies so that they all can find their direct dependencies.
            #
            # For example, gplates (or pygplates) needs to find its *direct* dependency libraries in 'lib/' and those dependencies need to find their *direct*
            # dependencies (also in 'lib/').
            #
            CODE [[
                # Set the RPATH in each installed dependency.
                foreach(_installed_dependency ${_installed_dependencies})
                    set_rpath(${_installed_dependency})
                endforeach()

                # Set the RPATH in each installed plugin (Qt and GDAL).
                foreach(_plugin ${QT_PLUGINS_INSTALLED} ${GDAL_PLUGINS_INSTALLED})
                    set_rpath(${_plugin})
                endforeach()

                # Set the RPATH in the installed gplates executable (or pygplates library).
                set_rpath(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_target_file_name})
            ]]
    )
endif()  # if (WIN32) ... elif (APPLE) ... else ...
