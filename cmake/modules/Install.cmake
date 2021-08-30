######################################################################################################
#                                                                                                    #
# Install targets.                                                                                   #
#                                                                                                    #
# This also enables targets to run on *other* machines (rather than just the *build* machine)        #
# by copying dependency artifacts into the install location on Windows and macOS (this is not        #
# typically required on Linux systems which have binary package managers that install dependencies). #
#                                                                                                    #
######################################################################################################

include(GNUInstallDirs)

#
# There are two targets we can install:
# - gplates
# - pygplates
#
# By default we only install 'gplates'. This is done by...
#
# Always specifying COMPONENT in each 'install()' command. And that component is either 'gplates' or 'pygplates'.
# When we specify 'pygplates' we also specify EXCLUDE_FROM_ALL which means the 'pygplates' component is only used for
# component-based installs. An example of a component-based install is running 'cmake --install . --component pygplates'
# after having built pygplates (eg, with 'cmake .' and then 'cmake --build . --target pygplates'). For a non-component install
# (eg, 'cmake --install .') 'pygplates' would be excluded and only 'gplates' installed (due to the EXCLUDE_FROM_ALL flag).
#
# However in most cases you wouldn't typically install directly like this.
# More likely you'd create a package using CPack. For CPack generators we've configured as monolithic (eg, NSIS, DragNDrop and Debian)
# the EXCLUDE_FROM_ALL flag will exclude the pygplates component. For CPack generators we've configured as component-based
# (eg, archive generators like ZIP) CPack will instead use the CPACK_COMPONENTS_ALL variables to select components to package (see Package.cmake).
#
# NOTE: THIS IS CURRENTLY THE PYGPLATES BRANCH.
#       YOU SHOULD ONLY BUILD 'pygplates'. YOU SHOULDN'T BUILD 'gplates' UNTIL THIS BRANCH IS FULLY MERGED TO TRUNK
#       (WHICH CAN ONLY HAPPEN ONCE WE'VE COMPLETELY UPDATED THE INTERNAL MODEL).
#

#
# Check some requirments for installing targets (such as minimum CMake version required).
#
if (GPLATES_INSTALL_STANDALONE)
    #
    # Install GPlates/pyGPlates as a standalone bundle (by copying dependency libraries during installation).
    #

    # Minimum CMake version required at *configure* time.
    #
    # This version of CMake is required to prevent errors at *configure* time.
    #
    # - Currently we're using Qt5 plugin targets which were added in CMake 3.12.
    #   Note that we don't delay this error until install time because we need to access the target
    #   to find its location (to set up the install command) and this needs to be done at configure time.
    set (CMAKE_VERSION_REQUIRED_AT_CONFIGURE_TIME 3.12)

    if (CMAKE_VERSION VERSION_LESS ${CMAKE_VERSION_REQUIRED_AT_CONFIGURE_TIME})
        message(FATAL_ERROR "CMake version ${CMAKE_VERSION_REQUIRED_AT_CONFIGURE_TIME} or greater is needed when GPLATES_INSTALL_STANDALONE is ON")
    endif()

    # Minimum CMake version required at *install* time.
    #
    # This version of CMake is required to prevent errors at *install* time.
    #
    # - Currently we're using file(GET_RUNTIME_DEPENDENCIES) which was added in CMake 3.16.
    # - And we use FOLLOW_SYMLINK_CHAIN in file(INSTALL) which requires CMake 3.15.
    # - And we also use generator expressions in install(CODE) which requires CMake 3.14.
    set (CMAKE_VERSION_REQUIRED_AT_INSTALL_TIME 3.16)

    # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
    function(install_check_cmake_version install_component)
        # Check the CMake minimum requirement at *install* time thus allowing users to build with a lower requirement
        # (if they just plan to run the build locally and don't plan to install/deploy).
        install(
                CODE "
                    if (CMAKE_VERSION VERSION_LESS ${CMAKE_VERSION_REQUIRED_AT_INSTALL_TIME})
                        message(FATAL_ERROR \"CMake version ${CMAKE_VERSION_REQUIRED_AT_INSTALL_TIME} or greater is required when *installing* ${install_component}\")
                    endif()
                "
                COMPONENT ${install_component} ${ARGN}
        )
    endfunction()
    install_check_cmake_version(gplates)
    install_check_cmake_version(pygplates EXCLUDE_FROM_ALL)

    # On Apple, warn if a code signing identity has not been specified.
    #
    # This can avoid wasted time trying to notarize a package (created via cpack) only to fail because it was not code signed.
    if (APPLE)
        # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
        function(install_check_code_signing_identity install_component)
            # Check at *install* time thus allowing users to build without a code signing identity
            # (if they just plan to run the build locally and don't plan to deploy to other machines).
            install(
                    CODE "
                        set(CODE_SIGN_IDENTITY [[${GPLATES_APPLE_CODE_SIGN_IDENTITY}]])
                        if (NOT CODE_SIGN_IDENTITY)
                            message(WARNING [[Code signing identity not specified - please set GPLATES_APPLE_CODE_SIGN_IDENTITY before distributing to other machines]])
                        endif()
                    "
                    COMPONENT ${install_component} ${ARGN}
            )
        endfunction()
        install_check_code_signing_identity(gplates)
        install_check_code_signing_identity(pygplates EXCLUDE_FROM_ALL)
    endif()
endif()


#
# Install gplates/pygplates targets.
#
# Support all target configurations. Ie, no need for the CONFIGURATIONS option in install(TARGETS ...), which is equivalent to...
#
#   install(TARGETS gplates
#       CONFIGURATIONS Release RelWithDebInfo MinSizeRel Debug
#       RUNTIME ...
#       BUNDLE ...
#       LIBRARY ...)
#
# This applies to all install(TARGETS), not just this one.
# Note that if we only supported Release then we'd have to specify 'CONFIGURATIONS Release' for every install() command (not just TARGETS).
#
# Note: GPlates uses RUNTIME and BUNDLE entity types.
#       PyGPlates is a module library which always uses the LIBRARY entity type (according to CMake: "Module libraries are always treated as LIBRARY targets").
#
if (GPLATES_INSTALL_STANDALONE)
    #
    # For standalone we want to bundle everything together so it's relocatable, and it's easier to place gplates/pygplates
    # in the base install directory (along with 'qt.conf', which has to be in the same directory as the exectuable).
    #
    # For Windows this means 'gplates.exe' ultimately gets installed into, for example, "C:\Program Files\GPlates\GPlates 2.3.0"
    # instead of "C:\Program Files\GPlates\GPlates 2.3.0\bin". And we copy the dependency DLLs into the same directory as gplates (so it can find them).
    # For macOS this means you immediately see the app bundle in the base directory (rather than in a 'bin' sub-directory).
    # For Linux the standalone version is typically packaged as an archive (not a '.deb') and the extracted gplates executable will be immediately visible (in base directory).
    set(STANDALONE_BASE_INSTALL_DIR_gplates .)
    #
    # Similar logic applies to 'pygplates' except we install into a 'pygplates' directory since we make pygplates a "Python package" with the pygplates module library in the
    # 'pygplates/' directory as well as an '__init__.py' to find its runtime location (needed to locate the GDAL/PROJ data bundled with pygplates).
    #
    # Note that we only do this for standalone installations because non-standalone installations have GDAL/PROJ installed in a standard location and so GDAL/PROJ are able to
    # find their data directories, which means we don't need to bundle them up with pygplates and so we don't need to make pygplates a "Python package" (with an '__init__.py' file).
    # In other words, we just leave it as a single pygplates shared library file (such as 'pygplates.so' or 'pygplates.pyd') and don't need a 'pygplates/' directory.
    set(STANDALONE_BASE_INSTALL_DIR_pygplates pygplates)
    
    # Install the gplates/pygplates targets.
    install(TARGETS gplates
        RUNTIME # Windows and Linux
            DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}
            COMPONENT gplates
        BUNDLE # Apple
            DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}
            COMPONENT gplates)
    install(TARGETS pygplates
        LIBRARY # Windows, Apple and Linux
            DESTINATION ${STANDALONE_BASE_INSTALL_DIR_pygplates}
            COMPONENT pygplates
            EXCLUDE_FROM_ALL)
else() # not standalone
    #
    # When not a standalone installation just use the standard install locations ('bin' and 'lib').
    #
    install(TARGETS gplates
        RUNTIME # Windows and Linux
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT gplates
        BUNDLE # Apple
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT gplates)
    install(TARGETS pygplates
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT pygplates
            EXCLUDE_FROM_ALL)
endif()


#
# Install Python scripts (but only for gplates target/component).
#
foreach (_script hellinger.py hellinger_maths.py)
    if (EXISTS "${GPlates_SOURCE_DIR}/scripts/${_script}")
        if (GPLATES_INSTALL_STANDALONE)
            # For standalone we want to bundle everything together so it's relocatable.
            if (APPLE)
                install(FILES "${GPlates_SOURCE_DIR}/scripts/${_script}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/gplates.app/Contents/Resources/scripts COMPONENT gplates)
            else()
                install(FILES "${GPlates_SOURCE_DIR}/scripts/${_script}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/scripts COMPONENT gplates)
            endif()
        else()
            install(FILES "${GPlates_SOURCE_DIR}/scripts/${_script}" DESTINATION share/gplates/scripts COMPONENT gplates)
        endif()
    endif()
endforeach()

# Install geodata if requested (but only for gplates target/component).
#
# The variables GPLATES_INSTALL_GEO_DATA and GPLATES_INSTALL_GEO_DATA_DIR are cache variables that the user can set to control this.
#
if (GPLATES_INSTALL_GEO_DATA)
    # Remove the trailing '/', if there is one, so that we can then
    # append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
    #
    #   "The last component of each directory name is appended to the destination directory but
    #    a trailing slash may be used to avoid this because it leaves the last component empty"
    #
    string(REGEX REPLACE "/+$" "" _SOURCE_GEO_DATA_DIR "${GPLATES_INSTALL_GEO_DATA_DIR}")

    #
    # Note: Depending on the installation location ${CMAKE_INSTALL_PREFIX} a path length limit might be
    #       exceeded since some of the geodata paths can be quite long, and combined with ${CMAKE_INSTALL_PREFIX}
    #       could, for example, exceed 260 characters (MAX_PATH) on Windows (eg, when creating an NSIS package).
    #       This can even happen on the latest Windows 10 with long paths opted in.
    #       For example, when packaging with NSIS you can get a geodata file with a path like the following:
    #           <build_dir>\_CPack_Packages\win64\NSIS\gplates_2.3.0_win64\GeoData\<geo_data_file>
    #       ...and currently <geo_data_file> can reach 160 chars, which when added to the middle part
    #       '\_CPack_Packages\...' of ~60 chars becomes ~220 chars leaving only 40 chars for <build_dir>.
    #
    #       Which means you'll need a build directory path that's under 40 characters long (which is pretty short).
    #       Something like "C:\gplates\build\trunk-py37\" (which is already 28 characters).
    #
    if (GPLATES_INSTALL_STANDALONE)
        # For standalone we want to bundle everything together so it's relocatable.
        install(DIRECTORY ${_SOURCE_GEO_DATA_DIR}/ DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/GeoData COMPONENT gplates)
    else()
        install(DIRECTORY ${_SOURCE_GEO_DATA_DIR}/ DESTINATION share/gplates/GeoData COMPONENT gplates)
    endif()
endif()

# Install Linux man page.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")  # Linux
    if (EXISTS "${GPlates_SOURCE_DIR}/doc/gplates.1.gz")
        if (GPLATES_INSTALL_STANDALONE)
            # For standalone we want to bundle everything together so it's relocatable.
            install(FILES  "${GPlates_SOURCE_DIR}/doc/gplates.1.gz" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/man1 COMPONENT gplates)
        else()
            install(FILES  "${GPlates_SOURCE_DIR}/doc/gplates.1.gz" DESTINATION share/man/man1 COMPONENT gplates)
        endif()
    endif()
endif()


#
# Whether to install GPlates as a standalone bundle (by copying dependency libraries during installation).
#
# On Windows and Apple we have install code to fix up GPlates for deployment to another machine
# (which mainly involves copying dependency libraries into the install location, which subsequently gets packaged).
# This is always enabled, so we don't provide an option to the user to disable it.
#
# On Linux systems we don't enable by default (see ConfigDefault.cmake) the copying of dependency libraries because there we rely
# on the Linux binary package manager to install them (for example, we create a '.deb' package that only *lists* the dependencies,
# which are then installed on the target system if not already there).
# However we allow the user to enable this in case they want to create a standalone bundle for their own use case.
#
if (GPLATES_INSTALL_STANDALONE)
    #
    # Configure install code to fix up GPlates for deployment to another machine.
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
    #       So we use this for the bulk of code to avoid unwanted variable transfer (instead using variables defined at *install* time)
    #       and to avoid having to escape characters (like double quotes).
    #       An example of this is ${CMAKE_INSTALL_PREFIX} where we use square brackets (CODE [[<code>]]) to ensure it is expanded only at
    #       install time (not at configure time). This is important because it can be different. For example, at configure time it might
    #       default to "c:/Program Files/${PROJECT_NAME}", but when packaging, the install prefix will instead be the staging area.
    #


    #
    # Function to install the Python standard library.
    #
    # Note: The Python standard library is only installed for the 'gplates' component which has an embedded Python interpreter
    #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
    #
    function(install_python_standard_library)
        if (APPLE)
            # On Apple we're expecting Python to be a framework. Ideally we should already have installed the Python framework
            # (ie, this function should be called after the frameworks have been installed), but this is not absolutely necessary
            # (since we could install the Python standard library first and then install the Python framework library/resources second).
            if (GPLATES_PYTHON_STDLIB_DIR MATCHES "/Python\\.framework/")
                # Convert, for example, '/opt/local/Library/Frameworks/Python.framework/Versions/3.8/lib/python3.8' to
                # 'gplates.app/Contents/Frameworks/Python.framework/Versions/3.8/lib/python3.8'.
                string(REGEX REPLACE "^.*/(Python\\.framework/.*)$" "gplates.app/Contents/Frameworks/\\1" _PYTHON_STDLIB_INSTALL_DIR ${GPLATES_PYTHON_STDLIB_DIR})
            else()
                message(FATAL_ERROR "Expected Python to be a framework")
            endif()
        else() # Windows or Linux
            # Find the relative path from the Python prefix directory to the standard library directory.
            # We'll use this as the standard library install location relative to our install prefix.
            file(RELATIVE_PATH _PYTHON_STDLIB_INSTALL_DIR ${GPLATES_PYTHON_PREFIX_DIR} ${GPLATES_PYTHON_STDLIB_DIR})
        endif()

        # Remove the trailing '/', if there is one, so that we can then
        # append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
        #
        #   "The last component of each directory name is appended to the destination directory but
        #    a trailing slash may be used to avoid this because it leaves the last component empty"
        #
        string(REGEX REPLACE "/+$" "" _PYTHON_STDLIB_DIR "${GPLATES_PYTHON_STDLIB_DIR}")
        # Install the Python standard library.
        install(DIRECTORY "${_PYTHON_STDLIB_DIR}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/${_PYTHON_STDLIB_INSTALL_DIR} COMPONENT gplates)

        # On Windows there's also a 'DLLs/' sibling directory of the 'Lib/' directory.
        if (WIN32)
            get_filename_component(_PYTHON_DLLS_DIR "${_PYTHON_STDLIB_DIR}" DIRECTORY)
            set(_PYTHON_DLLS_DIR "${_PYTHON_DLLS_DIR}/DLLs")
            if (EXISTS "${_PYTHON_DLLS_DIR}")
                install(DIRECTORY "${_PYTHON_DLLS_DIR}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/DLLs COMPONENT gplates)
            endif()
        endif()

        # On Apple there are some shared '.so' libraries in Python framework that need code signing.
        if (APPLE)
            # There's a directory called, for example, 'Python.framework/Versions/3.8/lib/python3.8/lib-dynload/' that is in 'sys.path' and contains '.so' libraries.
            # We need to codesign and secure timestamp these (otherwise Apple notarization fails).
            # So we use our codesign() function - it is defined later but that's fine since this code is not executed until after codesign() has been defined.
            install(
                CODE "set(STANDALONE_BASE_INSTALL_DIR [[${STANDALONE_BASE_INSTALL_DIR_gplates}]])"
                CODE "set(_PYTHON_STDLIB_INSTALL_DIR [[${_PYTHON_STDLIB_INSTALL_DIR}]])"
                CODE [[
                    file(GLOB _python_dynload_libs "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_PYTHON_STDLIB_INSTALL_DIR}/lib-dynload/*.so")
                    foreach(_python_dynload_lib ${_python_dynload_libs})
                        codesign(${_python_dynload_lib})
                    endforeach()
                ]]

                COMPONENT gplates
            )
        endif()
    endfunction()

    ########################################################################################################################
    # Create and install an "__init__.py" file for pygplates in same directory as the pygplates library (on all platforms) #
    ########################################################################################################################
    #
    # This is because we make pygplates a "Python package" where the pygplates module library is in the *base* 'pygplates/' directory
    # as well as an '__init__.py' to find its runtime location (needed to locate the GDAL/PROJ data bundled with pygplates).
    set(PYGPLATES_INIT_PY "${CMAKE_CURRENT_BINARY_DIR}/__init__.py")
    # Note that we allow no indentation in the file content to avoid Python 'unexpected indent' errors.
    file(WRITE "${PYGPLATES_INIT_PY}" [[
# Import the pygplates shared library (C++).
from .pygplates import *
# Import any private symbols (with leading underscore).
from .pygplates import __version__
from .pygplates import __doc__

# Let the pygplates shared library (C++) know of its imported location.
import os.path
pygplates._post_import(os.path.dirname(__file__))

# Now that we've imported symbols from the pygplates shared library (C++) into
# the namespace of this package (also called pygplates) we can remove it.
# This is so we don't have pygplates.<symbol> and pygplates.pygplates.<symbol>.
del pygplates
]])
    install(FILES "${PYGPLATES_INIT_PY}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_pygplates} COMPONENT pygplates EXCLUDE_FROM_ALL)

    #############################################################################################
    # Copy the Proj library data into standalone bundle (to avoid Proj error finding 'proj.db') #
    #############################################################################################
    #
    # Find the 'projinfo' command.
    find_program(PROJINFO_COMMAND "projinfo" PATHS ${PROJ_BINARY_DIRS})
    if (PROJINFO_COMMAND)
        # Run 'projinfo --searchpaths' to get a list of directories that Proj will look for resources in.
        # Note that 'projinfo' is new in Proj version 6.0 and the '--searchpaths' option is new in version 7.0.
        execute_process(COMMAND ${PROJINFO_COMMAND} --searchpaths
            RESULT_VARIABLE _projinfo_result
            OUTPUT_VARIABLE _projinfo_output
            ERROR_QUIET)
        if (NOT _projinfo_result)  # success
            # Convert 'projinfo' output to a list of lines - we do this by converting newlines to the list separator character ';'.
            string(REPLACE "\n" ";" _projinfo_search_paths "${_projinfo_output}")
            # Search each path for 'proj.db'.
            foreach(_projinfo_search_path ${_projinfo_search_paths})
                file(TO_CMAKE_PATH ${_projinfo_search_path} _projinfo_search_path)
                if (EXISTS "${_projinfo_search_path}/proj.db")
                    set(_proj_data_dir ${_projinfo_search_path})
                    break()
                endif()
            endforeach()
            if (NOT _proj_data_dir)
                message(WARNING "Found proj resource dirs but did not find 'proj.db' - proj library data will not be included in standalone bundle.")
            endif()
        else()
            message(WARNING "'projinfo' does not support '--searchpaths' option - likely using Proj version older than 7.0 - proj library data will not be included in standalone bundle.")
        endif()
    else()
        message(WARNING "Unable to find 'projinfo' command - likely using Proj version older than 6.0 - proj library data will not be included in standalone bundle.")
    endif()
    #
    # Install the Proj data.
    if (_proj_data_dir)
        # Remove the trailing '/', if there is one, so that we can then append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
        #   "The last component of each directory name is appended to the destination directory but
        #    a trailing slash may be used to avoid this because it leaves the last component empty"
        string(REGEX REPLACE "/+$" "" _proj_data_dir "${_proj_data_dir}")
        if (APPLE)
            set(_gplates_proj_data_rel_base gplates.app/Contents/Resources/${GPLATES_STANDALONE_PROJ_DATA_DIR})
        else()
            set(_gplates_proj_data_rel_base ${GPLATES_STANDALONE_PROJ_DATA_DIR})
        endif()
        set(_pygplates_proj_data_rel_base ${GPLATES_STANDALONE_PROJ_DATA_DIR})
        install(DIRECTORY "${_proj_data_dir}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/${_gplates_proj_data_rel_base} COMPONENT gplates)
        install(DIRECTORY "${_proj_data_dir}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_pygplates}/${_pygplates_proj_data_rel_base} COMPONENT pygplates EXCLUDE_FROM_ALL)
    endif()

    #################################################################################################
    # Copy the GDAL library data into standalone bundle to avoid GDAL error finding 'gcs.csv'       #
    # (which was moved into 'proj.db' for GDAL >= 2.5, but there's other GDAL data files to bundle) #
    #################################################################################################
    #
    if (WIN32)
        # The 'gdal-config' command is not available on Windows. Instead we're expected to use the GDAL_DATA environment variable.
        set(_gdal_data_dir $ENV{GDAL_DATA})
        if (NOT _gdal_data_dir)
            message(WARNING "GDAL_DATA environment variable not set - GDAL library data will not be included in standalone bundle.")
        endif()
    else() # Apple or Linux
        # Find the 'gdal-config' command (should be able to find via PATH environment variable).
        find_program(GDAL_CONFIG_COMMAND "gdal-config")
        if (GDAL_CONFIG_COMMAND)
            # Run 'gdal-config --datadir' to get directory that GDAL will look for resources in.
            execute_process(COMMAND ${GDAL_CONFIG_COMMAND} --datadir
                RESULT_VARIABLE _gdal_config_result
                OUTPUT_VARIABLE _gdal_config_output
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (NOT _gdal_config_result)  # success
                set(_gdal_data_dir ${_gdal_config_output})
            else()
                message(WARNING "'gdal-config --datadir' failed - GDAL library data will not be included in standalone bundle.")
            endif()
        else()
            message(WARNING "Unable to find 'gdal-config' command - GDAL library data will not be included in standalone bundle.")
        endif()
    endif()
    #
    # Install the GDAL data.
    if (_gdal_data_dir)
        file(TO_CMAKE_PATH ${_gdal_data_dir} _gdal_data_dir)
        if (EXISTS "${_gdal_data_dir}")
            # Remove the trailing '/', if there is one, so that we can then append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
            #   "The last component of each directory name is appended to the destination directory but
            #    a trailing slash may be used to avoid this because it leaves the last component empty"
            string(REGEX REPLACE "/+$" "" _gdal_data_dir "${_gdal_data_dir}")
            if (APPLE)
                set(_gplates_gdal_data_rel_base gplates.app/Contents/Resources/${GPLATES_STANDALONE_GDAL_DATA_DIR})
            else()
                set(_gplates_gdal_data_rel_base ${GPLATES_STANDALONE_GDAL_DATA_DIR})
            endif()
            set(_pygplates_gdal_data_rel_base ${GPLATES_STANDALONE_GDAL_DATA_DIR})
            install(DIRECTORY "${_gdal_data_dir}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/${_gplates_gdal_data_rel_base} COMPONENT gplates)
            install(DIRECTORY "${_gdal_data_dir}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_pygplates}/${_pygplates_gdal_data_rel_base} COMPONENT pygplates EXCLUDE_FROM_ALL)
        else()
            message(WARNING "GDAL data directory \"${_gdal_data_dir}\" does not exist - GDAL library data will not be included in standalone bundle.")
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
        # Install the runtime libraries in same location as gplates.exe (and pygplates.pyd) so they can be found when executing gplates (or importing pygplates).
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates} COMPONENT gplates)
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${STANDALONE_BASE_INSTALL_DIR_pygplates} COMPONENT pygplates EXCLUDE_FROM_ALL)
    endif()

    #####################
    # Install "qt.conf" #
    #####################

    # Create the "qt.conf" file.
    set(QT_CONF_FILE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
    # We'll place the Qt plugins in the 'plugins/' sub-directory of the directory containing 'qt.conf'.
    set(QT_PLUGIN_DIR_BASENAME "plugins")
    file(WRITE "${QT_CONF_FILE}" "[Paths]\nPlugins = ${QT_PLUGIN_DIR_BASENAME}\n")

    # Install the "qt.conf" file for gplates.
    if (APPLE)
        # On macOS install into the bundle 'Resources' directory.
        install(FILES "${QT_CONF_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates}/gplates.app/Contents/Resources COMPONENT gplates)
    else() # Windows or Linux
        # On Windows and Linux install into same directory as executable.
        install(FILES "${QT_CONF_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_gplates} COMPONENT gplates)
    endif()
    # Install the "qt.conf" file for pygplates in same directory as pygplates library (on all platforms).
    install(FILES "${QT_CONF_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR_pygplates} COMPONENT pygplates EXCLUDE_FROM_ALL)

    ######################
    # Install Qt plugins #
    ######################

    # The 'plugins' directory (relative to base install location).
    #
    # For gplates...
    if (APPLE)
        # On macOS relative paths inside 'qt.conf' are relative to 'gplates.app/Contents/'.
        set(QT_PLUGINS_INSTALL_PREFIX_gplates "gplates.app/Contents/${QT_PLUGIN_DIR_BASENAME}")
    else() # Windows or Linux
        # On Windows, and Linux, relative paths inside 'qt.conf' are relative to the directory containing the executable.
        set(QT_PLUGINS_INSTALL_PREFIX_gplates "${QT_PLUGIN_DIR_BASENAME}")
    endif()
    # For pygplates...
    set(QT_PLUGINS_INSTALL_PREFIX_pygplates "${QT_PLUGIN_DIR_BASENAME}")

    # Function to install a Qt plugin target. Call as...
    #
    #   install_qt5_plugin(qt_plugin_target install_component [EXCLUDE_FROM_ALL])
    #
    # ...and the full path to installed plugin file will be added to 'installed_qt_plugin_list'.
    function(install_qt5_plugin qt_plugin_target install_component)
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
            set(_install_qt_plugin_dir ${STANDALONE_BASE_INSTALL_DIR_${install_component}}/${QT_PLUGINS_INSTALL_PREFIX_${install_component}}/${_qt_plugin_type})

            # Install the Qt plugin.
            install(FILES "${_qt_plugin_path}"
                DESTINATION ${_install_qt_plugin_dir}
                COMPONENT ${install_component}
                ${ARGN})

            # Use square brackets to avoid evaluating ${CMAKE_INSTALL_PREFIX} at configure time (should be done at install time).
            string(CONCAT _installed_qt_plugin
                [[${CMAKE_INSTALL_PREFIX}/]]
                ${_install_qt_plugin_dir}/
                ${_qt_plugin_file})

            # Add full path to installed plugin file to the plugin list.
            set(_installed_qt_plugin_list ${QT_PLUGINS_${install_component}})
            list(APPEND _installed_qt_plugin_list "${_installed_qt_plugin}")
            # Set caller's plugin list. This will be, for example, QT_PLUGINS_gplates or QT_PLUGINS_pygplates.
            set(QT_PLUGINS_${install_component} ${_installed_qt_plugin_list} PARENT_SCOPE)
        else()
            message(FATAL_ERROR "Qt plugin ${qt_plugin_target} not found")
        endif()
    endfunction()

    # Each installed plugin (full installed path) is added to QT_PLUGINS_<comp> (which is a list variable).
    # And each installed path has ${CMAKE_INSTALL_PREFIX} in it (to be evaluated at install time).
    # Later we will pass QT_PLUGINS_<comp> to file(GET_RUNTIME_DEPENDENCIES) to find its dependencies and install them also.

    # Install common platform *independent* plugins (used by GPlates and pyGPlates).
    # Note: This list was obtained by running the Qt deployment tool (windeployqt/macdeployqt) on GPlates (to see which plugins it deployed).
    install_qt5_plugin(Qt5::QGenericEnginePlugin gplates)
    install_qt5_plugin(Qt5::QGenericEnginePlugin pygplates EXCLUDE_FROM_ALL)
    install_qt5_plugin(Qt5::QSvgIconPlugin gplates)
    install_qt5_plugin(Qt5::QSvgIconPlugin pygplates EXCLUDE_FROM_ALL)
    install_qt5_plugin(Qt5::QGifPlugin gplates)
    install_qt5_plugin(Qt5::QGifPlugin pygplates EXCLUDE_FROM_ALL)
    install_qt5_plugin(Qt5::QICOPlugin gplates)
    install_qt5_plugin(Qt5::QICOPlugin pygplates EXCLUDE_FROM_ALL)
    install_qt5_plugin(Qt5::QJpegPlugin gplates)
    install_qt5_plugin(Qt5::QJpegPlugin pygplates EXCLUDE_FROM_ALL)
    install_qt5_plugin(Qt5::QSvgPlugin gplates)
    install_qt5_plugin(Qt5::QSvgPlugin pygplates EXCLUDE_FROM_ALL)
    # These are common to Windows and macOS only...
    if (WIN32 OR APPLE)
        install_qt5_plugin(Qt5::QICNSPlugin gplates)
        install_qt5_plugin(Qt5::QICNSPlugin pygplates EXCLUDE_FROM_ALL)
        install_qt5_plugin(Qt5::QTgaPlugin gplates)
        install_qt5_plugin(Qt5::QTgaPlugin pygplates EXCLUDE_FROM_ALL)
        install_qt5_plugin(Qt5::QTiffPlugin gplates)
        install_qt5_plugin(Qt5::QTiffPlugin pygplates EXCLUDE_FROM_ALL)
        install_qt5_plugin(Qt5::QWbmpPlugin gplates)
        install_qt5_plugin(Qt5::QWbmpPlugin pygplates EXCLUDE_FROM_ALL)
        install_qt5_plugin(Qt5::QWebpPlugin gplates)
        install_qt5_plugin(Qt5::QWebpPlugin pygplates EXCLUDE_FROM_ALL)
    endif()

    # Install platform *dependent* plugins (used by GPlates).
    # Note: This list was obtained by running the Qt deployment tool (windeployqt/macdeployqt) on GPlates (to see which plugins it deployed).
    if (WIN32)
        install_qt5_plugin(Qt5::QWindowsIntegrationPlugin gplates)
        install_qt5_plugin(Qt5::QWindowsVistaStylePlugin gplates)
    elseif (APPLE)
        install_qt5_plugin(Qt5::QCocoaIntegrationPlugin gplates)
        install_qt5_plugin(Qt5::QMacStylePlugin gplates)
    else() # Linux
        install_qt5_plugin(Qt5::QXcbIntegrationPlugin gplates)
        # The following plugins are needed otherwise GPlates generates the following error and then seg. faults:
        #  "QXcbIntegration: Cannot create platform OpenGL context, neither GLX nor EGL are enabled"
        # Actually installing only the Glx plugin solved the issue (on Ubuntu 20.04), but we'll also install Egl in case.
        install_qt5_plugin(Qt5::QXcbGlxIntegrationPlugin gplates)
        install_qt5_plugin(Qt5::QXcbEglIntegrationPlugin gplates)
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
        # The necessary 'api-ms-*' get installed when installing the Windows Universal CRT libraries (using InstallRequiredSystemLibraries).
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\]api-ms-.*]])
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\][Ss]ystem32[/\\].*]])
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\][Ss]ys[Ww][Oo][Ww]64[/\\].*]])
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
    # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
    function(install_get_runtime_dependencies install_component)

        # The *build* target: executable (for gplates) or module library (for pygplates).
        if (install_component STREQUAL "gplates")
            install(CODE [[set(_target_file "$<TARGET_FILE:gplates>")]] COMPONENT gplates ${ARGN})
        else() # pygplates
            install(CODE [[set(_target_file "$<TARGET_FILE:pygplates>")]] COMPONENT pygplates ${ARGN})
        endif()

        install(
                CODE "set(GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES}]])"
                CODE "set(GET_RUNTIME_DEPENDENCIES_DIRECTORIES [[${GET_RUNTIME_DEPENDENCIES_DIRECTORIES}]])"
                # Note: Using \"${QT_PLUGINS_<comp>}\"" instead of [[${QT_PLUGINS_<comp>}]] because install code needs to evaluate
                #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_<comp>). And a side note, it does this at install time...
                CODE "set(QT_PLUGINS \"${QT_PLUGINS_${install_component}}\")"
                CODE "set(install_component [[${install_component}]])"
                CODE [[
                    unset(ARGUMENT_EXECUTABLES)
                    unset(ARGUMENT_BUNDLE_EXECUTABLE)
                    # Search the Qt plugins regardless of whether installing gplates or pygplates.
                    set(ARGUMENT_MODULES MODULES ${QT_PLUGINS})
                    # Component 'gplates' is an executable and component 'pygplates' is a module.
                    if (install_component STREQUAL "gplates")
                        # Add gplates to the list of executables to search.
                        set(ARGUMENT_BUNDLE_EXECUTABLE BUNDLE_EXECUTABLE "${_target_file}")  # gplates
                        set(ARGUMENT_EXECUTABLES EXECUTABLES "${_target_file}")  # gplates
                    else() # pygplates
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
                        # Also search the Qt plugins (since they're not discoverable because not dynamically linked)...
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

                COMPONENT ${install_component} ${ARGN}
        )
    endfunction()
    install_get_runtime_dependencies(gplates)
    install_get_runtime_dependencies(pygplates EXCLUDE_FROM_ALL)


    #
    # Install the dependency libraries.
    #
    if (WIN32)

        # On Windows we simply copy the dependency DLLs to the install prefix location (where 'gplates.exe', or 'pygplates.pyd', is)
        # so that they will get found at runtime by virtue of being in the same directory.
        #
        # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
        function(install_resolved_dependencies install_component)
            install(
                    CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR_${install_component}})"
                    CODE [[
                        # Install the dependency libraries in the *install* location.
                        foreach(_resolved_dependency ${_resolved_dependencies})
                            file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}")
                        endforeach()
                    ]]

                    COMPONENT ${install_component} ${ARGN}
            )
        endfunction()
        install_resolved_dependencies(gplates)
        install_resolved_dependencies(pygplates EXCLUDE_FROM_ALL)

        # Install the Python standard library.
        #
        # Note: The Python standard library is only installed for the 'gplates' component which has an embedded Python interpreter
        #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
        install_python_standard_library()

    elseif (APPLE)

        #
        # On macOS we need to:
        #   1 - Copy each resolved direct and indirect dependency (of GPlates/pyGPlates and its Qt plugins) into the appropriate location inside the
        #       installed GPlates application bundle (or base install directory of pygplates) depending of whether a regular '.dylib' or a framework.
        #   2 - Fix up the path to each *direct* dependency of GPlates/pyGPlates, its Qt plugins and their resolved dependencies
        #       (ie, each dependency will depend on other dependencies in turn and must point to their location within the installation).
        #   3 - Code sign GPlates/pyGPlates, its Qt plugins and their resolved dependencies with a valid Developer ID certificate.
        #       For GPlates we also then code sign the entire application *bundle* (for pyGPlates, the 'pygplates.so' library has already been signed).
        #   4 - Notarize the application bundle. Although this is not done here (during the installation phase) because we package it into a DMG during
        #       the CPack packaging phase (see Package.cmake) and the DMG is what should be uploaded for notarization.
        #       So the entire notarization process is currently done outside of CMake/CPack and should follow the procedure outlined here:
        #           https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution/customizing_the_notarization_workflow
        #
        #       For GPlates this amounts to uploading DMG to Apple for notarization, checking for successful notarization and then stapling the notarization ticket to the
        #       DMG file. Once that is all done the DMG can be distributed to users. Note however that, if you're only using CMake < 3.19, then you also need to manually
        #       code sign the DMG file prior to notarization upload (for CMake >= 3.19 we handle it during the packaging phase using CPACK_POST_BUILD_SCRIPTS).
        #
        #       For pyGPlates you can get CPack to produce a ZIP archive (notarization does not support TBZ2 for example) containing the code signed
        #       dependency libraries (and pygplates). However it's better to use only the 'install' phase (ie, not use CPack) with something like
        #         'cmake --install . --component pygplates --prefix pygplates_macOS)'
        #       and then manually archive it into a zip file using 'ditto' (which can stored extended attributes; and actually appears to be used by CPack)
        #         'ditto -c -k --keepParent pygplates_macOS pygplates_macOS.zip'
        #       The reason this is preferred over CPack is, with CPack, the top-level directory name will have 'gplates' in it instead of 'pygplates'
        #       (because we don't have control over the top-level directory name in the zip file), and so you'd then have to manually extract the zip file
        #       created by CPack, rename the top-level directory and then re-zip using 'ditto'.
        #       The next step is to upload the zip archive to Apple for notarization and check for successful notarization.
        #       However you cannot staple a notarization ticket to a ZIP archive (you must instead staple each item in the archive and then create a new archive).
        #       If an item is not stapled then Gatekeeper finds the ticket online (stapling is so the ticket can be found when the network is offline).
        #       So currently we don't staple the items.
        #       Note that soon we will use Conda to build pyGPlates packages and no longer rely on Apple notarization because the Conda package manager will then
        #       be responsible for installing pygplates on the user's computer, and so conda will then be responsible for quarantine.
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
        # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
        function(install_codesign install_component)
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

                COMPONENT ${install_component} ${ARGN}
            )
        endfunction()
        install_codesign(gplates)
        install_codesign(pygplates EXCLUDE_FROM_ALL)

        # Copy each resolved dependency (of GPlates/pyGPlates and its Qt plugins) into the appropriate location inside the installed GPlates application bundle
        # (or base install directory of pygplates) depending of whether a regular '.dylib' or a framework.
        #
        # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
        function(install_resolved_dependencies install_component)
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
                    # Copy each resolved dependency (of GPlates/pyGPlates and its Qt plugins) into the appropriate location inside the installed GPlates application bundle
                    # (or base install directory of pygplates) depending of whether a regular '.dylib' or a framework.
                    #
                    CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR_${install_component}})"
                    CODE [[
                        set(_installed_dependencies)
                        foreach(_resolved_dependency ${_resolved_dependencies})
                            # If the resolved dependency is inside a framework then copy the framework into the GPlates bundle or pyGPlates install location
                            # (but only copy the resolved dependency library and the framework 'Resources/' directory).
                            if (_resolved_dependency MATCHES "[^/]+\\.framework/")
                                if (install_component STREQUAL "gplates")
                                    # Install in the 'Contents/Frameworks/' sub-directory of the 'gplates' app bundle.
                                    install_framework(${_resolved_dependency} "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/Frameworks" _installed_dependency)
                                else() # pygplates
                                    # Install in the 'Frameworks/' sub-directory of base install directory of pygplates.
                                    install_framework(${_resolved_dependency} "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/Frameworks" _installed_dependency)
                                endif()
                            else()  # regular '.dylib' ...
                                # Ensure we copy symlinks (using FOLLOW_SYMLINK_CHAIN). For example, with 'libCGAL.13.dylib -> libCGAL.13.0.3.dylib' both the symlink
                                # 'libCGAL.13.dylib' and the dereferenced library 'libCGAL.13.0.3.dylib' are copied, otherwise just the symlink would be copied.
                                if (install_component STREQUAL "gplates")
                                    # Install in the 'Contents/MacOS/' sub-directory of 'gplates' app bundle.
                                    file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS" FOLLOW_SYMLINK_CHAIN)
                                else() # pygplates
                                    # Install in the 'lib/' sub-directory of base install directory of pygplates.
                                    file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib" FOLLOW_SYMLINK_CHAIN)
                                endif()

                                if (install_component STREQUAL "gplates")
                                    # Get '${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/dependency.dylib' from resolved dependency.
                                    string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/\\1" _installed_dependency "${_resolved_dependency}")
                                else() # pygplates
                                    # Get '${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/dependency.dylib' from resolved dependency.
                                    string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/\\1" _installed_dependency "${_resolved_dependency}")
                                endif()
                            endif()

                            # Add installed dependency to the list.
                            list(APPEND _installed_dependencies "${_installed_dependency}")
                        endforeach()
                    ]]

                    COMPONENT ${install_component} ${ARGN}
            )
        endfunction()
        install_resolved_dependencies(gplates)
        install_resolved_dependencies(pygplates EXCLUDE_FROM_ALL)

        # Install the Python standard library.
        #
        # Note: The Python standard library is only installed for the 'gplates' component which has an embedded Python interpreter
        #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
        install_python_standard_library()

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

        # Fix up the path to each *direct* dependency of GPlates/pyGPlates, its Qt plugins and their installed dependencies.
        # Each dependency will depend on other dependencies in turn and must point to their location within the installation.
        # At the same time code sign GPlates/pyGPlates, its Qt plugins and their installed dependencies with a valid Developer ID certificate.
        #
        # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
        function(install_fix_dependencies install_component)
            install(
                    #
                    # Function to find the relative path from the directory of the installed file to the specified installed dependency library.
                    #
                    # This is only needed for installing pyGPlates because it uses @loader_path which is different between the pygplates library,
                    # its Qt plugins and dependency frameworks/libraries (whereas GPlates uses @executable_path which is fixed for all).
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
                    CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR_${install_component}})"
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
                                    if (install_component STREQUAL "gplates")
                                        # For example, "@executable_path/../Frameworks/Dependency.framework/Versions/2/Dependency".
                                        string(REGEX REPLACE "^.*/([^/]+\\.framework/.*)$" "@executable_path/../Frameworks/\\1"
                                                _installed_dependency_file_install_name "${_dependency_file_install_name}")
                                    else() # pygplates
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
                                    if (install_component STREQUAL "gplates")
                                        # Non-framework librares are installed in same directory as executable.
                                        # For example, "@executable_path/../MacOS/dependency.dylib".
                                        string(REGEX REPLACE "^.*/([^/]+)$" "@executable_path/../MacOS/\\1"
                                                _installed_dependency_file_install_name "${_dependency_file_install_name}")
                                    else() # pygplates
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

                    # Note: Using \"${QT_PLUGINS_<comp>}\"" instead of [[${QT_PLUGINS_<comp>}]] because install code needs to evaluate
                    #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_<comp>). And a side note, it does this at install time...
                    CODE "set(QT_PLUGINS \"${QT_PLUGINS_${install_component}}\")"
                    #
                    # Fix up the path to each *direct* dependency of GPlates/pyGPlates, its Qt plugins and their installed dependencies.
                    #
                    # At the same time code sign GPlates/pyGPlates, its Qt plugins and their installed dependencies with a valid Developer ID certificate (if available).
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

                        # And finally fix dependencies and code sign the GPlates application bundle (or the pyGPlates library).
                        #
                        if (install_component STREQUAL "gplates")
                            # Fix the dependency install names in the installed gplates executable.
                            fix_dependency_install_names(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/gplates)
                            # Sign *after* fixing dependencies (since we cannot modify after signing).
                            #
                            # NOTE: We sign the entire installed bundle (not just the 'gplates.app/Contents/MacOS/gplates' executable).
                            #       And this must be done as the last step.
                            codesign(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app)
                        else() # pygplates
                            # Fix the dependency install names in the installed pygplates library.
                            fix_dependency_install_names(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/$<TARGET_FILE_NAME:pygplates>)
                            # Sign *after* fixing dependencies (since we cannot modify after signing).
                            codesign(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/$<TARGET_FILE_NAME:pygplates>)
                        endif()
                    ]]

                    COMPONENT ${install_component} ${ARGN}
            )
        endfunction()
        install_fix_dependencies(gplates)
        install_fix_dependencies(pygplates EXCLUDE_FROM_ALL)

    else()  # Linux

        #
        # On Linux we need to:
        #   1 - copy each resolved direct and indirect dependency library (of GPlates/pyGPlates and its Qt plugins) into the 'lib/' sub-directory of base install directory,
        #   2 - specify an appropriate RPATH for GPlates/pyGPlates, its Qt plugins and their resolved dependencies
        #       (ie, each dependency will depend on other dependencies in turn and must point to their location, ie, in 'lib/').
        #

        # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
        function(install_resolved_dependencies install_component)
            install(
                    #
                    # Copy each resolved dependency (of GPlates/pyGPlates and its Qt plugins) into the 'lib/' sub-directory of base install directory.
                    #
                    # On Linux we simply copy the dependency shared libraries to the 'lib/' sub-directory of the
                    # install prefix location so that they will get found at runtime from an RPATH of '$ORIGIN/lib' where $ORIGIN is
                    # the location of the gplates executable (or pyGPlates library) in the base install directory.
                    #
                    CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR_${install_component}})"
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

                    COMPONENT ${install_component} ${ARGN}
            )
        endfunction()
        install_resolved_dependencies(gplates)
        install_resolved_dependencies(pygplates EXCLUDE_FROM_ALL)

        # Install the Python standard library.
        #
        # Note: The Python standard library is only installed for the 'gplates' component which has an embedded Python interpreter
        #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
        install_python_standard_library()

        # Find the 'patchelf' command.
        find_program(PATCHELF "patchelf")
        if (NOT PATCHELF)
            message(FATAL_ERROR "Unable to find 'patchelf' command - cannot set RPATH - please install 'patchelf', for example 'sudo apt install patchelf' on Ubuntu")
        endif()

        # Wrapping 'install' command in a function because each 'install' handles a single component and we have two components (gplates and pygplates).
        function(install_fix_dependencies install_component)
            install(
                    CODE "set(PATCHELF [[${PATCHELF}]])"
                    CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR_${install_component}})"
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

                    # Note: Using \"${QT_PLUGINS_<comp>}\"" instead of [[${QT_PLUGINS_<comp>}]] because install code needs to evaluate
                    #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_<comp>). And a side note, it does this at install time...
                    CODE "set(QT_PLUGINS \"${QT_PLUGINS_${install_component}}\")"
                    #
                    # Set the RPATH of gplates/pygplates, its Qt plugins and their installed dependencies so that they all can find their direct dependencies.
                    #
                    # For example, gplates/pygplates needs to find its *direct* dependency libraries in 'lib/' and those dependencies need to find their *direct*
                    # dependencies (also in 'lib/').
                    #
                    CODE [[
                        # Set the RPATH in each installed dependency.
                        foreach(_installed_dependency ${_installed_dependencies})
                            set_rpath(${_installed_dependency})
                        endforeach()

                        # Set the RPATH in each installed Qt plugin.
                        foreach(_qt_plugin ${QT_PLUGINS})
                            set_rpath(${_qt_plugin})
                        endforeach()

                        # Set the RPATH in the installed gplates executable (or pygplates library).
                        if (install_component STREQUAL "gplates")
                            set_rpath(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/$<TARGET_FILE_NAME:gplates>)
                        else() # pygplates
                            set_rpath(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/$<TARGET_FILE_NAME:pygplates>)
                        endif()
                    ]]

                    COMPONENT ${install_component} ${ARGN}
            )
        endfunction()
        install_fix_dependencies(gplates)
        install_fix_dependencies(pygplates EXCLUDE_FROM_ALL)
    endif()
endif()
