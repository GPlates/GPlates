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

#ifndef GPLATESDATAMINING_DATAOPERATOR_H
#define GPLATESDATAMINING_DATAOPERATOR_H

#include <sstream>
#include <vector>
#include <QDebug>

#include "AssociationOperator.h"
#include "DataTable.h"
#include "DataOperatorTypes.h"
#include "GetValueFromPropertyVisitor.h"

#include "feature-visitors/ShapefileAttributeFinder.h"

#include "model/TopLevelProperty.h"

namespace GPlatesDataMining
{
	/*
	*TODO
	*/
	struct DataOperatorParameters
	{
		DataOperatorParameters():
			d_is_shape_file_attr(false)
		{ }
		bool d_is_shape_file_attr;
	};

	class DataOperator
	{
	public:
		typedef  std::map< QString, DataOperatorType > DataOperatorNameMap;

		static DataOperatorNameMap d_data_operator_name_map;
		
		virtual
		~DataOperator(){;}
		
		virtual
		void
		get_data(
				const AssociationOperator::AssociatedCollection& input,
				const QString& attr_name,
				DataRow& data_row) = 0 ;
	protected:
		
		/*
		* Comments
		*/
		boost::optional<
				GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type
						>
		get_property_by_name(
				GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
				QString name);
		
		/*
		* Comments
		*/
		template < class DataType >
		void
		get_value(
				GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type property,
				std::vector< DataType >& data)
		{
			GetValueFromPropertyVisitor< DataType > visitor;

			property->accept_visitor(visitor);
			data = visitor.get_data();
		}

		/*
		* Comments
		* temporary hacking code for shapefileattribute.
		*/
		void
		get_value(
				GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type property,
				std::vector< QVariant >& data,
				QString shape_attr_name)
		{
			GPlatesFeatureVisitors::ShapefileAttributeFinder visitor(shape_attr_name);

			property->accept_visitor(visitor);
			data.insert(
					data.end(),
					visitor.found_qvariants_begin(),
					visitor.found_qvariants_end());
		}

		/*
		* Comments
		*/
		void
		get_closest_features(
				const AssociationOperator::AssociatedCollection&,
				std::vector< GPlatesModel::FeatureHandle::const_weak_ref >&);

		/*
		* Comments
		*/
		boost::optional< GPlatesModel::FeatureHandle::const_weak_ref > 
		get_closest_feature(
				const AssociationOperator::AssociatedCollection&);

		/*
		* Comments
		*/
		boost::optional< GPlatesModel::FeatureHandle::weak_ref >
		get_closest_feature(
				AssociationOperator::AssociatedCollection&);


		DataOperator(){}
	};

	
}
#endif


