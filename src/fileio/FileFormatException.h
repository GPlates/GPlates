/*$Id$*/
/// Exception used when a problem is found with the format of a Plates file being read in.
/**
 * @file
 *
 * Most recent change:
 * $Author$
 * $Date$
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

#ifndef _GPLATES_FILEIO_FILEFORMATEXCEPTION_H_
#define _GPLATES_FILEIO_FILEFORMATEXCEPTION_H_

#include "FileIOException.h"

namespace GPlatesFileIO {
  
  // This exception is thrown when there is  a problem with the Format of a file being read in by GPlates.
  class FileFormatException : public FileIOException {
   public:
    
    /// message is a description of the conditions which cause the invariant to be violated.
    FileFormatException(const char* message) : _message(message) { }
    
    virtual ~FileFormatException() { }
    
   protected:
    virtual const char * ExceptionName() const { return "FileFormatException"; }
    
    virtual std::string Message() const { return _message; }
    
   private:
    std::string _message;
  };
  
} // End namespace

#endif  // _GPLATES_FILEIO_FILEFORMATEXCEPTION_H_
