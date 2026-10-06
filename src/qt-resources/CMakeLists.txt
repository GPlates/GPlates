#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
#
set(srcs
    gpgim.qrc
    opengl.qrc
    python.qrc
    qt_resources.qrc
)

if (NOT TARGET gplates-lib)
    # The pygplates module needs only the GPGIM XML ("gpgim.qrc") and the python API
    # sources ("python.qrc") - the OpenGL shaders and GUI images are GPlates-only.
    list(REMOVE_ITEM srcs opengl.qrc qt_resources.qrc)

    # The macOS wheels need this compiled-in 'qt.conf', or the first Qt log message (qDebug,
    # qWarning, ...) segfaults. Logging makes Qt look up its install prefix, which a framework
    # build of Qt does by asking CoreFoundation for the 'org.qt-project.QtCore' bundle. But
    # delocate copies only the QtCore binary into 'pygplates/.dylibs/', without the framework's
    # 'Info.plist' that made it a bundle, so the lookup finds nothing and Qt dereferences null
    # ('getRelocatablePrefix' in 'qlibraryinfo.cpp'; unguarded from Qt 6.7 to at least 6.10).
    # Given paths by a 'qt.conf', Qt never looks for the bundle. Only the compiled-in resource
    # works: Qt's other places for one belong to the executable, which is Python.
    #
    # The prefix leads nowhere on purpose: pyGPlates uses only Qt Core, so Qt has no plugins or
    # translations to find there. (A relative one would resolve against the working directory.)
    #
    # Only a build whose dependencies a repair tool (delocate) will vendor needs it, ie with
    # 'GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES' off. Otherwise our own install copies
    # each framework whole ('InstallSharedLibraryDependencies.cmake'), and conda builds never move
    # Qt. Installed *without* that repair, the module would link the environment's QtCore, and this
    # resource would then set the prefix of a Qt that other modules in the process (eg, PyQt) may
    # share. Such a build is only meant as the input to a repair. GPlates keeps its frameworks
    # whole too, and has its own 'qt.conf' ('Install.cmake').
    #
    # Both files are generated into the build tree, side by side (a '.qrc' names its files relative
    # to itself). 'file(CONFIGURE)' rewrites a file only when its content changes, so a reconfigure
    # does not rerun rcc.
    # ('GPLATES_INSTALL_STANDALONE' too, since the option above is unset when it is off - and a
    # non-standalone build links the environment's Qt in place, so must not get the resource.)
    if (APPLE AND SKBUILD AND NOT GPLATES_CONDA_BUILD AND GPLATES_INSTALL_STANDALONE AND
        NOT GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES)
        set(_qt_conf_dir "${CMAKE_CURRENT_BINARY_DIR}/pygplates_wheel")
        file(CONFIGURE OUTPUT "${_qt_conf_dir}/qt.conf"
            CONTENT "[Paths]\nPrefix = /nonexistent\n")
        file(CONFIGURE OUTPUT "${_qt_conf_dir}/qt_conf.qrc"
            CONTENT "<RCC>\n  <qresource prefix=\"/qt/etc\">\n    <file>qt.conf</file>\n  </qresource>\n</RCC>\n")
        list(APPEND srcs "${_qt_conf_dir}/qt_conf.qrc")
        unset(_qt_conf_dir)
    endif()
endif()

# Add the source files to the gplates static library (or pygplates extension module).
target_sources(${SOURCE_TARGET} PRIVATE ${srcs})
