#include <sstream>
#include <iostream>
#include <list>
#include <map>
#include <cstring>

#include "IO.h"
#include "LineBuffer.h"
#include "PrimitiveDataTypes.h"
#include "PlatesDataTypes.h"
#include "../global/FPData.h"
#include "FileFormatException.h"
#include "../global/Assert.h"
#include "IOFunctions.h"
#include "InvalidDataException.h"
#include "../geo/TimeWindow.h"

namespace GPlatesFileIO { // GPlatesFileIO  
  
  /*
   * The possible values for the plotter code.
   * Don't worry too much about what this means.
   */
  enum { PEN_DOWN = 2, PEN_UP = 3 };

  void readinplateboundarydata(const char *filename, std::istream &inputstream, std::map< GPlatesGlobal::rgid_t, PlatesPlate> &platesdata) { // The inputstream must point to the beginning of an already open file containing plates boundary data.  
    LineBuffer lb(inputstream, filename); // This creates the line buffer that will be used for reading in the file contents.
    
    while (readIsochron(lb, platesdata)) { } // Each file can contain multiple polylines. So keep reading until we get false returned.
  }

  bool readIsochron(LineBuffer &lb, std::map< GPlatesGlobal::rgid_t, PlatesPlate> &platesdata) { // given a LineBuffer, reads in the file into two strings and a list of LatLonPoints

    std::string first_line = readFirstLineOfIsochronHeader(lb);
    if (first_line=="") return false; // If we can't read anything then we've reached the end of the file so we can stop.
    
    std::string second_line = readSecondLineOfIsochronHeader(lb);

    size_t num_points_to_expect =
      parseLine(second_line, lb);

    std::list< LatLonPoint > points;
    readIsochronPoints(lb, points, num_points_to_expect);
    
    // **** Now we have read in the data, we need to parse it and put it in the PlatesPlate struct. ****
    
    GPlatesGlobal::rgid_t plateid=extractplateid(first_line); // First find the plate id
    platesdata[plateid]._plate_id=plateid;     // The plates are indexed by the id, but also contain it.

    fpdata_t begin=extractbegintime(second_line); // The polyline headers need this data.
    fpdata_t end=extractendtime(second_line);
    GPlatesGeo::TimeWindow lifetime(begin, end);
    
    PlatesPolyLineHeader newpolylineheader(first_line, second_line, plateid, lifetime); // First make the header for the new polyline.
    
    PlatesPolyLine newpolyline(newpolylineheader); // Then the polyline itself - which so far includes only its header.
    newpolyline._points=points; // Then put the latlonpoints in.
    
    platesdata[plateid]._polylines.push_back(newpolyline); // Now we just add this polyline to the list of polylines in the plate.
    
    return true;
  }
  
  fpdata_t extractbegintime(const std::string &secondline) {
    std::string beginstring=substring(secondline, 5, 10); // The begin data is characters 5 to 10.
    std::istringstream tempstream(beginstring); // We can convert it to a number like this.
    fpdata_t begin;
    tempstream >> begin;
    return begin;
  }
  
  fpdata_t extractendtime(const std::string &secondline) {
    std::string endstring=substring(secondline, 12, 17); // The end data is characters 12 to 17.
    std::istringstream tempstream(endstring); // We can convert it to a number like this.
    fpdata_t end;
    tempstream >> end;
    return end;
  }
  
  GPlatesGlobal::rgid_t extractplateid(const std::string &firstline) { // The plateid is the first four characters of the file, which should be followed by a space.
    Assert(firstline[4]==' ', FileFormatException("Error in parsing the plate_id from the first line of the file."));
    return substring(firstline, 0, 3);
  }
  
  /*
   * A reasonable maximum length for each line of an isochron header.
   * This length does not include a terminating character.
   */
  enum { ISOCHRON_HEADER_LINE_LEN = 8000 }; // Before this was 80 - but that is actually the exact length of many lines - so this is now really large to allow for exceptionally long lines.
  
  
  std::string
  readFirstLineOfIsochronHeader(LineBuffer &lb) { // takes a reference to a line buffer and returns the first line read in a string.
    
    char buf[ISOCHRON_HEADER_LINE_LEN + 1];
    
    if ( ! lb.getline(buf, sizeof(buf) ) || empty(buf)) {
      return std::string("");
    }
    
    return std::string(buf);
  }
  
  std::string
  readSecondLineOfIsochronHeader(LineBuffer &lb) { // takes a reference to a line buffer and returns the first line read in a string.
    
    char buf[ISOCHRON_HEADER_LINE_LEN + 1];
    
    if ( ! lb.getline(buf, sizeof(buf))) {
      
      // for some reason, the read was considered "unsuccessful"
      std::ostringstream oss("Unsuccessful read from ");
      oss << lb
	  << " while attempting to read line 2 of isochron header";

      throw FileFormatException(oss.str().c_str());
    }
    
    return std::string(buf);
  }
    
  // *** Warning - there are two different functions both called parseLine ***
  size_t
  parseLine(const std::string &line, const LineBuffer &lb) { // This just returns the last item in the second line of the file (which is an integer).
      
    if (line.size()<39) { // The second line of the header must always be at least 39 characters long or something is wrong.
	
      std::ostringstream oss("Second line of the isochron header too short at ");
      oss << lb;
	
      throw FileFormatException(oss.str().c_str());
    }
      
    std::istringstream tempstringstream(substring(line, 34, 38)); // Take the substring which corresponds to the integer (and possibly some leading zeros).
    if (line[33]!=' ') { // Check that the number is preceeded a space, and followed by a space, new line or the end of the string.
      std::ostringstream oss("Second line of the isochron header contains an error at ");
      oss << lb;
	
      throw FileFormatException(oss.str().c_str());
    }
      
    int last_item; // Convert it to an integer and put it in last item.
    tempstringstream >> last_item;
      
    /*
     * Since a polyline must contain at least two points,
     * the value of the last item on the line should be
     * an integer >= 2.
     */
    if (last_item < 2) {
	
      std::ostringstream oss("Invalid value ");
      oss << last_item
	  << " for last item of the second line of the isochron header at "
	  << lb;
	
      throw FileFormatException(oss.str().c_str());
    }
      
    // Since it can never be < zero, let's return it as a size_t
    return static_cast< size_t >(last_item);
  }

  /*
   * A reasonable maximum length for each line representing an isochron point.
   * This length does not include a terminating character.
   */
  enum { ISOCHRON_POINT_LINE_LEN = 40 };
    
  void
  readIsochronPoints(LineBuffer &lb, std::list< LatLonPoint > &points, size_t num_points_to_expect) { // Just read all the LatLon points from the file into the list points.
    
    /*
     * The number of points to expect was specified in the isochron header.
     * We have asserted that it must be at least 2.  Read the first point.
     */
    std::string first_line = readIsochronPoint(lb);

    LatLonPoint first_point = parseLine(first_line, lb, PEN_UP);
    points.push_back(first_point);

    /*
     * We've already read the first point, and we don't want to read
     * the "terminating point" inside this loop.
     */
    for (size_t n = 1; n < num_points_to_expect; n++) {

      std::string line = readIsochronPoint(lb);

      LatLonPoint point = parseLine(line,
				    lb, PEN_DOWN);
      points.push_back(point);
    }
    
    /*
     * Now, finally, read the "terminating point".
     * This is not really a valid "point", since its lat and lon
     * are both 99.0.
     */

    std::string term_line = readIsochronPoint(lb);

    parseTerminatingLine(term_line, lb);

  }
    
  std::string
  readIsochronPoint(LineBuffer &lb) { // Just read a line of the file from lb and return it as a string.
    
    char buf[ISOCHRON_POINT_LINE_LEN + 1];
    
    if ( ! lb.getline(buf, sizeof(buf))) {
	
      // for some reason, the read was considered "unsuccessful"
      std::ostringstream oss("Unsuccessful read from ");
      oss << lb
	  << " while attempting to read an isochron point";
      
      throw FileFormatException(oss.str().c_str());
    }
    
    return std::string(buf);
  }
  
  // *** Warning - there are two different functions both called parseLine ***
  LatLonPoint
  parseLine(const std::string &line, const LineBuffer &lb, int expected_plotter_code) { //parseline
      
    /*
     * This line is composed of two doubles (the lat/lon of the
     * point) and an int (a plotter code).
     */
    fpdata_t lat, lon;
    int plotter_code;
      
    std::istringstream iss(line);
      
    lat = attemptToReadCoord(iss, lb, "latitude");
    if (90.0 < lat || lat < -90.0) {
	  
      /*
       * The latitude read was outside the valid range of
       * values for a latitude (which is [-90.0, 90.0]).
       */
      std::ostringstream oss("Invalid value (");
      oss << lat
	  << ") for latitude found at "
	  << lb;
	
      throw InvalidDataException(oss.str().c_str());
    }
      
    lon = attemptToReadCoord(iss, lb, "longitude");
    if (180.0 < lon || lon <= -180.0) {
	
      /*
       * The longitude read was outside the valid range of
       * values for a longitude (which is (-180.0, 180.0]).
       */
      std::ostringstream oss("Invalid value (");
      oss << lon
	  << ") for longitude found at "
	  << lb;
	
      throw InvalidDataException(oss.str().c_str());
    }
      
    plotter_code = attemptToReadPlotterCode(iss, lb);
    if (plotter_code != expected_plotter_code) {
	
      /*
       * The plotter code which was read was not
       * the code which was expected.
       */
      std::ostringstream oss("Unexpected value (");
      oss << plotter_code
	  << ") for plotter code found at "
	  << lb;
	
      throw InvalidDataException(oss.str().c_str());
    }
      
    return LatLonPoint(lat, lon);
  }
    
  void
  parseTerminatingLine(const std::string &line, const LineBuffer &lb) { // parseTerminatingLine
      
    /*
     * This line is composed of two doubles (the lat/lon of the
     * point) and an int (a plotter code).
     */
    fpdata_t lat, lon;
    int plotter_code;
      
    std::istringstream iss(line);
      
    lat = attemptToReadCoord(iss, lb, "latitude");
    if (lat != 99.0) {
	
      /*
       * The latitude read was not the expected (constant)
       * latitude of a terminating point (which is 99.0).
       */
      std::ostringstream oss("Invalid value (");
      oss << lat
	  << ") for latitude of terminating point found at "
	  << lb;
	
      throw InvalidDataException(oss.str().c_str());
    }
      
    lon = attemptToReadCoord(iss, lb, "longitude");
    if (lon != 99.0) {
	
      /*
       * The longitude read was not the expected (constant)
       * longitude of a terminating point (which is 99.0).
       */
      std::ostringstream oss("Invalid value (");
      oss << lon
	  << ") for longitude of terminating point found at "
	  << lb;
	
      throw InvalidDataException(oss.str().c_str());
    }

    plotter_code = attemptToReadPlotterCode(iss, lb);
    if (plotter_code != PEN_UP) {
	
      /*
       * The plotter code which was read was not
       * the code which was expected.
       */
      std::ostringstream oss("Unexpected value (");
      oss << plotter_code
	  << ") for plotter code of terminating point found at "
	  << lb;
	
      throw InvalidDataException(oss.str().c_str());
    }
  } 


  fpdata_t
  attemptToReadCoord(std::istringstream &iss, const LineBuffer &lb, const char *desc) {

    fpdata_t d;
    if ( ! (iss >> d)) {

      // for some reason, unable to read a double
      std::ostringstream oss("Unable to extract a floating-point coord value from ");
      oss << lb
	  << " while attempting to parse the "
	  << desc
	  << " longitude of a point";

      throw FileFormatException(oss.str().c_str());
    }

    return d;
  }

  int
  attemptToReadPlotterCode(std::istringstream &iss, const LineBuffer &lb) {

    int i;
    if ( ! (iss >> i)) {

      // for some reason, unable to read an int
      std::ostringstream oss("Unable to extract an integer value from ");
      oss << lb
	  << " while attempting to parse the plotter code of a point";

      throw FileFormatException(oss.str().c_str());
    }

    return i;
  }
}
