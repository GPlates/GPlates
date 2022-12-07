#
# Read the list of input GLSL shader files (*.vert, *.frag, *.tesc, *.tese, *.comp) to compile.
#
# These GLSL shader files are compiled to binary Vulkan SPIR-V shader files that are then listed in the "vulkan.qrc" file
# and hence are included as resources in the GPlates static library (or pyGPlates extension module).
#
# The "read_sources_util()" function is in "Utils.cmake".
#
read_sources_util(cmake_shader_sources.txt shaders)

# Find the "glslc" compiler.
find_program(GLSLC "glslc")
if (NOT GLSLC)
    message(FATAL_ERROR "Unable to find Vulkan shader compiler 'glslc'.")
endif()

# The "vulkan.qrc" file listing the compiled Vulkan SPIR-V shaders.
#
# We always create the "vulkan.qrc.in" file and only copy it to the "vulkan.qrc" file
# when its contents actually change (eg, adding/removing a shader).
set(vulkan_qrc_in ${CMAKE_CURRENT_BINARY_DIR}/vulkan.qrc.in)
set(vulkan_qrc ${CMAKE_CURRENT_BINARY_DIR}/vulkan.qrc)

# List of all input GLSL shaders and their dependencies (starts off empty).
set(glsl_shaders_and_dependencies)
# List of output Vulkan SPIR-V shaders (starts off empty).
set(vulkan_shaders)

# Write first part of "vulkan.qrc.in" file.
file(WRITE ${vulkan_qrc_in} [[
<RCC>
    <qresource prefix="/">
]])

foreach(shader IN LISTS shaders)

    # The input GLSL shader filename.
    set(glsl_shader ${CMAKE_CURRENT_SOURCE_DIR}/${shader})
    # The output Vulkan SPIR-V shader filename.
    set(vulkan_shader ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv)

    #
    # Find the #include dependencies of the current GLSL shader.
    #
    # Note: It appears that using an absolute path to the shader (with ${glsl_shader}) causes
    #       the listed dependencies to also contain absolute paths.
    #
    execute_process(
        COMMAND ${GLSLC} --target-env=vulkan1.0 -std=450core -M -MT target ${glsl_shader}
        RESULT_VARIABLE _glslc_result
        OUTPUT_VARIABLE _glslc__dependency_output)
    if (_glslc_result)
        message(FATAL_ERROR "Failed to compile GLSL shader to Vulkan SPIR-V: ${glsl_shader}")
    endif()

    #
    # Extract #include dependencies of the current GLSL shader from command output.
    #
    # The command output looks like "target: <dep1> [<dep2> ...]".
    #
    string(REPLACE "target:" "" glsl_shader_dependencies ${_glslc__dependency_output})
    string(STRIP ${glsl_shader_dependencies} glsl_shader_dependencies)
    # Convert space-separated string to a list (replace spaces with semi-colons).
    string(REPLACE " " ";" glsl_shader_dependencies ${glsl_shader_dependencies})

    #
    # Custom command to compile the current GLSL shader to a Vulkan SPIR-V shader.
    #
    # Command is run whenever the #include dependencies of the current GLSL shader are modified.
    #
    get_filename_component(vulkan_shader_dir ${vulkan_shader} DIRECTORY)
    add_custom_command(
        OUTPUT ${vulkan_shader}
        # Make sure Vulkan shader directory exists...
        COMMAND ${CMAKE_COMMAND} -E make_directory ${vulkan_shader_dir}
        # Compile GLSL shader to Vulkan SPIR-V shader...
        COMMAND ${GLSLC} --target-env=vulkan1.0 -std=450core -c ${glsl_shader} -o ${vulkan_shader}
        DEPENDS ${glsl_shader_dependencies})

    # Append the current vulkan shader file basename to a "<file></file>"" line of the "vulkan.qrc.in" file.
    # Note: We add a newline to the string being appended.
    file(RELATIVE_PATH vulkan_shader_basename ${CMAKE_CURRENT_BINARY_DIR} ${vulkan_shader})
    file(APPEND ${vulkan_qrc_in} "        <file>${vulkan_shader_basename}</file>
")

    # Keep a list of all GLSL shaders and their dependencies.
    list(APPEND glsl_shaders_and_dependencies ${glsl_shader_dependencies})
    # Keep a list of all compiled Vulkan SPIR-V shaders.
    list(APPEND vulkan_shaders ${vulkan_shader})

endforeach()

# Write last part of "vulkan.qrc.in" file.
file(APPEND ${vulkan_qrc_in} [[
    </qresource>
</RCC>
]])

# Copy the "vulkan.qrc.in" file (that we just wrote) to the "vulkan.qrc" file.
# But only if the contents change - this avoids unnecessarily triggering re-generation
# of the "qrc_vulkan.cpp" file (from "vulkan.qrc") every time CMake runs - instead it'll
# only get re-generated when we add or remove a shader file (in variable "shaders").
configure_file(${vulkan_qrc_in} ${vulkan_qrc} COPYONLY)

# Allow user to choose whether to keep GLSL dependencies up-to-date whenever a GLSL file is modified.
#
# By default we disable triggering a CMake re-run whenever an input GLSL file is modified.
# Most modifications will not add or remove a #include line, so most of the time we'd be
# needlessly triggering a lengthy CMake re-run when it's not needed.
option(GPLATES_VULKAN_UPDATE_GLSL_DEPENDENCIES "Re-run CMake when any GLSL file is modified (in case #include added/removed)." false)
if (GPLATES_VULKAN_UPDATE_GLSL_DEPENDENCIES)
    # If any of the input GLSL shaders or their #include dependencies are modified then
    # trigger a CMake re-run so that the dependencies for the custom compile commands
    # can be re-calculated. This is in case the user adds or removes a #include line.
    list(REMOVE_DUPLICATES glsl_shaders_and_dependencies)
    foreach(glsl_file IN LISTS glsl_shaders_and_dependencies)
        # The current GLSL file relative to the current source directory.
        file(RELATIVE_PATH glsl_file_rel_source_dir ${CMAKE_CURRENT_SOURCE_DIR} ${glsl_file})
        # The ".depend" file is just a dummy file (in binary directory).
        configure_file(${glsl_file} ${CMAKE_CURRENT_BINARY_DIR}/${glsl_file_rel_source_dir}.depend COPYONLY)
    endforeach()
endif()

# Add the vulkan shaders "vulkan.qrc" file to the gplates static library (or pygplates extension module).
target_sources(${SOURCE_TARGET} PRIVATE ${vulkan_qrc})

# It seems we need a custom target to ensure our Vulkan shaders get compiled (when their dependency files are modified).
#
# I thought you could chain dependencies in multiple 'add_custom_command()' calls (in same directory scope) as long as
# the output of the top-level 'add_custom_command()' was the source of a target. In our case the top-level custom command
# would be from adding "vulkan.qrc" to ${SOURCE_TARGET} (using 'target_sources()') which would create "qrc_vulkan.cpp"
# from "vulkan.qrc", and it would chain to our custom compile command above.
# However that doesn't seem to work (and using 'qt_add_resources()' instead of AUTORCC didn't work either).
#
# So we'll just make our custom commands be dependencies of a custom target and then add that as a dependency of ${SOURCE_TARGET}.
# This means our Vulkan SPIR-V shaders will get compiled (when their dependency GLSL shaders are modified). And since the
# Vulkan SPIR-V shaders are listed in "vulkan.qrc" (and have changed), the "vulkan.qrc" file will get compiled to "qrc_vulkan.cpp" by Qt.
add_custom_target(vulkan-spirv DEPENDS ${vulkan_shaders})
add_dependencies(${SOURCE_TARGET} vulkan-spirv)
