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
#ifndef GPLATES_UNIT_TEST_TESTSUITEFILTER_H
#define GPLATES_UNIT_TEST_TESTSUITEFILTER_H

#include <vector>
#include <string>

#include "utils/Singleton.h"

namespace GPlatesUnitTest
{
	typedef std::vector<
			std::vector<std::string> > FilterData;

	class TestSuiteFilter :
		public GPlatesUtils::Singleton<TestSuiteFilter>
	{

		GPLATES_SINGLETON_CONSTRUCTOR_DEF(TestSuiteFilter)
	
	public:
		
		static
		void
		set_filter_string(
				std::string);
		
		bool 
		is_empty(
				unsigned depth);
			
		bool 
		pass(
				std::string test_suite_name,
				unsigned depth);

		bool
		is_match(
				std::string, std::string);

		FilterData
		get_filter()
		{
			return d_filter;
		}

	protected:
		static FilterData d_filter;
	};
}
#endif //GPLATES_UNIT_TEST_TESTSUITEFILTER_H


