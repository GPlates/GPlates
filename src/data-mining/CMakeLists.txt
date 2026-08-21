#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
#
set(srcs
    CheckAttrTypeVisitor.h
    CoRegConfigurationTable.cc
    CoRegConfigurationTable.h
    CoRegFilter.cc
    CoRegFilter.h
    CoRegFilterCache.cc
    CoRegFilterCache.h
    CoRegFilterMapReduceFactory.cc
    CoRegFilterMapReduceFactory.h
    CoRegMapper.h
    CoRegReducer.h
    DataMiningCache.h
    DataMiningUtils.cc
    DataMiningUtils.h
    DataSelector.cc
    DataSelector.h
    DataTable.cc
    DataTable.h
    GetValueFromPropertyVisitor.cc
    GetValueFromPropertyVisitor.h
    LookupReducer.cc
    LookupReducer.h
    MaxReducer.h
    MeanReducer.h
    MedianReducer.h
    MinReducer.h
    OpaqueData.h
    OpaqueDataToDouble.h
    OpaqueDataToQString.h
    PercentileReducer.h
    PopulateShapeFileAttributesVisitor.cc
    PopulateShapeFileAttributesVisitor.h
    RegionOfInterestFilter.cc
    RegionOfInterestFilter.h
    RFGToPropertyValueMapper.h
    RFGToRelationalPropertyMapper.h
    ScribeExportDataMining.h
    SeedSelfFilter.cc
    SeedSelfFilter.h
    Types.h
    VoteReducer.h
    WeightedMeanReducer.h
)

if (TARGET gplates-lib)
    # Add the source files to the gplates static library.
    # (The pygplates module compiles nothing from this directory - see
    # "cmake/pygplates_source_closure.py".)
    target_sources_util(gplates-lib PRIVATE ${srcs})
endif()
