/* Copyright (C) 2003 The GPlates Consortium
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
 *   Mike Dowman <mdowman@geosci.usyd.edu.au> & James Boyden <jboyden@geosci.usyd.edu.au>

This file reads in files in the plates format which is as follows:-

The Plates String Format

Header

1st Line

2 digit integer    region number       (These two togeth are also called
2digit integer     reference number  (a reference number.)
space
4 digit integer    string number
space
ascii string       geographic description of the data

2nd Line

space
3 digit integer    plate identification number
space
6 character, 1dp float   time of appearance
space
6 character, 1dp float   time of disappearance
space
2 ascii chararacters   code for the type of data
4digit integer      data type number
1 ascii character   code letter for further description of the data
3 digit integer     plate identification number
space
3 digit integer     colour code
space
5 digit integer     how many points are in the string.

Each latitude-longitude line (there are any number of these - 
each on a separate line)

space
3 digit integer
space 
9 character, 4dp float
space
9 character, 4dp float
space
1 digit integer

Final Line

space
99.0000
3 spaces
99.0000
space
3

N.B. Everything is right alligned, and any gaps on the left are filled up with spaces. (For example if a three digit integer is between 0 and 9, two spaces will be added on the left, so that it takes up three characters.)

N.B. Some items are NOT delimited by spaces, and sometimes they can be missing! (e.g. the single ascii character in line two can be a space.)

This code requires that each polyline contains at least two points, and that the plotter code is 3 on the first code, and 2 thereafter, except for the terminating line when it becomes 3 again. This might not be the case with all Plates files - if that is the case then this is a bug.

*/

#ifndef _GPLATES_FILEIO_IO_H_
#define _GPLATES_FILEIO_IO_H_

#include <sstream>
#include <iostream>
#include <list>
#include <map>
#include <cstring>

#include "../global/FPData.h"
#include "../global/types.h"
#include "PlatesDataTypes.h"
#include "LineBuffer.h"

namespace GPlatesFileIO {

  using GPlatesGlobal::rgid_t;

  void temp(int num);

  void readinplateboundarydata(const char *filename, std::istream &inputstream, std::map< rgid_t, PlatesPlate> &platesdata); // The inputstream must point to the beginning of an already open file containing plates boundary data.  

  bool readIsochron(LineBuffer &lb, std::map< rgid_t, PlatesPlate> &platesdata); // given a LineBuffer, reads in the file into two strings and a list of LatLonPoints

  fpdata_t extractbegintime(const std::string &secondline);

  fpdata_t extractendtime(const std::string &secondline);

  rgid_t extractplateid(const std::string &firstline); // The plateid is the first four characters of the file, which should be followed by a space.

  std::string readFirstLineOfIsochronHeader(LineBuffer &lb); // takes a reference to a line buffer and returns the first line read in a string.

  std::string readSecondLineOfIsochronHeader(LineBuffer &lb); // takes a reference to a line buffer and returns the first line read in a string.

  void readIsochronPoints(LineBuffer &lb, std::list< LatLonPoint > &points, size_t num_points_to_expect); // Just read all the LatLon points from the file into the list points.

  std::string readIsochronPoint(LineBuffer &lb); // Just read a line of the file from lb and return it as a string.

  // *** Warning - there are two different functions both called parseLine ***
  LatLonPoint parseLine(const std::string &line, const LineBuffer &lb, int expected_plotter_code);
  
  fpdata_t attemptToReadCoord(std::istringstream &iss, const LineBuffer &lb, const char *desc);
  
  int attemptToReadPlotterCode(std::istringstream &iss, const LineBuffer &lb);
  
  void parseTerminatingLine(const std::string &line, const LineBuffer &lb);
    
  void parseTerminatingLine(const std::string &line, const LineBuffer &lb);
  
  size_t parseLine(const std::string &line, const LineBuffer &lb); // This just returns the last item in the second line of the file (which is an integer).

}

#endif






