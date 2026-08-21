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
    qt_widgets.qrc
)

if (NOT TARGET gplates-lib)
    # The pygplates module needs only the GPGIM XML ("gpgim.qrc") and the python API
    # sources ("python.qrc") - the OpenGL shaders and GUI images are GPlates-only.
    list(REMOVE_ITEM srcs opengl.qrc qt_widgets.qrc)
endif()

# Add the source files to the gplates static library (or pygplates extension module).
target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})
