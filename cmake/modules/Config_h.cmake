#
# Set some variables needed when generating 'global/config.h' file from 'global/config.h.in'.
#

# Do we have boost.python.numpy?
#
# Only available for Boost >= 1.63, and if boost.python.numpy installed since
# it's currently optional (because we don't actually use it yet).
if (TARGET Boost::${GPLATES_BOOST_PYTHON_NUMPY_COMPONENT_NAME})
  set(GPLATES_HAVE_BOOST_PYTHON_NUMPY 1)
endif()

# Do we have the NumPy C-API include directories?
#
# If GPLATES_PYTHON_NUMPY_INCLUDE_DIRS is set then it's also been added
# to the target include directories so we don't have to do that here
# (in other words in the source code we just need "#include <numpy/arrayobject.h>").
if (GPLATES_PYTHON_NUMPY_INCLUDE_DIRS)
  # Also make sure "numpy/arrayobject.h" actually exists.
  if (EXISTS "${GPLATES_PYTHON_NUMPY_INCLUDE_DIRS}/numpy/arrayobject.h")
    set(GPLATES_HAVE_NUMPY_C_API 1)
  endif()
endif()

# The sub-directories of standalone base installation directory (or sub-dirs of
# 'gplates.app/Contents/Resources/' dir of base installation for GPlates macOS app bundle)
# to place data/plugins from the Proj (eg, 'proj.db') and GDAL libraries.
#
# Note: Only used when GPLATES_INSTALL_STANDALONE is true.
set(GPLATES_STANDALONE_PROJ_DATA_DIR proj_data)
set(GPLATES_STANDALONE_GDAL_DATA_DIR gdal_data)
set(GPLATES_STANDALONE_GDAL_PLUGINS_DIR gdal_plugins)

# GPLATES_STANDALONE_PYTHON_STDLIB_DIR
#
# The sub-directory of standalone base installation directory (or sub-dir of 'gplates.app/Contents/Frameworks/' dir
# of base installation for GPlates macOS app bundle) to place the Python standard library.
#
# Note: Only used when GPLATES_INSTALL_STANDALONE is true.
#       And only used for gplates (not pygplates since that's imported by an external
#       non-embedded Python interpreter that has its own Python standard library).
if (GPLATES_INSTALL_STANDALONE AND GPLATES_BUILD_GPLATES)
  if (APPLE)
    # On Apple, Python may either be a framework (eg, MacPorts) or a normal prefix layout (eg, conda).
    if (GPLATES_PYTHON_STDLIB_DIR MATCHES "/Python\\.framework/")
        # Framework Python (eg, MacPorts).
        # Convert, for example, '/opt/local/Library/Frameworks/Python.framework/Versions/3.8/lib/python3.8' to
        # 'Python.framework/Versions/3.8/lib/python3.8'.
        string(REGEX REPLACE "^.*/(Python\\.framework/.*)$" "\\1" GPLATES_STANDALONE_PYTHON_STDLIB_DIR ${GPLATES_PYTHON_STDLIB_DIR})
    else()
        # Non-framework Python (eg, conda).
        # Use the path of the standard library relative to the Python prefix (eg, 'lib/python3.14').
        # This gets installed under 'gplates.app/Contents/Resources/' (see Install.cmake) and is located
        # there at runtime (see 'src/file-io/StandaloneBundle.cc') - a loose directory tree cannot go under
        # the code-signed 'Contents/Frameworks/'. Since it is not a framework, the embedded interpreter is
        # told its home explicitly (see 'src/gui/PythonManager.cc').
        file(RELATIVE_PATH GPLATES_STANDALONE_PYTHON_STDLIB_DIR ${GPLATES_PYTHON_PREFIX_DIR} ${GPLATES_PYTHON_STDLIB_DIR})
    endif()
  else() # Windows or Linux
    # Find the relative path from the Python prefix directory to the standard library directory.
    # We'll use this as the standard library install location relative to the standalone base installation directory.
    file(RELATIVE_PATH GPLATES_STANDALONE_PYTHON_STDLIB_DIR ${GPLATES_PYTHON_PREFIX_DIR} ${GPLATES_PYTHON_STDLIB_DIR})
  endif()
endif()
