/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <QDebug>

#include "ScribeTestSuite.h"
#include "TranscribeTest.h"


GPlatesUnitTest::ScribeTestSuite::ScribeTestSuite(
		unsigned level) : 
	GPlatesUnitTest::GPlatesTestSuite("ScribeTestSuite")
{
	init(level);
}

void 
GPlatesUnitTest::ScribeTestSuite::construct_maps()
{
	ADD_TESTSUITE(Transcribe);
}