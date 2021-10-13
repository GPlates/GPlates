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
#ifndef GPLATESDATAMINING_RFGTOPROPERTYVALUEMAPPER_H
#define GPLATESDATAMINING_RFGTOPROPERTYVALUEMAPPER_H

#include <vector>
#include <QDebug>

#include <boost/foreach.hpp>

#include "DataTable.h"
#include "GetValueFromPropertyVisitor.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "utils/FilterMapOutputHandler.h"

namespace
{
	OpaqueData
	get_property_value_by_name(
			GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
			QString name)
	{
		using namespace GPlatesModel;
		FeatureHandle::const_iterator it = feature_ref->begin();
		FeatureHandle::const_iterator it_end = feature_ref->end();
		for(; it != it_end; it++)
		{
			if((*it)->property_name().get_name() == GPlatesUtils::make_icu_string_from_qstring(name))
			{
				GetValueFromPropertyVisitor<OpaqueData> visitor;
				(*it)->accept_visitor(visitor);
				std::vector<OpaqueData>& data_vec = visitor.get_data(); 
				if(!data_vec.empty())
				{
					return data_vec.at(0);
				}
			}
		}
		return boost::none;
	}


	OpaqueData
	convert_qvariant_to_Opaque_data(
			const QVariant& data)
	{
		switch (data.type())
		{
		case QVariant::Bool:
			return OpaqueData(data.toBool());
			break;

		case QVariant::Int:
			return OpaqueData(data.toInt());
			break;
			
		case QVariant::Double:
			return OpaqueData(data.toDouble());
			break;
	
		case QVariant::String:
			return OpaqueData(data.toString());
			break;
		default:
			return EmptyData;
		}
	}

	
	OpaqueData
	get_shape_file_value_by_name(
			GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
			QString name)
	{
		using namespace GPlatesModel;
		FeatureHandle::const_iterator it = feature_ref->begin();
		FeatureHandle::const_iterator it_end = feature_ref->end();
		for(; it != it_end; it++)
		{
			if((*it)->property_name().get_name() == "shapefileAttributes")
			{
				GPlatesFeatureVisitors::ShapefileAttributeFinder visitor(name);
				(*it)->accept_visitor(visitor);
				if(1 != std::distance(visitor.found_qvariants_begin(),visitor.found_qvariants_end()))
				{
					qDebug() << "More than one property found in shape file attribute.";
					qDebug() << "But this is a one to one mapping. So, only return the first value.";
					qDebug() << "More than one shape file attributes have the same name. Please check you data.";
				}
				return convert_qvariant_to_Opaque_data(*visitor.found_qvariants_begin());
			}
		}
		return EmptyData;
	}
}


namespace GPlatesDataMining
{
	using namespace GPlatesUtils;
	
	typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> InputSequence;
	typedef std::vector<OpaqueData> OutputSequence;

	/*	
	*	TODO:
	*	Comments....
	*/
	class RFGToPropertyValueMapper
	{
	public:

		explicit
		RFGToPropertyValueMapper(
				const QString& attr_name,
				bool is_shapefile_attr = false):
			d_attr_name(attr_name),
			d_is_shapefile_attr(is_shapefile_attr)
			{	}

		/*
		* TODO: comments....
		*/
		template< class OutputType, class OutputMode>
		int
		operator()(
				InputSequence::const_iterator input_begin,
				InputSequence::const_iterator input_end,
				FilterMapOutputHandler<OutputType, OutputMode> &handler)
		{
			int count = 0;
			for(; input_begin != input_end; input_begin++)
			{
				if(!d_is_shapefile_attr)
				{
					handler.insert(
						get_property_value_by_name((*input_begin)->get_feature_ref(), d_attr_name));
				}
				else
				{
					handler.insert(
						get_shape_file_value_by_name((*input_begin)->get_feature_ref(), d_attr_name));
				}
				count++;
			}
			return count;
		}

		virtual
		~RFGToPropertyValueMapper(){}			


	protected:
		
		const QString d_attr_name;
		bool d_is_shapefile_attr;
	};
}
#endif //GPLATESDATAMINING_RFGTOPROPERTYVALUEMAPPER_H





