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

#ifndef GPLATES_UTILS_BINARYREDUCER_H
#define GPLATES_UTILS_BINARYREDUCER_H

#include "Reducer.h"

namespace GPlatesUtils
{
	/*
	* TODO: comments....
	*/
	template< 
			typename InputIterator, 
			typename OutputDataType, 
			typename BinaryFunction = OutputDataType (*) (typename InputIterator::value_type, typename InputIterator::value_type)>
	class BinaryReducer : 
		public Reducer< InputIterator, OutputDataType >
	{
	public:	
		/*
		* TODO: comments....
		*/
		explicit
		BinaryReducer(
				BinaryFunction binary_fun) :
			d_binary_fun(binary_fun)
		{ }

		/*
		* TODO: comments....
		*/
		virtual
		OutputDataType
		operator()(
				InputIterator input_begin,
				InputIterator input_end ) 
		{
			boost::optional<  typename Reducer< InputIterator, OutputDataType >::InputValueType > ret = boost::none;
			for(; input_begin != input_end ; ++input_begin)
			{
				if(!ret)
				{
					ret = *input_begin;
				}
				 else
				{
					ret = d_binary_fun(ret,*input_begin); 
				}
			}
			return *ret;
		}
	protected:
		BinaryFunction d_binary_fun;
		BinaryReducer();
	};
}

#endif
