/* $Id:  $ */

/**
 * \file 
 * $Revision: 7584 $
 * $Date: 2010-02-10 19:29:36 +1100 (Wed, 10 Feb 2010) $
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
#include <sstream>
#include <QDebug>

// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
#include "global/CompilerWarnings.h"
DISABLE_GCC_WARNING("-Wshadow")

#include "unit-test/CoregTest.h"
#include "app-logic/CoRegistrationData.h"
#include "data-mining/DataSelector.h"
#include "data-mining/DataMiningUtils.h"
#include "data-mining/OpaqueDataToQString.h"
#include "model/ModelInterface.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/FeatureCollectionReaderWriter.h"

//gplates-unit-test.exe --detect_memory_leaks=0 --G_test_to_run=*/Coreg

const QString unit_test_data_path = "./unit-test-data/";
const QString cfg_file = "coreg_input_table.txt";

using namespace GPlatesAppLogic;
using namespace GPlatesDataMining;

GPlatesUnitTest::CoregTestSuite::CoregTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"CoregTestSuite")
{
	init(level);
} 

GPlatesUnitTest::CoregTest::CoregTest()
{
	load_test_data();
}

void
GPlatesUnitTest::CoregTest::load_test_data()
{
	d_rotation_fc = 
		DataMiningUtils::load_files(
				load_cfg(
						unit_test_data_path + cfg_file,
						"rotation files"),
				d_loaded_files);

	d_seed_fc =
		DataMiningUtils::load_files(
				load_cfg(
						unit_test_data_path + cfg_file,
						"seed files"),
				d_loaded_files);

	d_coreg_fc =
		DataMiningUtils::load_files(
				load_cfg(
						unit_test_data_path + cfg_file,
						"coreg files"),
				d_loaded_files);
	
	if(d_rotation_fc.empty()) 
		qDebug() << "No rotation file.";
	if(d_seed_fc.empty())
		qDebug() << "No seed file.";
	if(d_coreg_fc.empty())
		qDebug() << "No coreg file.";

	QString tmp = 
		load_one_line_cfg(
				unit_test_data_path + cfg_file,
				"output path");
	d_output_path = tmp.size() ? tmp : "./";
}

void
GPlatesUnitTest::CoregTest::test(double time)
{
	//Create the reconstruction tree.
	ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
		ReconstructUtils::create_reconstruction_tree(
				time, //time
				0, //anchor plate
				d_rotation_fc);
	
	//seed
	std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type> reconstructed_seeds;
	reconstructed_seeds.push_back(
			ReconstructUtils::reconstruct(
					reconstruction_tree,
					ReconstructLayerTaskParams(),
					d_seed_fc));


	// co-registration features collection.
	std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type> reconstructed_coreg;
	reconstructed_coreg.push_back(
			ReconstructUtils::reconstruct(
					reconstruction_tree,
					ReconstructLayerTaskParams(),
					d_coreg_fc));


	CoRegistrationData::non_null_ptr_type data_ptr(new CoRegistrationData(reconstruction_tree));

	CoRegConfigurationTable input_table;
	populate_cfg_table(input_table,cfg_file);

	boost::shared_ptr< DataSelector > selector( 
			DataSelector::create(input_table) );

	selector->select(
			reconstructed_seeds, 
			reconstructed_coreg, 
			data_ptr->data_table());

	DataSelector::set_data_table(data_ptr->data_table());
	d_output_prefix = load_one_line_cfg(unit_test_data_path + cfg_file, "output prefix");
	DataSelector::get_data_table().export_as_CSV(get_output_name(time));
#ifdef _DEBUG
	std::cout << data_ptr->data_table() << std::endl;
#endif
	return;
}

#include <fstream>
namespace
{
	std::map<QString, QStringList>
	load_result_data(
			const QString& filename)
	{
		std::map<QString, QStringList> ret;
		std::ifstream ifs ( filename.toStdString().c_str() , std::ifstream::in );
		char buf[65535];
		while (ifs.good())
		{
			ifs.getline(buf,65535);
			QString s(buf);
			s = s.trimmed().simplified();
			if(s.startsWith("GPlates")) // real data starts with GPlates.
			{
				QStringList list = s.split(',');
				QString id = list.takeFirst();
				ret[id] = list;
			}
		}
		ifs.close();
		return ret;
	}
}

bool
GPlatesUnitTest::CoregTest::check_result(double time)
{
	QString o_filename = get_output_name(time);
	std::ostringstream os;
	os.precision(2);
	os << std::fixed  << time;
	QString data_filename = unit_test_data_path + QString("/coreg_data_")+os.str().c_str()+".csv";
	std::map<QString, QStringList> output =	load_result_data(o_filename);
	std::map<QString, QStringList> expected = load_result_data(data_filename);
	std::size_t data_size = output.size();
	if(data_size != expected.size()) 
		return false;
	std::map<QString, QStringList>::iterator it1 = output.begin();
	std::map<QString, QStringList>::iterator it2 = expected.begin();
	for(std::size_t i=0;i<data_size;i++,it2++,it1++)
	{
		if((it1->first != it2->first) || (it1->second != it2->second))
		{
			std::cout << it1->first.toStdString() << std::endl;
			std::cout << it2->first.toStdString() << std::endl;
			qDebug() << it1->second ;
			qDebug() << it2->second ;
			std::cout <<  "test at time["<<time<<"] failed!" << std::endl;
			return false;
		}
	}
	std::cout <<  "test at time["<<time<<"] succeed!" << std::endl;
	return true;
}

void
GPlatesUnitTest::CoregTest::populate_cfg_table(
		GPlatesDataMining::CoRegConfigurationTable& table,
		const QString& filename)
{
	enum ColName
	{
		FC_NAME,//0
		COREG_OP,//1
		ATTR_NAME,//2
		DATA_OP,//3
		SHAPE_ATTR,//4
		COL_NUM//5
	};
	std::map<QString, AssociationOperatorType> coreg_op_map;
	coreg_op_map["Region_of_Interest"] = REGION_OF_INTEREST;

	std::map<QString, AttributeType> attr_map;
	attr_map["Distance"] =			DISTANCE_ATTRIBUTE;
	attr_map["Presence"] =			PRESENCE_ATTRIBUTE;
	attr_map["Number_In_Region"] =	NUMBER_OF_PRESENCE_ATTRIBUTE;

	std::map<QString, DataOperatorType> data_op_map;
	data_op_map["Min"] =				DATA_OPERATOR_MIN;
	data_op_map["Max"] = 				DATA_OPERATOR_MAX;
	data_op_map["Mean"] = 				DATA_OPERATOR_MEAN;
	data_op_map["Median"] = 			DATA_OPERATOR_MEDIAN;
	data_op_map["Lookup"] = 			DATA_OPERATOR_LOOKUP;
	data_op_map["Vote"] = 				DATA_OPERATOR_VOTE;
	data_op_map["Weighted Mean"] =		DATA_OPERATOR_WEIGHTED_MEAN;
	data_op_map["Percentile"] = 		DATA_OPERATOR_PERCENTILE;
	
	std::vector<QString> table_lines = load_cfg(unit_test_data_path + filename,"input table");
	
	BOOST_FOREACH(const QString& line, table_lines)
	{
		QString tmp_line = line.trimmed().simplified();
		QStringList items = tmp_line.split(',');
		if(items.size() != COL_NUM)
		{
			qWarning() << "Invalid configuration line.";
			continue;
		}

		ConfigurationTableRow row;
		BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type file, d_loaded_files)
		{
			if(file->get_reference().get_file_info().get_display_name(false) == items[FC_NAME])
			{
				qDebug() << "Find the feature collection.";
				row.target_feature_collection_handle = file->get_reference().get_feature_collection();
			} 
		}

		QString& s_ref = items[COREG_OP];
		s_ref.insert(s_ref.indexOf('('),' '); // The blank space is essential for stringstream to work.
		std::stringstream ss(s_ref.trimmed().toStdString().c_str());
		std::string op_type_str;
		double d = 0.0;
		ss >> op_type_str; ss.ignore(256,'('); ss >> d; 
		row.association_operator_type = coreg_op_map[op_type_str.c_str()];
		row.association_parameters.d_ROI_range = d;

		std::map<QString, AttributeType>::iterator it = attr_map.find(items[ATTR_NAME].trimmed());
		row.attr_type = it != attr_map.end() ?  it->second : CO_REGISTRATION_ATTRIBUTE;
		row.attribute_name = items[ATTR_NAME].trimmed();
		
		row.data_operator_type = data_op_map[items[DATA_OP].trimmed()];

		row.data_operator_parameters.d_is_shape_file_attr =
			items[SHAPE_ATTR].trimmed() == "true" ? true : false;
		
		table.push_back(row);
	}
}

std::vector<QString>
GPlatesUnitTest::CoregTest::load_cfg(
		const QString& cfg_filename,
		const QString& section_name)
{
	std::string str;
	std::vector<QString> ret;
	std::ifstream ifs ( cfg_filename.toStdString().c_str() , std::ifstream::in );
	
	while (ifs.good()) // go to the line starting with section_name.
	{
		std::getline(ifs,str);
		QString s(str.c_str());
		s = s.trimmed().simplified();
		if(s.startsWith(section_name))
			break;
	}
	
	while (ifs.good())
	{
		std::getline(ifs,str);
		QString s(str.c_str());
		s = s.trimmed().simplified();

		if(s.startsWith("#"))//skip comments
			continue;
		if(s.isEmpty()) //finish at an empty line.
			break;
		ret.push_back(s);
		qDebug() << s;
	}
	return ret;
}

void 
GPlatesUnitTest::CoregTest::test_case_1()
{
	std::cout <<  "Begin to test co-registration case 1..." << std::endl;
	bool result = true;
	std::size_t s_time = 140, e_time = 0, inc_time = 10;
	QStringList times = load_one_line_cfg(unit_test_data_path + cfg_file, "time_range").split(',');
	if(times.size()==3)
	{
		s_time		=	times[0].trimmed().toInt();
		e_time		=	times[1].trimmed().toInt();
		inc_time	=	times[2].trimmed().toInt();
	}
	
	std::size_t current = e_time;
	for(; s_time >= current; current+=10)
	{
		test(current);
	}
	current = e_time;
	for(; s_time >= current; current+=10)
	{
		BOOST_CHECK(result &= check_result(current));
	}
	if(!result)
		std::cout <<  "testing co-registration case 1 failed..." << std::endl;
	std::cout <<  "End of testing co-registration case 1..." << std::endl;
}

void 
GPlatesUnitTest::CoregTest::test_case_2()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CoregTest::test_case_3()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CoregTest::test_case_4()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CoregTest::test_case_5()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CoregTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::CoregTest::test_case_7()
{
	//Add you test code here
	return;
}

void
GPlatesUnitTest::CoregTestSuite::construct_maps()
{
	boost::shared_ptr<CoregTest> instance(
		new CoregTest());

	ADD_TESTCASE(CoregTest,test_case_1);
	ADD_TESTCASE(CoregTest,test_case_2);
	ADD_TESTCASE(CoregTest,test_case_3);
	ADD_TESTCASE(CoregTest,test_case_4);
	ADD_TESTCASE(CoregTest,test_case_5);
	ADD_TESTCASE(CoregTest,test_case_6);
	ADD_TESTCASE(CoregTest,test_case_7);
}

