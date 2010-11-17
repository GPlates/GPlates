/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <iostream>

#include <QDebug>

#include "unit-test/DataSelectorTest.h"
#include "data-mining/DataSelector.h"
#include "data-mining/OpaqueDataToQString.h"

#include "model/ModelInterface.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/FileInfo.h"
#include "file-io/File.h"
#include "file-io/FeatureCollectionReaderWriter.h"

//copy the following code into directory level test suite file
//for example, if the test class is in data-mining directory, the following code will
//be copied to DataMiningTestSuite.cc
//#include "unit-test/DataSelectorTest.h"
//ADD_TESTSUITE(DataSelector);


GPlatesUnitTest::DataSelectorTestSuite::DataSelectorTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"DataSelectorTestSuite")
{
	init(level);
} 

std::vector<
		GPlatesModel::FeatureCollectionHandle::const_weak_ref>
GPlatesUnitTest::DataSelectorTest::read_files(
		const QStringList &filenames)
{
	
	std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref> feature_collections;

	GPlatesFileIO::ReadErrorAccumulation read_errors;

	QStringList::const_iterator filename_iter = filenames.begin();
	QStringList::const_iterator filename_end = filenames.end();
	for ( ; filename_iter != filename_end; ++filename_iter)
	{
		const QString &filename = *filename_iter;

		GPlatesFileIO::FileInfo file_info(filename);

#if 0
		// Read the feature collection from file.
		const GPlatesFileIO::File::Reference file = GPlatesFileIO::read_feature_collection(
			file_info, d_model, read_errors);

		d_files.push_back(file);


		// Files that have been freshly loaded from disk are by definition, clean.
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref = 
				file.get_feature_collection();
		feature_collections.push_back(feature_collection_ref);
#endif 
	}
	return feature_collections;
}

void
GPlatesUnitTest::DataSelectorTest::MockInputTable(
		GPlatesDataMining::CoRegConfigurationTable& table)
{
	using namespace GPlatesDataMining;
	QStringList files;
	files.append("data/target1.gpml");	
	std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref> 
		target_collection = read_files(files);

	ConfigurationTableRow info;
	info.target_feature_collection_handle = target_collection[0];
	info.association_operator_type = REGION_OF_INTEREST;
	info.association_parameters.d_ROI_range = 5000;
	
	info.attribute_name = "name";
	info.data_operator_type = DATA_OPERATOR_LOOKUP;
	table.push_back(info);
	info.data_operator_type = DATA_OPERATOR_MIN_DISTANCE;
	table.push_back(info);
	info.data_operator_type = DATA_OPERATOR_RRESENCE;
	table.push_back(info);
	info.data_operator_type = DATA_OPERATOR_NUM_IN_ROI;
	table.push_back(info);
}

void 
GPlatesUnitTest::DataSelectorTest::test_case_1()
{
	//Add you test code here
	BOOST_TEST_MESSAGE( "DataSelectorTest::test_case_1()...." );
	
	typedef GPlatesModel::FeatureCollectionHandle::const_weak_ref SeedCollectionType;
	typedef std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref> TargetCollectionType;

	QStringList files;
	files.append("data/seed_points.gpml");
	SeedCollectionType seed_collection = read_files(files)[0];
	files.clear();
	files.append("data/target1.gpml");	
	TargetCollectionType target_collection = read_files(files);


	using namespace GPlatesDataMining;

	CoRegConfigurationTable input_table;

	MockInputTable(input_table);
	
	boost::scoped_ptr< DataSelector > selector( DataSelector::create(input_table) );

	DataTable result;

	selector->select(
			seed_collection,
			0,
			result);

	DataTable::iterator iter = result.begin();
	DataTable::iterator iter_end = result.end();

	std::cout<<"print out the result data table"<<std::endl;

	for(; iter != iter_end; iter++) //for each row
	{
		//get each cell
		for(unsigned i=0; i<(*iter)->size(); i++)
		{
			OpaqueData o_data;
			(*iter)->get_cell(i,o_data);

			std::cout<<
				boost::apply_visitor(
						ConvertOpaqueDataToString(),
						o_data).toStdString()
				<< " | ";
		}
		std::cout<<std::endl;
	}

	result.export_as_CSV(QString("test_export.csv"));

	return;
}

void 
GPlatesUnitTest::DataSelectorTest::test_case_2()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataSelectorTest::test_case_3()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataSelectorTest::test_case_4()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataSelectorTest::test_case_5()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataSelectorTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataSelectorTest::test_case_7()
{
	//Add you test code here
	return;
}

void
GPlatesUnitTest::DataSelectorTestSuite::construct_maps()
{
	boost::shared_ptr<DataSelectorTest> instance(
		new DataSelectorTest());

	ADD_TESTCASE(DataSelectorTest,test_case_1);
	ADD_TESTCASE(DataSelectorTest,test_case_2);
	ADD_TESTCASE(DataSelectorTest,test_case_3);
	ADD_TESTCASE(DataSelectorTest,test_case_4);
	ADD_TESTCASE(DataSelectorTest,test_case_5);
	ADD_TESTCASE(DataSelectorTest,test_case_6);
	ADD_TESTCASE(DataSelectorTest,test_case_7);
}

