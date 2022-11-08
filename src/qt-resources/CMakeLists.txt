# Add the resource files in the current directory to the gplates static library (or pygplates extension module).
target_sources_util(${SOURCE_TARGET} PRIVATE qt_resources.qrc)

# Traverse gpgim sub-directory.
add_subdirectory(gpgim)

# Traverse python sub-directory.
add_subdirectory(python)

# Traverse vulkan sub-directory.
add_subdirectory(vulkan)
