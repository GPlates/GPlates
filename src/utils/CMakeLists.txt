# Current binary directory must be included first for g++ pre-compiled headers.
# There's a '_pch.h' file in current source and current binary directory however the '_pch.h.gch' pre-compiled header
# is only in current binary directory and it will only be used by the compiler if it's in the same directory as the found '_pch.h' file.
# So we need to ensure that we find the '_pch.h' file from the current binary directory first.
include_directories(${CMAKE_CURRENT_BINARY_DIR})
# Current source directory (directory that this 'CMakeLists.txt' file is in).
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# The variables used here are set in the parent 'CMakeList.txt' file.
include_directories(${SYSTEM_INCLUDE_FLAG} ${external_INCLUDE_DIRS})
include_directories(${internal_INCLUDE_DIRS})

set(srcs
    AnimationSequenceUtils.cc
    AnimationSequenceUtils.h
    CallStackTracker.cc
    CallStackTracker.h
    CommandLineParser.cc
    CommandLineParser.h
    ExportAnimationStrategyExporterID.h
    ExportAnimationStrategyFactory.cc
    ExportAnimationStrategyFactory.h
    ExportFileNameTemplateValidator.cc
    ExportFileNameTemplateValidator.h
    ExportTemplateFilenameSequence.cc
    ExportTemplateFilenameSequence.h
    ExportTemplateFilenameSequenceFormats.cc
    ExportTemplateFilenameSequenceFormats.h
    ExportTemplateFilenameSequenceImpl.cc
    ExportTemplateFilenameSequenceImpl.h
    FloatingPointComparisons.cc
    FloatingPointComparisons.h
    GeometryCreationUtils.h
    IdStringSet.cc
    IdStringSet.h
    LatLonAreaSampling.h
    non_null_intrusive_ptr.h
    NullIntrusivePointerHandler.cc
    NullIntrusivePointerHandler.h
    NullNonNullIntrusivePointerException.h
    Profile.cc
    Profile.h
    ReferenceCount.h
    ScopeGuard.h
    Singleton.h
    SmartNodeLinkedList.h
    StringFormattingUtils.cc
    StringFormattingUtils.h
    StringSet.cc
    StringSet.h
    StringUtils.h
    UnicodeStringUtils.cc
    UnicodeStringUtils.h
    UniqueId.cc
    UniqueId.h
    VirtualProxy.h
    XmlNamespaces.h
)


# We have a nasty case of a variable shadow warning in a boost
# header "boost/pool/object_pool.hpp". Since it's a system header it should
# not emit a warning but it does on g++ 4.1.2 and g++ 4.1.3 (only ones tested
# so far). Luckily this header is only used by Profile.cc and so we can
# use the nasty hack of turning off shadow warnings just for this source file.
# 'COMPILE_FLAGS' just adds to the default compiler flags.
# TODO: find a better way of doing this sort of thing.
if (CMAKE_COMPILER_IS_GNUCXX)
	set_source_files_properties(Profile.cc PROPERTIES COMPILE_FLAGS "-Wno-shadow")
endif (CMAKE_COMPILER_IS_GNUCXX)


set(target_name utils)

# Does the following:
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - adds dependencies on generated header files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} STATIC ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
