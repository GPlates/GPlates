/*$Id$*/
/// Reads in Plates data rotation files.
/**
 * @file
 *
 * Most recent change:
 * $Author$
 * $Date$
 *
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
 *   Mike Dowman <mdowman@geosci.usyd.edu.au> and James Boyden <jboyden@geosci.usyd.edu.au>
 *
 * This file reads in files in the plates rotation file format. This format arranges data in columns separated by spaces. The columns appear in the following order, left to right.
 *
 * integer moving plate number 
 * float   time
 * float   latitude
 * float   longitude
 * float   angle
 * integer fixed plate number
 * string  comment (comment is immediately preceeded by as explanation mark, as in !comment).
 *
 * Each line of the file is in the same format, but colums don't necessarily line up.
 *
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
  
  const int MAXIMUM_LENGTH_OF_ROTATION_DATA_LINE=500; // Make sure that this is big enough for anything we're likely to encounter.
 
  /// This functions reads in a Plates rotation data file and puts its contents in the map.   
  /** The inputstream must point to the beginning of an already open file containing plates rotation data.
   */
  void ReadInPlateRotationData(const char *filename, std::istream &input_stream, std::map< rgid_t, std::multimap< fpdata_t, FiniteRotation > > &rotation_data); 

  /// Reads one line of the file and puts it in the map - returns false iff there is no more file to be read.
  bool ReadRotationLine(LineBuffer &lb, std::map< GPlatesGlobal::rgid_t, std::multimap< fpdata_t, FiniteRotation > > &rotation_data);

  /// Reads in an rgid_t from the istringstream - exception will be thrown if it fails.
  rgid_t AttemptToReadRgid_t(std::istringstream &iss, const LineBuffer &lb);
  
  /// Reads in a float from the istringstream - exception will be thrown if it fails.
  double AttemptToReadFloat(std::istringstream &iss, const LineBuffer &lb);

}

#endif // _ROTATION_DATA_FILE_READER_H
