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
    #
    # Note: A system runtime library installed here (eg, VCRUNTIME140.dll) can subsequently be
    #       overwritten by a copy found by file(GET_RUNTIME_DEPENDENCIES) below, since both install
    #       to the same destination and the dependency install runs afterwards. This happens in a
    #       conda environment (which ships its own VCRUNTIME140.dll on the PATH). It's benign because
    #       both are valid MSVC runtimes, but be aware the installed copy may not be the Visual Studio
    #       redistributable one that InstallRequiredSystemLibraries provides.
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
        # The *source* Qt/GDAL plugin files (not the installed copies). We scan these for their runtime
        # dependencies because some libraries (eg, from conda) use relative rpaths (eg, '@loader_path/...')
        # that only resolve at the source location, not the (as-yet unpopulated) install location. These
        # are plain absolute paths (no ${CMAKE_INSTALL_PREFIX}), so use square brackets.
        CODE "set(QT_PLUGINS_SOURCE [[${QT_PLUGINS_SOURCE}]])"
        CODE "set(GDAL_PLUGINS_SOURCE [[${GDAL_PLUGINS_SOURCE}]])"
        # Note: Using \"${QT_PLUGINS_INSTALLED}\"" instead of [[${QT_PLUGINS_INSTALLED}]] because install code needs to evaluate
        #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_INSTALLED). And a side note, it does this at install time...
        CODE "set(QT_PLUGINS_INSTALLED \"${QT_PLUGINS_INSTALLED}\")"
        CODE "set(GDAL_PLUGINS_INSTALLED \"${GDAL_PLUGINS_INSTALLED}\")"
        CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
        # Needed to locate the bundled Python site-packages (only installed for the 'gplates' target).
        CODE "set(STANDALONE_BASE_INSTALL_DIR [[${STANDALONE_BASE_INSTALL_DIR}]])"
        CODE "set(GPLATES_PYTHON_STDLIB_INSTALL_PREFIX [[${GPLATES_PYTHON_STDLIB_INSTALL_PREFIX}]])"
        # The *source* Python standard library directory (not the installed copy) - used to scan the
        # source Python extension modules for dependencies (same relative-rpath reason as the plugins above).
        CODE "set(GPLATES_PYTHON_STDLIB_DIR [[${GPLATES_PYTHON_STDLIB_DIR}]])"
        # Need to set any relevant CMake policies here since install code apparently does not have access to the
        # max policy version specified in cmake_minimum_required().
        # Policy CMP0207 was introduced in CMake 4.3...
        CODE [[
            if(POLICY CMP0207)
                cmake_policy(SET CMP0207 NEW)
            endif()
            # Allow the CMP0009 policy change below to be made inside this included install script
            # without a CMP0011 warning (CMP0011 NEW gives included scripts automatic policy push/pop).
            if(POLICY CMP0011)
                cmake_policy(SET CMP0011 NEW)
            endif()
            # Don't follow symlinks in file(GLOB_RECURSE) below (eg, when scanning bundled Python
            # extension modules, and later the installed frameworks which contain 'Versions/Current'
            # symlinks). This avoids the CMP0009 developer warning and avoids visiting the same '.so'
            # files twice (once via the real versioned path and once via the 'Current' symlink).
            if(POLICY CMP0009)
                cmake_policy(SET CMP0009 NEW)
            endif()
        ]]
        CODE [[
            unset(ARGUMENT_EXECUTABLES)
            unset(ARGUMENT_BUNDLE_EXECUTABLE)
            # Search the Qt/GDAL plugins regardless of whether installing gplates or pygplates.
            #
            # Note: We search the *source* plugin files (not the installed copies). Their runtime
            #       dependencies are the same, but some libraries (eg, from conda) use relative rpaths
            #       (eg, '@loader_path/...') that resolve to the dependency libraries only at the source
            #       location; at the install location those libraries are not present yet (they are what
            #       we are about to discover and copy), so scanning the installed copies would leave those
            #       dependencies unresolved. The installed copies still get their dependency paths fixed
            #       up (and are codesigned) further below.
            set(ARGUMENT_MODULES MODULES ${QT_PLUGINS_SOURCE} ${GDAL_PLUGINS_SOURCE})
            # Target 'gplates' is an executable and target 'pygplates' is a module.
            if (GPLATES_BUILD_GPLATES)  # GPlates ...
                # Add gplates to the list of executables to search.
                set(ARGUMENT_BUNDLE_EXECUTABLE BUNDLE_EXECUTABLE "${_target_file}")  # gplates
                set(ARGUMENT_EXECUTABLES EXECUTABLES "${_target_file}")  # gplates
            else()  # pyGPlates ...
                # Add pygplates to the list of modules to search.
                set(ARGUMENT_MODULES ${ARGUMENT_MODULES} "${_target_file}")  # pygplates
            endif()

            # For the 'gplates' target (which embeds a Python interpreter) also search the bundled
            # Python extension modules (eg, numpy's '.pyd' on Windows or '.so' on macOS/Linux) so
            # that their native dependency libraries get discovered and installed. The Python standard
            # library (including 'site-packages') is copied wholesale into the standalone bundle (see
            # Install.cmake), but is not otherwise scanned for dependencies. Without this, eg, numpy
            # fails to import when running gplates outside the environment it was built in (because
            # its BLAS/LAPACK backend library was never bundled).
            #
            # Note: This is gated on GPLATES_BUILD_GPLATES because only 'gplates' installs the
            #       Python standard library (and hence site-packages); 'pygplates' does not (so
            #       there is nothing to search), and its dependencies are handled separately (eg,
            #       by auditwheel/delocate/delvewheel when building pyGPlates wheels).
            unset(_python_backend_libraries)
            if (GPLATES_BUILD_GPLATES)
                # The *source* site-packages directory (the one copied wholesale into the bundle). We scan
                # the source extension modules (not the installed copies) for the same relative-rpath reason
                # as the plugins above - eg, numpy's '.so' reaches its BLAS/LAPACK backend via an
                # '@loader_path'-relative rpath that only resolves at the source location.
                set(_source_site_packages "${GPLATES_PYTHON_STDLIB_DIR}/site-packages")
                if (EXISTS "${_source_site_packages}")
                    # Python extension modules are '.pyd' on Windows and '.so' on macOS/Linux
                    # (only the platform-appropriate suffix will actually match anything).
                    file(GLOB_RECURSE _site_packages_modules
                        "${_source_site_packages}/*.pyd"
                        "${_source_site_packages}/*.so")
                    if (_site_packages_modules)
                        set(ARGUMENT_MODULES ${ARGUMENT_MODULES} ${_site_packages_modules})
                    endif()
                endif()

                # On Windows, numpy (from conda) reaches its BLAS/LAPACK backend (OpenBLAS) through the
                # netlib shim DLLs (libblas/libcblas/liblapack), which use *export forwarding* to reach
                # 'openblas.dll'. Export forwarders are invisible to file(GET_RUNTIME_DEPENDENCIES) (it
                # reads import tables), so the backend is not discovered by scanning the '.pyd' modules
                # alone. Locate it explicitly in the dependency search directories so we can (a) search
                # it for *its* dependencies (below) and (b) install it (further below).
                #
                # Note: This is only needed on Windows. On macOS/Linux the extension modules link their
                #       backend ('.dylib'/'.so') directly (no export forwarders), so scanning the
                #       modules above is sufficient to discover it.
                if (WIN32)
                    foreach(_search_directory ${GET_RUNTIME_DEPENDENCIES_DIRECTORIES})
                        file(GLOB _backend_in_directory "${_search_directory}/*openblas*.dll")
                        if (_backend_in_directory)
                            list(APPEND _python_backend_libraries ${_backend_in_directory})
                        endif()
                    endforeach()
                    if (_python_backend_libraries)
                        list(REMOVE_DUPLICATES _python_backend_libraries)
                        set(ARGUMENT_MODULES ${ARGUMENT_MODULES} ${_python_backend_libraries})
                    endif()
                endif()
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

            # Fail if any unresolved dependencies.
            if (_unresolved_dependencies)
                message(FATAL_ERROR "There were unresolved dependencies of \"${_target_file}\":
                    ${_unresolved_dependencies}")
            endif()

            # Resolve conflicting dependencies (same DLL basename found in more than one search
            # directory). This is normal for conda, which ships some DLLs (eg, zlib.dll) in both
            # the environment root, beside python.exe, and in Library/bin. A conflicting dependency
            # is *not* added to the resolved dependencies by file(GET_RUNTIME_DEPENDENCIES), so we
            # must handle it ourselves (otherwise it would silently be omitted from the install).
            # If all copies of a conflicting dependency are byte-identical then we simply install
            # one of them; if they genuinely differ then we fail (eg, an accidental mix of Qt5 and
            # Qt6 libraries in the search directories).
            #
            # Note: file(GET_RUNTIME_DEPENDENCIES) sets '<prefix>_FILENAMES' (and one
            #       '<prefix>_<filename>' list per conflicting filename), but does not set a
            #       variable named '<prefix>' itself.
            foreach(_conflicting_filename ${_conflicting_dependencies_FILENAMES})
                set(_conflicting_candidates ${_conflicting_dependencies_${_conflicting_filename}})
                list(GET _conflicting_candidates 0 _chosen_candidate)
                file(SHA256 "${_chosen_candidate}" _chosen_candidate_hash)
                set(_conflicting_candidates_identical TRUE)
                foreach(_conflicting_candidate ${_conflicting_candidates})
                    file(SHA256 "${_conflicting_candidate}" _conflicting_candidate_hash)
                    if (NOT _conflicting_candidate_hash STREQUAL _chosen_candidate_hash)
                        set(_conflicting_candidates_identical FALSE)
                    endif()
                endforeach()
                if (_conflicting_candidates_identical)
                    message(STATUS "Multiple identical copies of dependency \"${_conflicting_filename}\" found; installing \"${_chosen_candidate}\".")
                    list(APPEND _resolved_dependencies "${_chosen_candidate}")
                else()
                    message(FATAL_ERROR "Conflicting dependency \"${_conflicting_filename}\" of \"${_target_file}\" resolves to differing libraries:
                        ${_conflicting_candidates}")
                endif()
            endforeach()

            # Install the Python BLAS/LAPACK backend DLL(s) (eg, OpenBLAS) themselves.
            # file(GET_RUNTIME_DEPENDENCIES) searched them (above) for *their* dependencies, but a
            # searched module is not itself added to the resolved dependencies - so add them here.
            if (_python_backend_libraries)
                list(APPEND _resolved_dependencies ${_python_backend_libraries})
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

                        # A resolved dependency is often referenced (and hence resolved) via a symlink rather than the
                        # real library file. For example, consumers link '@rpath/libFoo.6.dylib' and conda ships
                        # 'libFoo.6.dylib -> libFoo.6.11.1.dylib' (and similarly 'libopenblasp-r0.3.33.dylib -> libopenblas.0.dylib').
                        # FOLLOW_SYMLINK_CHAIN copied both the symlink and its real target, so resolve to the *real* file
                        # here and fix/sign that (leaving the symlink a symlink). This matters for code signing: if we
                        # instead process the symlink, 'install_name_tool' replaces it with a second modified *real* file
                        # (which we then sign) while the true versioned library is left untouched - keeping its original,
                        # now-invalidated ad-hoc signature - so 'codesign --deep' and notarization reject the bundle with
                        # "code or signature have been modified". Resolving to the real file ensures it is the one signed.
                        get_filename_component(_installed_dependency "${_installed_dependency}" REALPATH)
                    endif()

                    # Add installed dependency to the list.
                    list(APPEND _installed_dependencies "${_installed_dependency}")
                endforeach()

                # Several resolved dependencies can be symlinks to the same real library (eg, openblas ships multiple
                # differently-named symlinks pointing at 'libopenblas.0.dylib'), which now collapse to the same real
                # file above, so remove duplicates to avoid fixing/signing (and later RPATH-ing) the same file twice.
                list(REMOVE_DUPLICATES _installed_dependencies)
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
                    #
                    # Only run this if there's at least one '-change' option (ie, at least one non-system dependency to fix up).
                    # Some installed files (eg, Python standard library extension modules such as '_datetime.cpython-*.so')
                    # depend *only* on system libraries (in '/usr/lib' or '/System'), which we skip above. In that case the
                    # options list is empty and running 'install_name_tool <installed-file>' with no operations would fail
                    # (install_name_tool prints its usage message and returns a non-zero exit code when given no operations).
                    if (_change_installed_dependency_file_install_names_options)
                        execute_process(
                            COMMAND ${INSTALL_NAME_TOOL} ${_change_installed_dependency_file_install_names_options} ${installed_file}
                            RESULT_VARIABLE _install_name_tool_result
                            ERROR_VARIABLE _install_name_tool_error)
                        if (_install_name_tool_result)
                            message(FATAL_ERROR "${INSTALL_NAME_TOOL} failed: ${_install_name_tool_error}")
                        endif()
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
                # Don't follow symlinks when recursively globbing '.so' files in the installed frameworks below.
                # Frameworks contain a 'Versions/Current' symlink, so following symlinks would visit each '.so'
                # twice (and emit the CMP0009 developer warning).
                # (CMP0011 NEW avoids a warning about changing a policy inside this included install script.)
                if(POLICY CMP0011)
                    cmake_policy(SET CMP0011 NEW)
                endif()
                if(POLICY CMP0009)
                    cmake_policy(SET CMP0009 NEW)
                endif()

                # Fix the dependency install names in each installed dependency, and then codesign the dependency.
                foreach(_installed_dependency ${_installed_dependencies})
                    fix_dependency_install_names(${_installed_dependency})
                    # Sign *after* fixing dependencies (since we cannot modify after signing).
                    codesign(${_installed_dependency})
                endforeach()

                # Get a unique list of installed frameworks that our installed dependencies are contained within (if any).
                set(_installed_frameworks)
                foreach(_installed_dependency ${_installed_dependencies})
                    if (_installed_dependency MATCHES "/[^/]+\\.framework/")
                        # Get the framework directory (the path up to and including '<name>.framework').
                        # For example, '.../gplates.app/Contents/Frameworks/Python.framework'.
                        string(REGEX REPLACE "^(.*/[^/]+\\.framework)/.*$" "\\1" _installed_framework ${_installed_dependency})
                        # Add installed framework to the list.
                        list(APPEND _installed_frameworks "${_installed_framework}")
                    endif()
                endforeach()
                # Remove duplicate frameworks, just in case multiple installed dependencies came from the same framework
                # (and hence that framework got added to the list multiple times).
                list(REMOVE_DUPLICATES _installed_frameworks)

                # Fix dependency install names in, and codesign, the shared '.so' libraries contained within the installed frameworks
                # (after which we codesign the frameworks themselves).
                #
                # For example, there are some shared '.so' libraries in the Python framework that are not dependencies of GPlates/pyGPlates
                # (and hence have not had their dependency install names fixed, nor been codesigned).
                # An example is a directory called 'Python.framework/Versions/3.8/lib/python3.8/lib-dynload/' that contains '.so' libraries (and is in 'sys.path').
                # There's also site packages (eg, in 'Python.framework/Versions/3.8/lib/python3.8/site-packages/') like NumPy that contain '.so' libraries.
                #
                # These '.so' libraries need:
                #   - their dependency install names fixed, so that any *non-system* dependencies (eg, numpy's BLAS/LAPACK backend, which we
                #     now bundle by scanning these '.so' modules in the file(GET_RUNTIME_DEPENDENCIES) step above) are referenced from inside
                #     the bundle (eg, "@executable_path/../MacOS/...") rather than from their original (eg, Macports "/opt/local/lib/...")
                #     location - otherwise, eg, 'import numpy' fails when running gplates outside the environment it was built in.
                #     Extension modules that only depend on system libraries (eg, in '/usr/lib' or '/System') are left unchanged.
                #   - code signing (otherwise Apple notarization fails).
                # Note that fixing the dependency install names must happen *before* codesigning (since we cannot modify after signing).
                #
                # Originally we only applied this logic to the Python framework (since the other frameworks, like the Qt frameworks, don't typically have '.so' libraries).
                # However, we now apply the same logic to all installed frameworks (just in case the other frameworks add '.so' libraries in the future).
                #
                # Note: The Python standard library is only installed for the 'gplates' target which has an embedded Python interpreter
                #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
                #       So it will only get installed (and therefore processed here) for the 'gplates' target.
                #
                foreach(_installed_framework ${_installed_frameworks})
                    # Recursively search for '.so' files within the installed framework (if any).
                    file(GLOB_RECURSE _installed_framework_shared_libs "${_installed_framework}/*.so")
                    foreach(_shared_lib ${_installed_framework_shared_libs})
                        # Fix dependency install names *before* codesigning (since we cannot modify after signing).
                        fix_dependency_install_names(${_shared_lib})
                        codesign(${_shared_lib})
                    endforeach()

                    # Then codesign the framework itself.
                    # This must be done *after* codesigning its contents.
                    #
                    # Note: Previously we did not need to do this.
                    #       But it appears we do now, otherwise we can get the error "a sealed resource is missing or invalid" for the Python framework.
                    codesign(${_installed_framework})
                endforeach()

                # For a non-framework (eg, conda) bundled Python, the standard library is installed
                # *outside* any '.framework' (eg, in 'gplates.app/Contents/Resources/lib/python3.14'),
                # so its extension modules ('.so') were not covered by the installed-frameworks loop above.
                # Fix their dependency install names (so any *non-system* dependencies - eg, numpy's
                # BLAS/LAPACK backend, which we now bundle - reference inside the bundle rather than their
                # original build-machine location) and codesign them (otherwise Apple notarization fails).
                # For a framework Python this is skipped (its '.so' files were already handled above).
                #
                # Note: The Python standard library is only installed for the 'gplates' target (which has an
                #       embedded Python interpreter), so this only applies there.
                if (GPLATES_BUILD_GPLATES AND NOT GPLATES_PYTHON_STDLIB_INSTALL_PREFIX MATCHES "\\.framework/")
                    set(_installed_python_stdlib "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${GPLATES_PYTHON_STDLIB_INSTALL_PREFIX}")
                    if (EXISTS "${_installed_python_stdlib}")
                        file(GLOB_RECURSE _installed_python_shared_libs "${_installed_python_stdlib}/*.so")
                        foreach(_shared_lib ${_installed_python_shared_libs})
                            # Skip symlinks - operate on the real target files (which the glob returns
                            # directly) and avoid any symlink that dangles within the bundle.
                            if (NOT IS_SYMLINK "${_shared_lib}")
                                # Fix dependency install names *before* codesigning (since we cannot modify after signing).
                                fix_dependency_install_names(${_shared_lib})
                                codesign(${_shared_lib})
                            endif()
                        endforeach()
                    endif()
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
            CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
            CODE "set(GPLATES_PYTHON_STDLIB_INSTALL_PREFIX [[${GPLATES_PYTHON_STDLIB_INSTALL_PREFIX}]])"
            # The *build* target filename: executable (for gplates) or module library (for pygplates).
            CODE "set(_target_file_name \"$<TARGET_FILE_NAME:${BUILD_TARGET}>\")"
            #
            # Set the RPATH of gplates (or pygplates), its Qt/GDAL plugins and their installed dependencies so that they all can find their direct dependencies.
            #
            # For example, gplates (or pygplates) needs to find its *direct* dependency libraries in 'lib/' and those dependencies need to find their *direct*
            # dependencies (also in 'lib/').
            #
            CODE [[
                # Don't follow symlinks when recursively globbing '.so' files in the installed Python
                # standard library below (avoids the CMP0009 developer warning and visiting the same
                # '.so' twice via any symlinks).
                # (CMP0011 NEW avoids a warning about changing a policy inside this included install script.)
                if(POLICY CMP0011)
                    cmake_policy(SET CMP0011 NEW)
                endif()
                if(POLICY CMP0009)
                    cmake_policy(SET CMP0009 NEW)
                endif()

                # Set the RPATH in each installed dependency.
                foreach(_installed_dependency ${_installed_dependencies})
                    set_rpath(${_installed_dependency})
                endforeach()

                # Set the RPATH in each installed plugin (Qt and GDAL).
                foreach(_plugin ${QT_PLUGINS_INSTALLED} ${GDAL_PLUGINS_INSTALLED})
                    set_rpath(${_plugin})
                endforeach()

                # Set the RPATH in the bundled Python extension modules (eg, numpy's '.so' files in the
                # installed Python standard library / site-packages) so that they can find their now-bundled
                # native dependencies (eg, the BLAS/LAPACK backend) in the 'lib/' sub-directory - otherwise,
                # eg, 'import numpy' fails when running gplates outside the environment it was built in.
                # Their non-system dependencies are discovered and bundled by scanning these modules in the
                # file(GET_RUNTIME_DEPENDENCIES) step above.
                #
                # Note: The Python standard library is only installed for the 'gplates' target which has an
                #       embedded Python interpreter (not 'pygplates'). So it will only get processed here for
                #       the 'gplates' target.
                if (GPLATES_BUILD_GPLATES)
                    set(_installed_python_stdlib "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${GPLATES_PYTHON_STDLIB_INSTALL_PREFIX}")
                    if (EXISTS "${_installed_python_stdlib}")
                        file(GLOB_RECURSE _installed_python_shared_libs "${_installed_python_stdlib}/*.so")
                        foreach(_shared_lib ${_installed_python_shared_libs})
                            # Skip symlinks. 'patchelf' follows them to their target, but some are dangling
                            # within the bundle - eg, Ubuntu's 'config-*/libpython3.10.so' points to
                            # '../../x86_64-linux-gnu/libpython3.10.so.1', which lives outside the bundled
                            # standard library - which makes patchelf fail. The real extension modules are
                            # regular files and are still patched directly by this loop.
                            if (NOT IS_SYMLINK "${_shared_lib}")
                                set_rpath(${_shared_lib})
                            endif()
                        endforeach()
                    endif()
                endif()

                # Set the RPATH in the installed gplates executable (or pygplates library).
                set_rpath(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_target_file_name})
            ]]
    )
endif()  # if (WIN32) ... elif (APPLE) ... else ...
