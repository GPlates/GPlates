/* $Id: NumInROIDataOperator.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file 
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

#ifndef GPLATESDATAMINING_NUMINROIDATAOPERATOR_H
#define GPLATESDATAMINING_NUMINROIDATAOPERATOR_H

#include "DataOperator.h"
#include "GetValueFromPropertyVisitor.h"

namespace GPlatesDataMining
{

	
	/*
	*Comments...
	*/
	
	class NumInROIDataOperator : 
		public DataOperator
	{
	public:
		
		friend class DataOperatorFactory;
		
		/*
		* Comments...
		*/
		virtual
		inline
		void
		get_data(
				const AssociationOperator::AssociatedCollection& input,		/*In*/
				const QString&		attr_name,	/*In*/
				DataRow&	data_row)	/*Out*/
		{			
			data_row.append_cell( 
					OpaqueData(static_cast<unsigned>(input.d_associated_features.size())));	
		}

	protected:

		DataOperatorParameters d_cfg;
		
		NumInROIDataOperator(
				DataOperatorParameters& cfg) :
			d_cfg(cfg)
		{ }
		NumInROIDataOperator()
		{ }

	};
}


#endif
