/* $Id: GenericMapper.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file validate filename template.
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
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

#ifndef GPLATES_UTILS_GENERICTRANSFORMER_H
#define GPLATES_UTILS_GENERICTRANSFORMER_H

#include "FilterMapOutputHandler.h"
#include "Mapper.h"

namespace GPlatesUtils
{
	/*
	* TODO: comments....
	*/
	template< 
			class InputIterator, 
			class OutputIterator, 
			class Implementation >
	class GenericMapper : 
		public Mapper< 
				InputIterator, 
				OutputIterator, 
				std::vector<typename OutputIterator::value_type> >
	{
	public:	
		/*
		* TODO: comments....
		*/
		explicit
		GenericMapper(
				Implementation impl):
			d_impl(impl)
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
			#ifdef _DEBUG
			qDebug() << "Enter Generic map operator()_1";
			#endif

			OutputIterator result_begin, result_end;
			result_end = result_begin = result;

			FilterMapOutputHandler<OutputIterator, OUTPUT_BY_ITERATOR> handler(result);
			int len = d_impl(input_begin, input_end, handler);
			
			std::advance(result_end, len);
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
			#ifdef _DEBUG
			qDebug() << "Enter Generic map operator()_2";
			#endif
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
			#ifdef _DEBUG
			qDebug() << "Enter Generic map operator()_3";
			#endif
			FilterMapOutputHandler<std::vector<typename OutputIterator::value_type>, OUTPUT_BY_CONTAINER> handler(result);
			d_impl(input_begin, input_end, handler);

			return boost::make_tuple(
					d_output_data.begin(), 
					d_output_data.end());
		}



		/*
		* TODO: comments....
		*/
		boost::tuple<
				OutputIterator, //result begin
				OutputIterator> //result end
		operator<< (
				boost::tuple<
						InputIterator,
						InputIterator> ) 
		{
			//TO BE IMPLEMENTED.
			OutputIterator it;
			return boost::make_tuple(it,it);
		}

	protected:
		std::vector<typename OutputIterator::value_type> d_output_data;
		GenericMapper();
		Implementation d_impl;
	};
}

#endif
