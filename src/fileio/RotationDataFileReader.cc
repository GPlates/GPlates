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

  void readinplaterotationdata(const char *filename, std::istream &inputstream, std::map< rgid_t, std::multimap< fpdata_t, FiniteRotation > > &rotationdata) { // The inputstream must point to the beginning of an already open file containing plates rotation data.
    LineBuffer lb(inputstream, filename); // This creates the line buffer that will be used for reading in the file contents.
    
    while (readrotationline(lb, rotationdata)) { } // Keep going as long as we can read a non-empty line.
    
  }  // When there are no more lines we're done.    
  
  bool readrotationline(LineBuffer &lb, std::map< rgid_t, std::multimap< fpdata_t, FiniteRotation > > &rotationdata) {
    
    char buf[MAXIMUMLENGTHOFROTATIONDATALINE+1]; // First read a line of the file into a buffer.
    if (! lb.getline(buf, sizeof(buf)) || empty(buf)) return false; // We keep going until there is no more file to be read.
       
    std::string tempstring(buf);
    std::istringstream lineoffile(tempstring); // Now create an input string and read each item off in the order in which they occur.
    
    rgid_t rotatingplate=attemptToReadrgid_t(lineoffile, lb);
    fpdata_t time=attemptToReadFloat(lineoffile, lb);

    fpdata_t lat=attemptToReadFloat(lineoffile, lb);
    fpdata_t lon=attemptToReadFloat(lineoffile, lb);
    fpdata_t angle=attemptToReadFloat(lineoffile, lb);
    
    rgid_t fixedplate=attemptToReadrgid_t(lineoffile, lb);

    // Now start to build the values into the data structures.

    LatLonPoint latlon(lat, lon); // First pack the lat and lon points into a LatLonPoint
    EulerRotation theeulerrotation(latlon, angle); // Then put these into a Euler rotation together with the angle.
    FiniteRotation thefiniterotation(time, fixedplate, theeulerrotation); // Finally everything goes into the FiniteRotation.
    
    rotationdata[rotatingplate].insert(std::pair<fpdata_t, FiniteRotation>(time, thefiniterotation));
	
    return true;
  }

  rgid_t attemptToReadrgid_t(std::istringstream &iss, const LineBuffer &lb) {
    
    int i;

    if ( ! (iss >> i)) {
      
      // for some reason, unable to read an int
      std::ostringstream oss("Unable to extract an int from ");
      oss << lb
	  << " while attempting to parse a rotation data file.";
      
      throw FileFormatException(oss.str().c_str());
    }
    
    std::stringstream thestream; // N.B. These normally have leading zero(s) if they are less than three digits.
    thestream << std::setw(3) << std::setfill('0')<< i << std::setw(0); // We want all rgids to be padded out so they are three digits long. (E.g. 001 instead of just 1).
    rgid_t thergid_t;
    thestream >> thergid_t;

    return thergid_t;
  }
  
  double attemptToReadFloat(std::istringstream &iss, const LineBuffer &lb) {
    
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
