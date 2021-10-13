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

#ifndef GPLATESDATAMINING_LOOKUPDATAOPERATOR_H
#define GPLATESDATAMINING_LOOKUPDATAOPERATOR_H

#include "sstream"

#include "AssociationOperator.h"
#include "DataOperator.h"
#include "GetValueFromPropertyVisitor.h"

#include "model/TopLevelProperty.h"

namespace GPlatesDataMining
{
	/*
	*Comments...
	*/
	class LookUpDataOperator : 
		public DataOperator
	{
	public:
		
		friend class DataOperatorFactory;
		
		virtual
		~LookUpDataOperator(){;}
		
		virtual
		void
		get_data(
				const AssociationOperator::AssociatedCollection& input,
				const QString& attr_name,
				DataRow& data_row);
			
	protected:	
		boost::optional< QString >
		get_qstring_from_feature(
				const AssociationOperator::AssociatedCollection& input,
				const QString&	attr_name);

		boost::optional< QString >
		get_qstring_from_shape_attr(
				const AssociationOperator::AssociatedCollection& input,
				const QString& attr_name);

		DataOperatorParameters d_cfg;
		
		LookUpDataOperator(
				DataOperatorParameters& cfg) :
			d_cfg(cfg)
		{ }
		LookUpDataOperator()
		{ }

	};
}

#endif


