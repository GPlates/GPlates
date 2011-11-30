/* $Id: FeatureCollection.cc 11961 2011-07-07 03:49:38Z mchin $ */

/**
 * \file 
 * $Revision: 11961 $
 * $Date: 2011-07-07 13:49:38 +1000 (Thu, 07 Jul 2011) $
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
#ifndef GPLATES_API_FEATURE_H
#define GPLATES_API_FEATURE_H

#include "global/python.h"
#include "model/FeatureHandle.h"
#include "utils/FeatureUtils.h"
#include "data-mining/DataMiningUtils.h"
#include "data-mining/OpaqueDataToDouble.h"
#include "data-mining/OpaqueDataToQString.h"

#if !defined(GPLATES_NO_PYTHON)

namespace bp=boost::python;

namespace GPlatesApi
{
	class Feature
	{
	public:
		Feature(){ }

		Feature(GPlatesModel::FeatureHandle::weak_ref w_ref) :
			d_handle(w_ref)
		{ }

		
		bp::object
		id()
		{
			if(!d_handle.is_valid())
				return bp::object();

			const QByteArray buf = d_handle->feature_id().get().qstring().toUtf8();
			return bp::str(buf.data());
		}

		
		bp::tuple
		valid_time()
		{
			if(!d_handle.is_valid())
				return bp::tuple();

			GPlatesMaths::Real start, end;
			boost::tie(start, end) = GPlatesUtils::get_start_end_time(d_handle.handle_ptr());
			return bp::make_tuple(start.dval(),end.dval());
		}

		
		operator GPlatesModel::FeatureHandle::weak_ref()
		{
			return d_handle;
		}

		
		bp::object
		type()
		{
			if(!d_handle.is_valid())
				return bp::object();

			const QByteArray buf = d_handle->feature_type().get_name().qstring().toUtf8();
			return bp::str(buf.data());
		}

		
		unsigned long
		plate_id()
		{
			if(!d_handle.is_valid())
				return 0;

			boost::optional<unsigned long> pid =
				GPlatesUtils::get_int_plate_id(d_handle.handle_ptr());
			return pid ? *pid : 0;
		}

		
		bp::object
		get_property(bp::object name_)
		{
			using namespace GPlatesDataMining;
			QString name = QString::fromUtf8(bp::extract<const char*>(name_));
			OpaqueData data = DataMiningUtils::get_property_value_by_name(d_handle, name);
			
			if(is_empty_opaque(data))
			{
				data = DataMiningUtils::get_shape_file_value_by_name(d_handle, name);

				if(is_empty_opaque(data))
					return bp::object();
			}
			
			if(boost::optional<double> int_tmp = 
				boost::apply_visitor(ConvertOpaqueDataToDouble(), data))
			{
				return bp::object(*int_tmp);
			}

			if(boost::optional<QString> str_tmp = 
				boost::apply_visitor(ConvertOpaqueDataToString(), data))
			{
				const QByteArray buf = str_tmp->toUtf8();
				return bp::str(buf.data());
			}
			
			return bp::object();
		}

	private:
		GPlatesModel::FeatureHandle::weak_ref d_handle;
	};
}
#endif   //GPLATES_NO_PYTHON
#endif  // GPLATES_API_FEATURE_H

