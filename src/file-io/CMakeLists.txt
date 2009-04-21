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
    ErrorOpeningFileForReadingException.h
    ErrorOpeningFileForWritingException.cc
    ErrorOpeningFileForWritingException.h
    ErrorOpeningPipeFromGzipException.h
    ErrorOpeningPipeToGzipException.h
    ExternalProgram.cc
    ExternalProgram.h
    FeatureCollectionFileFormat.cc
    FeatureCollectionFileFormat.h
    FeaturePropertiesMap.cc
    FeaturePropertiesMap.h
    FeatureWriter.h
    FileFormatNotSupportedException.h
    FileInfo.cc
    FileInfo.h
    GdalReader.cc
    GdalReader.h
    GeometryExporter.h
    GMTFormatGeometryExporter.cc
    GMTFormatGeometryExporter.h
    GMTFormatHeader.cc
    GMTFormatHeader.h
    GMTFormatReconstructedFeatureGeometryExport.cc
    GMTFormatReconstructedFeatureGeometryExport.h
    GMTFormatWriter.cc
    GMTFormatWriter.h
    GpmlOnePointFiveOutputVisitor.cc
    GpmlOnePointFiveOutputVisitor.h
    GpmlOnePointSixOutputVisitor.cc
    GpmlOnePointSixOutputVisitor.h
    GpmlOnePointSixReader.cc
    GpmlOnePointSixReader.h
    GpmlReaderUtils.cc
    GpmlReaderUtils.h
    LineReader.h
    PlatesLineFormatGeometryExporter.cc
    PlatesLineFormatGeometryExporter.h
    PlatesLineFormatHeaderVisitor.cc
    PlatesLineFormatHeaderVisitor.h
    PlatesLineFormatReader.cc
    PlatesLineFormatReader.h
    PlatesLineFormatWriter.cc
    PlatesLineFormatWriter.h
    PlatesRotationFormatReader.cc
    PlatesRotationFormatReader.h
    PlatesRotationFormatWriter.cc
    PlatesRotationFormatWriter.h
    PropertyCreationUtils.cc
    PropertyCreationUtils.h
    PropertyMapper.h
    RasterReader.cc
    RasterReader.h
    ReadErrorAccumulation.h
    ReadErrorOccurrence.cc
    ReadErrorOccurrence.h
    ReadErrors.h
    ReconstructedFeatureGeometryExport.cc
    ReconstructedFeatureGeometryExport.h
    ShapeFileReader.cc
    ShapeFileReader.h
    ShapefileXmlReader.cc
    ShapefileXmlReader.h
    ShapefileXmlWriter.cc
    ShapefileXmlWriter.h
    StructurePropertyCreatorMap.cc
    StructurePropertyCreatorMap.h
    XmlOutputInterface.cc
    XmlOutputInterface.h
    XmlWriter.cc
    XmlWriter.h
)





set(target_name file-io)

# Does the following:
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - adds dependencies on generated header files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
