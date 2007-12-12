#include "StringFormattingUtils.h"
#include <sstream>
#include <iomanip>
#include "global/Assert.h"


std::string
GPlatesUtils::formatted_real_to_string(const GPlatesMaths::Real &val, 
		unsigned width, unsigned prec)
{
	GPlatesGlobal::Assert(width > 0,
		InvalidFormattingParametersException(
			"Attempt to format a real number using a negative width."));
	GPlatesGlobal::Assert(prec > 0,
		InvalidFormattingParametersException(
			"Attempt to format a real number using a negative precision."));

	// The number 3 below is the number of characters required to
	// represent (1) the decimal point, (2) the minus sign, and (3)
	// at least one digit to the left of the decimal point.
	GPlatesGlobal::Assert(width >= (prec + 3), 
		InvalidFormattingParametersException(
			"Attempted to format a real number with parameters that don't "\
			"leave enough space for the decimal point, sign, and integral part."));

	std::ostringstream oss;
	oss << std::setw(width)			// Use 'width' chars of space
		<< std::right				// Right-justify
		<< std::setfill(' ')		// Fill in with spaces
		<< std::fixed 				// Always use decimal notation
		<< std::showpoint			// Always show the decimal point
		<< std::setprecision(prec)	// Show prec digits after the decimal point
		<< val.dval();				// Print the actual value

	return oss.str();
}


std::string
GPlatesUtils::formatted_int_to_string(int val, unsigned width, char fill_char)
{
	GPlatesGlobal::Assert(width > 0,
		InvalidFormattingParametersException(
			"Attempt to format an integer using a negative width."));

	std::ostringstream oss;
	oss << std::setw(width) 
		<< std::right 
		<< std::setfill(fill_char) 
		<< val;
	return oss.str();
}

