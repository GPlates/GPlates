#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# You can either explicitly add/remove source files here, or
# run 'python cmake/add_sources.py' to automatically list them (here).
#
# Note that CMake discourages use of the 'file(GLOB)' CMake command to automatically collect source files.
# One of the reasons is if source files are added or removed, CMake is not automatically re-run,
# so the build is unaware of the change.
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

# Add the source files to the gplates static library.
target_sources_util(gplates-lib PRIVATE ${srcs})

# Add the source files to the pygplates extension module.
#
# Note this is the external Python library that is not embedded inside GPlates.
# All sources files compiled in pygplates need to be position independent and hence
# compiled differently (POSITION_INDEPENDENT_CODE target property is set on pygplates).
set(pygplates_srcs ${srcs})
target_sources_util(pygplates PRIVATE ${pygplates_srcs})
