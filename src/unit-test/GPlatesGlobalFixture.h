/* $Id$ */

/**
* \file 
* $Revision$
* $Date$
* 
* Copyright (C) 2009 The University of Sydney, Australia
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
#ifndef GPLATES_UNIT_TEST_GLOBALFIXTURE_H
#define GPLATES_UNIT_TEST_GLOBALFIXTURE_H

#include <boost/test/unit_test.hpp>
#include <fstream>
#include <iostream>
	
struct GPlatesGlobalFixture 
{
	GPlatesGlobalFixture() :
		test_log( "GPlates_unit_test.log" )
	{
		boost::unit_test::unit_test_log.set_stream( test_log );
	}

	~GPlatesGlobalFixture()                             
	{
		boost::unit_test::unit_test_log.set_stream( std::cout );
	}

	std::ofstream test_log;
};

#endif //GPLATES_UNIT_TEST_GLOBALFIXTURE_H

