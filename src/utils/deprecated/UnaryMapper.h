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

#ifndef GPLATES_UTILS_UNARYTRANSFORMER_H
#define GPLATES_UTILS_UNARYTRANSFORMER_H

#include "Mapper.h"

namespace GPlatesUtils
{
	/*
	* TODO: comments....
	*/
	template<
			typename InputIterator, 
			typename OutputIterator,
			typename UnaryFuntion = bool(*)(typename InputIterator::value_type) >
	class UnaryMapper : 
		public Mapper< InputIterator, OutputIterator>
	{
		
	public:

		/*
		* TODO: comments....
		*/
		explicit
		UnaryMapper(
				UnaryFuntion unary_fun) :
			d_unary_fun(unary_fun)
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
				boost::optional<OutputIterator> result = boost::none)
		{
			std::cout<< "running unary transformer" <<std::endl;
			OutputIterator result_begin, result_end;
			if(result)
			{
				result_begin = *result;
			}
			else
			{
				this->d_output_data.assign(
						std::distance(input_begin, input_end),
						*d_unary_fun(*input_begin));

				result_begin = this->d_output_data.begin();
				result_begin++;
				input_begin++;
			}
			result_end = result_begin;
			
			for( ; input_begin != input_end; ++input_begin, ++result_end)
			{
				*result_end = d_unary_fun(*input_begin);
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
		operator<< (
				boost::tuple<
						InputIterator,
						InputIterator> input) 
		{
			OutputIterator dummy1, dummy2;
			return boost::make_tuple(dummy2,dummy1);
		}
	protected:
		UnaryFuntion d_unary_fun;
		
		UnaryMapper();
			
	};
}
	
#endif
