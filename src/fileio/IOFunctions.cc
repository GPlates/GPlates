#include "IOFunctions.h"
#include "../global/Assert.h"
#include "InvalidDataException.h"
#include <string>

namespace GPlatesFileIO {

  bool 
  Empty(const char *buf) {
    size_t index=std::strspn(buf, " \t\n"); // Ignore any spaces or new lines
    return buf[index]=='\0'; // If after these we just find the end of the string, then the string is empty.
  }

  std::string 
  SubString(std::string whole_string, int start, int end) {
    static const int MAXIMUM_SIZE_OF_SUBSTRING=99; // The substring can be up to 99 characters.
    GPlatesGlobal::Assert(end-start<=MAXIMUM_SIZE_OF_SUBSTRING-1, InvalidDataException("Error in function substring - parameters out of range.")); // If the substring is more than 99 characters long we have an error.
    char substring[MAXIMUM_SIZE_OF_SUBSTRING+1]; 
    char *index=substring;
    while (start<=end) {
      *index=whole_string[start];
      ++start;
      ++index;
    }
    *index='\0';
    
    return std::string(substring);
  }

}
