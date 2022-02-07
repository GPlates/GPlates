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

#include "unit-test/DataAssociationDataTableTest.h"

#include "data-mining/OpaqueDataToQString.h"

GPlatesUnitTest::DataAssociationDataTableTestSuite::DataAssociationDataTableTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"DataAssociationDataTableTestSuite")
{
	init(level);
} 


void
GPlatesUnitTest::DataAssociationDataTableTestSuite::construct_maps()
{
	boost::shared_ptr<DataAssociationDataTableTest> instance(
		new DataAssociationDataTableTest());

	ADD_TESTCASE(DataAssociationDataTableTest,test_data_table);
}

void
GPlatesUnitTest::DataAssociationDataTableTest::test_data_table()
{

	BOOST_TEST_MESSAGE( "DataAssociationDataTableTest::test_data_table." );

	GPlatesDataMining::DataRowSharedPtr row(
			new GPlatesDataMining::DataRow);

	row->append_cell(
			GPlatesDataMining::OpaqueData(7));
	row->append_cell(
			GPlatesDataMining::OpaqueData("hello world!"));
	row->append_cell(
			GPlatesDataMining::OpaqueData(true));
			
	d_data_table->push_back(row);
	d_data_table->push_back(row);
	d_data_table->push_back(row);

	GPlatesDataMining::DataRowSharedPtr ret_row = d_data_table->at(0);

	GPlatesDataMining::OpaqueData o_data;
	QString con_str;
	boost::optional< int > j;
	ret_row->get_cell(
			0, 
			o_data);
	j = boost::get<boost::optional< int > >(
			o_data);
	con_str = boost::apply_visitor(
			GPlatesDataMining::ConvertOpaqueDataToString(),
			o_data);
	std::cout << "the int is: " << *j << std::endl;


	boost::optional< QString > str_r;
	ret_row->get_cell(
			1,
			o_data);
	str_r = boost::get<boost::optional< QString > >(
			o_data);
	con_str = boost::apply_visitor(
			GPlatesDataMining::ConvertOpaqueDataToString(),
			o_data);
	std::cout<<"the string is: "<<(*str_r).toStdString()<<std::endl;

	boost::optional< bool > ret_b;
	ret_row->get_cell(
			2,
			o_data);
	ret_b = boost::get<boost::optional< bool > >(
			o_data);
	con_str = boost::apply_visitor(
			GPlatesDataMining::ConvertOpaqueDataToString(),
			o_data);
	std::cout << "the bool is: " << *ret_b << std::endl;

	d_data_table->export_as_CSV(QString("export_as_CSV.csv"));

}
