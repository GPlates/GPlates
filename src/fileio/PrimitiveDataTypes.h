/* $Id$ */

/**
 * \file 
 * This file contains a collection of primitive data types which will be
 * used by the PLATES-format parser, and will probably be used by the
 * GPlates-format parser.  They are labelled "primitive" in this context
 * because they are so simple;  they are really only intended to be
 * temporary place-holders, providing data-types for the parsing
 * before the geometry engine takes over.  They are also intended to
 * provide something resembling an "interface" to the parsers, and a
 * layer of abstraction from the (possibly-changing) geometry engine.
 * These types are currently also all structs due to their primitive
 * and temporary nature;  if necessary, they may become classes (with
 * a correspondingly more complicated interface of accessors and
 * modifiers).
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_PRIMITIVEDATATYPES_H_
#define _GPLATES_FILEIO_PRIMITIVEDATATYPES_H_

#include "../global/types.h"

namespace GPlatesFileIO
{
	using namespace GPlatesGlobal;

	struct LatLonPoint {

		fpdata_t _lat, _lon;

		// no default constructor

		LatLonPoint(fpdata_t lat, fpdata_t lon)
		 : _lat(lat), _lon(lon) {  }
	};


	struct EulerRotation {

		LatLonPoint _pole;
		fpdata_t    _angle;  // degrees

		// no default constructor

		EulerRotation(const LatLonPoint &pole, fpdata_t angle)
		 : _pole(pole), _angle(angle) {  }
	};


	struct FiniteRotation {

		fpdata_t      _time;  // Millions of years ago
		rgid_t        _fixed_rg;
		EulerRotation _rot;

		// no default constructor

		FiniteRotation(fpdata_t time, const rgid_t &fixed_rg,
		 const EulerRotation &rot)

		 : _time(time), _fixed_rg(fixed_rg), _rot(rot) {  }
	};
}

#endif  // _GPLATES_FILEIO_PRIMITIVEDATATYPES_H_
