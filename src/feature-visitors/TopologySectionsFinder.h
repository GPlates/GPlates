/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008, 2009 California Institute of Technology
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

#ifndef GPLATES_FEATUREVISITORS_TOPOLOGY_SECTIONS_FINDER_H
#define GPLATES_FEATUREVISITORS_TOPOLOGY_SECTIONS_FINDER_H

#include <vector>
#include <sstream>
#include <iostream>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/noncopyable.hpp>

#include <QTreeWidget>
#include <QLocale>
#include <QDebug>
#include <QList>
#include <QString>

#include "app-logic/Reconstruction.h"

#include "global/types.h"

#include "gui/TopologySectionsContainer.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"

#include "model/types.h"
#include "model/FeatureId.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"
#include "model/Model.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"
#include "model/WeakReference.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlTopologicalSection.h"

namespace GPlatesPropertyValues
{
	class GpmlKeyValueDictionaryElement;
	class GpmlTimeSample;
	class GpmlTimeWindow;
	class GpmlTopologicalSection;
}


namespace GPlatesFeatureVisitors
{
	class TopologySectionsFinder :
			public GPlatesModel::ConstFeatureVisitor,
			private boost::noncopyable
	{
	public:
		explicit
		TopologySectionsFinder();

		virtual
		~TopologySectionsFinder () 
		{  }

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);

		void
		process_gpml_time_window(
				const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window);

		virtual
		void
		visit_gpml_topological_polygon(
		 		const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon);

		virtual
		void
		visit_gpml_topological_line_section(
				const GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section);

		virtual
		void
		visit_gpml_topological_point(
				const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point);

		void
		report();

		GPlatesGui::TopologySectionsContainer::const_iterator
		found_rows_begin()
		{
			return d_table_rows.begin();
		}
			
		GPlatesGui::TopologySectionsContainer::const_iterator
		found_rows_end()
		{
			return d_table_rows.end();
		}

		int 
		number_of_rows()
		{
			return d_table_rows.size();
		}

	private:
		// Collection of TableRows built from this features Topology data
		GPlatesGui::TopologySectionsContainer::container_type d_table_rows;
	};
}

#endif  // GPLATES_FEATUREVISITORS_TOPOLOGY_SECTIONS_FINDER_H
