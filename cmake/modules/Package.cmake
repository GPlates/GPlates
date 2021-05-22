################################################################################################
#                                                                                              #
# Package targets.                                                                             #
#                                                                                              #
# Package targets to run on *other* machines (rather than just the *build* machine).           #
#                                                                                              #
################################################################################################

######################################
# Specify default package generators #
######################################

#   CPACK_GENERATOR - List of CPack generators to use.
#
#   If not specified, CPack will create a set of options following the naming pattern CPACK_BINARY_<GENNAME>
#   (e.g., CPACK_BINARY_NSIS) allowing the user to enable/disable individual generators. If the -G option is
#   given on the cpack command line, it will override this variable and any CPACK_BINARY_<GENNAME> options.
#
#   CPACK_SOURCE_GENERATOR - List of generators used for the source packages.
#
#   As with CPACK_GENERATOR, if this is not specified then CPack will create a set of options
#   (e.g., CPACK_SOURCE_ZIP) allowing users to select which packages will be generated.
#
if (WIN32)
    # For binary packages default to a NSIS installer (need to install version 3 prior to creating NSIS package).
    # Also create a ZIP (binary) archive by default so that users can install without admin privileges.
    SET(CPACK_GENERATOR NSIS ZIP)
    # For source packages default to a ZIP archive.
    SET(CPACK_SOURCE_GENERATOR ZIP)
elseif (APPLE)
    # For binary packages default to a drag'n'drop disk image
    # (which is a ".dmg" with the GPlates app bundle and a sym link to /Applications in it).
    SET(CPACK_GENERATOR DragNDrop)
    # For source packages default to a bzipped tarball (.tar.bz2).
    SET(CPACK_SOURCE_GENERATOR TBZ2)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # For binary packages default to a Debian package.
    SET(CPACK_GENERATOR DEB)
    # For source packages default to a bzipped tarball (.tar.bz2).
    SET(CPACK_SOURCE_GENERATOR TBZ2)
endif()


############################
# Some non-CPack variables #
############################

# Where all the distribution files are located.
SET(GPLATES_SOURCE_DISTRIBUTION_DIR "${GPlates_SOURCE_DIR}/cmake/distribution")


#########################################################
# CPack configuration variables common to all platforms #
#########################################################

#   CPACK_PACKAGE_NAME - The name of the package (or application).
#
#   If not specified, defaults to the project name.
#
SET(CPACK_PACKAGE_NAME "${PROJECT_NAME}")

#   CPACK_PACKAGE_VENDOR - The name of the package vendor
#
SET(CPACK_PACKAGE_VENDOR "${GPLATES_PACKAGE_VENDOR}")

#   CPACK_PACKAGE_VERSION_MAJOR - Package major Version
#
SET(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")

#   CPACK_PACKAGE_VERSION_MINOR - Package minor Version
#
SET(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")

#   CPACK_PACKAGE_VERSION_PATCH - Package patch Version
#
SET(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

#   CPACK_PACKAGE_VERSION - Package full version, used internally.
#
#   By default, this is built from CPACK_PACKAGE_VERSION_MAJOR, CPACK_PACKAGE_VERSION_MINOR, and CPACK_PACKAGE_VERSION_PATCH.
#
# NOTE: Normally we can't read CPACK variables until after "include(CPack)". However these CPACK variables have been set above.
#
# For a public release this is equivalent to PROJECT_VERSION (as set by project() command in top-level CMakeLists.txt).
SET(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
#
# For a *non*-public release build append '.<svn-version>'.
# Note: GPlates_WC_LAST_CHANGED_REV is only defined when GPLATES_PUBLIC_RELEASE is not defined (ie, not a public release).
if (GPlates_WC_LAST_CHANGED_REV)
    SET(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}.${GPlates_WC_LAST_CHANGED_REV}")
endif()

#   CPACK_PACKAGE_FILE_NAME - The name of the package file to generate, not including the extension.
#
#   For example, cmake-2.6.1-Linux-i686.
#
if (WIN32)
    # NOTE: We can't access CPACK variables here (ie, at CMake configure time) so we can't access CPACK_SYSTEM_NAME which is defined as:
    #           "CPACK_SYSTEM_NAME defaults to the value of CMAKE_SYSTEM_NAME, except on Windows where it will be win32 or win64"
    #       So instead we'll implement our own CPACK_SYSTEM_NAME.
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        SET(_CPACK_SYSTEM_NAME win64)
    else()
        SET(_CPACK_SYSTEM_NAME win32)
    endif()
    SET(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${_CPACK_SYSTEM_NAME}")
else()
    SET(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()

#   CPACK_PACKAGE_DESCRIPTION_FILE - A text file used to describe the project.
#
#   Used, for example, the introduction screen of a CPack-generated Windows installer to describe the project.
#
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${GPLATES_SOURCE_DISTRIBUTION_DIR}/PackageDescription.txt")

#   CPACK_PACKAGE_DESCRIPTION_SUMMARY - Short description of the project (only a few words).
#
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${GPLATES_PACKAGE_DESCRIPTION_SUMMARY}")

#   CPACK_RESOURCE_FILE_LICENSE - License file for the project, which will typically be displayed to the user
#                                 (often with an explicit "Accept" button, for graphical installers) prior to installation.
#
SET(CPACK_RESOURCE_FILE_LICENSE "${GPLATES_SOURCE_DISTRIBUTION_DIR}/LicenseFile.txt")

#   CPACK_RESOURCE_FILE_README - License to be embedded in the installer.
#
#   It will typically be displayed to the user by the produced installer (often with an explicit "Accept" button,
#   for graphical installers) prior to installation. This license file is NOT added to the installed files but is
#   used by some CPack generators like NSIS. If you want to install a license file (may be the same as this one)
#   along with your project, you must add an appropriate CMake install() command in your CMakeLists.txt.
#
SET(CPACK_RESOURCE_FILE_README "${GPLATES_SOURCE_DISTRIBUTION_DIR}/PackageReadMe.txt")

#   CPACK_RESOURCE_FILE_WELCOME - Welcome file to be embedded in the installer.
#
#   It welcomes users to this installer. Typically used in the graphical installers on Windows and Mac OS X.
#
SET(CPACK_RESOURCE_FILE_WELCOME "${GPLATES_SOURCE_DISTRIBUTION_DIR}/PackageWelcome.txt")

#   CPACK_PACKAGE_EXECUTABLES - Lists each of the executables and associated text label to be used to create Start Menu shortcuts.
#
#   For example, setting this to the list ccmake;CMake will create a shortcut named "CMake" that will execute the installed executable ccmake.
#   Not all CPack generators use it (at least NSIS, WIX and OSXX11 do).
#
SET(CPACK_PACKAGE_EXECUTABLES "gplates;${PROJECT_NAME}")

#   CPACK_STRIP_FILES - List of files to be stripped.
#
#   Starting with CMake 2.6.0, CPACK_STRIP_FILES will be a boolean variable which enables stripping of all files
#   (a list of files evaluates to TRUE in CMake, so this change is compatible).
#
SET(CPACK_STRIP_FILES TRUE)

#   CPACK_SOURCE_PACKAGE_FILE_NAME - The name of the source package, e.g., cmake-2.6.1
#
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-src")

#   CPACK_SOURCE_STRIP_FILES - List of files in the source tree that will be stripped.
#
#   Starting with CMake 2.6.0, CPACK_SOURCE_STRIP_FILES will be a boolean variable which enables stripping of all files
#   (a list of files evaluates to TRUE in CMake, so this change is compatible).
#
SET(CPACK_SOURCE_STRIP_FILES FALSE)

#   CPACK_SOURCE_IGNORE_FILES - Pattern of files in the source tree that won't be packaged when building a source package.
#
#   This is a list of patterns, e.g., /CVS/;/\\.svn/;\\.swp$;\\.#;/#;.*~;cscope.*
#
# Skip:
# - directories starting with '.', and
# - directories named '__pycache__'.
SET(CPACK_SOURCE_IGNORE_FILES "/\\.[^/]+/" "/__pycache__/")

#   CPACK_VERBATIM_VARIABLES - If set to TRUE, values of variables prefixed with CPACK_ will be escaped before being written
#                              to the configuration files, so that the cpack program receives them exactly as they were specified.
#
#   If not, characters like quotes and backslashes can cause parsing errors or alter the value received by the cpack program.
#   Defaults to FALSE for backwards compatibility.
#
set(CPACK_VERBATIM_VARIABLES TRUE)

#   CPACK_MONOLITHIC_INSTALL - Disables the component-based installation mechanism.
#
#   When set, the component specification is ignored and all installed items are put in a single "MONOLITHIC" package.
#   Some CPack generators do monolithic packaging by default and may be asked to do component packaging by setting
#   CPACK_<GENNAME>_COMPONENT_INSTALL to TRUE.
#
SET(CPACK_MONOLITHIC_INSTALL "TRUE")


########
# NSIS #
########


#   CPACK_NSIS_PACKAGE_NAME - The title displayed at the top of the installer.
#
SET(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION}")

#   CPACK_NSIS_DISPLAY_NAME - The display name string that appears in the Windows Apps & features in Control Panel.
#
SET(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION}")

#   CPACK_NSIS_MUI_ICON - An icon filename.
#
#   The name of a *.ico file used as the main icon for the generated install program.
#
SET(CPACK_NSIS_MUI_ICON "${GPLATES_SOURCE_DISTRIBUTION_DIR}\\gplates_desktop_icon.ico")

#   CPACK_NSIS_MUI_UNIICON - An icon filename.
#
#   The name of a *.ico file used as the main icon for the generated uninstall program.
#
SET(CPACK_NSIS_MUI_UNIICON "${GPLATES_SOURCE_DISTRIBUTION_DIR}\\gplates_desktop_icon.ico")

#   CPACK_NSIS_INSTALLED_ICON_NAME - A path to the executable that contains the installer icon.
#
SET(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")

#   CPACK_NSIS_HELP_LINK - URL to a web site providing assistance in installing your application.
#
SET(CPACK_NSIS_HELP_LINK "http://www.gplates.org")

#   CPACK_NSIS_URL_INFO_ABOUT - URL to a web site providing more information about your application.
#
SET(CPACK_NSIS_URL_INFO_ABOUT "http://www.gplates.org")

#   CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL - Ask about uninstalling previous versions first.
#
#   If this is set to ON, then an installer will look for previous installed versions and if one is found,
#   ask the user whether to uninstall it before proceeding with the install.
#
# We'll turn this off because the user might like to have several different versions installed at the same time.
# And if the user wants to uninstall a previous version they can still do so.
SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL OFF)

#   CPACK_NSIS_MODIFY_PATH - If this is set to "ON", then an extra page will appear in the installer that will allow the
#                            user to choose whether the program directory should be added to the system PATH variable.
#
# GPlates can be used on the command-line so the user might choose to add it to PATH.
SET(CPACK_NSIS_MODIFY_PATH ON)

#   CPACK_NSIS_EXECUTABLES_DIRECTORY - Creating NSIS Start Menu links assumes that they are in bin unless this variable is set.
#
#   For example, you would set this to exec if your executables are in an exec directory.
#
# Our executable is in the root of the install directory.
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")


#############
# DragNDrop #
#############
#
# Currently no DragNDrop-specific variables need setting.
# See "PackageGeneratorOverrides.cmake" for overrides of the general variables (non-generator specific).

#######
# DEB #
#######

#   CPACK_DEBIAN_FILE_NAME (CPACK_DEBIAN_<COMPONENT>_FILE_NAME) - Package file name.
#
#   Default : <CPACK_PACKAGE_FILE_NAME>[-<component>].deb
#
#   This may be set to DEB-DEFAULT to allow the CPack DEB generator to generate package file name by itself in deb format:
#
#       <PackageName>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb
#
#   Alternatively provided package file name must end with either .deb or .ipk suffix.
#
# NOTE: Instead of specifying DEB-DEFAULT we emulate it so that the SVN revision number gets included in the package filename.
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(_CPACK_DEB_ARCH amd64)
else()
    SET(_CPACK_DEB_ARCH i386)
endif()
STRING(TOLOWER "${PROJECT_NAME}" _CPACK_DEBIAN_PACKAGE_NAME_LOWER)
SET(CPACK_DEBIAN_FILE_NAME "${_CPACK_DEBIAN_PACKAGE_NAME_LOWER}_${CPACK_PACKAGE_VERSION}_${_CPACK_DEB_ARCH}.deb")

#   CPACK_DEBIAN_PACKAGE_HOMEPAGE - The URL of the web site for this package, preferably (when applicable) the site
#                                   from which the original source can be obtained and any additional upstream documentation
#                                   or information may be found.
#
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://www.gplates.org")

#   CPACK_DEBIAN_PACKAGE_MAINTAINER - The Debian package maintainer.
#
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "https://www.gplates.org")

#   CPACK_DEBIAN_PACKAGE_SECTION (CPACK_DEBIAN_<COMPONENT>_PACKAGE_SECTION) - Set Section control field e.g. admin, devel, doc, ...
#
SET(CPACK_DEBIAN_PACKAGE_SECTION science)

#   CPACK_DEBIAN_PACKAGE_SHLIBDEPS (CPACK_DEBIAN_<COMPONENT>_PACKAGE_SHLIBDEPS) - May be set to ON in order to use dpkg-shlibdeps
#                                                                                 to generate better package dependency list.
#
#   Default: CPACK_DEBIAN_PACKAGE_SHLIBDEPS if set or OFF.
#
#   Note: You may need set CMAKE_INSTALL_RPATH to an appropriate value if you use this feature, because if you don't
#         dpkg-shlibdeps may fail to find your own shared libs.
#         See https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
#
#   Note: You can also set CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS to an appropriate value if you use this feature, in order to
#         please dpkg-shlibdeps. However, you should only do this for private shared libraries that could not get resolved otherwise.
#
# NOTE: You'll need to install the 'dpkg-dev' package ('sudo apt install dpkg-dev') for 'dpkg-shlibdeps' to be available.
#       Otherwise you'll get the following warning message and dependencies will not be listed in the generated package...
#           "CPACK_DEBIAN_PACKAGE_DEPENDS not set, the package will have no dependencies"
SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

#   CPACK_DEB_COMPONENT_INSTALL - Enable component packaging for CPackDEB.
#
#   If enabled (ON) multiple packages are generated. By default a single package containing files of all components is generated.
#
SET(CPACK_DEB_COMPONENT_INSTALL OFF)


#########
# CPack #
#########

#   CPACK_PROJECT_CONFIG_FILE - CPack-time project CPack configuration file.
#
#   This file is included at cpack time, once per generator after CPack has set CPACK_GENERATOR
#   to the actual generator being used. It allows per-generator setting of CPACK_* variables at cpack time.
#
# Specify our own configuration file to handle generator-specific settings for CPACK variables that are used by multiple generators
# (eg, NSIS, DragNDrop). For example, CPACK_PACKAGE_ICON uses different icon formats for different generators.
# This is not needed for CPACK_<GENERATOR>_ variables (since they only apply to a specific generator).
set(CPACK_PROJECT_CONFIG_FILE "${GPlates_BINARY_DIR}/cmake/modules/PackageGeneratorOverrides.cmake")
#
# The configuration file only has access to CPACK_ variables (since it's only loaded when cpack runs).
# So we need to transfer any other variables across now (during CMake configuration).
configure_file(${GPlates_SOURCE_DIR}/cmake/modules/PackageGeneratorOverrides.cmake.in ${CPACK_PROJECT_CONFIG_FILE} @ONLY)

# CPack will generate a target 'package' that, when built, will install the targets specified
# by any 'install' commands into a staging area and package them.
INCLUDE(CPack)
