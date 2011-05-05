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
#ifndef GPLATES_UNIT_TEST_COREG_TEST_H
#define GPLATES_UNIT_TEST_COREG_TEST_H

#include <vector>
#include <boost/test/unit_test.hpp>

#include "unit-test/GPlatesTestSuite.h"
#include "file-io/FileInfo.h"
#include "file-io/File.h"

namespace GPlatesDataMining
{
	class CoRegConfigurationTable;
}

namespace GPlatesUnitTest
{
	class CoregTest
	{
	public:
		CoregTest();

		void 
		test_case_1();

		void 
		test_case_2();

		void 
		test_case_3();

		void 
		test_case_4();

		void 
		test_case_5();

		void 
		test_case_6();

		void 
		test_case_7();

	private:
		/*
		* Load test data files.
		*/
		void
		load_test_data();

		/*
		* Return particular section of configuration file. 
		*/
		std::vector<QString>
		load_cfg(
				const QString& cfg_file,
				const QString& section_name);

		inline
		QString
		load_one_line_cfg(
				const QString& cfg_file,
				const QString& section_name)
		{
			std::vector<QString> cfgs = load_cfg(cfg_file,section_name);
			return cfgs.size() ? cfgs[0] : QString();
		}

		/*
		* Run the test at certain time.
		*/
		void
		test(double time);

		/*
		* Check the result at certain time.
		*/
		bool
		check_result(double time);

		/*
		* Populate CoRegConfigurationTable from configuration file.
		*/
		void
		populate_cfg_table(
				GPlatesDataMining::CoRegConfigurationTable& table,
				const QString& filename);
		
		inline
		QString
		get_output_name(double time)
		{
			QString prefix = d_output_prefix.size() ? d_output_prefix : "coreg";
			return prefix + "." + QString().setNum(time);
		}

		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_loaded_files;
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_rotation_fc;
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_seed_fc;
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_coreg_fc;
		QString d_output_prefix;
		QString d_output_path;
	};

	
	class CoregTestSuite : 
		public GPlatesUnitTest::GPlatesTestSuite
	{
	public:
		CoregTestSuite(
				unsigned depth);

	protected:
		void 
		construct_maps();
	};
}
#endif //GPLATES_UNIT_TEST_COREG_TEST_H 



