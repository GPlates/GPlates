/*$ID$*/
/// Exception used when some data value in a Plates file being read in is out of range.
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

#ifndef _GPLATES_FILEIO_INVALIDDATAEXCEPTION_H_
#define _GPLATES_FILEIO_INVALIDDATAEXCEPTION_H_

#include "FileIOException.h"

namespace GPlatesFileIO {
  
  // This exception is thrown when there is  a problem with the value of a data item in a file being read in by GPlates.
  class InvalidDataException : public FileIOException {
  public:
    InvalidDataException(const char* message) : _message(message) { }
    
    virtual ~InvalidDataException() { }
    
  protected:
    virtual const char * ExceptionName() const { return "InvalidDataException"; }
    
    virtual std::string Message() const { return _message; }
    
   private:
    std::string _message;
  };
  
 } // End namespace

#endif  // _GPLATES_FILEIO_INVALIDDATAEXCEPTION_H_
