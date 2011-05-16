/* $Id: LookupDataOperator.cc 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file .
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

#include "LookupDataOperator.h"

using namespace GPlatesDataMining;

void
LookUpDataOperator::get_data(
		const AssociationOperator::AssociatedCollection& input,
		const QString& attr_name,
		DataRow& data_row)
{
	boost::optional< QString > str = boost::none;
	if(d_cfg.d_is_shape_file_attr)
	{
		str = 
			get_qstring_from_shape_attr(
					input,
					attr_name);
	}
	else
	{
		str = 
			get_qstring_from_feature(
					input,
					attr_name);
	}
	if(str)
	{
		data_row.append_cell( OpaqueData(*str) );
	}
	else
	{
		data_row.append_cell( OpaqueData(EmptyData) );
	}
}

boost::optional< QString >
LookUpDataOperator::get_qstring_from_feature(
		const AssociationOperator::AssociatedCollection& input,
		const QString&	attr_name)
{
	boost::optional< GPlatesModel::FeatureHandle::const_weak_ref > feature_ref = 
			get_closest_feature(input);
	if(!feature_ref)
	{
		return boost::none;
	}

	boost::optional< GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type > 
			property = get_property_by_name( *feature_ref, attr_name);

	std::vector < QString > tmp;
	if(property)
	{
		get_value(*property, tmp);
	}
	
	if(tmp.size() < 1)
	{
		qWarning() << "No Value found in lookup operator.";
		return boost::none;
	}

	if(tmp.size() > 1)
	{
		qWarning() << tmp.size() << " strings have been found, only return the first one for lookup operator";
	}
	return boost::optional< QString >( tmp.at(0) );
}

/*temporary hacking function for shapefileattribute.*/
boost::optional< QString >
LookUpDataOperator::get_qstring_from_shape_attr(
		const AssociationOperator::AssociatedCollection& input,
		const QString& attr_name)
{
	boost::optional< GPlatesModel::FeatureHandle::const_weak_ref > feature_ref = 
			get_closest_feature(input);
	if(!feature_ref)
	{
		return boost::none;
	}

	std::ostringstream str_buf;
	boost::optional< QString > str_opt = boost::none;
	QString str;
	
	boost::optional< GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type > 
			property = get_property_by_name( *feature_ref, "shapefileAttributes");
	
	std::vector< QVariant > tmp_variant;
	if(property)
	{
		get_value(*property, tmp_variant, attr_name);
	}
	else			
	{
		qWarning() << "No shapefileAttributes has been found.";
		return boost::none;
	}

	if(tmp_variant.size() < 1)
	{
		qWarning() << "No Value found in lookup operator.";
		return boost::none;
	}

	if(tmp_variant.size() > 1)
	{
		qWarning() << tmp_variant.size() << " values have been found, only return the first one for lookup operator";
	}
	return boost::optional< QString >( tmp_variant.at(0).toString() );
}



