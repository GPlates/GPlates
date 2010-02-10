/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>

#include <QDebug>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include "unit-test/TestSuiteFilter.h"

void
GPlatesUnitTest::TestSuiteFilter::set_filter_string(
		std::string filter_str)
{
	d_filter.clear();

	boost::char_separator<char> sep("/"); 
	boost::tokenizer<boost::char_separator<char> > tokens(filter_str, sep); 
	
	for( boost::tokenizer<boost::char_separator<char> >::iterator it1=tokens.begin();
		it1 != tokens.end();
		it1++)
	{ 
		std::string sub_str = *it1;

		boost::char_separator<char> sub_sep(","); 
		boost::tokenizer<boost::char_separator<char> > sub_tokens(sub_str, sub_sep);
		std::vector<std::string> sub_filter;
	
		for(boost::tokenizer<boost::char_separator<char> >::iterator it2 = sub_tokens.begin();
			it2 != sub_tokens.end();
			it2++)
		{ 
			std::string str = *it2;
			sub_filter.push_back(str);
		}
		d_filter.push_back(sub_filter);
	} 
}

bool
GPlatesUnitTest::TestSuiteFilter::is_empty(
		unsigned depth)
{
	try{
		if(depth >= d_filter.size())
		{
			return true;
		}
		else
		{
			return d_filter.at(depth).empty();
		}
	}
	catch(...)
	{
		qWarning() << "Unexpected exception caught in GPlatesUnitTest::TestSuiteFilter::is_empty(unsigned depth)!!";
		return true;
	}
}

bool 
GPlatesUnitTest::TestSuiteFilter::pass(
		std::string test_suite_name,
		unsigned depth)
{
	if(is_empty(depth))
	{
		return true;
	}
	else
	{
		for(unsigned i = 0; i < d_filter[depth].size(); i++)
		{
			if(is_match(test_suite_name,d_filter[depth][i]))
			{
				return true;
			}
		}
		return false;
	}
}

bool
GPlatesUnitTest::TestSuiteFilter::is_match(std::string str, std::string pattern)
{
	if(pattern == "*" || str == pattern || pattern == "")
	{
		return true;
	}
	if(pattern[0] == '*')//leading '*'
	{
		if(str.length() < pattern.length()-1)
		{
			return false;
		}
		else
		{
			size_t pos = str.length() - (pattern.length()-1);
	
			if(str.substr(pos, pattern.length()-1) == pattern.substr(1))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}	
	
	if(pattern[pattern.length()-1] == '*')//tailing '*'
	{
		if(str.substr(0, pattern.length()-1) == pattern.substr(0, pattern.length()-1))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

GPlatesUnitTest::FilterData
GPlatesUnitTest::TestSuiteFilter::d_filter(0);

GPlatesUnitTest::TestSuiteFilter::TestSuiteFilter()
{ }


