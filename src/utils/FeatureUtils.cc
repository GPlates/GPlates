/* $Id: XmlNamespaces.cc 10554 2010-12-13 05:57:08Z mchin $ */

/**
 * \file
 *
 * Most recent change:
 *   $Date: 2010-12-13 16:57:08 +1100 (Mon, 13 Dec 2010) $
 *
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "FeatureUtils.h"

#include "model/FeatureVisitor.h"
#include "model/FeatureHandle.h"
#include "model/QualifiedXmlName.h"

#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimePeriod.h"

using namespace GPlatesPropertyValues;
using namespace GPlatesModel;

class PropertyFinder:
		public ConstFeatureVisitor
{
public:
	boost::optional<GPlatesModel::integer_plate_id_type>
	int_plate_id() const
	{
		return d_plate_id;
	}

	boost::optional<GPlatesPropertyValues::GeoTimeInstant>
	start_time()
	{
		return d_start_time;
	
	}

	boost::optional<GPlatesPropertyValues::GeoTimeInstant>
	end_time()
	{
		return d_end_time;
	}

	void
	visit_gpml_plate_id(
			ConstFeatureVisitor::gpml_plate_id_type& id) 
	{
		d_plate_id = id.get_value();
	}

	void
	visit_gpml_constant_value(
			ConstFeatureVisitor::gpml_constant_value_type &v)
	{
		v.get_value()->accept_visitor(*this);
	}

	void
	visit_gml_time_period(
			ConstFeatureVisitor::gml_time_period_type &gml_time_period)
	{
		d_start_time = gml_time_period.begin()->get_time_position();
		d_end_time   = gml_time_period.end()->get_time_position();
	}

private:
	boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;
	boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_start_time;
	boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_end_time;
};


boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesUtils::get_recon_plate_id_as_int(
		const GPlatesModel::FeatureHandle *feature_ptr)
{
	using namespace GPlatesModel;
	boost::optional<GPlatesModel::integer_plate_id_type> id = boost::none;
	if(feature_ptr)
	{
		static const PropertyName recon_pid_name = PropertyName::create_gpml("reconstructionPlateId");
		PropertyFinder finder;
		FeatureHandle::const_iterator iter = feature_ptr->begin(), end = feature_ptr->end();
		for ( ; iter != end; ++iter)
		{
			if((*iter)->get_property_name() == recon_pid_name)
			{
				(*iter)->accept_visitor(finder);
				id = finder.int_plate_id();
				break;
			}
		}
	}
	return id;
}


boost::optional<GPlatesMaths::Real>
GPlatesUtils::get_age(
		const GPlatesModel::FeatureHandle* feature_ptr,
		const GPlatesMaths::Real current_time)
{
	PropertyFinder finder;
	boost::optional<GPlatesPropertyValues::GeoTimeInstant> time = boost::none;
	GPlatesModel::FeatureHandle::const_iterator 
		iter = feature_ptr->begin(),
		end = feature_ptr->end();
	
	for ( ; iter != end; ++iter)
	{
		(*iter)->accept_visitor(finder);
		time = finder.start_time();
		if(time)
			break;
	}
	if(time)
	{
		if (time->is_distant_past())
		{
			return GPlatesMaths::Real::positive_infinity();
		}
		else if (time->is_distant_future())
		{
			return GPlatesMaths::Real::negative_infinity();
		}
		else
		{
			return GPlatesMaths::Real(time->value() - current_time.dval());
		}
	}
	return boost::none;
}

namespace
{
	GPlatesMaths::Real
	to_real(const GPlatesPropertyValues::GeoTimeInstant& time)
	{
		if (time.is_distant_past())
		{
			return GPlatesMaths::Real::positive_infinity();
		}
		else if (time.is_distant_future())
		{
			return GPlatesMaths::Real::negative_infinity();
		}
		else
		{
			return GPlatesMaths::Real(time.value());
		}
	}
}

boost::tuple<
		GPlatesMaths::Real, 
		GPlatesMaths::Real>
GPlatesUtils::get_start_end_time(
		const GPlatesModel::FeatureHandle* feature_ptr)
{
	PropertyFinder finder;
	boost::optional<GPlatesPropertyValues::GeoTimeInstant> 
			s_time = boost::none, e_time = boost::none;
	GPlatesModel::FeatureHandle::const_iterator 
		iter = feature_ptr->begin(),
		end = feature_ptr->end();
	
	for ( ; iter != end; ++iter)
	{
		(*iter)->accept_visitor(finder);
		s_time = finder.start_time();
		e_time = finder.end_time();
		if(s_time || e_time)
			break;
	}
	GPlatesMaths::Real st =  0.0, et = 0.0;
	if(s_time)
	{
		st = to_real(*s_time);
	}
	if(e_time)
	{
		et = to_real(*e_time);
	}
	return boost::make_tuple(st,et);
}


boost::optional<GPlatesMaths::Real>
GPlatesUtils::get_begin_time(
		const GPlatesModel::FeatureHandle* feature_ptr)
{
	PropertyFinder finder;
	
	boost::optional<GPlatesPropertyValues::GeoTimeInstant> s_time = GPlatesPropertyValues::GeoTimeInstant(0.0);
	s_time = boost::none; // This strange initialization is a workaround for Ubuntu Precise 32-bit. The g++ doesn't like boost::optional.
	
	GPlatesModel::FeatureHandle::const_iterator 
		iter = feature_ptr->begin(),
		end = feature_ptr->end();

	for ( ; iter != end; ++iter)
	{
		(*iter)->accept_visitor(finder);
		s_time = finder.start_time();
		if(s_time)
			break;
	}
	if(s_time)
	{
		return to_real(*s_time);
	}
	else
	{
		return boost::none;
	}
}


boost::optional<GPlatesModel::PropertyName>
GPlatesUtils::convert_property_name(
		const QString& name)
{
	QRegExp rx("^\\s*\\b(gpml|gml)\\b\\s*:\\s*\\b(\\w+)\\b\\s*"); // (gpml|gmp):(name)
	rx.indexIn(name);
	QString prefix = rx.cap(1);
	QString short_name = rx.cap(2);
	qDebug() << "prefix: " << prefix; 
	qDebug() << "name: " << short_name; 
	
	boost::optional<GPlatesModel::PropertyName> ret = boost::none;
	if(prefix.length() == 0 || short_name.length() == 0)
	{
		qWarning() << "The prefix or name is empty.";
		qWarning() << prefix << " : " << short_name;
	}

	if(prefix == "gpml")
	{
		ret = GPlatesModel::PropertyName::create_gpml(short_name);
	}

	if(prefix == "gml")
	{
		ret = GPlatesModel::PropertyName::create_gml(short_name);
	}

	return ret; 
}



























