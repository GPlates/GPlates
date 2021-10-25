/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_UNIT_TEST_SCRIBEEXPORTUNITTEST_H
#define GPLATES_UNIT_TEST_SCRIBEEXPORTUNITTEST_H

#include "TranscribeTest.h"


/**
 * Scribe export registered classes/types in the 'unit-test' source sub-directory.
 *
 * See "ScribeExportRegistration.h" for more details.
 *
 *******************************************************************************
 * WARNING: Changing the string ids will break backward/forward compatibility. *
 *******************************************************************************
 */
#define SCRIBE_EXPORT_UNIT_TEST \
		\
		((GPlatesUnitTest::TranscribePrimitivesTest::Data::NonDefaultConstructable, \
			"GPlatesUnitTest::TranscribePrimitivesTest::Data::NonDefaultConstructable")) \
		\
		((GPlatesUnitTest::TranscribeInheritanceTest::D, \
			"GPlatesUnitTest::TranscribeInheritanceTest::D")) \
		\
		((GPlatesUnitTest::TranscribeCompatibilityTest::Derived, \
			"GPlatesUnitTest::TranscribeCompatibilityTest::Derived")) \
		\


#endif // GPLATES_UNIT_TEST_SCRIBEEXPORTUNITTEST_H
