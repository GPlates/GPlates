#include <iostream>
#include "ErrorHandler.h"
#include "XMLString.h"

using namespace GPlatesFileIO;
using xercesc::SAXParseException;

static std::ostream&
Print(std::ostream& os, const SAXParseException& ex)
{
	os <<  "  File:    " << XMLString(ex.getSystemId()) << std::endl
		<< "  Line:    " << ex.getLineNumber() << std::endl
		<< "  Column:  " << ex.getColumnNumber() << std::endl
		<< "  Message: " << XMLString(ex.getMessage()) << std::endl;
	return os;
}

void
ErrorHandler::warning(const SAXParseException& ex)
{
	std::cerr << "Warning: " << std::endl;
	Print(std::cerr, ex);
	std::cerr << std::endl;
}

void
ErrorHandler::error(const SAXParseException& ex)
{
	std::cerr << "Error: " << std::endl;
	Print(std::cerr, ex);
	std::cerr << std::endl;
}

void
ErrorHandler::fatalError(const SAXParseException& ex)
{
	std::cerr << "Fatal Error: " << std::endl;
	Print(std::cerr, ex);
	std::cerr << std::endl;
}
