#include <iostream>
#include <xercesc/sax/Locator.hpp>
#include "GPlatesFileHandler.h"
#include "XMLString.h"

using namespace GPlatesFileIO;

static std::ostream&
Print(std::ostream& os, const xercesc::Locator& loc)
{
	os <<  "Public Id: " << XMLString(loc.getPublicId()) << std::endl
		<< "System Id: " << XMLString(loc.getSystemId()) << std::endl
		<< "Line:      " << loc.getLineNumber() << std::endl
		<< "Column:    " << loc.getColumnNumber() << std::endl;
	return os;
}

void
GPlatesFileHandler::endDocument()
{
	// Blah.
}

void
GPlatesFileHandler::startElement(const XMLCh* const name, 
 xercesc::AttributeList& attrs)
{
	std::cout << "GPlatesFileHandler::startElement("
	 << XMLString(name) << "):" << std::endl;
	Print(std::cout, *_locator);
	size_t len = attrs.getLength();
	for (size_t i = 0; i < len; ++i) {
		std::cout << "\t" << XMLString(attrs.getName(i)) << std::endl;
		std::cout << "\t = " << XMLString(attrs.getValue(i)) << std::endl;
	}
}

void
GPlatesFileHandler::endElement(const XMLCh* const name)
{
	std::cout << "GPlatesFileHandler("
	 << XMLString(name) << ")." << std::endl << std::endl;
}
