#include "IOFunctions.h"
#include "../global/Assert.h"
#include "InvalidDataException.h"
#include <string>

namespace GPlatesFileIO {

  bool empty(const char *buf) {
    size_t index=std::strspn(buf, " \t\n"); // Ignore any spaces or new lines
    return buf[index]=='\0'; // If after these we just find the end of the string, then the string is empty.
  }

  std::string substring(std::string wholestring, int start, int end) {
    GPlatesGlobal::Assert(end-start<=98, InvalidDataException("Error in function substring - parameters out of range.")); // If the substring is more than 99 characters long we have an error.
    char substring[100]; // The substring can be up to 99 characters.
    char *index=substring;
    while (start<=end) {
      *index=wholestring[start];
      ++start;
      ++index;
    }
    *index='\0';
    
    return std::string(substring);
  }

}
