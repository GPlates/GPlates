/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <QtGlobal>
#include <QHeaderView>
#include <QTreeWidget>
#include <QUndoStack>
#include <QToolButton>
#include <QMessageBox>
#include <QLocale>
#include <QDebug>
#include <QString>

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include "PlateClosureWidget.h"
// #include "PlateClosureWidgetUndoCommands.h"

#include "ViewportWindow.h"
#include "ExportCoordinatesDialog.h"
#include "CreateFeatureDialog.h"

#include "maths/InvalidLatLonException.h"
#include "maths/InvalidLatLonCoordinateException.h"
#include "maths/LatLonPointConversions.h"
#include "maths/GeometryOnSphere.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/Real.h"

#include "gui/FeatureFocus.h"

#include "model/FeatureHandle.h"
#include "model/ReconstructionGeometry.h"
#include "model/ModelUtils.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"

#include "feature-visitors/PlateIdFinder.h"
#include "feature-visitors/GmlTimePeriodFinder.h"
#include "feature-visitors/XsStringFinder.h"
#include "feature-visitors/ViewFeatureGeometriesWidgetPopulator.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlOldPlatesHeader.h"

#include "property-values/TemplateTypeParameterType.h"

namespace
{
	/**
	 * Borrowed from FeatureTableModel.cc.
	 */
	QString
	format_time_instant(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant)
	{
		QLocale locale;
		if (time_instant.time_position().is_real()) {
			return locale.toString(time_instant.time_position().value());
		} else if (time_instant.time_position().is_distant_past()) {
			return QObject::tr("past");
		} else if (time_instant.time_position().is_distant_future()) {
			return QObject::tr("future");
		} else {
			return QObject::tr("<invalid>");
		}
	}

	/**
	 * This typedef is used wherever geometry (of some unknown type) is expected.
	 * It is a boost::optional because creation of geometry may fail for various reasons.
	 */
	typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;
	
	
	/**
	 * Determines what fragment of geometry the top-level tree widget item
	 * would become, given the current configuration of the PlateClosureWidget
	 * and the position and number of children in this top-level item.
	 */
	QString
	calculate_label_for_item(
			GPlatesQtWidgets::PlateClosureWidget::GeometryType target_geom_type,
			int position,
			QTreeWidgetItem *item)
	{
		QString label;
		// Pick a sensible default.
		switch (target_geom_type)
		{
		default:
				label = QObject::tr("<Error: unknown GeometryType>");
				break;
		case GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON:
				label = "gml:LineString";
				break;
		case GPlatesQtWidgets::PlateClosureWidget::DEFORMINGPLATE:
				label = "gml:MultiPoint";
				break;
		}
		
		// Override that default for particular edge cases.
		int children = item->childCount();
		if (children == 0) {
			label = "";
		} else if (children == 1) {
			label = "gml:Point";
		} else if (children == 2 && target_geom_type == GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON) {
			label = "gml:LineString";
		}
		// FIXME:  We need to handle the situation in which the user wants to digitise a
		// polygon, and there are 3 distinct adjacent points, but the first and last points
		// are equal.  (This should result in a gml:LineString.)
		
		// PlateClosure Polygon gives special meaning to the first entry.
		if (target_geom_type == GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON) {
			if (position == 0) {
				label = QObject::tr("exterior: %1").arg(label);
			} else {
				label = QObject::tr("interior: %1").arg(label);
			}
		}
		
		return label;
	}


	/**
	 * Creates 'appropriate' geometry given the available 
	 * Examines QTreeWidgetItems, the number of points available, and the user's
	 * intentions. 
	 * to call upon the appropriate anonymous namespace geometry creation function.
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GPlatesUtils::GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	geometry_opt_ptr_type
	create_geometry_from_table_items(
			std::vector<GPlatesMaths::PointOnSphere> &points,
			GPlatesQtWidgets::PlateClosureWidget::GeometryType target_geom_type,
			GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
	{
		// FIXME: Only handles the unbroken line and single-ring cases.

		// There's no guarantee that adjacent points in the table aren't identical.
		std::vector<GPlatesMaths::PointOnSphere>::size_type num_points =
				GPlatesMaths::count_distinct_adjacent_points(points);

		std::vector<GPlatesMaths::PointOnSphere>::iterator itr;
		for ( itr = points.begin() ; itr != points.end(); ++itr)
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*itr);
			std::cout << "POINTS: " << llp << std::endl;
		}
	

		// FIXME: I think... we need some way to add data() to the 'header' QTWIs, so that
		// we can immediately discover which bits are supposed to be polygon exteriors etc.
		// Then the function calculate_label_for_item could do all our 'tagging' of
		// geometry parts, and -this- function wouldn't need to duplicate the logic.

		// FIXME 2: We should have a 'try {  } catch {  }' block to catch any exceptions
		// thrown during the instantiation of the geometries.

		// This will become a proper 'try {  } catch {  } block' when we get around to it.
		try
		{
			switch (target_geom_type)
			{
			default:
				// FIXME: Exception.
				qDebug() << "Unknown geometry type, not implemented yet!";
				return boost::none;

			case GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON:
				// FIXME: I'd really like to wrap this up into a function pointer.
				if (num_points == 0) {
					validity = GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
					return boost::none;
				} else if (num_points == 1) {
					return GPlatesUtils::create_point_on_sphere(points, validity);
				} else if (num_points == 2) {
					return GPlatesUtils::create_polyline_on_sphere(points, validity);
				} else if (num_points == 3 && points.front() == points.back()) {
					return GPlatesUtils::create_polyline_on_sphere(points, validity);
				} else {
					return GPlatesUtils::create_polygon_on_sphere(points, validity);
				}
				break;
			}
			// Should never reach here.
		} catch (...) {
			throw;
		}
		return boost::none;
	}
}



// 
// Constructor
// 
GPlatesQtWidgets::PlateClosureWidget::PlateClosureWidget(
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesModel::ModelInterface &model_interface,
		ViewportWindow &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_focus_ptr(&feature_focus),
	d_view_state_ptr(&view_state_),
	d_create_feature_dialog(new CreateFeatureDialog(model_interface, view_state_, this)),
	d_geometry_type(GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON),
	d_geometry_opt_ptr(boost::none)
{
	setupUi(this);

	clear();

	setDisabled(true);

	// Subscribe to focus events. We can discard the FeatureFocus reference afterwards.
	QObject::connect(&feature_focus,
		SIGNAL(focus_changed(GPlatesModel::FeatureHandle::weak_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
		this,
		SLOT(display_feature(GPlatesModel::FeatureHandle::weak_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));

	QObject::connect(&feature_focus,
		SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
		this,
		SLOT(display_feature(GPlatesModel::FeatureHandle::weak_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));

	// Use Coordinates in Reverse
	QObject::connect(button_use_coordinates_in_reverse, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_use_coordinates_in_reverse()));
	
	// Choose Feature button 
	QObject::connect(button_choose_feature, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_choose_feature()));
	
	// Remove Feature button 
	QObject::connect(button_remove_feature, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_remove_feature()));
	
	// Clear button to clear points from table and start over.
	QObject::connect(button_clear_feature, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_clear()));
	
	// Create... button to open the Create Feature dialog.
	QObject::connect(button_create_platepolygon, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_create()));	
	
	// Cancel button to cancel the process 
	QObject::connect(button_cancel, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_cancel()));	
	
	// Get everything else ready that may need to be set up more than once.
	initialise_geometry(PLATEPOLYGON);
}


void
GPlatesQtWidgets::PlateClosureWidget::clear()
{
	// Clear the widgets
	lineedit_type->clear();
	lineedit_name->clear();
	lineedit_plate_id->clear();
	lineedit_time_of_appearance->clear();
	lineedit_time_of_disappearance->clear();
	lineedit_clicked_geometry->clear();
	lineedit_first->clear();
	lineedit_last->clear();
}


// Display the clicked feature data in the widgets 
void
GPlatesQtWidgets::PlateClosureWidget::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
	// Clear the fields first, then fill in those that we have data for.
	clear();

	// always check your weak_refs!
	if ( ! feature_ref.is_valid() ) {
std::cout << "PlateClosureWidget::display_feature: ref NOT valid" << std::endl;


		// Don't Disable the whole widget with setDisabled(true); ...
		// ... just disable some widgets
		button_use_coordinates_in_reverse->setEnabled(false);
		button_choose_feature->setEnabled(false);
		button_remove_feature->setEnabled(false);
		button_clear_feature->setEnabled(false);

		return;
	} else {
std::cout << "PlateClosureWidget::display_feature: ref valid" << std::endl;

		setDisabled(false);

		button_use_coordinates_in_reverse->setEnabled(true);
		button_choose_feature->setEnabled(true);
		button_remove_feature->setEnabled(true);
		button_clear_feature->setEnabled(true);
	}
	
	// Populate the widget from the FeatureHandle:
	
	// Feature Type.
	lineedit_type->setText(GPlatesUtils::make_qstring_from_icu_string(
			feature_ref->feature_type().build_aliased_name()));
	
	// Feature Name.
	// FIXME: Need to adapt according to user's current codeSpace setting.
	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");
	GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
	string_finder.visit_feature_handle(*feature_ref);
	if (string_finder.found_strings_begin() != string_finder.found_strings_end()) {
		// The feature has one or more name properties. Use the first one for now.
		GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name = 
				*string_finder.found_strings_begin();
		
		lineedit_name->setText(GPlatesUtils::make_qstring(name->value()));
		lineedit_name->setCursorPosition(0);
	}

	// Plate ID.
	static const GPlatesModel::PropertyName plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	GPlatesFeatureVisitors::PlateIdFinder plate_id_finder(plate_id_property_name);
	plate_id_finder.visit_feature_handle(*feature_ref);
	if (plate_id_finder.found_plate_ids_begin() != plate_id_finder.found_plate_ids_end()) {
		// The feature has a reconstruction plate ID.
		GPlatesModel::integer_plate_id_type recon_plate_id =
				*plate_id_finder.found_plate_ids_begin();
		
		lineedit_plate_id->setText(QString::number(recon_plate_id));
	}
	
	// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");
	GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(valid_time_property_name);
	time_period_finder.visit_feature_handle(*feature_ref);
	if (time_period_finder.found_time_periods_begin() != time_period_finder.found_time_periods_end()) {
		// The feature has a gml:validTime property.
		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
				*time_period_finder.found_time_periods_begin();
		
		lineedit_time_of_appearance->setText(format_time_instant(*(time_period->begin())));
		lineedit_time_of_disappearance->setText(format_time_instant(*(time_period->end())));
	}

	if (associated_rfg) {
		// There was an associated Reconstructed Feature Geometry (RFG), which means there
		// was a clicked geometry.
		GPlatesModel::FeatureHandle::properties_iterator geometry_property =
				associated_rfg->property();
		if (*geometry_property != NULL) {
			lineedit_clicked_geometry->setText(
					GPlatesUtils::make_qstring_from_icu_string(
						(*geometry_property)->property_name().build_aliased_name()));
		} else {
			lineedit_clicked_geometry->setText(tr("<No longer valid>"));
		}
	}

	//FIXME: what to do here?
	if ( ! feature_ref.is_valid()) {
		return;
	}

	// create a dummy tree
	QTreeWidget *tree_geometry = new QTreeWidget(this);
	tree_geometry->hide();
	// use the tree and the populator to get coords
	GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator populator(
		d_view_state_ptr->reconstruction(), *tree_geometry);
	populator.visit_feature_handle(*feature_ref);
	d_first_coord = populator.get_first_coordinate();
	d_last_coord = populator.get_last_coordinate();
	// fill the widgets
	lineedit_first->setText(d_first_coord);
	lineedit_last->setText(d_last_coord);
	// clean up
	delete tree_geometry;
}


void
GPlatesQtWidgets::PlateClosureWidget::set_click_point(double lat, double lon)
{
	d_click_point_lat = lat;
	d_click_point_lon = lon;
}

void
GPlatesQtWidgets::PlateClosureWidget::initialise_geometry(
		GeometryType geom_type)
{
	clear();
	d_use_reverse = false;
	d_geometry_type = geom_type;
}


void
GPlatesQtWidgets::PlateClosureWidget::change_geometry_type(
		GeometryType geom_type)
{
	if (geom_type == d_geometry_type) {
		// Convert from one type of desired geometry to the exact same type.
		// i.e. do nothing.
		return;
	}
	
#if 0
	undo_stack().push(new GPlatesUndoCommands::PlateClosureChangeGeometryType(
			*this, d_geometry_type, geom_type));
#endif
}

// 
// Button Handlers  and support functions 
// 

void
GPlatesQtWidgets::PlateClosureWidget::handle_use_coordinates_in_reverse()
{
std::cout << "handle_use_coordinates_in_reverse" << std::endl;

	// pointers to the tables
	GPlatesGui::FeatureTableModel &segments_table = 
		d_view_state_ptr->segments_feature_table_model();

	GPlatesGui::FeatureTableModel &clicked_table = 
		d_view_state_ptr->feature_table_model();

	//
	// Determine which feature to reverse
	//

	// Clicked Table is the current tab
	// so we just want to reverse the display 
	if ( d_view_state_ptr->get_tab() == 0)
	{
		if ( clicked_table.current_index().isValid() )
		{
			// int index = clicked_table.current_index().row();
std::cout << "use rever; tab 0 ; is valid" << std::endl;
			// just set the widget's flag
			d_use_reverse = !d_use_reverse;

			if ( d_use_reverse ) {
				lineedit_first->setText( d_last_coord );
				lineedit_last->setText( d_first_coord );
			} else {
				lineedit_first->setText( d_first_coord );
				lineedit_last->setText( d_last_coord );
			}
		}
	}

	// Segments Table is the current tab 
	if ( d_view_state_ptr->get_tab() == 2 )
	{
std::cout << "use rever; tab 2" << std::endl;
std::cout << "use rever; tab 2; row=" << segments_table.current_index().row() << std::endl;

		if ( segments_table.current_index().isValid() )
		{
			int index = segments_table.current_index().row();

			// re-set the flag in the vector
			d_use_reverse_flags.at(index) = !d_use_reverse_flags.at(index);

std::cout << "use rever; tab 2 ; is valid; index=" << index 
<< "; use=" << d_use_reverse_flags.at(index) << std::endl;

			if ( d_use_reverse_flags.at(index) ) {
				lineedit_first->setText( d_last_coord );
				lineedit_last->setText( d_first_coord );
			} else {
				lineedit_first->setText( d_first_coord );
				lineedit_last->setText( d_last_coord );
			}

			// process the segments table
			update_geometry();
			return;
		}
	}
		
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_choose_feature()
{
	// flip tab to segments table
	d_view_state_ptr->change_tab( 2 );

	// pointers to the tables
	GPlatesGui::FeatureTableModel &clicked_table = 
		d_view_state_ptr->feature_table_model();

	GPlatesGui::FeatureTableModel &segments_table = 
		d_view_state_ptr->segments_feature_table_model();

	// transfer data to segments table
	int click_index = clicked_table.current_index().row();
	segments_table.begin_insert_features(0, 0);
	segments_table.geometry_sequence().push_back(
		clicked_table.geometry_sequence().at(click_index) );
	segments_table.end_insert_features();

	// highlight the segments table row for this feature
	int segments_index = segments_table.geometry_sequence().size() - 1;
	d_view_state_ptr->highlight_segments_table_row( segments_index );

	// append the current flag
	d_use_reverse_flags.push_back(d_use_reverse);
	// reset the current flag
	d_use_reverse = false;

	// append the current click_point
	d_click_points.push_back( std::make_pair( d_click_point_lat, d_click_point_lon ) );

	// process the segments table
	update_geometry();

	// FIXME: this undoes the connection to highlight_segments_table_row 
	// uset the focus
	// d_feature_focus_ptr->unset_focus();

	// clear the "Clicked" table
	d_view_state_ptr->feature_table_model().clear();
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_remove_feature()
{
	// flip tab to Segments Table
	d_view_state_ptr->change_tab( 2 );

	// pointer to the table
	GPlatesGui::FeatureTableModel &segments_table = 
		d_view_state_ptr->segments_feature_table_model();

	if ( segments_table.current_index().isValid() )
	{
		// get current selected index
		int index = segments_table.current_index().row();

		// erase that element from the Segments Table
		segments_table.begin_remove_features(index, index);
		segments_table.geometry_sequence().erase(
			segments_table.geometry_sequence().begin() + index);
		segments_table.end_remove_features();

		// remove the click_point and reverse flags
		d_click_points.erase( d_click_points.begin() + index );
		d_use_reverse_flags.erase( d_use_reverse_flags.begin() + index );

		// clear out the widgets
		clear();

		// process the segments table
		update_geometry();
	}

}



void
GPlatesQtWidgets::PlateClosureWidget::handle_clear()
{
	// Clear all geometry from the table.
	// undo_stack().push(new GPlatesUndoCommands::PlateClosureClearGeometry(*this));

	// clear the "Clicked" table
	d_view_state_ptr->feature_table_model().clear();

	// clear the widget
	clear();
}


void
GPlatesQtWidgets::PlateClosureWidget::handle_create()
{
	// do one final update 
	update_geometry();

	// pointer to the Segments table
	GPlatesGui::FeatureTableModel &segments_table = 
		d_view_state_ptr->segments_feature_table_model();

	// check for an empty segments table
	if ( ! segments_table.geometry_sequence().empty() ) 
	{
		// tell dialog that we are creating a topological feature
		d_create_feature_dialog->set_topological();

		bool success = d_create_feature_dialog->display();

		if ( ! success) {
			// The user cancelled the creation process. 
			// Return early and do not reset the widget.
			return;
		}
	} else {
		QMessageBox::warning(this, tr("No boundary segments selected for new feature"),
				tr("There are no valid boundray sements to use for creating a new feature."),
				QMessageBox::Ok);
		return;
	}

	// FIXME: Undo! but that ties into the main 'model' QUndoStack.
	// So the simplest thing to do at this stage is clear the 'digitisation' undo stack.
	undo_stack().clear();
	
	// Then, when we're all done, reset the widget for new input.
	initialise_geometry(d_geometry_type);

	// Clear the widgets
	handle_clear();

	// Clear the tables
	d_view_state_ptr->segments_feature_table_model().clear();
	d_view_state_ptr->feature_table_model().clear();

	// enmpty the vertex list
	d_vertex_list.clear();
	d_tmp_vertex_list.clear();

	// clear this tool's layer
	d_view_state_ptr->globe_canvas().globe().rendered_geometry_layers().plate_closure_layer().clear();
	d_view_state_ptr->globe_canvas().update_canvas();

	// change tab to Clicked table
	d_view_state_ptr->change_tab( 0 );

std::cout << "END of handle_create()" << std::endl;
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_cancel()
{
	// clear the widgets
	handle_clear();

	// clear the tables
	d_view_state_ptr->segments_feature_table_model().clear();
	d_view_state_ptr->feature_table_model().clear();

	// flip tab to clicked table
	d_view_state_ptr->change_tab( 0 );

	// enmpty the vertex list
	d_vertex_list.clear();
	d_tmp_vertex_list.clear();

	// clear the drawing layer
	d_view_state_ptr->globe_canvas().globe().rendered_geometry_layers().plate_closure_layer().clear();
	d_view_state_ptr->globe_canvas().update_canvas();

	// unset the feature focus
	d_feature_focus_ptr->unset_focus();

}


// Please keep these geometries ordered alphabetically.
void
GPlatesQtWidgets::PlateClosureWidget::visit_multi_point_on_sphere(
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
{
std::cout << "multipoint geom" << std::endl;
	GPlatesMaths::MultiPointOnSphere::const_iterator beg = multi_point_on_sphere->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator end = multi_point_on_sphere->end();
	GPlatesMaths::MultiPointOnSphere::const_iterator itr = beg;
	for ( ; itr != end ; ++itr)
	{
		// simply append the point to the working list
		d_tmp_vertex_list.push_back( *itr );
	}
	d_tmp_check_intersections = false;
}

void
GPlatesQtWidgets::PlateClosureWidget::visit_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{ 
std::cout << "point geom" << std::endl;

	// simply append the point to the working list
	d_tmp_vertex_list.push_back( *point_on_sphere );

	// set the d_tmp vars to create a source geometry property delegate 
	d_tmp_property_name = "position";
	d_tmp_value_type = "Point";

	const GPlatesModel::FeatureId fid(d_tmp_feature_id);

	const GPlatesModel::PropertyName prop_name =
		GPlatesModel::PropertyName::create_gpml( d_tmp_property_name);

	const GPlatesPropertyValues::TemplateTypeParameterType value_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gml( d_tmp_value_type );

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd_ptr = 
		GPlatesPropertyValues::GpmlPropertyDelegate::create( 
			fid,
			prop_name,
			value_type
		);
			
	// create a GpmlTopologicalPoint from the delegate
	GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type gtp_ptr =
		GPlatesPropertyValues::GpmlTopologicalPoint::create(pd_ptr);

	// Fill the vector of GpmlTopologicalSection::non_null_ptr_type 
	d_sections_ptrs.push_back( gtp_ptr );

	d_tmp_check_intersections = false;
}

void
GPlatesQtWidgets::PlateClosureWidget::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
std::cout << "polygon geom" << std::endl;
}

void
GPlatesQtWidgets::PlateClosureWidget::visit_polyline_on_sphere(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{  
std::cout << "polyline geom" << std::endl;

	// Set the d_tmp vars to create a source geometry property delegate 
	d_tmp_property_name = "centerLineOf";
	d_tmp_value_type = "LineString";

	const GPlatesModel::FeatureId fid(d_tmp_feature_id);

	const GPlatesModel::PropertyName prop_name =
		GPlatesModel::PropertyName::create_gpml( d_tmp_property_name);

	const GPlatesPropertyValues::TemplateTypeParameterType value_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gml( d_tmp_value_type );

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd_ptr = 
		GPlatesPropertyValues::GpmlPropertyDelegate::create( 
			fid,
			prop_name,
			value_type
		);
			

	// Write out each point of the polyline.
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = 
		polyline_on_sphere->vertex_begin();

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = 
		polyline_on_sphere->vertex_end();

	std::vector<GPlatesMaths::PointOnSphere> polyline_vertices;
	polyline_vertices.clear();
	
	for ( ; iter != end; ++iter) 
	{
		polyline_vertices.push_back( *iter );
	}

	// check for reverse flag
	if (d_use_reverse) 
	{
std::cout << "polyline geom REVERSE" << std::endl;
		d_tmp_vertex_list.insert( 
			d_tmp_vertex_list.end(), 
			polyline_vertices.rbegin(), polyline_vertices.rend() );
	}
	else 
	{
std::cout << "polyline geom normal" << std::endl;
		d_tmp_vertex_list.insert( 
			d_tmp_vertex_list.end(), 
			polyline_vertices.begin(), polyline_vertices.end() );
	}

	d_tmp_check_intersections = true;

	// create a GpmlTopologicalPoint from the delegate
	GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type gtls_ptr =
		GPlatesPropertyValues::GpmlTopologicalLineSection::create(
			pd_ptr,
			boost::none,
			boost::none,
			d_use_reverse);

	// Fill the vector of GpmlTopologicalSection::non_null_ptr_type 
	d_sections_ptrs.push_back( gtls_ptr );
}



void
GPlatesQtWidgets::PlateClosureWidget::update_geometry()
{
std::cout << "update_geometry: " << std::endl;

	// clear this tool's layer
	d_view_state_ptr->globe_canvas().globe().rendered_geometry_layers().plate_closure_layer().clear();
	d_view_state_ptr->globe_canvas().update_canvas();

	// loop over Segments Table to fill d_vertex_list
	create_sections_from_segments();

	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;

	// create the temp geom.
	geometry_opt_ptr_type geometry_opt_ptr = 
		create_geometry_from_table_items( d_vertex_list, d_geometry_type, validity);

	d_geometry_opt_ptr = geometry_opt_ptr;

	draw_temporary_geometry();
}

void
GPlatesQtWidgets::PlateClosureWidget::draw_temporary_geometry()
{
	d_view_state_ptr->globe_canvas().globe().rendered_geometry_layers().plate_closure_layer().clear();
	d_view_state_ptr->globe_canvas().update_canvas();

	if (d_geometry_opt_ptr) 
	{
		GPlatesGui::PlatesColourTable::const_iterator colour = &GPlatesGui::Colour::BLACK;

		d_view_state_ptr->globe_canvas().globe().rendered_geometry_layers().plate_closure_layer().push_back(
				GPlatesGui::RenderedGeometry(*d_geometry_opt_ptr, colour));
	}

	// update the canvas 
	d_view_state_ptr->globe_canvas().update_canvas();
}


void
GPlatesQtWidgets::PlateClosureWidget::create_sections_from_segments()
{
	// clear the working lists
	d_vertex_list.clear();
	d_tmp_vertex_list.clear();

	d_sections_ptrs.clear();

	// access the segments table
	GPlatesGui::FeatureTableModel &segments_table = 
		d_view_state_ptr->segments_feature_table_model();

	// super short cut for empty table
	if ( segments_table.geometry_sequence().empty() ) { return; }

	// get the size of the table
	d_tmp_segments_size = segments_table.geometry_sequence().size();

	// super short cut for single feature on thie list
	if ( d_tmp_segments_size == 1 )
	{
		// visit the geoms. ; will fill d_tmp_ vars
		std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator iter;
		iter = segments_table.geometry_sequence().begin();
		(*iter)->geometry()->accept_visitor(*this);

		// simply insert tmp items on the list
		d_vertex_list.insert( d_vertex_list.end(), 
			d_tmp_vertex_list.begin(), d_tmp_vertex_list.end() );
		return;
	}

	// else the list is > 2 

	// loop over each geom in the Segments Table
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator iter;
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator end;

	iter = segments_table.geometry_sequence().begin();
	end = segments_table.geometry_sequence().end();

	int d_tmp_index = 0;
	for ( ; iter != end ; ++iter)
	{
// FIXME: remove this diagnostic 
std::cout << "create_sections_from_segments: d_tmp_index = " << d_tmp_index << std::endl;

		GPlatesModel::ReconstructionGeometry *rg = iter->get();

		GPlatesModel::ReconstructedFeatureGeometry *rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);

		static const GPlatesModel::PropertyName name_property_name =
			GPlatesModel::PropertyName::create_gml("name");

		GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
		string_finder.visit_feature_handle( *(rfg->feature_ref()) );
		if (string_finder.found_strings_begin() != string_finder.found_strings_end()) 
		{
			GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name =
				 *string_finder.found_strings_begin();
			qDebug() << "name=" << GPlatesUtils::make_qstring( name->value() );
		}
// FIXME: remove this diagnostic 



		// set the tmp reverse flag to this feature's flag
		d_use_reverse = d_use_reverse_flags.at(d_tmp_index);
std::cout << "create_sections_from_segments: d_use_rev = " << d_use_reverse << std::endl;

		//
		// Visit the geoms. ; will fill d_tmp_ vars 
		//
		(*iter)->geometry()->accept_visitor(*this);

		// Create a source geometry property delegate 

		d_tmp_feature_id = rfg->feature_ref()->feature_id();

		const GPlatesModel::FeatureId fid(d_tmp_feature_id);

		const GPlatesModel::PropertyName prop_name =
			GPlatesModel::PropertyName::create_gpml( d_tmp_property_name);

		const GPlatesPropertyValues::TemplateTypeParameterType value_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml( d_tmp_value_type );

		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd = 
			GPlatesPropertyValues::GpmlPropertyDelegate::create( 
				fid,
				prop_name,
				value_type
			);
				

		// check for intersection
		if ( d_tmp_check_intersections )
		{
			//get_vertex_list_from_intersectons;
			d_vertex_list.insert( d_vertex_list.end(), 
				d_tmp_vertex_list.begin(), d_tmp_vertex_list.end() );
		}
		else
		{
			// simply insert tmp items on the list
			d_vertex_list.insert( d_vertex_list.end(), 
				d_tmp_vertex_list.begin(), d_tmp_vertex_list.end() );
		}

		// update counter d_tmp_index
		++d_tmp_index;
	}

}

void
GPlatesQtWidgets::PlateClosureWidget::get_vertex_list_from_intersection()
{

		// set the tmp click point to this feture's click point
		d_click_point_lat = d_click_points.at(d_tmp_index).first;
		d_click_point_lon = d_click_points.at(d_tmp_index).second;

}

// FIXME : save this code for ref

#if 0
		const GPlatesModel::FeatureId fid = rfg->feature_ref()->feature_id();
			
		const GPlatesModel::PropertyName prop_name =
			GPlatesModel::PropertyName::create_gpml("gpml:centerLineOf");

		const GPlatesPropertyValues::TemplateTypeParameterType value_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("gml:LineString");
			
		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd = 
			GPlatesPropertyValues::GpmlPropertyDelegate::create( 
				fid,
				prop_name,
				value_type
			);
				
		//d_source_geometry_property_delegate_ptrs.push_back( pd );
#endif

void
GPlatesQtWidgets::PlateClosureWidget::append_boundary_to_feature(
	GPlatesModel::FeatureHandle::weak_ref feature)
{
std::cout << "PlateClosureWidget::append_boundary_value_to_feature " << std::endl;

// FIXME: remove this diagnostic 
		static const GPlatesModel::PropertyName name_property_name =
			GPlatesModel::PropertyName::create_gml("name");
		GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
		string_finder.visit_feature_handle( *feature );
		if (string_finder.found_strings_begin() != string_finder.found_strings_end()) 
		{
			GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name =
				 *string_finder.found_strings_begin();
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature: name=" 
<< GPlatesUtils::make_qstring( name->value() );
		}
// FIXME: remove this diagnostic 

	// create the TopologicalPolygon
	GPlatesModel::PropertyValue::non_null_ptr_type topo_poly_value =
		GPlatesPropertyValues::GpmlTopologicalPolygon::create(d_sections_ptrs);

	const GPlatesPropertyValues::TemplateTypeParameterType topo_poly_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPolygon");

	// create the ConstantValue
	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value =
		GPlatesPropertyValues::GpmlConstantValue::create(topo_poly_value, topo_poly_type);

	// get the time period for the feature
	// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(valid_time_property_name);
	time_period_finder.visit_feature_handle( *feature );

	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period = 
		*time_period_finder.found_time_periods_begin();

	//GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type *tp = 
	GPlatesPropertyValues::GmlTimePeriod* tp = 
	const_cast<GPlatesPropertyValues::GmlTimePeriod *>( time_period.get() );

	// GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type ttpp =
	GPlatesUtils::non_null_intrusive_ptr<
		GPlatesPropertyValues::GmlTimePeriod, 
		GPlatesUtils::NullIntrusivePointerHandler> ttpp(
				tp,
				GPlatesUtils::NullIntrusivePointerHandler()
		);


	// create the TimeWindow
	GPlatesPropertyValues::GpmlTimeWindow tw = GPlatesPropertyValues::GpmlTimeWindow(
			constant_value, 
			ttpp,
			topo_poly_type);

	// use the time window
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows;

	time_windows.push_back(tw);

	// create the PiecewiseAggregation
	GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type aggregation =
		GPlatesPropertyValues::GpmlPiecewiseAggregation::create(time_windows, topo_poly_type);
	
	// Add a gpml:boundary Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
		aggregation,
		GPlatesModel::PropertyName::create_gpml("boundary"),
		feature);


	d_view_state_ptr->reconstruct();
}

