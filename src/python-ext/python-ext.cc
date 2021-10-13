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
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <vector>

void
register_co_registration();

using namespace boost::python;
BOOST_PYTHON_MODULE(gplates_ext)
{
 	class_<std::vector<std::string> >("StrVec")
 		.def(vector_indexing_suite<std::vector<std::string> >());

	register_co_registration();
}









