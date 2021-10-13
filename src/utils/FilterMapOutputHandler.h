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

#ifndef GPLATES_UTILS_FILTERMAPOUTPUTHANDLER_H
#define GPLATES_UTILS_FILTERMAPOUTPUTHANDLER_H

#include <QDebug>

#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/utility/enable_if.hpp>


namespace GPlatesUtils
{
	struct OUTPUT_BY_ITERATOR { };
	struct OUTPUT_BY_CONTAINER{ };

	namespace
	{
		template< typename OutputHandle, typename OutputMode >
		inline
		typename boost::enable_if<
				typename boost::is_same<
						OutputMode,
						OUTPUT_BY_ITERATOR>, void>::type
		_insert(
				OutputHandle &handle,
				const typename OutputHandle::value_type &value) 
		{
			*handle = value;
			handle++;
		}


		template< typename OutputHandle, typename OutputMode >
		inline
		typename boost::enable_if<
				typename boost::is_same<
						OutputMode,
						OUTPUT_BY_CONTAINER>, void>::type
		_insert(
				OutputHandle &handle,
				const typename OutputHandle::value_type &value) 
		{
			handle.push_back(value);
		}
	}


	/*
	* TODO: comments....
	*/
	template< typename OutputHandle, typename OutputMode >
	class FilterMapOutputHandler 
	{
	public:	

		/*
		* TODO: comments....
		*/
		explicit
		FilterMapOutputHandler(
				OutputHandle &output_handle):
			d_output_handle(output_handle)
		{ }


		/*
		* TODO: comments....
		*/
		inline
		void
		insert(
				const typename OutputHandle::value_type &value) 
		{
			_insert<OutputHandle, OutputMode>(d_output_handle,value);
		}

	protected:
		OutputHandle &d_output_handle;
		FilterMapOutputHandler();
	};
}

#endif //GPLATES_UTILS_FILTERMAPOUTPUTHANDLER_H

