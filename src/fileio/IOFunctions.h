#ifndef _GPLATES_FILEIO_IOFUNCTIONS_H
#define _GPLATES_FILEIO_IOFUNCTIONS_H

#include <string>

namespace GPlatesFileIO {

  bool empty(const char *buf); // Returns true iff the buffer is empty of anything except spaces, tabs or newlines.

  std::string substring(std::string wholestring, int start, int end); // Returns the substrng starting at start and ending at end. (N.B. end is actually the last character included, not one after it.)

}

#endif // _GPLATES_FILEIO_IOFUNCTIONS_H
