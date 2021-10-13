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

#ifndef GPLATES_UTILS_GENERICREDUCERIMPL_H
#define GPLATES_UTILS_GENERICREDUCERIMPL_H

namespace GPlatesUtils
{
	/*
	* TODO: comments....
	*/
	template< class InputIterator, class OutputDataType>
	class GenericReducerImpl
	{
	public:
		/*
		* TODO: comments....
		*/
		virtual
		OutputDataType
		operator()(
				InputIterator input_begin,
				InputIterator input_end) = 0;

		virtual
		~GenericReducerImpl()
		{ }
					
	};
}

#endif

