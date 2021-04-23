################################################################################################
#                                                                                              #
# Install targets.                                                                             #
#                                                                                              #
# This also enables targets to run on *other* machines (rather than just the *build* machine)  #
# on Windows and macOS (by copying dependency artifacts into the install location).            #
#                                                                                              #
################################################################################################

#
# Set the minimum CMake version required for installing targets.
#
if (WIN32 OR APPLE)
    # Currently we're using file(GET_RUNTIME_DEPENDENCIES) which was added in CMake 3.16.
    # And we use FOLLOW_SYMLINK_CHAIN in file(INSTALL) which requires CMake 3.15.
    # And we also use generator expressions in install(CODE) which requires CMake 3.14.
    set (CMAKE_VERSION_REQUIRED_FOR_INSTALLATION 3.16)
else()
    # No real requirement on Linux currently (just set to same value specified by cmake_minimum_required() in top level 'CMakeLists.txt').
    set (CMAKE_VERSION_REQUIRED_FOR_INSTALLATION ${CMAKE_MINIMUM_REQUIRED_VERSION})
endif()

# Check the CMake minimum requirement at *install* time thus allowing users to build with a lower requirement
# (if they just plan to run the build locally and don't plan to install/deploy).
install(
        CODE "
            if (CMAKE_VERSION VERSION_LESS ${CMAKE_VERSION_REQUIRED_FOR_INSTALLATION})
                message(FATAL_ERROR \"CMake ${CMAKE_VERSION_REQUIRED_FOR_INSTALLATION} is required when *installing* GPlates/pyGPlates\")
            endif()
        "
)


include(GNUInstallDirs)
if (WIN32 OR APPLE)
    # On Windows and macOS we install to the *base* install directory (not 'bin/' sub-directory).
    # For Windows this is because we'll copy the dependency DLLs into the same directory as gplates (so it can find them).
    # For macOS this is because we want the app bundle to be in the base directory so when it's packaged you immediately see the bundle.
    install(TARGETS gplates
        # Support all configurations (ie, no need for CONFIGURATIONS option).
        # Note that if we only supported Release then we'd have to specify 'CONFIGURATIONS Release' for every install() command (not just TARGETS)...
        #
        # CONFIGURATIONS Release RelWithDebInfo MinSizeRel Debug
        RUNTIME DESTINATION .  # Windows
        BUNDLE DESTINATION .)  # Apple
else() # Linux...
    install(TARGETS gplates
        # Support all configurations (ie, no need for CONFIGURATIONS option).
        # Note that if we only supported Release then we'd have to specify 'CONFIGURATIONS Release' for every install() command (not just TARGETS)...
        #
        # CONFIGURATIONS Release RelWithDebInfo MinSizeRel Debug
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})  # Linux
endif()


if(EXISTS "${GPlates_SOURCE_DIR}/scripts/hellinger.py")
    if (WIN32)
        install(FILES  "${GPlates_SOURCE_DIR}/scripts/hellinger.py" DESTINATION scripts/)
    elseif (APPLE)
        install(FILES  "${GPlates_SOURCE_DIR}/scripts/hellinger.py" DESTINATION gplates.app/Contents/Resources/scripts/)
    else()
        install(FILES  "${GPlates_SOURCE_DIR}/scripts/hellinger.py" DESTINATION share/gplates/scripts/)
    endif()
endif()

if(EXISTS "${GPlates_SOURCE_DIR}/scripts/hellinger_maths.py")
    if (WIN32)
        install(FILES  "${GPlates_SOURCE_DIR}/scripts/hellinger_maths.py" DESTINATION scripts/)
    elseif (APPLE)
        install(FILES  "${GPlates_SOURCE_DIR}/scripts/hellinger_maths.py" DESTINATION gplates.app/Contents/Resources/scripts/)
    else()
        install(FILES  "${GPlates_SOURCE_DIR}/scripts/hellinger_maths.py" DESTINATION share/gplates/scripts/)
    endif()
endif()

if(EXISTS "${GPlates_SOURCE_DIR}/SampleData/")
    if (WIN32 OR APPLE)
        install(DIRECTORY SampleData/ DESTINATION SampleData/)
    else()
        install(DIRECTORY SampleData/ DESTINATION share/gplates/SampleData/)
    endif()
endif()

if (NOT (WIN32 OR APPLE))
    if(EXISTS "${GPlates_SOURCE_DIR}/doc/gplates.1.gz")
        install(FILES  "${GPlates_SOURCE_DIR}/doc/gplates.1.gz" DESTINATION share/man/man1/)
    endif()
endif()


#
# On Windows and Apple we have a post-install script to fix up GPlates for deployment to another machine
# (which mainly involves copying dependency libraries into the install location, which subsequently gets packaged).
#
# We don't do this for Linux systems because there we rely on the Linux distribution to provide the dependency libraries.
#
if (WIN32 OR APPLE)
    #
    # Configure a post-install script to fix up GPlates for deployment to another machine.
    #
    # Note that we don't get Qt to deploy its libraries/plugins to our install location (using windeployqt/macdeployqt).
    # Instead we find the Qt library dependencies ourself and we explicitly list the Qt plugins we expect to use.
    # On Windows: The official pre-built Qt binaries are configured for 'dynamic' OpenGL (see https://doc.qt.io/qt-5/windows-requirements.html).
    #             This means the normal desktop OpenGL drivers will be used where sufficient, otherwise Qt falls back to ANGLE or software OpenGL.
    #             This fallback relies on the ANGLE or software DLLs being present. They are dynamically loaded, rather than being dynamically linked, and
    #             so are not found by file(GET_RUNTIME_DEPENDENCIES) and so are not deployed. However 'windeployqt' will include those ANGLE and software DLLs.
    #             But the fact that we're not using 'windeployqt' is fine because GPlates uses OpenGL 3.3 which is above what ANGLE and software supports
    #             and so Qt cannot fallback. Hence not deploying the ANGLE and software DLLs is not a problem. GPlates also tells Qt not to fall back by
    #             specifying the Qt::AA_UseDesktopOpenGL attribute (in the C++ code).
    # On macOS:   The Qt deployment tool 'macdeployqt' actually deploys more than just the Qt libraries/plugins (and the libraries they depend on).
    #             It also deploys all libraries that GPlates depends on (eg, Boost, GDAL, etc). But we're already doing this via file(GET_RUNTIME_DEPENDENCIES)
    #             and we're explicitly listing the Qt plugins. So we don't really need to use 'macdeployqt'.
    #
    
    #
    # Run the deployment code in the install prefix location.
    #
    # This consists of a bunch of "install(CODE ...)" commands to install code into "cmake_install.cmake" that CMake in turn
    # executes at 'install' time into the install prefix location ${CMAKE_INSTALL_PREFIX} (evaluated at 'install' time).
    #
    # Note: The command "install(CODE)" requires CMake 3.14 since we're using generator expressions in the code.
    #
    # Note: When using CODE with double quotes, as with install(CODE "<code>"), variable subsitution is *enabled*.
    #       So we use this when transferring variables.
    #       However when using square brackets, as with install(CODE [[<code>]]), variable subitition is *disabled* (as is escaping).
    #       So we use this for the bulk of code to avoid unwanted variable transfer (instead using variables defined in the install scope)
    #       and to avoid having to escape characters (like double quotes).
    #       An example of this is ${CMAKE_INSTALL_PREFIX} where we use square brackets (CODE [[<code>]]) to ensure it is expanded only at
    #       install time (not at configure time). This is important because it can be different. For example, at configure time it might
    #       default to "c:/Program Files/${PROJECT_NAME}", but when packaging, the install prefix will instead be the staging area.
    #

    ###############################################
    # Install the Visual Studio runtime libraries #
    ###############################################
    #
    # Note that Qt5 should also be using the same runtime libraries because it should be using the same compiler
    # since, when installing Qt, we selected the pre-built components appropriate for our compiler.
    # For example, "MSVC 2015 64-bit" when compiling 64-bit using Visual Studio 2015.
    #
    if (MSVC)
        set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)  # "install the Windows Universal CRT libraries for app-local deployment (e.g. to Windows XP)"
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        include(InstallRequiredSystemLibraries)
        # Note that gplates.exe is in the base install directory (see above install(TARGETS) call)
        # so we install the runtime libraries there also (so they can be found when executing gplates).
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION .)
    endif()

    #####################
    # Install "qt.conf" #
    #####################

    # Create the "qt.conf" file.
    set(QT_CONF_FILE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
    set(QT_PLUGIN_DIR_BASENAME "plugins")
    file(WRITE "${QT_CONF_FILE}" "[Paths]\nPlugins = ${QT_PLUGIN_DIR_BASENAME}\n")

    # Install the "qt.conf" file.
    install(CODE "set(QT_CONF_FILE [[${QT_CONF_FILE}]])")
    if (WIN32)
        # On Windows install into the base install directory.
        install(CODE [[file(INSTALL "${QT_CONF_FILE}" DESTINATION "${CMAKE_INSTALL_PREFIX}")]])
    elseif (APPLE)
        # On macOS install into the bundle 'Resources' directory.
        install(CODE [[file(INSTALL "${QT_CONF_FILE}" DESTINATION "${CMAKE_INSTALL_PREFIX}/gplates.app/Contents/Resources")]])
    endif()

    ######################
    # Install Qt plugins #
    ######################

    # The 'plugins' directory relative to ${CMAKE_INSTALL_PREFIX}.
    if (WIN32)
        set(QT_PLUGINS_INSTALL_PREFIX "${QT_PLUGIN_DIR_BASENAME}")
    elseif (APPLE)
        set(QT_PLUGINS_INSTALL_PREFIX "gplates.app/Contents/${QT_PLUGIN_DIR_BASENAME}")
    endif()

    # Function to install a Qt plugin target. Call as...
    #
    #   install_qt5_plugin(qt_plugin_target, installed_qt_plugin_list)
    #
    # ...and the full path to installed plugin file will be added to 'installed_qt_plugin_list'.
    function(install_qt5_plugin qt_plugin_target installed_qt_plugin_list)
        # Get the target file location of the Qt plugin target.
        #
        # Note that we have access to Qt imported targets (like Qt5::QJpegPlugin) because we
        # (the file containing this code) gets included by 'src/CMakeLists.txt' (which has found
        # Qt and imported its targets) and so the Qt imported targets are visible to us.
        get_target_property(_qt_plugin_path "${qt_plugin_target}" LOCATION)

        if(EXISTS "${_qt_plugin_path}")
            # Extract plugin type (eg, 'imageformats') and filename.
            get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
            get_filename_component(_qt_plugin_type "${_qt_plugin_path}" DIRECTORY)
            get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)

            # The plugin install directory (relative to ${CMAKE_INSTALL_PREFIX}).
            set(_install_qt_plugin_dest "${QT_PLUGINS_INSTALL_PREFIX}/${_qt_plugin_type}")

            # Install the Qt plugin.
            install(FILES "${_qt_plugin_path}" DESTINATION "${_install_qt_plugin_dest}")

            # Use square brackets to avoid evaluating ${CMAKE_INSTALL_PREFIX} at configure time (should be done at install time).
            string(CONCAT _installed_qt_plugin [[${CMAKE_INSTALL_PREFIX}/]] "${_install_qt_plugin_dest}/${_qt_plugin_file}")

            # Add full path to installed plugin file to the plugin list.
            set(_installed_qt_plugin_list ${${installed_qt_plugin_list}})
            list(APPEND _installed_qt_plugin_list "${_installed_qt_plugin}")
            set(${installed_qt_plugin_list} ${_installed_qt_plugin_list} PARENT_SCOPE)  # Set caller's plugin list
        else()
            message(FATAL_ERROR "Qt plugin ${qt_plugin_target} not found")
        endif()
    endfunction()

    # Each installed plugin (full installed path) is added to QT_PLUGINS (which is a list variable).
    # And each installed path has ${CMAKE_INSTALL_PREFIX} in it (to be evaluated at install time).
    # Later we will pass QT_PLUGINS to file(GET_RUNTIME_DEPENDENCIES) to find its dependencies and install them also.

    # Install common platform *independent* plugins (used by GPlates).
    # Note: This list was obtained by running the Qt deployment tool (windeployqt/macdeployqt) on GPlates (to see which plugins it deployed).
    install_qt5_plugin(Qt5::QGenericEnginePlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QSvgIconPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QGifPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QICNSPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QICOPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QJpegPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QSvgPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QTgaPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QTiffPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QWbmpPlugin QT_PLUGINS)
    install_qt5_plugin(Qt5::QWebpPlugin QT_PLUGINS)

    # Install platform *dependent* plugins (used by GPlates).
    # Note: This list was obtained by running the Qt deployment tool (windeployqt/macdeployqt) on GPlates (to see which plugins it deployed).
    if (WIN32)
        install_qt5_plugin(Qt5::QWindowsIntegrationPlugin QT_PLUGINS)
        install_qt5_plugin(Qt5::QWindowsVistaStylePlugin QT_PLUGINS)
    elseif (APPLE)
        install_qt5_plugin(Qt5::QCocoaIntegrationPlugin QT_PLUGINS)
        install_qt5_plugin(Qt5::QMacStylePlugin QT_PLUGINS)
        install_qt5_plugin(Qt5::QMngPlugin QT_PLUGINS)
    endif()

    #######################################################
    # Install all dynamically linked dependency libraries #
    #######################################################

    # List of regular expressions to exclude when searching runtime dependencies.
    # List of directories when searching.
    if (WIN32)
        # On Windows exclude 'api-ms-', 'System32' and 'SysWOW64'.
        # The necessary 'api-ms-*' get installed when installing the Windows Universal CRT libraries (using InstallRequiredSystemLibraries).
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\]api-ms-.*]])
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\][Ss]ystem32[/\\].*]])
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\][Ss]ys[Ww][Oo][Ww]64[/\\].*]])
        # On Windows search for DLLs using the PATH environment variable.
        set(GET_RUNTIME_DEPENDENCIES_DIRECTORIES $ENV{PATH})
    elseif (APPLE)
        # On macOS exclude '/usr/lib' and '/System'.
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[/usr/lib.*]])
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[/System.*]])
    endif()

    #
    # Find the dependency libraries.
    #
    # Note: file(GET_RUNTIME_DEPENDENCIES) requires CMake 3.16.
    #
    install(
            CODE "set(GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES}]])"
            CODE "set(GET_RUNTIME_DEPENDENCIES_DIRECTORIES [[${GET_RUNTIME_DEPENDENCIES_DIRECTORIES}]])"
            # Note: Using \"${QT_PLUGINS}\"" instead of [[${QT_PLUGINS}]] because install code needs to evaluate
            #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS). And a side note, it does this at install time...
            CODE "set(QT_PLUGINS \"${QT_PLUGINS}\")"
            CODE [[
                file(GET_RUNTIME_DEPENDENCIES
                    # Search the *build* target, but we'll later install its dependencies into the *install* location.
                    EXECUTABLES "$<TARGET_FILE:gplates>"
                    # Also search the Qt plugins (since they're not discoverable because not dynamically linked)...
                    MODULES ${QT_PLUGINS}
                    BUNDLE_EXECUTABLE "$<TARGET_FILE:gplates>"  # Ignored on non-Apple platforms
                    RESOLVED_DEPENDENCIES_VAR _resolved_dependencies
                    UNRESOLVED_DEPENDENCIES_VAR _unresolved_dependencies,
                    CONFLICTING_DEPENDENCIES_PREFIX _conflicting_dependencies,
                    DIRECTORIES ${GET_RUNTIME_DEPENDENCIES_DIRECTORIES}
                    PRE_EXCLUDE_REGEXES ${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES}
                    POST_EXCLUDE_REGEXES ${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES})

                # Fail if any unresolved/conflicting dependencies.
                if (_unresolved_dependencies)
                    message(FATAL_ERROR [=[There were unresolved dependencies of "$<TARGET_FILE:gplates>":
                        ${_unresolved_dependencies}]=])
                endif()
                if (_conflicting_dependencies)
                    message(FATAL_ERROR [=[There were conflicting dependencies of "$<TARGET_FILE:gplates>":
                        ${_conflicting_dependencies}]=])
                endif()
            ]]
    )

    #
    # Install the dependency libraries.
    #
    if (WIN32)

        # On Windows we simply copy the dependency DLLs to the install prefix location (where 'gplates.exe' is)
        # where they will get found at runtime by virtue of being in the same directory as 'gplates.exe'.
        install(
                CODE [[
                    # Install the dependency libraries in the *install* location.
                    foreach(_resolved_dependency ${_resolved_dependencies})
                        file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}")
                    endforeach()
                ]]
        )

    elseif (APPLE)

        #
        # On macOS we need to:
        #   1 - copy each resolved direct and indirect dependency (of GPlates and its Qt plugins) into the appropriate location inside the
        #       installed GPlates application bundle (depending of whether a regular '.dylib' or a framework),
        #   2 - fix up the path to each *direct* dependency of GPlates, its Qt plugins and their resolved dependencies
        #       (ie, each dependency will depend on other dependencies in turn and must point to their location within the installed bundle),
        #   3 - code sign GPlates, its Qt plugins and their resolved dependencies with a valid Developer ID certificate and finally code sign the application bundle,
        #   4 - notarize the application bundle (although this is done outside of CMake since it requires uploading to Apple and waiting for them to respond).
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
        # Currently we get around that by enable the 'disable-library-validation' entitlement - although it would be better if we
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
        #
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

        # Copy each resolved dependency (of GPlates and its Qt plugins) into the appropriate location inside the installed GPlates application bundle
        # (depending of whether a regular '.dylib' or a framework).
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

                        # If the framework being installed is Python and there's a 'lib/' directory (in same directory as resolved dependency library) then
                        # copy it to the equivalent directory in the installed framework. This contains the Python libraries (including 'site-packages').
                        # For example, if there's a '/.../Dependency.framework/Versions/3.8/lib' directory.
                        if (_resolved_dependency_dir MATCHES "/Python.framework/" AND EXISTS "${_resolved_dependency_dir}/lib")
                            # For example, copy '/.../Python.framework/Versions/3.8/lib' to '${install_framework_prefix}/Python.framework/Versions/3.8'.
                            #
                            # Note: We don't use the FOLLOW_SYMLINK_CHAIN option since that copies, for example, 'Python.framework/Versions/3.8/Python'
                            #       into 'Python.framework/Versions/3.8/lib/' (due to 'libpython3.8.dylib' symlink in there). But we just want to leave
                            #       'libpython3.8.dylib' pointing to '../Python' which, in turn, we've just installed above (and will later codesign).
                            #       Everything is relatively self-contained in 'Python.framework/Versions/3.8/lib/' so we should be fine without following symlinks.
                            file(INSTALL "${_resolved_dependency_dir}/lib" DESTINATION "${_install_dependency_dir}")

                            # Extract, for example, '3.8' from 'Python.framework/Versions/3.8'.
                            string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _python_version "${_install_dependency_dir}")

                            # There's a directory called, for example, 'Python.framework/Versions/3.8/lib/python3.8/lib-dynload/' that is in 'sys.path' and contains '.so' libraries.
                            # We need to codesign and secure timestamp these (otherwise Apple notarization fails), so use our codesign() function defined above.
                            file(GLOB _python_dynload_libs "${_install_dependency_dir}/lib/python${_python_version}/lib-dynload/*.so")
                            foreach(_python_dynload_lib ${_python_dynload_libs})
                                codesign(${_python_dynload_lib})
                            endforeach()
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
                # Copy each resolved dependency (of GPlates and its Qt plugins) into the appropriate location inside the installed GPlates application bundle
                # (depending of whether a regular '.dylib' or a framework).
                #
                CODE [[
                    set(_installed_dependencies)
                    foreach(_resolved_dependency ${_resolved_dependencies})
                        # If the resolved dependency is inside a framework then copy the framework into the bundle
                        # (but only copy the resolved dependency library and the framework 'Resources/' directory).
                        if (_resolved_dependency MATCHES "[^/]+\\.framework/")
                            # Install in the 'Contents/Frameworks/' sub-directory of app bundle.
                            install_framework(${_resolved_dependency} "${CMAKE_INSTALL_PREFIX}/gplates.app/Contents/Frameworks" _installed_dependency)
                        else()
                            # Install in the 'Contents/MacOS/' sub-directory of app bundle.
                            # Ensure we copy symlinks (using FOLLOW_SYMLINK_CHAIN). For example, with 'libCGAL.13.dylib -> libCGAL.13.0.3.dylib' both the symlink
                            # 'libCGAL.13.dylib' and the dereferenced library 'libCGAL.13.0.3.dylib' are copied, otherwise just the symlink would be copied.
                            file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/gplates.app/Contents/MacOS" FOLLOW_SYMLINK_CHAIN)

                            # Get '${CMAKE_INSTALL_PREFIX}/gplates.app/Contents/MacOS/dependency.dylib' from resolved dependency.
                            string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/gplates.app/Contents/MacOS/\\1" _installed_dependency "${_resolved_dependency}")
                        endif()

                        # Add installed dependency to the list.
                        list(APPEND _installed_dependencies "${_installed_dependency}")
                    endforeach()
                ]]
        )

        # Find the 'otool' command.
        find_program(OTOOL "otool")
        if (NOT OTOOL)
            message(FATAL_ERROR "Unable to find 'otool' command - cannot fix dependency paths to reference inside installed bundle")
        endif()

        # Find the 'install_name_tool' command.
        find_program(INSTALL_NAME_TOOL "install_name_tool")
        if (NOT INSTALL_NAME_TOOL)
            message(FATAL_ERROR "Unable to find 'install_name_tool' command - cannot fix dependency paths to reference inside installed bundle")
        endif()

        # Fix up the path to each *direct* dependency of GPlates, its Qt plugins and their installed dependencies.
        # Each dependency will depend on other dependencies in turn and must point to their location within the installed bundle.
        # At the same time code sign GPlates, its Qt plugins and their installed dependencies with a valid Developer ID certificate.
        install(
                CODE "set(OTOOL [[${OTOOL}]])"
                CODE "set(INSTALL_NAME_TOOL [[${INSTALL_NAME_TOOL}]])"
                #
                # Function to change the dependency install names in the specified install file so that it
                # references the installed dependency file's location relative to the bundle's executable.
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
                            string(REGEX REPLACE "^(.*)\\(.*\\).*$" "\\1" _dependency_file_install_name "${_otool_output_line}")
                            string(STRIP ${_dependency_file_install_name} _dependency_file_install_name)
                            if (_dependency_file_install_name)  # Might be that the last line is empty for some reason
                                list(APPEND _dependency_file_install_names "${_dependency_file_install_name}")
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
                                # For example, "@executable_path/../Frameworks/Dependency.framework/Versions/2/Dependency".
                                string(REGEX REPLACE "^.*/([^/]+\\.framework/.*)$" "@executable_path/../Frameworks/\\1"
                                        _installed_dependency_file_install_name "${_dependency_file_install_name}")
                            else()  # it's a regular shared library...
                                # Non-framework librares are installed in same directory as executable.
                                # For example, "@executable_path/../MacOS/dependency.dylib".
                                string(REGEX REPLACE "^.*/([^/]+)$" "@executable_path/../MacOS/\\1"
                                        _installed_dependency_file_install_name "${_dependency_file_install_name}")
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
                        #       For example, 'install_name_tool -id gplates ${CMAKE_INSTALL_PREFIX}/gplates.app/Contents/MacOS/gplates'.
                        execute_process(
                            COMMAND ${INSTALL_NAME_TOOL} -id ${_installed_file_install_name} ${installed_file}
                            RESULT_VARIABLE _install_name_tool_result
                            ERROR_VARIABLE _install_name_tool_error)
                        if (_install_name_tool_result)
                            message(FATAL_ERROR "${INSTALL_NAME_TOOL} failed: ${_install_name_tool_error}")
                        endif()
                    endfunction()
                ]]

                # Note: Using \"${QT_PLUGINS}\"" instead of [[${QT_PLUGINS}]] because install code needs to evaluate
                #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS). And a side note, it does this at install time...
                CODE "set(QT_PLUGINS \"${QT_PLUGINS}\")"
                #
                # Fix up the path to each *direct* dependency of GPlates, its Qt plugins and their installed dependencies.
                #
                # At the same time code sign GPlates, its Qt plugins and their installed dependencies with a valid Developer ID certificate (if available).
                #
                CODE [[
                    # Fix the dependency install names in each installed dependency.
                    foreach(_installed_dependency ${_installed_dependencies})
                        fix_dependency_install_names(${_installed_dependency})
                        # Sign *after* fixing dependencies (since we cannot modify after signing).
                        codesign(${_installed_dependency})
                    endforeach()

                    # Fix the dependency install names in each installed Qt plugin.
                    foreach(_qt_plugin ${QT_PLUGINS})
                        fix_dependency_install_names(${_qt_plugin})
                        # Sign *after* fixing dependencies (since we cannot modify after signing).
                        codesign(${_qt_plugin})
                    endforeach()

                    # Fix the dependency install names in the gplates executable.
                    fix_dependency_install_names(${CMAKE_INSTALL_PREFIX}/gplates.app/Contents/MacOS/gplates)

                    # And finally code sign the application bundle,
                    #
                    # Sign *after* fixing dependencies (since we cannot modify after signing).
                    # Note that we sign the entire installed bundle last.
                    codesign(${CMAKE_INSTALL_PREFIX}/gplates.app)
                ]]
        )
    endif()
endif()
