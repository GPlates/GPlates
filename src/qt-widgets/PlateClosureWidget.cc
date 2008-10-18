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

#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"

#include "feature-visitors/PlateIdFinder.h"
#include "feature-visitors/GmlTimePeriodFinder.h"
#include "feature-visitors/XsStringFinder.h"
#include "feature-visitors/ViewFeatureGeometriesWidgetPopulator.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/GeoTimeInstant.h"

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
		setDisabled(true);
		return;
	} else {
		setDisabled(false);
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


#if 0
void
GPlatesQtWidgets::PlateClosureWidget::set_click_point( const GPlatesMaths::PointOnSphere &pos)
{
	d_click_points.push_back( pos );
}
#endif

void
GPlatesQtWidgets::PlateClosureWidget::initialise_geometry(
		GeometryType geom_type)
{
	clear();
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
std::cout << "use rever" << std::endl;
	// pointers to the tables
	GPlatesGui::FeatureTableModel &segments_table = 
		d_view_state_ptr->segments_feature_table_model();

	GPlatesGui::FeatureTableModel &clicked_table = 
		d_view_state_ptr->feature_table_model();

	// Determine which feature to reverse

	// Segments Table is the tab 
	if ( d_view_state_ptr->get_tab() == 2 )
	{
		if (segments_table.current_index().isValid() )
		{
			int index = segments_table.current_index().row();

			// re-set the flag in the vector
			d_use_reverse_flags.at(index) = !d_use_reverse_flags.at(index);

std::cout << "use rever; tab 2 ; is valid" << std::endl;
			// process the segments table
			update_geometry();
			return;
		}
	}
		
	// Clicked Table is the tab
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

			// no change the d_use_reverse_flags vector,
			// handle_choose_feature does that.
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
	segments_table.begin_insert_features(0, 0);
	int index = clicked_table.current_index().row();
	segments_table.geometry_sequence().push_back( clicked_table.geometry_sequence().at(index) );
	segments_table.end_insert_features();

	// append the current flag
	d_use_reverse_flags.push_back(d_use_reverse);
	// reset to false
	d_use_reverse = false;

	// clear out the older featrure
	clear();

	// process the segments table
	update_geometry();
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_remove_feature()
{
	// flip tab to Segments Table
	d_view_state_ptr->change_tab( 2 );

	// pointer to the table
	GPlatesGui::FeatureTableModel &segments_table = 
		d_view_state_ptr->segments_feature_table_model();

	// get current selected index
	int index = segments_table.current_index().row();

	// erase that element from the Segments Table
	segments_table.begin_remove_features(index, index);
	segments_table.geometry_sequence().erase(
		segments_table.geometry_sequence().begin() + index);
	segments_table.end_remove_features();

	// remove the click_point and reverse flags
	// d_click_points.erase( d_click_points.begin() + index );
	d_use_reverse_flags.erase( d_use_reverse_flags.begin() + index );

	// clear out the widgets
	clear();

	// process the segments table
	update_geometry();
}



void
GPlatesQtWidgets::PlateClosureWidget::handle_clear()
{
	// Clear all geometry from the table.
	// undo_stack().push(new GPlatesUndoCommands::PlateClosureClearGeometry(*this));

	// clear the clicked table
	d_view_state_ptr->feature_table_model().clear();

	// clear the widget
	clear();
}


void
GPlatesQtWidgets::PlateClosureWidget::handle_create()
{
	d_create_feature_dialog->set_topological();

	bool success = d_create_feature_dialog->display();
	if ( ! success) {
		// The user cancelled the creation process. Return early and do not reset
		// the digitisation widget.
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
	m_vertex_list.clear();

	// flip tab to clicked table
	d_view_state_ptr->change_tab( 0 );
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_cancel()
{
	// Clear the widgets
	handle_clear();

	// Clear the tables
	d_view_state_ptr->segments_feature_table_model().clear();
	d_view_state_ptr->feature_table_model().clear();

	// flip tab to clicked table
	d_view_state_ptr->change_tab( 0 );

	// clear the drawing laye
	// GlobeCanvas &canvas = d_view_state_ptr->globe_canvas();

	d_view_state_ptr->globe_canvas().globe().rendered_geometry_layers().plate_closure_layer().clear();
	d_view_state_ptr->globe_canvas().update_canvas();
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
		m_vertex_list.push_back( *itr );
	}
}

void
GPlatesQtWidgets::PlateClosureWidget::visit_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{ 
std::cout << "point geom" << std::endl;

	// simply append the point to the working list
	m_vertex_list.push_back( *point_on_sphere );
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

	// check for flag
	if (d_use_reverse) 
	{
		m_vertex_list.insert( m_vertex_list.end(), 
			polyline_vertices.rbegin(), polyline_vertices.rend() );
	}
	else 
	{
		m_vertex_list.insert( m_vertex_list.end(), 
			polyline_vertices.begin(), polyline_vertices.end() );
	}
}



void
GPlatesQtWidgets::PlateClosureWidget::update_geometry()
{
	// access the segments table
	GPlatesGui::FeatureTableModel &segments_table = 
		d_view_state_ptr->segments_feature_table_model();

	// short cut for empty table
	if ( segments_table.geometry_sequence().empty() ) { return; }

	// loop over each geom
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator iter;
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator end;

	iter = segments_table.geometry_sequence().begin();
	end = segments_table.geometry_sequence().end();

	int i = 0;
	for ( ; iter != end ; ++iter)
	{
		// set the widget's reverse flag to this feature's flag
		d_use_reverse = d_use_reverse_flags.at(i);

		// visit the geoms.
		(*iter)->geometry()->accept_visitor(*this);
	}	

	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;

	// create the temp geom.
	geometry_opt_ptr_type geometry_opt_ptr = 
		create_geometry_from_table_items( m_vertex_list, d_geometry_type, validity);

	d_geometry_opt_ptr = geometry_opt_ptr;

	draw_temporary_geometry();
}


void
GPlatesQtWidgets::PlateClosureWidget::draw_temporary_geometry()
{
	GlobeCanvas &canvas = d_view_state_ptr->globe_canvas();
	GPlatesGui::RenderedGeometryLayers &layers = canvas.globe().rendered_geometry_layers();
	layers.plate_closure_layer().clear();

	if (d_geometry_opt_ptr) 
	{
std::cout << "draw_temporary_geometry()" << std::endl;
		GPlatesGui::PlatesColourTable::const_iterator colour = &GPlatesGui::Colour::BLACK;

		layers.plate_closure_layer().push_back(
				GPlatesGui::RenderedGeometry(*d_geometry_opt_ptr, colour));
	}
	canvas.update_canvas();
}

