#include <map>
#include <sstream>
#include <iostream>
#include <utility>
#include <iomanip>

#include "RotationDataFileReader.h"
#include "FileFormatException.h"
#include "LineBuffer.h"
#include "PrimitiveDataTypes.h"
#include "../global/types.h"
#include "IOFunctions.h"

namespace GPlatesFileIO {

  using GPlatesGlobal::rgid_t;
  using GPlatesGlobal::fpdata_t;

  void 
  ReadInPlateRotationData(const char *filename, std::istream &input_stream, std::map< rgid_t, std::multimap< fpdata_t, FiniteRotation > > &rotation_data) { // The inputstream must point to the beginning of an already open file containing plates rotation data.
    LineBuffer lb(input_stream, filename); // This creates the line buffer that will be used for reading in the file contents.
    
    while (ReadRotationLine(lb, rotation_data)) // Keep going as long as we can read a non-empty line.
      { } 

  }  // When there are no more lines we're done.    
  
  bool 
  ReadRotationLine(LineBuffer &lb, std::map< rgid_t, std::multimap< fpdata_t, FiniteRotation > > &rotation_data) {
    
    char buf[MAXIMUM_LENGTH_OF_ROTATION_DATA_LINE+1]; // First read a line of the file into a buffer.
    if (! lb.getline(buf, sizeof(buf)) || Empty(buf)) return false; // We keep going until there is no more file to be read.
       
    std::string temp_string(buf);
    std::istringstream line_of_file(temp_string); // Now create an input string and read each item off in the order in which they occur.
    
    rgid_t rotating_plate=AttemptToReadRgid_t(line_of_file, lb);
    fpdata_t time=AttemptToReadFloat(line_of_file, lb);

    fpdata_t lat=AttemptToReadFloat(line_of_file, lb);
    fpdata_t lon=AttemptToReadFloat(line_of_file, lb);
    fpdata_t angle=AttemptToReadFloat(line_of_file, lb);
    
    rgid_t fixed_plate=AttemptToReadRgid_t(line_of_file, lb);

    // Now start to build the values into the data structures.

    LatLonPoint lat_lon(lat, lon); // First pack the lat and lon points into a LatLonPoint
    EulerRotation the_euler_rotation(lat_lon, angle); // Then put these into a Euler rotation together with the angle.
    FiniteRotation the_finite_rotation(time, fixed_plate, the_euler_rotation); // Finally everything goes into the FiniteRotation.
    
    rotation_data[rotating_plate].insert(std::pair<fpdata_t, FiniteRotation>(time, the_finite_rotation));
	
    return true;
  }

  rgid_t 
  AttemptToReadRgid_t(std::istringstream &iss, const LineBuffer &lb) {
    
    int i;

    if ( ! (iss >> i)) {
      
      // for some reason, unable to read an int
      std::ostringstream oss("Unable to extract an int from ");
      oss << lb
	  << " while attempting to parse a rotation data file.";
      
      throw FileFormatException(oss.str().c_str());
    }
    
    std::stringstream the_stream; // N.B. These normally have leading zero(s) if they are less than three digits.
    the_stream << std::setw(3) << std::setfill('0')<< i << std::setw(0); // We want all rgids to be padded out so they are three digits long. (E.g. 001 instead of just 1).
    rgid_t the_rgid;
    the_stream >> the_rgid;

    return the_rgid;
  }
  
  double 
  AttemptToReadFloat(std::istringstream &iss, const LineBuffer &lb) {
    
    double f;
    
    if ( ! (iss >> f)) {
      
      // for some reason, unable to read a float
      std::ostringstream oss("Unable to extract a float from ");
      oss << lb
	  << " while attempting to parse a rotation data file.";
      
      throw FileFormatException(oss.str().c_str());
    }
    return f;
  }

}
