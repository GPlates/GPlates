/*
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
 *   Mike Dowman <mdowman@geosci.usyd.edu.au> and James Boyden <jboyden@geosci.usyd.edu.au>

 This file reads in files in the plates rotation file format. This format arranges data in columns separated by spaces. The columns appear in the following order, left to right.

 integer moving plate number 
 float   time
 float   latitude
 float   longitude
 float   angle
 integer fixed plate number
 string  comment (comment is immediately preceeded by as explanation mark, as in !comment).

 Each line of the file is in the same format, but colums don't necessarily line up.

*/

#ifndef _ROTATION_DATA_FILE_READER_H
#define _ROTATION_DATA_FILE_READER_H

#include <map>

#include "PrimitiveDataTypes.h"
#include "LineBuffer.h"
#include "PrimitiveDataTypes.h"
#include "IOFunctions.h"
#include "../global/types.h"

namespace GPlatesFileIO {
  
  const int MAXIMUMLENGTHOFROTATIONDATALINE=500; // Make sure that this is big enough for anything we're likely to encounter.
  
  void readinplaterotationdata(const char *filename, std::istream &inputstream, std::map< rgid_t, std::multimap< fpdata_t, FiniteRotation > > &rotationdata); // The inputstream must point to the beginning of an already open file containing plates rotation data.

  bool readrotationline(LineBuffer &lb, std::map< GPlatesGlobal::rgid_t, std::multimap< fpdata_t, FiniteRotation > > &rotationdata);

  rgid_t attemptToReadrgid_t(std::istringstream &iss, const LineBuffer &lb);
  
  double attemptToReadFloat(std::istringstream &iss, const LineBuffer &lb);

}

#endif // _ROTATION_DATA_FILE_READER_H
