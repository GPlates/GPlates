/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include "FeatureTableModel.h"

#include <utility>
#include <vector>
#include <QApplication>
#include <QHeaderView>
#include <QString>
#include <QLocale>
#include <QDateTime>
#include <QFont>
#include <QDebug>
#include <boost/none.hpp>

#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructGraph.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/GeometryFinder.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/types.h"
#include "model/FeatureHandle.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/XsString.h"

#include "utils/Profile.h"
#include "utils/QtFormattingUtils.h"
#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryUtils.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))


namespace
{
	typedef const QVariant (*table_cell_accessor_type)(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry);
	
	struct ColumnHeadingInfo
	{
		const char *label;
		const char *tooltip;
		int width;
		QHeaderView::ResizeMode resize_mode;
		table_cell_accessor_type accessor;
		QFlags<Qt::AlignmentFlag> alignment;
	};


	// Accessor functions for table cells:
	
	const QVariant
	null_table_accessor(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		return QVariant();
	}


	const boost::optional<GPlatesModel::FeatureHandle::weak_ref>
	get_feature_weak_ref_if_valid(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		// See if reconstruction geometry references a feature.
		return GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(geometry);
	}


	const QVariant
	get_feature_type(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			return QVariant(convert_qualified_xml_name_to_qstring((*weak_ref)->feature_type()));
		}
		return QVariant();
	}

	
	const QVariant
	get_reconstruction_plate_id_from_properties(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			bool should_print_debugging_message = false)
	{
		static const GPlatesModel::PropertyName plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;
		if (GPlatesFeatureVisitors::get_property_value(
				feature, plate_id_property_name, recon_plate_id))
		{
			// The feature has a reconstruction plate ID.
			if (should_print_debugging_message) {
				std::cerr << "Debug log: No cached reconstruction plate ID in RFG,\n"
						<< "but reconstruction plate ID found in feature." << std::endl;
			}
			return QVariant(static_cast<quint32>(recon_plate_id->get_value()));
		} else {
			// The feature doesn't have a reconstruction plate ID.
			return QVariant();
		}
	}


	const QVariant
	get_plate_id(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref)
		{
			// See if type derived from ReconstructionGeometry has a plate id.
			boost::optional<GPlatesModel::integer_plate_id_type> found_plate_id =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id(geometry);
			if (found_plate_id) {
				return QVariant(static_cast<quint32>(*found_plate_id));
			}
			else
			{
				// Otherwise, there wasn't a reconstruction plate ID.  Let's find
				// the reconstruction plate ID the hard way -- by iterating through
				// all the properties of the referenced feature.
				return get_reconstruction_plate_id_from_properties(*weak_ref, true);
			}
		}
		return QVariant();
	}


	const QString
	format_time_instant(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant)
	{
		QLocale locale;
		if (time_instant.get_time_position().is_real()) {
			return locale.toString(time_instant.get_time_position().value());
		} else if (time_instant.get_time_position().is_distant_past()) {
			return QObject::tr("past");
		} else if (time_instant.get_time_position().is_distant_future()) {
			return QObject::tr("future");
		} else {
			return QObject::tr("<invalid>");
		}
	}


	const QString
	format_time_period(
			const GPlatesPropertyValues::GmlTimePeriod &time_period)
	{
		return QObject::tr("%1 - %2")
				.arg(format_time_instant(*(time_period.get_begin())))
				.arg(format_time_instant(*(time_period.get_end())));
	}


	const QVariant
	get_time_begin(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
				GPlatesModel::PropertyName::create_gml("validTime");

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			const GPlatesPropertyValues::GmlTimePeriod *time_period;
			if (GPlatesFeatureVisitors::get_property_value(
					*weak_ref, valid_time_property_name, time_period))
			{
				// The feature has a gml:validTime property.
				// FIXME: This could be from a gpml:TimeVariantFeature, OR a gpml:InstantaneousFeature,
				// in the latter case it has a slightly different meaning and we should be displaying the
				// gpml:reconstructedTime property instead.
				return format_time_instant(*(time_period->get_begin()));
			}
		}
		return QVariant();
	}


	const QVariant
	get_time_end(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
				GPlatesModel::PropertyName::create_gml("validTime");

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			const GPlatesPropertyValues::GmlTimePeriod *time_period;
			if (GPlatesFeatureVisitors::get_property_value(
					*weak_ref, valid_time_property_name, time_period))
			{
				// The feature has a gml:validTime property.
				// FIXME: This could be from a gpml:TimeVariantFeature, OR a gpml:InstantaneousFeature,
				// in the latter case it has a slightly different meaning and we should be displaying the
				// gpml:reconstructedTime property instead.
				return format_time_instant(*(time_period->get_end()));
			}
		}
		return QVariant();
	}


	const QVariant
	get_name(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		// FIXME: Need to adapt according to user's current codeSpace setting.
		static const GPlatesModel::PropertyName name_property_name = 
				GPlatesModel::PropertyName::create_gml("name");

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			const GPlatesPropertyValues::XsString *name;
			if (GPlatesFeatureVisitors::get_property_value(
					*weak_ref, name_property_name, name))
			{
				// The feature has one or more name properties.  Use the first one
				// for now.
				return GPlatesUtils::make_qstring(name->get_value());
			}
		}
		return QVariant();
	}


	const QVariant
	get_description(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		static const GPlatesModel::PropertyName description_property_name = 
				GPlatesModel::PropertyName::create_gml("description");

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			const GPlatesPropertyValues::XsString *description;
			if (GPlatesFeatureVisitors::get_property_value(
					*weak_ref, description_property_name, description))
			{
				// The feature has one or more description properties.  Use the
				// first one for now.
				return GPlatesUtils::make_qstring(description->get_value());
			}
		}
		return QVariant();
	}


	const QString
	format_point_or_vertex(
			const GPlatesMaths::PointOnSphere &point_or_vertex)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point_or_vertex);
		QLocale locale;
		QString str = QObject::tr("(%1 ; %2)")
				.arg(locale.toString(llp.latitude()))
				.arg(locale.toString(llp.longitude()));
		return str;
	}


	const QString
	format_geometry_point(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point)
	{
		return QObject::tr("point: %1")
				.arg(format_point_or_vertex(*point));
	}


	const QString
	format_geometry_multi_point(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point)
	{
		QString begin_str = format_point_or_vertex(multi_point->start_point());
		QString end_str = format_point_or_vertex(multi_point->end_point());
		QString middle_str;
		if (multi_point->number_of_points() == 3) {
			middle_str = QObject::tr("... 1 more vertex ... ");
		} else if (multi_point->number_of_points() > 3) {
			middle_str = QObject::tr("... %1 more vertices ... ")
					.arg(multi_point->number_of_points() - 2);
		}
		return QObject::tr("multi-point: %1 %3%2")
				.arg(begin_str)
				.arg(end_str)
				.arg(middle_str);
	}


	const QString
	format_geometry_polygon(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon)
	{
		QString begin_str = format_point_or_vertex(polygon->first_vertex());
		QString end_str = format_point_or_vertex(polygon->last_vertex());
		QString middle_str;
		if (polygon->number_of_vertices() == 3) {
			middle_str = QObject::tr("... 1 more vertex ... ");
		} else if (polygon->number_of_vertices() > 3) {
			middle_str = QObject::tr("... %1 more vertices ... ")
					.arg(polygon->number_of_vertices() - 2);
		}
		return QObject::tr("polygon: %1 %3%2")
				.arg(begin_str)
				.arg(end_str)
				.arg(middle_str);
	}


	const QString
	format_geometry_polyline(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline)
	{
		QString begin_str = format_point_or_vertex(polyline->start_point());
		QString end_str = format_point_or_vertex(polyline->end_point());
		QString middle_str;
		if (polyline->number_of_vertices() == 3) {
			middle_str = QObject::tr("... 1 more vertex ... ");
		} else if (polyline->number_of_vertices() > 3) {
			middle_str = QObject::tr("... %1 more vertices ... ")
					.arg(polyline->number_of_vertices() - 2);
		}
		return QObject::tr("polyline: %1 %3%2")
				.arg(begin_str)
				.arg(end_str)
				.arg(middle_str);
	}


	/**
	 * This is a Visitor to obtain a summary of the geometry-on-sphere as a string.
	 * 
	 * FIXME: This would be great in a separate file. The Query and Edit Feature
	 * dialogs could make use of it.
	 */
	class GeometryOnSphereSummaryAsStringVisitor:
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GeometryOnSphereSummaryAsStringVisitor()
		{  }

		~GeometryOnSphereSummaryAsStringVisitor()
		{  }

		const QString &
		geometry_summary() const
		{
			return d_string;
		}

		// Please keep these geometries ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_string = format_geometry_multi_point(multi_point_on_sphere);
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_string = format_geometry_point(point_on_sphere);
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_string = format_geometry_polygon(polygon_on_sphere);
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_string = format_geometry_polyline(polyline_on_sphere);
		}

	private:
		QString d_string;
	};


	template <typename ReconstructionGeometryPointer>
	const boost::optional<GPlatesModel::FeatureHandle::iterator>
	get_geometry_property_if_valid(
			ReconstructionGeometryPointer geometry)
	{
		// See if type derived from ReconstructionGeometry has a valid geometry property.
		return GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(geometry);
	}


	const QVariant
	get_present_day_geometry(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::iterator> property =
				get_geometry_property_if_valid(geometry);
		if (property) {
			GPlatesFeatureVisitors::GeometryFinder geometry_finder;
			(**property)->accept_visitor(geometry_finder);
			if (geometry_finder.has_found_geometries()) {
				GeometryOnSphereSummaryAsStringVisitor geometry_visitor;
				(*geometry_finder.found_geometries_begin())->accept_visitor(geometry_visitor);
				return QVariant(geometry_visitor.geometry_summary());
			}
		}
		return QVariant();
	}


	const QVariant
	get_clicked_geometry_property(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::iterator> property =
				get_geometry_property_if_valid(geometry);
		if (property) {
			return QVariant(convert_qualified_xml_name_to_qstring((**property)->get_property_name()));
		}
		return QVariant();
	}


	const QVariant
	get_creation_time(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref_opt =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref_opt) {
			// Convert raw time_t into a more useful QDateTime.
			QDateTime created = QDateTime::fromTime_t((*weak_ref_opt)->creation_time());
			return GPlatesUtils::qdatetime_to_elapsed_duration(created);
		}
		return QVariant();
	}
			


	// The dispatch table for the above functions:
	
	static const ColumnHeadingInfo column_heading_info_table[] = {
		{ QT_TR_NOOP("Feature type"), QT_TR_NOOP("The type of this feature"),
				140, QHeaderView::ResizeToContents,
				get_feature_type, Qt::AlignLeft | Qt::AlignVCenter },

		{ QT_TR_NOOP("Plate ID"), QT_TR_NOOP("The plate ID used to reconstruct this feature"),
				60, QHeaderView::ResizeToContents, // Note: used to be Fixed.
				get_plate_id, Qt::AlignCenter },

		{ QT_TR_NOOP("Name"), QT_TR_NOOP("A convenient label for this feature"),
				140, QHeaderView::ResizeToContents,
				get_name, Qt::AlignLeft | Qt::AlignVCenter },

		{ QT_TR_NOOP("Clicked geometry"), QT_TR_NOOP("The geometry which was clicked"),
				140, QHeaderView::ResizeToContents,
				get_clicked_geometry_property, Qt::AlignLeft | Qt::AlignVCenter },

		{ QT_TR_NOOP("Begin"), QT_TR_NOOP("The time of appearance (Ma)"),
				60, QHeaderView::ResizeToContents, // Note: used to be Fixed.
				get_time_begin, Qt::AlignCenter },

		{ QT_TR_NOOP("End"), QT_TR_NOOP("The time of disappearance (Ma)"),
				60, QHeaderView::ResizeToContents, // Note: used to be Fixed.
				get_time_end, Qt::AlignCenter }, 

		{ QT_TR_NOOP("Created"), QT_TR_NOOP("How long ago the feature data was created (or loaded into GPlates)"),
				140, QHeaderView::ResizeToContents,
				get_creation_time, Qt::AlignCenter },

		{ QT_TR_NOOP("Present-day geometry (lat ; lon)"), QT_TR_NOOP("A summary of the present-day coordinates"),
				240, QHeaderView::ResizeToContents,
				get_present_day_geometry, Qt::AlignCenter },
	};


	const QString
	get_column_heading(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return QObject::tr("");
		}
		return QObject::tr(column_heading_info_table[column].label);
	}

	const QString
	get_column_tooltip(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return QObject::tr("");
		}
		return QObject::tr(column_heading_info_table[column].tooltip);
	}

	int
	get_column_width(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return 0;
		}
		return column_heading_info_table[column].width;
	}

	table_cell_accessor_type
	get_column_accessor(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return null_table_accessor;
		}
		return column_heading_info_table[column].accessor;
	}

	QFlags<Qt::AlignmentFlag>
	get_column_alignment(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return Qt::AlignLeft | Qt::AlignVCenter;
		}
		return column_heading_info_table[column].alignment;
	}
}


GPlatesGui::FeatureTableModel::ReconstructionGeometryRow::ReconstructionGeometryRow(
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry_,
		const GPlatesAppLogic::ReconstructGraph &reconstruct_graph_) :
	reconstruction_geometry(reconstruction_geometry_)
{
}


GPlatesGui::FeatureTableModel::FeatureTableModel(
		FeatureFocus &feature_focus,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		QObject *parent_):
	QAbstractTableModel(parent_),
	d_feature_focus_ptr(&feature_focus),
	d_rendered_geometry_collection(rendered_geometry_collection)
{
	// Get notified whenever the rendered geometry collection gets updated.
	QObject::connect(
			&d_rendered_geometry_collection,
			SIGNAL(collection_was_updated(
					GPlatesViewOperations::RenderedGeometryCollection &,
					GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
			this,
			SLOT(handle_rendered_geometry_collection_update()));

	QObject::connect(d_feature_focus_ptr,
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(handle_feature_modified(GPlatesGui::FeatureFocus &)));
}



int
GPlatesGui::FeatureTableModel::rowCount(
		const QModelIndex &parent_) const
{
	return static_cast<int>(d_sequence.size());
}

int
GPlatesGui::FeatureTableModel::columnCount(
		const QModelIndex &parent_) const
{
	return NUM_ELEMS(column_heading_info_table);
}

Qt::ItemFlags
GPlatesGui::FeatureTableModel::flags(
		const QModelIndex &idx) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
GPlatesGui::FeatureTableModel::headerData(
		int section,
		Qt::Orientation orientation,
		int role) const
{
	// The new way we are attempting to return an appropriate vertical
	// and horizontal size in the Qt::SizeHintRole for the header!
	
	// We are only interested in modifying the horizontal header.
	if (orientation == Qt::Horizontal) {
		// No need to supply tooltip data, etc. for headers. We are only interested in a
		// few roles.

		if (role == Qt::DisplayRole) {
			return get_column_heading(section);

		} else if (role == Qt::ToolTipRole) {
			return get_column_tooltip(section);
			
#if 0
		// Commenting this out because the logic is chopping off the descenders of
		// letters (like the bottom part of 'g').

		} else if (role == Qt::SizeHintRole) {
			// Annoyingly, the metrics alone do not appear to be sufficient to supply
			// the height of the header, so a few pixels are added.
			//
			// Furthermore, for some reason, the height is now being set correctly but
			// the width does not updated until a call to
			// QTableView::resizeColumnsToContents();
			return QSize(get_column_width(section), fm.height()+4);
#endif		
		} else {
			return QVariant();
		}
		
	} else {
		// Vertical header; ignore.
		return QVariant();
	}
}


QVariant
GPlatesGui::FeatureTableModel::data(
		const QModelIndex &idx,
		int role) const
{
	if ( ! idx.isValid()) {
		return QVariant();
	}
	if (idx.row() < 0 || idx.row() >= rowCount()) {
		return QVariant();
	}
	
	if (role == Qt::DisplayRole) {
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type geometry =
				d_sequence.at(idx.row()).reconstruction_geometry;

		// Cell contents is returned via call to column-specific dispatch function.
		table_cell_accessor_type accessor = get_column_accessor(idx.column());
		return (*accessor)(geometry);
	} else if (role == Qt::TextAlignmentRole) {
		return QVariant(get_column_alignment(idx.column()));
	}
	return QVariant();
}


void
GPlatesGui::FeatureTableModel::handle_selection_change(
		const QItemSelection &selected,
		const QItemSelection &deselected)
{
	if (selected.indexes().isEmpty()) {
		d_feature_focus_ptr->unset_focus();
		return;
	}
	// We assume that the view has been constrained to allow only single-row selections,
	// so only concern ourselves with the first index in the list.
	QModelIndex idx = selected.indexes().first();
	if ( ! idx.isValid()) {
		return;
	}
	const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type rg =
			d_sequence.at(idx.row()).reconstruction_geometry;

	// set the current index
	d_current_index = idx;

	// See if reconstruction geometry references a feature.
	boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(rg);
	if (feature_ref)
	{
		// When the user clicks a line of the table, we change the currently focused
		// feature.
		//
		d_feature_focus_ptr->set_focus(feature_ref.get(), rg);
	}
}


void
GPlatesGui::FeatureTableModel::handle_feature_modified(
		GPlatesGui::FeatureFocus &feature_focus)
{
	const GPlatesModel::FeatureHandle::weak_ref modified_feature_ref =
			feature_focus.focused_feature();

	// First, figure out which row(s) of the table (if any) contains the modified feature
	// weak-ref.  Note that, since each row of the table corresponds to a single geometry
	// rather than a single feature, there might be multiple rows which match this feature.
	int row = 0;
	geometry_sequence_type::const_iterator it = d_sequence.begin();
	geometry_sequence_type::const_iterator end = d_sequence.end();
	for ( ; it != end; ++it, ++row)
	{
		const GPlatesAppLogic::ReconstructionGeometry *rg = it->reconstruction_geometry.get();

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(rg);
		if (feature_ref)
		{
			// The RG references a feature, so let's look at the feature it's referencing.
			if (feature_ref.get() == modified_feature_ref)
			{
				QModelIndex idx_begin = index(row, 0);
				QModelIndex idx_end = index(row, NUM_ELEMS(column_heading_info_table) - 1);
				Q_EMIT dataChanged(idx_begin, idx_end);
			}
		}
		// Else it doesn't reference a feature.
	}
}


void
GPlatesGui::FeatureTableModel::handle_rendered_geometry_collection_update()
{
	//PROFILE_FUNC();

	std::vector<int> rows_to_remove;

	// Get all reconstruction geometries from the rendered geometry collection RECONSTRUCTION layer.
	// NOTE: This is done here (ie, outside the loop below) because it only needs to be done once for all features.
	GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type all_reconstruction_geoms_in_reconstruction_layer;
	GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries(
			all_reconstruction_geoms_in_reconstruction_layer,
			d_rendered_geometry_collection,
			// All reconstruction geometries go into the RECONSTRUCTION_LAYER...
			GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Iterate over the reconstruction geometries and update them if possible for
	// the new reconstruction.
	const int start_row = 0;
	int row = start_row;
	geometry_sequence_type::iterator it = d_sequence.begin();
	geometry_sequence_type::iterator end = d_sequence.end();
	for ( ; it != end; ++it, ++row)
	{
		// Find the new ReconstructionGeometry, if any, from inside the current Reconstruction
		// that corresponds to the current ReconstructionGeometry.
		const GPlatesAppLogic::ReconstructionGeometry &old_rg = *it->reconstruction_geometry;

		// If no new reconstruction geometry could be found then it's possible the
		// current reconstruction time is outside the begin/end valid time range of the
		// current feature in which case we'll just leave it in case the time changes back again
		// in which case the reconstruction geometry will become highlighted again.
		GPlatesAppLogic::ReconstructionGeometryUtils::reconstruction_geom_seq_type reconstruction_geometries_observing_feature;
		if (!GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
				reconstruction_geometries_observing_feature,
				all_reconstruction_geoms_in_reconstruction_layer,
				old_rg))
		{
			//rows_to_remove.push_back(row);
			continue;
		}

		// Change the reconstruction geometry for the current row.
		//
		// If there was more than one match then pick the first found.
		// NOTE: We can get more than one match if the same feature is reconstructed in two different
		// layers - each layer will produce a different ReconstructionGeometry.
		// Since we're arbitrarily picking the first match we might not pick the one associated with
		// the original ReconstructionGeometry. To fix this will require a way to identify which
		// layer the original ReconstructionGeometry came from.
		it->reconstruction_geometry = reconstruction_geometries_observing_feature.front();
	}

	if (row > 0)
	{
		//PROFILE_BLOCK("dataChanged");

		const int last_row = row - 1;

		// Notify of the changed rows.
		QModelIndex idx_begin = index(start_row, 0);
		QModelIndex idx_end = index(last_row, NUM_ELEMS(column_heading_info_table) - 1);
		Q_EMIT dataChanged(idx_begin, idx_end);
	}

	if (rows_to_remove.empty())
	{
		return;
	}

	// Remove rows that don't have a ReconstructionGeometry for the current reconstruction.
	// NOTE: We're removing the rows with higher indices first to avoid removing the wrong
	// subsequent rows.
	std::vector<int>::const_reverse_iterator remove_rows_iter = rows_to_remove.rbegin();
	std::vector<int>::const_reverse_iterator remove_rows_end = rows_to_remove.rend();
	for ( ; remove_rows_iter != remove_rows_end; ++remove_rows_iter)
	{
		const int row_to_remove = *remove_rows_iter;

		// Get the iterator to the reconstruction geometry that we'll be removing.
		geometry_sequence_type::iterator geometry_iter = d_sequence.begin();
		std::advance(geometry_iter, row_to_remove);

		// Remove the row.
		begin_remove_features(row_to_remove, row_to_remove);
		d_sequence.erase(geometry_iter);
		end_remove_features();
	}
}


void
GPlatesGui::FeatureTableModel::set_default_resize_modes(
		QHeaderView &header)
{
	for (int column = 0; column < static_cast<int>(NUM_ELEMS(column_heading_info_table)); ++column) {
		header.setResizeMode(column, column_heading_info_table[column].resize_mode);
	}
}


QModelIndex
GPlatesGui::FeatureTableModel::get_index_for_geometry(
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry)
{
	// Figure out which row of the table (if any) contains the target geometry.
	geometry_sequence_type::const_iterator it = d_sequence.begin();
	geometry_sequence_type::const_iterator end = d_sequence.end();
	int row = 0;
	for ( ; it != end; ++it, ++row) {
		if (it->reconstruction_geometry == reconstruction_geometry) {
			QModelIndex idx = index(row, 0);
			return idx;
		}
	}
	
	// No such feature exists in our table, return an invalid index.
	return QModelIndex();
}
