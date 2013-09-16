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

#include "unit-test/GenerateVelocityDomainCitcomsTest.h"

#include "app-logic/GenerateVelocityDomainCitcoms.h"
#include "model/ModelInterface.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/FileInfo.h"
#include "file-io/File.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "feature-visitors/GeometryFinder.h"

//copy the following code into directory level test suite file
//for example, if the test class is in data-mining directory, the following code will
//be copied to DataMiningTestSuite.cc
//#include "unit-test/GenerateVelocityDomainCitcomsTest.h"
//ADD_TESTSUITE(GenerateVelocityDomainCitcoms);


GPlatesUnitTest::GenerateVelocityDomainCitcomsTestSuite::GenerateVelocityDomainCitcomsTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"GenerateVelocityDomainCitcomsTestSuite")
{
	init(level);
} 


std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref>
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::load_mesh_files(
		int res)
{
	std::cout << "checking points with resolution: [ " << res << " ] ." << std::endl;
	std::vector< QString > file_names;
	QString res_str = QString::number(res);
	
	for(int i = 0; i < 12; i++)
	{
		std::string file_name = "unit-test-data/%r.mesh.%n.gpml.gz";
		QString i_str = QString::number(i);
		file_name.replace(file_name.find("%r"), 2, res_str.toStdString());
		file_name.replace(file_name.find("%n"), 2, i_str.toStdString());
		file_names.push_back(file_name.c_str());
	}
	
	std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref> feature_collections;

	GPlatesFileIO::ReadErrorAccumulation read_errors;

	std::vector< QString >::const_iterator filename_iter = file_names.begin();
	std::vector< QString >::const_iterator filename_end = file_names.end();
	
	for ( ; filename_iter != filename_end; ++filename_iter)
	{
		const QString &filename = *filename_iter;

		GPlatesFileIO::FileInfo file_info(filename);
		GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(file_info);


		if(!file_exists(file_info))
		{
			std::cout << "[Failed] Cannot find file: " << filename.toStdString() <<std::endl;
			continue;
		}
		else
		{
			std::cout << "Loading file: " << filename.toStdString() <<std::endl;
		}

		// Read the feature collection from file.
		d_file_format_registry.read_feature_collection(file->get_reference(), read_errors);

		d_files.push_back(file);

		// Files that have been freshly loaded from disk are by definition, clean.
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref = 
			file->get_reference().get_feature_collection();
		feature_collections.push_back(feature_collection_ref);
	}

	return feature_collections;
}

bool 
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::check( 
		int resolution )
{
	std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref>
		citcoms = load_mesh_files(resolution);

	if(citcoms.size() ==0)
	{
		return false;
	}

	std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref>::iterator
		it = citcoms.begin();
	std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref>::iterator
		it_end = citcoms.end();

	std::vector<
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		citcoms_multipoints;

	for(; it != it_end; it++)
	{
		GPlatesModel::FeatureCollectionHandle::const_iterator inner_it = (*it)->begin();
		GPlatesModel::FeatureCollectionHandle::const_iterator inner_end = (*it)->end();
		for(; inner_it != inner_end; inner_it++)
		{
			GPlatesFeatureVisitors::GeometryFinder visitor;
			visitor.visit_feature((*inner_it)->reference());
			citcoms_multipoints.insert(
				citcoms_multipoints.end(),
				visitor.found_geometries_begin(),
				visitor.found_geometries_end());
		}

	}

// 	std::vector<
// 		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>
// 		gplates_multipoints;

	for(int i = 0; i < 12; i++)
	{
		const GPlatesMaths::MultiPointOnSphere* citcoms_multipoint = 
			dynamic_cast<const GPlatesMaths::MultiPointOnSphere*>(citcoms_multipoints[i].get());
		
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type gplates_multipoint = 
		GPlatesAppLogic::GenerateVelocityDomainCitcoms::generate_velocity_domain( (resolution-1) ,i);

		if(citcoms_multipoint)
		{
			if( GPlatesMaths::multi_points_are_ordered_equivalent( 
				(*citcoms_multipoint), 
				(*gplates_multipoint) ) )
			{
				std::cout<<" [OK] -- " << i << std::endl;
			}
			else
			{
				std::cout<<" [Failed] " << i <<std::endl;
				return false;
			}
		}
	}
	return true;
}


GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::GenerateVelocityDomainCitcomsTest() :
	d_gpgim(GPlatesModel::Gpgim::create()),
	d_file_format_registry(d_gpgim)
{
}

void 
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::test_case_1()
{
	BOOST_CHECK(check(9)	== true);
	BOOST_CHECK(check(17)	== true);
	BOOST_CHECK(check(33)	== true);
	BOOST_CHECK(check(65)	== true);
	BOOST_CHECK(check(129)	== true);
}

void 
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::test_case_2()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::test_case_3()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::test_case_4()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::test_case_5()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::GenerateVelocityDomainCitcomsTest::test_case_7()
{
	//Add you test code here
	return;
}

void
GPlatesUnitTest::GenerateVelocityDomainCitcomsTestSuite::construct_maps()
{
	boost::shared_ptr<GenerateVelocityDomainCitcomsTest> instance(
		new GenerateVelocityDomainCitcomsTest());

	ADD_TESTCASE(GenerateVelocityDomainCitcomsTest,test_case_1);
	ADD_TESTCASE(GenerateVelocityDomainCitcomsTest,test_case_2);
	ADD_TESTCASE(GenerateVelocityDomainCitcomsTest,test_case_3);
	ADD_TESTCASE(GenerateVelocityDomainCitcomsTest,test_case_4);
	ADD_TESTCASE(GenerateVelocityDomainCitcomsTest,test_case_5);
	ADD_TESTCASE(GenerateVelocityDomainCitcomsTest,test_case_6);
	ADD_TESTCASE(GenerateVelocityDomainCitcomsTest,test_case_7);
}

