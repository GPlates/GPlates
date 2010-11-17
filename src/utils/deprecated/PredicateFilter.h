/* $Id$ */

/**
 * \file validate filename template.
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

#ifndef GPLATES_UTILS_PREDICATEFILTER_H
#define GPLATES_UTILS_PREDICATEFILTER_H

#include "Filter.h"

namespace GPlatesUtils
{
	/*
	* TODO: comments....
	*/
	template<
			typename InputIterator, 
			typename OutputIterator,
			typename Predicate = bool(*)(typename InputIterator::value_type)>
	class PredicateFilter : 
		public Filter< InputIterator, OutputIterator, std::vector<typename OutputIterator::value_type> >
	{
	public:

		/*
		* TODO: comments....
		*/
		explicit
		PredicateFilter(
				Predicate pre) :
			d_predicate(pre)
		{ }


		/*
		* TODO: comments....
		*/
		virtual
		boost::tuple<
				OutputIterator, //result begin
				OutputIterator> //result end
		operator()(
				InputIterator input_begin,
				InputIterator input_end,
				OutputIterator result)
		{
			OutputIterator result_begin, result_end;
			result_begin = result_end = result;
			
			for( ; input_begin != input_end; ++input_begin, ++result_end)
			{
				if (d_predicate(*input_begin)) 
				{
					*result_end = *input_begin;
				}
			}
			
			return boost::make_tuple(result_begin, result_end);
		}


		/*
		* TODO: comments....
		*/
		virtual
		boost::tuple<
				OutputIterator, //result begin
				OutputIterator> //result end
		operator()(
				InputIterator input_begin,
				InputIterator input_end)
		{
			return operator()(
					input_begin, 
					input_end,
					d_output_data);
		}


		/*
		* TODO: comments....
		*/
		virtual
		boost::tuple<
				OutputIterator, //result begin
				OutputIterator> //result end
		operator()(
				InputIterator input_begin,
				InputIterator input_end,
				std::vector<typename OutputIterator::value_type> &result)
		{
			for( ; input_begin != input_end; ++input_begin)
			{
				if (d_predicate(*input_begin)) 
				{
					result.push_back(*input_begin);
				}
			}
			
			return boost::make_tuple(
					d_output_data.begin(), 
					d_output_data.end());
		}


		/*
		* TODO: comments....
		*/
		virtual
		boost::tuple<
				OutputIterator, //result begin
				OutputIterator> //result end
		operator<< (
				boost::tuple<
						InputIterator,
						InputIterator> input) 
		{
			//TO BE IMPLEMENTED
			OutputIterator dummy1, dummy2;
			return boost::make_tuple(dummy2,dummy1);
		}
	protected:
		Predicate d_predicate;
		std::vector< typename OutputIterator::value_type > d_output_data;
		PredicateFilter();
	};
	
}

#endif

