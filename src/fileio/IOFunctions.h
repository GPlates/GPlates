/*$Id$*/
/// Functions used by RotationDataFileReader.cc and IO.cc
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
 * Author:
 *   Mike Dowman <mdowman@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_IOFUNCTIONS_H
#define _GPLATES_FILEIO_IOFUNCTIONS_H

#include <string>

namespace GPlatesFileIO {

  /// Returns true iff the buffer is empty of anything except spaces, tabs or newlines.
  bool empty(const char *buf); 

  /// Returns the substrng starting at start and ending at end. (N.B. end is actually the last character included, not one after it.)
  std::string substring(std::string wholestring, int start, int end); 

}

#endif // _GPLATES_FILEIO_IOFUNCTIONS_H
