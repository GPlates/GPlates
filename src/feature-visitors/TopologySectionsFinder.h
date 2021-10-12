/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008, 2009 California Institute of Technology
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

#include <QTreeWidget>
#include <QLocale>
#include <QDebug>
#include <QList>
#include <QString>

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
#include "model/ReconstructedFeatureGeometry.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"
#include "model/Reconstruction.h"
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
	class TopologySectionsFinder : public GPlatesModel::FeatureVisitor
	{
	public:
		explicit
		TopologySectionsFinder(
		
			std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &sections_ptrs,
			std::vector<GPlatesModel::FeatureId> &section_ids,
			std::vector< std::pair<double, double> > &click_points,
			std::vector<bool> &reverse_flags);

		virtual
		~TopologySectionsFinder () 
		{  }

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_gpml_constant_value(
			GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_piecewise_aggregation(
			GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);

		void
		process_gpml_time_window(
			GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window);

		virtual
		void
		visit_gpml_topological_polygon(
		 	GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon);

		virtual
		void
		visit_gpml_topological_line_section(
			GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section);

		virtual
		void
		visit_gpml_topological_intersection(
			GPlatesPropertyValues::GpmlTopologicalIntersection &gpml_toplogical_intersection);

		virtual
		void
		visit_gpml_topological_point(
			GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point);

		void
		report();

		std::vector<GPlatesGui::TopologySectionsContainer::TableRow>::iterator
		found_rows_begin()
		{
			return d_table_rows.begin();
		}
			
		std::vector<GPlatesGui::TopologySectionsContainer::TableRow>::iterator
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

		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
			*d_section_ptrs;

		std::vector<GPlatesModel::FeatureId> *d_section_ids;

		std::vector< std::pair<double, double> > *d_click_points;

		std::vector<bool> *d_reverse_flags;

		TopologySectionsFinder(
				const TopologySectionsFinder &);

		TopologySectionsFinder &
		operator=(
			const TopologySectionsFinder &);

		// working row ; populated by visit_* calls
		GPlatesGui::TopologySectionsContainer::TableRow d_table_row;

		// Collection of TableRows built from this features Topology data
		std::vector<GPlatesGui::TopologySectionsContainer::TableRow> d_table_rows;
	};
}

#endif  // GPLATES_FEATUREVISITORS_TOPOLOGY_SECTIONS_FINDER_H
