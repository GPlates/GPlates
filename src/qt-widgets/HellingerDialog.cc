/* $Id: HellingerDialog.cc 260 2012-05-30 13:47:23Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 260 $
 * $Date: 2012-05-30 15:47:23 +0200 (Wed, 30 May 2012) $
 *
 * Copyright (C) 2011, 2012, 2013, 2014, 2016 Geological Survey of Norway
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

// Workaround for compile error in <pyport.h> for Python versions less than 2.7.13 and 3.5.3.
// See https://bugs.python.org/issue10910
// Workaround involves including "global/python.h" at the top of some source files
// to ensure <Python.h> is included before <ctype.h>.
#include "global/python.h"

#include <vector>

#include <boost/foreach.hpp>

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>


#include "api/PythonInterpreterLocker.h"
#include "app-logic/AgeModelCollection.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "file-io/HellingerReader.h"
#include "file-io/HellingerWriter.h"
#include "global/CompilerWarnings.h"
#include "maths/PointOnSphere.h"
#include "presentation/ViewState.h"
#include "utils/ComponentManager.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "HellingerConfigurationDialog.h"
#include "HellingerConfigurationWidget.h"
#include "HellingerDialog.h"
#include "HellingerPointDialog.h"
#include "HellingerSegmentDialog.h"
#include "HellingerFitWidget.h"
#include "HellingerPickWidget.h"
#include "HellingerStatsDialog.h"
#include "HellingerThread.h"
#include "ReadErrorAccumulationDialog.h"

#include "QtWidgetUtils.h"


const double SLIDER_MULTIPLIER = -10000.;
const int DEFAULT_SYMBOL_SIZE = 2;
const int ENLARGED_SYMBOL_SIZE = 3;
const int POLE_ESTIMATE_SYMBOL_SIZE = 1;
const QString MAIN_PYTHON_FILENAME("hellinger.py");
const double DEFAULT_POINT_SIZE = 2;
const double DEFAULT_LINE_THICKNESS = 2;
const double ENLARGED_POINT_SIZE = 6;
const QString DEFAULT_OUTPUT_FILE_ROOT("hellinger_output");


// The following are related to the Hellinger tool in general, and not necessarily to this class/file.

// TODO: Farm out rendering functionality  to the canvas tool classes.
// TODO: consider interpreting other forms of chron embedded in the hellinger file name - see the GSFML site for examples

// FIXME: When switching back from a 3-way fit to a 2-way fit, we are throwing (and catching) IndeterminateArc... exceptions
// from the adjust pole tool ("Error updating angle" and "Error generating new end point"- investigate.
// Probably related to (lack of) initialisation of the new pole (i.e. pole-estimate 1-3) end points / angle.

// TODO: consider removing the d_is_enabled member from the HellingerPick structure.


namespace{

	enum TabPages
	{
		PICKS_TAB_PAGE,
		FIT_TAB_PAGE,

		NUM_TAB_PAGES
	};

#if 0
	/**
	 * @brief set_chron_time
	 * @param chron_time - if we find a matching chron string in the @param map, then @param chron_time is changed on return
	 * @param chron_string - QString of form <chron>.<y|o>
	 * where <y|o> can be "y" or "o", indicating which of the
	 * younger or older ends of the time interval is to be used.
	 *
	 * @param map - a map of <QString chron> to <double younger_time,double older_time>
	 */
	void
	set_chron_time(
			double &chron_time,
			const QString &chron_string,
			const GPlatesAppLogic::ApplicationState::chron_to_time_interval_map_type &map)
	{
		// Extract the "y" (younger) or "o" (older) field from the chron_string, if it exists.
		int i = chron_string.lastIndexOf(".");
		qDebug() << "index of last dot: " << i;
		QString younger_or_older_string = chron_string.right(chron_string.length() - i - 1);

		// Everything to the left of the last "." we will take as the base_chron_string,
		// which we'll use as a key in the chron-to-time-interval-map.
		QString base_chron_string = chron_string.left(i);
		qDebug() << "base: " << base_chron_string;
		qDebug() << "end: " << younger_or_older_string;

		static const QString younger_string = QString("y");
		static const QString older_string = QString("n");

		// We have to use the younger or older ends of the time interval.If we
		// don't find any sensible indication of this in the chron_string, we'll
		// choose (arbitrarily) the younger, so we'll make this the default behaviour.
		bool use_younger_end = true;

		if (younger_or_older_string == older_string)
		{
			use_younger_end = false;
		}

		GPlatesAppLogic::ApplicationState::chron_to_time_interval_map_type::const_iterator iter
				= map.find(base_chron_string);

		if (iter != map.end())
		{
			if (use_younger_end)
			{
				chron_time = iter->second.first;
			}
			else
			{
				chron_time = iter->second.second;
			}
		}
	}
#endif


	void
	check_output_file_root(
			QLineEdit *line_edit)
	{
		if (line_edit->text().isEmpty())
		{
			line_edit->setText(DEFAULT_OUTPUT_FILE_ROOT);
		}
	}

	QString
	create_results_filename(
			const QString &path,
			const QString &root)
	{
		return QString(path + QDir::separator() + root + "_results.dat");
	}

	/**
	 * @brief try_to_find_chron_time
	 * @param chron_string - QString of form.....
	 * @param age_model_collection - AgeModelCollection to be searched
	 * @return if we find a matching chron_string in the active AgeModel, we return an optional form of the time (Ma) of that chron.
	 */
	boost::optional<double>
	try_to_find_chron_time(
			const QString &chron_string,
			const GPlatesAppLogic::AgeModelCollection &age_model_collection)
	{
		boost::optional<const GPlatesAppLogic::AgeModel &> active_age_model =
				age_model_collection.get_active_age_model();

		if (active_age_model)
		{
			const GPlatesAppLogic::age_model_map_type &age_model_map = active_age_model->d_model;
			GPlatesAppLogic::age_model_map_type::const_iterator it = age_model_map.find(chron_string);

			if (it != age_model_map.end())
			{
				return boost::optional<double>(it->second);
			}
		}

		return boost::none;
	}

	bool
	edit_operation_active(const GPlatesQtWidgets::CanvasOperationType &type)
	{
		return type != GPlatesQtWidgets::SELECT_OPERATION;
	}

	std::pair<GPlatesQtWidgets::HellingerPlateIndex,GPlatesQtWidgets::HellingerPlateIndex>
	get_moving_plate_indices(
			const GPlatesQtWidgets::HellingerPlateIndex &fixed_index)
	{
		switch(fixed_index)
		{
		case GPlatesQtWidgets::PLATE_ONE_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_ONE_PICK_TYPE:
			return std::make_pair(GPlatesQtWidgets::PLATE_TWO_PICK_TYPE,GPlatesQtWidgets::PLATE_THREE_PICK_TYPE);
			break;
		case GPlatesQtWidgets::PLATE_TWO_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_TWO_PICK_TYPE:
			return std::make_pair(GPlatesQtWidgets::PLATE_ONE_PICK_TYPE,GPlatesQtWidgets::PLATE_THREE_PICK_TYPE);
			break;
		case GPlatesQtWidgets::PLATE_THREE_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_THREE_PICK_TYPE:
			return std::make_pair(GPlatesQtWidgets::PLATE_ONE_PICK_TYPE,GPlatesQtWidgets::PLATE_TWO_PICK_TYPE);
			break;
		default:
			return std::make_pair(GPlatesQtWidgets::PLATE_TWO_PICK_TYPE,GPlatesQtWidgets::PLATE_THREE_PICK_TYPE);
		}
	}

	void
	correct_directions_of_rotations(
			boost::optional<GPlatesQtWidgets::HellingerFitStructure> &fit_a,
			boost::optional<GPlatesQtWidgets::HellingerFitStructure> &fit_b,
			const GPlatesQtWidgets::HellingerPlateIndex &fixed_plate_index)
	{
		// If the fixed plate is 1: the two fitted rotation poles represent
		// plate 1 to 2, and plate 1 to 3 respectively. We need to reverse the angle in each
		// of these rotations.
		//
		// Similarly if the fixed plate is 2: the two poles we have are for
		// plate 1 to 2, and plate 2 to 3. Here we need to reverse only the second pole.
		//
		// If the fixed plate is 3, then both our rotation poles are OK (1 to 3, and 2 to 3),
		// so we don't need to do anything.
		switch(fixed_plate_index)
		{
		case GPlatesQtWidgets::PLATE_ONE_PICK_TYPE:
			if (fit_a)
			{
				fit_a.get().d_angle = - fit_a.get().d_angle;
			}
			if (fit_b)
			{
				fit_b.get().d_angle = - fit_b.get().d_angle;
			}
			break;
		case GPlatesQtWidgets::PLATE_TWO_PICK_TYPE:
			if (fit_b)
			{
				fit_b.get().d_angle = - fit_b.get().d_angle;
			}
			break;
		default:
			break;
		}
	}

	boost::optional<GPlatesMaths::FiniteRotation>
	get_rotation(
			const boost::optional<GPlatesQtWidgets::HellingerFitStructure> &fit,
			double fraction)
	{
		if (!fit)
		{
			return boost::none;
		}

		double angle_radians  = GPlatesMaths::convert_deg_to_rad(fraction*fit.get().d_angle);
		GPlatesMaths::PointOnSphere point = make_point_on_sphere(
					GPlatesMaths::LatLonPoint(fit.get().d_lat,fit.get().d_lon));

		return GPlatesMaths::FiniteRotation::create(point,angle_radians);
	}

	const GPlatesGui::Symbol &
	get_pick_symbol(
			const GPlatesQtWidgets::HellingerPlateIndex &index,
			bool use_enlarged_symbol_size = false)
	{
		static const GPlatesGui::Symbol default_plate_one_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, DEFAULT_SYMBOL_SIZE, true);
		static const GPlatesGui::Symbol default_plate_two_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::SQUARE, DEFAULT_SYMBOL_SIZE, false);
		static const GPlatesGui::Symbol default_plate_three_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::TRIANGLE, DEFAULT_SYMBOL_SIZE, false);
		static const GPlatesGui::Symbol enlarged_plate_one_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, ENLARGED_SYMBOL_SIZE, true);
		static const GPlatesGui::Symbol enlarged_plate_two_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::SQUARE, ENLARGED_SYMBOL_SIZE, false);
		static const GPlatesGui::Symbol enlarged_plate_three_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::TRIANGLE, ENLARGED_SYMBOL_SIZE, false);

		switch(index)
		{
		case GPlatesQtWidgets::PLATE_ONE_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_ONE_PICK_TYPE:
			return use_enlarged_symbol_size ? enlarged_plate_one_symbol : default_plate_one_symbol;
			break;
		case GPlatesQtWidgets::PLATE_TWO_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_TWO_PICK_TYPE:
			return use_enlarged_symbol_size ? enlarged_plate_two_symbol : default_plate_two_symbol;
			break;
		case GPlatesQtWidgets::PLATE_THREE_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_THREE_PICK_TYPE:
			return use_enlarged_symbol_size ? enlarged_plate_three_symbol : default_plate_three_symbol;
			break;
		default:
			return default_plate_one_symbol;
		}
	}

	void
	add_pick_geometry_to_layer(
			const GPlatesQtWidgets::HellingerPick &pick,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type &layer,
			const GPlatesGui::Colour &colour,
			bool use_enlarged_symbol_size = false)
	{
		const GPlatesMaths::LatLonPoint llp(pick.d_lat,pick.d_lon);
		const GPlatesMaths::PointOnSphere point = make_point_on_sphere(llp);

		GPlatesGui::Symbol symbol = get_pick_symbol(pick.d_segment_type,use_enlarged_symbol_size);


		GPlatesViewOperations::RenderedGeometry pick_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					point.get_non_null_pointer(),
					colour,
					DEFAULT_POINT_SIZE, /* point size */
					DEFAULT_LINE_THICKNESS, /* line thickness */
					false, /* fill polygon */
					false, /* fill polyline */
					GPlatesGui::Colour::get_white(), // dummy colour argument
					symbol);

		layer->add_rendered_geometry(pick_geometry);
	}

	void
	add_segment_geometries_to_layer(
			const GPlatesQtWidgets::hellinger_model_const_range_type &segment,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type &layer,
			const GPlatesGui::Colour &colour)
	{
		GPlatesQtWidgets::hellinger_model_type::const_iterator it = segment.first;
		for (; it != segment.second; ++it)
		{
			add_pick_geometry_to_layer(
						it->second,
						layer,
						colour);
		}
	}

	const GPlatesGui::Colour &
	get_segment_colour(
			int num_colour)
	{
		num_colour = num_colour%7;
		switch (num_colour)
		{
		case 0:
			return GPlatesGui::Colour::get_green();
			break;
		case 1:
			return GPlatesGui::Colour::get_blue();
			break;
		case 2:
			return GPlatesGui::Colour::get_maroon();
			break;
		case 3:
			return GPlatesGui::Colour::get_purple();
			break;
		case 4:
			return GPlatesGui::Colour::get_fuchsia();
			break;
		case 5:
			return GPlatesGui::Colour::get_olive();
			break;
		case 6:
			return GPlatesGui::Colour::get_navy();
			break;
		default:
			return GPlatesGui::Colour::get_navy();
		}
	}
}

GPlatesQtWidgets::HellingerDialog::HellingerDialog(
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ReadErrorAccumulationDialog &read_error_accumulation_dialog,
		QWidget *parent_):
	GPlatesDialog(
		parent_,
		//Qt::Window),
		Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_view_state(view_state),
	d_rendered_geom_collection_ptr(&view_state.get_rendered_geometry_collection()),
	d_read_error_accumulation_dialog(read_error_accumulation_dialog),
	d_hellinger_stats_dialog(0),
	d_hellinger_edit_point_dialog(new HellingerPointDialog(this,&d_hellinger_model,false)),
	d_hellinger_new_point_dialog(new HellingerPointDialog(this,&d_hellinger_model,true /* create new point */)),
	d_hellinger_edit_segment_dialog(new HellingerSegmentDialog(this,&d_hellinger_model)),
	d_hellinger_new_segment_dialog(new HellingerSegmentDialog(this,&d_hellinger_model,true)),
	d_hellinger_thread(new HellingerThread(this,&d_hellinger_model)),
	d_plate_1_id(0),
	d_plate_2_id(0),
	d_plate_3_id(0),
	d_recon_time(0.),
	d_chron_time(0.),
	d_moving_symbol(GPlatesGui::Symbol::CROSS, DEFAULT_SYMBOL_SIZE, true),
	d_fixed_symbol(GPlatesGui::Symbol::SQUARE, DEFAULT_SYMBOL_SIZE, false),
	d_thread_type(TWO_WAY_POLE_THREAD_TYPE),
	d_edit_point_is_enlarged(false),
	d_canvas_operation_type(SELECT_OPERATION),
	d_pole_estimate_12_llp(GPlatesMaths::LatLonPoint(0,0)),
	d_pole_estimate_13_llp(GPlatesMaths::LatLonPoint(0,0)),
	d_pole_estimate_12_angle(0.),
	d_pole_estimate_13_angle(0.),
	d_configuration_dialog(new HellingerConfigurationDialog(
							   d_configuration,
							   view_state.get_application_state(),
							   this)),
	d_pick_widget(new HellingerPickWidget(this,&d_hellinger_model)),
	d_fit_widget(new HellingerFitWidget(this,&d_hellinger_model)),
	d_adjust_pole_tool_is_active(false),
	d_open_directory_dialog(this,
							tr("Select output path"),
							view_state),
	d_output_path_is_valid(true),
	d_three_way_fitting_is_enabled(
		GPlatesUtils::ComponentManager::instance().is_enabled(
			GPlatesUtils::ComponentManager::Component::hellinger_three_plate()))
{
	setupUi(this);

	// We need to pass the main hellinger python file to boost::python::exec_file, hence we need to get the location of the
	// python scripts. This step (passing the python file) might not be necessary later.
	d_python_path = d_view_state.get_application_state().get_user_preferences().get_default_value("paths/python_system_script_dir").toString();

	// And we need a location to store some temporary files which are used in exchanging data between GPlates and the python scripts.
	//
	// NOTE: In Qt5, QStandardPaths::DataLocation (called QDesktopServices::DataLocation in Qt4) no longer has 'data/' in the path.
	d_path_for_temporary_files = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

	// This is the path for text files containing fit results
	//
	// NOTE: In Qt5, QStandardPaths::DataLocation (called QDesktopServices::DataLocation in Qt4) no longer has 'data/' in the path.
	d_output_file_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

	// The temporary location might not exist - if it doesn't, try to create it.
	QDir dir(d_path_for_temporary_files);
	if (!dir.exists())
	{
		if (!dir.mkpath(d_path_for_temporary_files))
		{
			qWarning() << "Unable to create folder for temporary hellinger files.";
		}
	}

	d_python_path.append(QDir::separator());
	d_path_for_temporary_files.append(QDir::separator());
	d_python_file = d_python_path + MAIN_PYTHON_FILENAME;
#if 0
	qDebug() << "Path used for storing temporary hellinger files: " << d_path_for_temporary_files;
	qDebug() << "Path used for hellinger python file:  " << d_python_path;
#endif
	set_up_connections();

	set_up_child_layers();

	d_configuration_dialog->read_values_from_settings();

	initialise_widgets();

	d_pick_widget->update_buttons();

	update_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::handle_pick_dialog_updated()
{
	d_fit_widget->update_enabled_state_of_estimate_widgets(d_adjust_pole_tool_is_active);
	d_fit_widget->update_buttons();
	update_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::handle_cancel()
{
	// TODO: This is where we would (if we can) interrupt the thread running the python code.
}

void
GPlatesQtWidgets::HellingerDialog::handle_finished_editing()
{
	this->setEnabled(true);
	d_canvas_operation_type = SELECT_OPERATION;
	d_pick_widget->update_buttons();
	d_editing_layer_ptr->clear_rendered_geometries();
	d_editing_layer_ptr->set_active(false);
	d_feature_highlight_layer_ptr->set_active(false);
	Q_EMIT finished_editing();
}

void
GPlatesQtWidgets::HellingerDialog::handle_update_point_editing()
{
	d_editing_layer_ptr->clear_rendered_geometries();
	if (is_in_edit_point_state())
	{
		add_pick_geometry_to_layer(
					d_hellinger_edit_point_dialog->current_pick(),
					d_editing_layer_ptr,
					GPlatesGui::Colour::get_yellow(),
					d_edit_point_is_enlarged);
	}
	else if (is_in_new_point_state())
	{
		add_pick_geometry_to_layer(
					d_hellinger_new_point_dialog->current_pick(),
					d_editing_layer_ptr,
					GPlatesGui::Colour::get_yellow(),
					d_edit_point_is_enlarged);
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_update_segment_editing()
{
	if (d_hellinger_edit_segment_dialog->current_pick())
	{
		d_editing_layer_ptr->clear_rendered_geometries();
		add_pick_geometry_to_layer(
					*(d_hellinger_edit_segment_dialog->current_pick()),
					d_editing_layer_ptr,
					GPlatesGui::Colour::get_yellow(),
					d_edit_point_is_enlarged);
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_active_age_model_changed()
{
	update_chron_time();
}

void
GPlatesQtWidgets::HellingerDialog::handle_settings_clicked()
{
	d_configuration_dialog->show();
}

void
GPlatesQtWidgets::HellingerDialog::handle_configuration_changed()
{
	update_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::handle_tab_changed(int tab_index)
{
	switch(tab_index){
	case PICKS_TAB_PAGE:
		d_pick_widget->update_after_switching_tabs();
		break;
	case FIT_TAB_PAGE:
		d_fit_widget->update_after_switching_tabs();
		break;
	default:
		break;
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_output_path_button_clicked()
{
	QString path = line_edit_output_path->text();

	d_open_directory_dialog.select_directory(path);

	QString new_path = d_open_directory_dialog.get_existing_directory();

	if (new_path.isEmpty())
	{
		return;
	}

	line_edit_output_path->setText(new_path);

	check_and_highlight_output_path();
	d_fit_widget->update_buttons();
}

void
GPlatesQtWidgets::HellingerDialog::handle_output_path_changed()
{
	line_edit_output_path->setPalette(QPalette());
}

void
GPlatesQtWidgets::HellingerDialog::handle_output_path_editing_finished()
{
	check_and_highlight_output_path();
	d_fit_widget->update_buttons();
}

void
GPlatesQtWidgets::HellingerDialog::initialise_widgets()
{


	// This tool is part of a workflow - all other workflows have widgets in the task panel pane, but this (panel)
	// isn't big enough for the Hellinger tool, so we need a dialog. That raises the question of how the dialog
	// should behave when switching workflows - should it disappear, should it have its own close button etc...
	// For the initial realease (1.5+) at least, I think we leave the close button visible and enabled.
#if 0
	button_close->hide();
#endif
	// For eventual insertion of generated pole into the model.
	groupbox_rotation->hide();

	// Export of .com files is complicated and probably of little practical benefit to user. Disable.
	button_export_com_file->hide();

	// Make the pole estimate widgets disabled. They will be enabled when
	// the AdjustPoleEstimate tool is selected
	enable_pole_estimate_widgets(false);

	tab_widget->clear();
	tab_widget->addTab(d_pick_widget,"&Magnetic Picks");
	tab_widget->addTab(d_fit_widget,"&Fit");

	line_edit_output_path->setText(d_output_file_path);
	line_edit_output_file_root->setText(DEFAULT_OUTPUT_FILE_ROOT);

	radio_button_fixed_3->setVisible(d_three_way_fitting_is_enabled);

	radio_button_fixed_1->setChecked(true);

	button_group_fixed_plate->setId(radio_button_fixed_1,PLATE_ONE_PICK_TYPE);
	button_group_fixed_plate->setId(radio_button_fixed_2,PLATE_TWO_PICK_TYPE);
	button_group_fixed_plate->setId(radio_button_fixed_3,PLATE_THREE_PICK_TYPE);
}

void
GPlatesQtWidgets::HellingerDialog::highlight_selected_pick(
		const HellingerPick &pick)
{
	GPlatesGui::Colour colour = pick.d_is_enabled ? GPlatesGui::Colour::get_white() : GPlatesGui::Colour::get_grey();
	add_pick_geometry_to_layer(pick,d_selection_layer_ptr,colour);
}

void
GPlatesQtWidgets::HellingerDialog::highlight_selected_segment(
		const int &segment_number)
{
	hellinger_segment_type segment = d_hellinger_model.get_segment(segment_number);

	BOOST_FOREACH(HellingerPick pick, segment)
	{
		highlight_selected_pick(pick);
	}
}



void
GPlatesQtWidgets::HellingerDialog::handle_edit_pick()
{

	boost::optional<unsigned int> segment = d_pick_widget->segment_number_of_selected_pick();
	boost::optional<unsigned int> row = d_pick_widget->selected_row();

	if (!(segment && row))
	{
		return;
	}

	d_canvas_operation_type = EDIT_POINT_OPERATION;

	Q_EMIT begin_edit_pick();

	d_editing_layer_ptr->set_active(true);

	this->setEnabled(false);

	d_hellinger_edit_point_dialog->begin_pick_operation();
	d_hellinger_edit_point_dialog->update_pick_from_model(*segment, *row);

	add_pick_geometry_to_layer((d_hellinger_model.get_pick(*segment,*row)->second),
							   d_editing_layer_ptr,GPlatesGui::Colour::get_yellow());

}

void
GPlatesQtWidgets::HellingerDialog::handle_edit_segment()
{
	d_canvas_operation_type = EDIT_SEGMENT_OPERATION;

	d_editing_layer_ptr->set_active(true);

	boost::optional<unsigned int> segment = d_pick_widget->selected_segment();

	if (!segment)
	{
		return;
	}

	this->setEnabled(false);

	d_hellinger_edit_segment_dialog->begin_segment_operation();
	d_hellinger_edit_segment_dialog->initialise_with_segment(*segment);


	add_segment_geometries_to_layer(
				d_hellinger_model.get_segment_as_range(*segment),
				d_editing_layer_ptr,
				GPlatesGui::Colour::get_yellow());

}

void
GPlatesQtWidgets::HellingerDialog::restore()
{
	activate_layers();
	d_pick_widget->restore();
	update_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::handle_import_hellinger_file()
{
	QString filters;
	filters = QObject::tr("Hellinger pick file (*.pick)");
	filters += ";;";
	filters += QObject::tr("Hellinger com file (*.com)");
	filters += ";;";

	QString active_filter = QObject::tr("All Hellinger files (*.pick *.com)");
	filters += active_filter;

	QString file_path = QFileDialog::getOpenFileName(
				this,
				QObject::tr("Open Hellinger .pick or .com file"),
				d_view_state.get_last_open_directory(),
				filters,
				&active_filter);

	if (file_path.isEmpty())
	{
		return;
	}
	QFile file(file_path);
	QFileInfo file_info(file.fileName());
	QStringList file_name = file_info.fileName().split(".", QString::SkipEmptyParts);
	QString type_file = file_name.last();

	QString path = file_info.path();


	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_error_accumulation_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();
	if (type_file == "pick")
	{
		if (GPlatesFileIO::HellingerReader::read_pick_file(file_path,d_hellinger_model,read_errors))
		{
			// Set the pick-file basename as our output filename root
			line_edit_output_file_root->setText(file_info.completeBaseName());
			line_import_file->setText(file_path);
		}
	}
	else if (type_file == "com")
	{
		if (GPlatesFileIO::HellingerReader::read_com_file(file_path,d_hellinger_model,read_errors))
		{
			QString pick_file_path = path + QDir::separator() +  d_hellinger_model.get_pick_filename();
			qDebug() << "pick_file_path " << pick_file_path;
			if (GPlatesFileIO::HellingerReader::read_pick_file(
						pick_file_path,
						d_hellinger_model,
						read_errors))
			{
				QFileInfo file_info_(d_hellinger_model.get_pick_filename());
				line_edit_output_file_root->setText(file_info_.completeBaseName());
				line_import_file->setText(file_path);
			}
		}
	}


	d_read_error_accumulation_dialog.update();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors)
	{
		d_read_error_accumulation_dialog.show();
	}

	update_chron_time();

	update_widgets_from_model();

	update_canvas();
}



void
GPlatesQtWidgets::HellingerDialog::handle_pole_estimate_12_changed(
		double lat,
		double lon)
{
	d_pole_estimate_12_llp = GPlatesMaths::LatLonPoint(
				lat,lon);

	Q_EMIT pole_estimate_12_lat_lon_changed(lat,lon);
}

void
GPlatesQtWidgets::HellingerDialog::handle_pole_estimate_12_angle_changed(
		double angle)
{
	d_pole_estimate_12_angle = angle;
	Q_EMIT pole_estimate_12_angle_changed(angle);
}


void
GPlatesQtWidgets::HellingerDialog::handle_pole_estimate_13_changed(
		double lat,
		double lon)
{
	d_pole_estimate_13_llp = GPlatesMaths::LatLonPoint(
				lat,lon);

	Q_EMIT pole_estimate_13_lat_lon_changed(lat,lon);
}

void
GPlatesQtWidgets::HellingerDialog::handle_pole_estimate_13_angle_changed(
		double angle)
{
	d_pole_estimate_13_angle = angle;
	Q_EMIT pole_estimate_13_angle_changed(angle);
}



void
GPlatesQtWidgets::HellingerDialog::update_pole_estimates(
		const GPlatesMaths::PointOnSphere &point_12,
		double &angle_12,
		const GPlatesMaths::PointOnSphere &point_13,
		double &angle_13)
{
	angle_12 = std::abs(angle_12);
	angle_13 = std::abs(angle_13);

	GPlatesMaths::LatLonPoint llp_12 = GPlatesMaths::make_lat_lon_point(point_12);
	GPlatesMaths::LatLonPoint llp_13 = GPlatesMaths::make_lat_lon_point(point_13);

	d_pole_estimate_12_llp = llp_12;
	d_pole_estimate_13_llp = llp_13;
	d_pole_estimate_12_angle = angle_12;
	d_pole_estimate_13_angle = angle_13;

	HellingerPoleEstimate estimate_12(llp_12.latitude(),llp_12.longitude(),angle_12);
	HellingerPoleEstimate estimate_13(llp_13.latitude(),llp_13.longitude(),angle_13);

	enable_pole_estimate_signals(false);

	d_fit_widget->set_estimate_12(estimate_12);
	d_fit_widget->set_estimate_13(estimate_13);

	enable_pole_estimate_signals(true);
}

void
GPlatesQtWidgets::HellingerDialog::set_state_for_pole_adjustment_tool(
		bool pole_adjustment_tool_is_active)
{
	set_adjust_pole_tool_is_active(pole_adjustment_tool_is_active);
	enable_pole_estimate_widgets(pole_adjustment_tool_is_active);
	set_layer_state_for_active_pole_tool(pole_adjustment_tool_is_active);
	d_pick_widget->update_buttons();

	// If we have just deactivated the pole adjustment tool, update our estimates on canvas.
	if (!pole_adjustment_tool_is_active)
	{
		update_estimates_on_canvas();
	}
}

const GPlatesQtWidgets::HellingerFitType &
GPlatesQtWidgets::HellingerDialog::get_fit_type()
{
	return d_hellinger_model.get_fit_type();
}


void
GPlatesQtWidgets::HellingerDialog::handle_calculate_uncertainties()
{
	switch(d_hellinger_model.get_fit_type())
	{
	case TWO_PLATE_FIT_TYPE:
		d_thread_type = TWO_WAY_UNCERTAINTY_THREAD_TYPE;
		break;
	case THREE_PLATE_FIT_TYPE:
		d_thread_type = THREE_WAY_UNCERTAINTY_THREAD_TYPE;
		break;
	default:
		return;
	}

	check_output_file_root(line_edit_output_file_root);

	d_hellinger_model.set_output_file_root(
				line_edit_output_file_root->text());

	d_hellinger_thread->initialise(
				d_python_file,
				d_output_file_path,
				line_edit_output_file_root->text(),
				d_path_for_temporary_files);

	QFile python_code(d_python_file);
	if (python_code.exists())
	{
		d_hellinger_model.clear_uncertainty_results();
		update_canvas();
		d_fit_widget->start_progress_bar();
		d_hellinger_thread->set_python_script_type(d_thread_type);
		qDebug() << d_hellinger_thread->path();
		d_hellinger_thread->start();
	}
	else
	{
		QString message;
		QTextStream(&message) << tr("The Hellinger python scripts could not be found.");
		QMessageBox::critical(this,tr("Python scripts not found"),message,QMessageBox::Ok,QMessageBox::Ok);
		qWarning() << message;
	}

}

void
GPlatesQtWidgets::HellingerDialog::handle_export_pick_file()
{
	QString file_name = QFileDialog::getSaveFileName(this, tr("Save File"), "",
													 tr("Hellinger Pick Files (*.pick);"));
	
	if (!file_name.isEmpty())
	{
		GPlatesFileIO::HellingerWriter::write_pick_file(
					file_name,d_hellinger_model,
					true /*export disabled poles */,
					true /*add missing pick extension*/);
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_export_com_file()
{
	QString file_name = QFileDialog::getSaveFileName(this,tr("Save settings file"),"",
													 tr("Hellinger .com files (*.com);"));

	if (!file_name.isEmpty())
	{
		// Update Hellinger model with data from UI
		update_model_from_file_related_data();
		d_fit_widget->update_model_from_fit_widgets();

		GPlatesFileIO::HellingerWriter::write_com_file(file_name,d_hellinger_model);
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_show_details()
{

	QString results_file_name = create_results_filename(
				d_output_file_path,
				line_edit_output_file_root->text());

	qDebug() << "results filename: " << results_file_name;

	if (!d_hellinger_stats_dialog)
	{
		d_hellinger_stats_dialog = new GPlatesQtWidgets::HellingerStatsDialog(
					results_file_name,
					this);
	}
	d_hellinger_stats_dialog->update();
	d_hellinger_stats_dialog->show();

}

void
GPlatesQtWidgets::HellingerDialog::handle_add_new_pick()
{
	d_canvas_operation_type = NEW_POINT_OPERATION;

	Q_EMIT begin_new_pick();

	d_editing_layer_ptr->set_active(true);
	d_feature_highlight_layer_ptr->set_active(true);

	if (boost::optional<unsigned int> segment = d_pick_widget->selected_segment())
	{
		d_hellinger_new_point_dialog->update_segment_number(*segment);
	}

	this->setEnabled(false);
	d_hellinger_new_point_dialog->begin_pick_operation();
}

void
GPlatesQtWidgets::HellingerDialog::handle_add_new_segment()
{
	d_canvas_operation_type = NEW_SEGMENT_OPERATION;

	d_editing_layer_ptr->set_active(true);

	this->setEnabled(false);
	d_hellinger_new_segment_dialog->begin_segment_operation();
}

void
GPlatesQtWidgets::HellingerDialog::handle_calculate_fit()
{
	if (!d_hellinger_model.segments_are_ordered())
	{
		QMessageBox message_box;
		message_box.setIcon(QMessageBox::Information);
		message_box.setWindowTitle(tr("Segment ordering"));
		message_box.setText(
					tr("The segments are not currently ordered. Press OK to reorder the segments and continue with the calculation."));
		message_box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
		message_box.setDefaultButton(QMessageBox::Ok);
		int ret = message_box.exec();

		if (ret == QMessageBox::Cancel)
		{
			return;
		}
		else
		{
			d_pick_widget->renumber_segments();
		}
	}

	check_output_file_root(line_edit_output_file_root);

	d_fit_widget->update_model_from_fit_widgets();

	d_hellinger_model.set_output_file_root(
				line_edit_output_file_root->text());

	d_hellinger_thread->initialise(
				d_python_file,
				d_output_file_path,
				line_edit_output_file_root->text(),
				d_path_for_temporary_files);

	QFile python_code(d_python_file);
	if (python_code.exists())
	{
		d_hellinger_model.clear_fit_results();
		d_hellinger_model.clear_uncertainty_results();
		update_canvas();

		// Export the picks in the model to file. The python scripts will
		// read the picks from that file and perform the fit on them.
		QString pick_filename = d_path_for_temporary_files + d_hellinger_thread->temp_pick_filename();
		GPlatesFileIO::HellingerWriter::write_pick_file(pick_filename,d_hellinger_model,false);

		switch(d_hellinger_model.get_fit_type())
		{
		case TWO_PLATE_FIT_TYPE:
			d_thread_type = TWO_WAY_POLE_THREAD_TYPE;
			break;
		case THREE_PLATE_FIT_TYPE:
			d_thread_type = THREE_WAY_POLE_THREAD_TYPE;
			break;
		}
		d_hellinger_thread->set_python_script_type(d_thread_type);

		d_fit_widget->start_progress_bar();
		d_hellinger_thread->start();
	}
	else
	{
		QString message;
		QTextStream(&message) << tr("The Hellinger python scripts could not be found.");
		QMessageBox::critical(this,tr("Python scripts not found"),message,QMessageBox::Ok,QMessageBox::Ok);
		qWarning() << message;
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_thread_finished()
{
	d_fit_widget->stop_progress_bar();
	if (d_hellinger_thread->thread_failed())
	{
		return;
	}
	try{
		if ((d_thread_type == TWO_WAY_POLE_THREAD_TYPE ||
			 d_thread_type == THREE_WAY_POLE_THREAD_TYPE))
		{
			QString path = d_path_for_temporary_files + d_hellinger_thread->temp_result_filename();
			GPlatesFileIO::HellingerReader::read_fit_results_from_temporary_fit_file(path,d_hellinger_model);
			d_fit_widget->update_after_pole_result();
			update_results_on_canvas();
		}
		else if(d_thread_type == TWO_WAY_UNCERTAINTY_THREAD_TYPE)
		{
			QString filename = d_output_file_path + QDir::separator() + d_hellinger_model.error_ellipse_filename();
			GPlatesFileIO::HellingerReader::read_error_ellipse(
						filename,d_hellinger_model);
			update_results_on_canvas();
		}
		else if (d_thread_type == THREE_WAY_UNCERTAINTY_THREAD_TYPE)
		{
			QString filename= d_output_file_path + QDir::separator() + d_hellinger_model.error_ellipse_filename(PLATES_1_2_PAIR_TYPE);
			GPlatesFileIO::HellingerReader::read_error_ellipse(
						filename,d_hellinger_model,PLATES_1_2_PAIR_TYPE);

			filename= d_output_file_path + QDir::separator() + d_hellinger_model.error_ellipse_filename(PLATES_1_3_PAIR_TYPE);
			GPlatesFileIO::HellingerReader::read_error_ellipse(
						filename,d_hellinger_model,PLATES_1_3_PAIR_TYPE);

			filename= d_output_file_path + QDir::separator() + d_hellinger_model.error_ellipse_filename(PLATES_2_3_PAIR_TYPE);
			GPlatesFileIO::HellingerReader::read_error_ellipse(
						filename,d_hellinger_model,PLATES_2_3_PAIR_TYPE);

			update_results_on_canvas();
		}
	}
	catch(...)
	{
		qWarning() << "There was a problem opening or reading a Hellinger results file.";
	}
}

void
GPlatesQtWidgets::HellingerDialog::update_widgets_from_model()
{
	d_pick_layer_ptr->set_active(true);
	d_pick_widget->update_from_model(true /* expand segments */);
	d_fit_widget->update_fit_widgets_from_model();
	d_fit_widget->update_enabled_state_of_estimate_widgets(d_adjust_pole_tool_is_active);


	bool three_plate_fit = d_three_way_fitting_is_enabled &&
							d_hellinger_model.get_fit_type(true) == THREE_PLATE_FIT_TYPE;

	radio_button_fixed_3->setEnabled(three_plate_fit);

	if (!three_plate_fit && radio_button_fixed_3->isChecked())
	{
		radio_button_fixed_1->setChecked(true);
	}
}

void
GPlatesQtWidgets::HellingerDialog::draw_pole_result(
		const double &lat,
		const double &lon,
		const HellingerConfigurationWidget::HellingerColour &colour)
{
	GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
				GPlatesMaths::LatLonPoint(lat,lon));

	GPlatesViewOperations::RenderedGeometry pole_geometry_arrow =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_radial_arrow(
				point,
				d_configuration.d_pole_arrow_height/*arrow_projected_length*/,
				d_configuration.d_pole_arrow_radius/*arrowhead_projected_size*/,
				0.5f/*ratio_arrowline_width_to_arrowhead_size*/,
				HellingerConfigurationWidget::get_colour_from_hellinger_colour(
					colour),
				GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_CROSS/*symbol_type*/,
				10.0f/*symbol_size*/,
				HellingerConfigurationWidget::get_colour_from_hellinger_colour(
					colour)/*symbol_colour*/);

	d_result_layer_ptr->add_rendered_geometry(pole_geometry_arrow);
}

void
GPlatesQtWidgets::HellingerDialog::update_results_on_canvas()
{
	boost::optional<GPlatesQtWidgets::HellingerFitStructure> fit_12 = d_hellinger_model.get_fit_12();
	boost::optional<GPlatesQtWidgets::HellingerFitStructure> fit_13 = d_hellinger_model.get_fit_13();
	boost::optional<GPlatesQtWidgets::HellingerFitStructure> fit_23 = d_hellinger_model.get_fit_23();
	d_result_layer_ptr->clear_rendered_geometries();
	if (fit_12 && d_fit_widget->show_result_12_checked())
	{
		draw_pole_result(fit_12.get().d_lat, fit_12.get().d_lon,
						 d_configuration.d_best_fit_pole_colour);
		draw_error_ellipse(PLATES_1_2_PAIR_TYPE);
	}
	if (fit_13 && d_fit_widget->show_result_13_checked())
	{
		draw_pole_result(fit_13.get().d_lat, fit_13.get().d_lon,d_configuration.d_best_fit_pole_colour);
		draw_error_ellipse(PLATES_1_3_PAIR_TYPE);
	}
	if (fit_23 && d_fit_widget->show_result_23_checked())
	{
		draw_pole_result(fit_23.get().d_lat, fit_23.get().d_lon,d_configuration.d_best_fit_pole_colour);
		draw_error_ellipse(PLATES_2_3_PAIR_TYPE);
	}

}

void
GPlatesQtWidgets::HellingerDialog::update_estimates_on_canvas()
{
	bool three_plate_fit = d_hellinger_model.get_fit_type() == THREE_PLATE_FIT_TYPE;

	HellingerPoleEstimate estimate_12 = d_fit_widget->estimate_12();
	HellingerPoleEstimate estimate_13 = d_fit_widget->estimate_13();

	d_pole_estimate_layer_ptr->clear_rendered_geometries();

	if (d_fit_widget->show_estimate_12_checked())
	{
		draw_pole_estimate(estimate_12, d_configuration.d_initial_estimate_pole_colour);
	}

	if (d_fit_widget->show_estimate_13_checked() && three_plate_fit)
	{
		draw_pole_estimate(estimate_13, d_configuration.d_initial_estimate_pole_colour);
	}
}

void
GPlatesQtWidgets::HellingerDialog::draw_error_ellipse(
		const HellingerPlatePairType &type)
{
	const std::vector<GPlatesMaths::LatLonPoint> &data_points = d_hellinger_model.error_ellipse_points(type);
	std::vector<GPlatesMaths::PointOnSphere> ellipse_points;
	if (!data_points.empty())
	{
		BOOST_FOREACH(GPlatesMaths::LatLonPoint llp,data_points)
		{
			ellipse_points.push_back(GPlatesMaths::make_point_on_sphere(llp));
		}

		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type ellipse_geometry_on_sphere = GPlatesMaths::PolylineOnSphere::create_on_heap(ellipse_points);
		GPlatesViewOperations::RenderedGeometry ellipse_rg = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					ellipse_geometry_on_sphere,
					HellingerConfigurationWidget::get_colour_from_hellinger_colour(
						d_configuration.d_ellipse_colour),
					0, /* point thickness, not used here */
					d_configuration.d_ellipse_line_thickness, /* line thickness */
					false, /* fill polygon */
					false, /* fill polyline */
					GPlatesGui::Colour::get_white()); /* fill colour, not used */


		d_result_layer_ptr->add_rendered_geometry(ellipse_rg);
	}

}

void
GPlatesQtWidgets::HellingerDialog::create_feature_collection()
{

}

void
GPlatesQtWidgets::HellingerDialog::handle_close()
{
	d_rendered_geom_collection_ptr->set_main_layer_active(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER,
				false);
	d_pick_widget->handle_close();
}



void
GPlatesQtWidgets::HellingerDialog::update_canvas()
{
	clear_rendered_geometries();
	draw_picks();
	update_estimates_on_canvas();
	update_results_on_canvas();
	draw_error_ellipse();
	update_selected_geometries();
}


void
GPlatesQtWidgets::HellingerDialog::update_edit_layer(
	const GPlatesMaths::PointOnSphere &pos_)
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos_);
	if (d_canvas_operation_type == EDIT_POINT_OPERATION)
	{
		d_hellinger_edit_point_dialog->update_pick_coords(llp);
	}
	if (d_canvas_operation_type == NEW_POINT_OPERATION)
	{
		d_hellinger_new_point_dialog->update_pick_coords(llp);
	}
	if (d_canvas_operation_type == NEW_SEGMENT_OPERATION)
	{
		d_hellinger_new_segment_dialog->update_pick_coords(llp);
	}

}

void GPlatesQtWidgets::HellingerDialog::set_enlarged_edit_geometry(bool enlarged)
{
	d_edit_point_is_enlarged = enlarged;
	handle_update_point_editing();
}


void GPlatesQtWidgets::HellingerDialog::set_feature_highlight(
		const GPlatesMaths::PointOnSphere &point)
{
	GPlatesViewOperations::RenderedGeometry highlight_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				point.get_non_null_pointer(),
				GPlatesGui::Colour::get_yellow(),
				ENLARGED_POINT_SIZE,
				DEFAULT_LINE_THICKNESS,
				false, /* fill polygon */
				false /* fill polyline */ );

	d_feature_highlight_layer_ptr->add_rendered_geometry(highlight_geometry);
}

void
GPlatesQtWidgets::HellingerDialog::update_after_new_or_edited_pick(
		const hellinger_model_type::const_iterator &it,
		const int segment_number)
{
	d_pick_widget->update_after_new_or_edited_pick(it,segment_number);
	enable_pole_estimate_widgets(d_adjust_pole_tool_is_active);
	update_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::update_after_new_or_edited_segment(
		const int segment_number)
{
	d_pick_widget->update_after_new_or_edited_segment(segment_number);
	enable_pole_estimate_widgets(d_adjust_pole_tool_is_active);
	update_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::enable_pole_estimate_widgets(
		bool enable)
{
	d_fit_widget->update_enabled_state_of_estimate_widgets(enable);
}

void
GPlatesQtWidgets::HellingerDialog::set_layer_state_for_active_pole_tool(
		bool pole_tool_is_active)
{
	d_pole_estimate_layer_ptr->set_active(!pole_tool_is_active);
}

const GPlatesMaths::LatLonPoint &
GPlatesQtWidgets::HellingerDialog::get_pole_estimate_12_lat_lon()
{
	return d_pole_estimate_12_llp;
}

const double &GPlatesQtWidgets::HellingerDialog::get_pole_estimate_12_angle()
{
	return d_pole_estimate_12_angle;
}

const GPlatesMaths::LatLonPoint &
GPlatesQtWidgets::HellingerDialog::get_pole_estimate_13_lat_lon()
{
	return d_pole_estimate_13_llp;
}

const double &GPlatesQtWidgets::HellingerDialog::get_pole_estimate_13_angle()
{
	return d_pole_estimate_13_angle;
}

void GPlatesQtWidgets::HellingerDialog::update_selected_geometries()
{
	boost::optional<hellinger_model_type::const_iterator> selected_pick
			= d_pick_widget->selected_pick();
	boost::optional<unsigned int> selected_segment
			= d_pick_widget->selected_segment();
	if (selected_pick)
	{
		highlight_selected_pick((*selected_pick)->second);
	}
	else if (selected_segment)
	{
		highlight_selected_segment(*selected_segment);
	}
}

void
GPlatesQtWidgets::HellingerDialog::draw_picks_of_plate_index(
		const HellingerPlateIndex &plate_index)
{
	hellinger_model_type::const_iterator it = d_hellinger_model.begin();

	int num_segment = 0;
	int num_colour = 0;

	for (; it != d_hellinger_model.end(); ++it)
	{
		if (it->second.d_is_enabled)
		{
			if (num_segment != it->first)
			{
				++num_colour;
				++num_segment;
			}

			if (it->second.d_segment_type == plate_index)
			{
				GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(it->second.d_lat,it->second.d_lon));

				GPlatesViewOperations::RenderedGeometry pick_geometry =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
							point.get_non_null_pointer(),
							get_segment_colour(num_colour),
							2, /* point thickness */
							2, /* line thickness */
							false, /* fill polygon */
							false, /* fill polyline */
							GPlatesGui::Colour::get_white(), // dummy colour argument
							get_pick_symbol(plate_index));

				d_pick_layer_ptr->add_rendered_geometry(pick_geometry);
			}
		}
	}
}


void GPlatesQtWidgets::HellingerDialog::draw_picks()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_pick_layer_ptr->clear_rendered_geometries();

	hellinger_model_type::const_iterator it = d_hellinger_model.begin();
	int num_segment = 0;
	int num_colour = 0;
	d_geometry_to_model_map.clear();
	for (; it != d_hellinger_model.end(); ++it)
	{
		if (it->second.d_is_enabled)
		{
			if (num_segment != it->first)
			{
				++num_colour;
				++num_segment;
			}

			add_pick_geometry_to_layer(it->second,
									   d_pick_layer_ptr,
									   get_segment_colour(num_colour),
									   false /* don't use enlarged symbol */);

			d_geometry_to_model_map.push_back(it);
		}
	}
}

void GPlatesQtWidgets::HellingerDialog::draw_pole_estimate(
		const HellingerPoleEstimate &estimate,
		const HellingerConfigurationWidget::HellingerColour &colour)
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	GPlatesMaths::LatLonPoint llp(estimate.d_lat, estimate.d_lon);
	GPlatesMaths::PointOnSphere pole = GPlatesMaths::make_point_on_sphere(
				llp);

	const GPlatesViewOperations::RenderedGeometry pole_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_radial_arrow(
				pole,
				d_configuration.d_pole_arrow_height/*arrow_projected_length*/,
				d_configuration.d_pole_arrow_radius/*arrowhead_projected_size*/,
				0.5f/*ratio_arrowline_width_to_arrowhead_size*/,
				HellingerConfigurationWidget::get_colour_from_hellinger_colour(
					colour) /*arrow_colour*/,
				GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_CROSS/*symbol_type*/,
				5.0f/*symbol_size*/,
				HellingerConfigurationWidget::get_colour_from_hellinger_colour(
					colour)/*symbol_colour*/);

	d_pole_estimate_layer_ptr->add_rendered_geometry(pole_geometry);

}

void GPlatesQtWidgets::HellingerDialog::hide_child_dialogs()
{
	if (d_hellinger_stats_dialog)
	{
		d_hellinger_stats_dialog->hide();
	}
	d_hellinger_edit_point_dialog->hide();
	d_hellinger_new_point_dialog->hide();
	d_hellinger_edit_segment_dialog->hide();
	d_hellinger_new_segment_dialog->hide();
}

void
GPlatesQtWidgets::HellingerDialog::update_chron_time()
{
	boost::optional<double> chron_time =
			try_to_find_chron_time(
				d_hellinger_model.get_chron_string(),
				d_view_state.get_application_state().get_age_model_collection());

	if (chron_time)
	{
		d_chron_time = *chron_time;
	}

	spinbox_chron->setValue(d_chron_time);

	// If the chron time has changed, we'll want to update the oldest possible time on
	// the reconstruction slider, and this may also require an update of the canvas.
	handle_chron_time_changed(d_chron_time);
}

void
GPlatesQtWidgets::HellingerDialog::update_model_from_file_related_data()
{
	d_hellinger_model.set_input_pick_filename(line_import_file->text());
}

void
GPlatesQtWidgets::HellingerDialog::enable_pole_estimate_signals(bool enable)
{
	if (enable)
	{
		QObject::connect(d_fit_widget,SIGNAL(pole_estimate_12_changed(double, double)),this,SLOT(handle_pole_estimate_12_changed(double, double)));
		QObject::connect(d_fit_widget,SIGNAL(pole_estimate_13_changed(double, double)),this,SLOT(handle_pole_estimate_13_changed(double, double)));
		QObject::connect(d_fit_widget,SIGNAL(pole_estimate_12_angle_changed(double)),this,SLOT(handle_pole_estimate_12_angle_changed(double)));
		QObject::connect(d_fit_widget,SIGNAL(pole_estimate_13_angle_changed(double)),this,SLOT(handle_pole_estimate_13_angle_changed(double)));
	}
	else
	{
		QObject::disconnect(d_fit_widget,SIGNAL(pole_estimate_12_changed(double, double)),this,SLOT(handle_pole_estimate_12_changed(double, double)));
		QObject::disconnect(d_fit_widget,SIGNAL(pole_estimate_13_changed(double, double)),this,SLOT(handle_pole_estimate_13_changed(double, double)));
		QObject::disconnect(d_fit_widget,SIGNAL(pole_estimate_12_angle_changed(double)),this,SLOT(handle_pole_estimate_12_angle_changed(double)));
		QObject::disconnect(d_fit_widget,SIGNAL(pole_estimate_13_angle_changed(double)),this,SLOT(handle_pole_estimate_13_angle_changed(double)));
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_show_estimate_checkboxes_clicked()
{
	update_estimates_on_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::handle_show_result_checkboxes_clicked()
{
	update_results_on_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::reconstruct_picks()
{

	clear_pick_geometries();

	HellingerPlateIndex fixed = static_cast<HellingerPlateIndex>(button_group_fixed_plate->checkedId());

	// moving_a and moving_b are the indices of the two moving plate types.
	HellingerPlateIndex moving_a = get_moving_plate_indices(static_cast<HellingerPlateIndex>(button_group_fixed_plate->checkedId())).first;
	HellingerPlateIndex moving_b = get_moving_plate_indices(static_cast<HellingerPlateIndex>(button_group_fixed_plate->checkedId())).second;

	// Draw the fixed plate picks.
	draw_picks_of_plate_index(fixed);

	// fit_a and fit_b are the best-fit rotation poles for the two moving plates w.r.t the fixed plate.
	boost::optional<GPlatesQtWidgets::HellingerFitStructure> fit_a;
	boost::optional<GPlatesQtWidgets::HellingerFitStructure> fit_b;


	switch(fixed)
	{
	case PLATE_ONE_PICK_TYPE:
		fit_a = d_hellinger_model.get_fit_12();
		fit_b = d_hellinger_model.get_fit_13();
		break;
	case PLATE_TWO_PICK_TYPE:
		fit_a = d_hellinger_model.get_fit_12();
		fit_b = d_hellinger_model.get_fit_23();
		break;
	case PLATE_THREE_PICK_TYPE:
		fit_a = d_hellinger_model.get_fit_13();
		fit_b = d_hellinger_model.get_fit_23();
		break;
	default:
		fit_a = d_hellinger_model.get_fit_12();
		fit_b = d_hellinger_model.get_fit_13();
	}

	correct_directions_of_rotations(fit_a,fit_b,fixed);

	double recon_time = spinbox_recon_time->value();
	double time_fraction = recon_time/spinbox_chron->value();

	boost::optional<GPlatesMaths::FiniteRotation> rot_a = get_rotation(fit_a,time_fraction);
	boost::optional<GPlatesMaths::FiniteRotation> rot_b = get_rotation(fit_b,time_fraction);

	if (recon_time > 0 )
	{
		hellinger_model_type::const_iterator it = d_hellinger_model.begin();

		int num_segment = 0;
		int num_colour = 0;
		for (; it != d_hellinger_model.end(); ++it)
		{
			if (it->second.d_is_enabled)
			{
				if (num_segment != it->first)
				{
					++num_colour;
					++num_segment;
				}

				if ((it->second.d_segment_type == moving_a) ||
						(it->second.d_segment_type == moving_b))
				{
					GPlatesMaths::PointOnSphere point = make_point_on_sphere(
								GPlatesMaths::LatLonPoint(it->second.d_lat,it->second.d_lon));
					if ((it->second.d_segment_type == moving_a) && rot_a)
					{
						point = (*rot_a)*point;
					}
						else if((it->second.d_segment_type == moving_b) && rot_b)
					{
						point = (*rot_b)*point;
					}

					GPlatesViewOperations::RenderedGeometry pick_geometry =
							GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
								point.get_non_null_pointer(),
								get_segment_colour(num_colour),
								2, /* point thickness */
								2, /* line thickness */
								false, /* fill polygon */
								false, /* fill polyline */
								GPlatesGui::Colour::get_white(), // dummy colour argument
								get_pick_symbol(it->second.d_segment_type));

					d_pick_layer_ptr->add_rendered_geometry(pick_geometry);
				}
			}
		}
	}
	else
	{
		draw_picks_of_plate_index(moving_a);
		draw_picks_of_plate_index(moving_b);
	}


}

void
GPlatesQtWidgets::HellingerDialog::handle_chron_time_changed(
		const double &time)
{
	d_chron_time = spinbox_chron->value();
	slider_recon_time->setMinimum(d_chron_time*SLIDER_MULTIPLIER);
	slider_recon_time->setMaximum(0.);

	spinbox_recon_time->setMaximum(time);

	if (d_recon_time > d_chron_time)
	{
		d_recon_time = d_chron_time;

	}

	reconstruct_picks();

}

void
GPlatesQtWidgets::HellingerDialog::handle_recon_time_spinbox_changed(
		const double &time)
{
	slider_recon_time->setValue(SLIDER_MULTIPLIER*time);

	reconstruct_picks();
}

void
GPlatesQtWidgets::HellingerDialog::handle_recon_time_slider_changed(
		const int &value)
{
	spinbox_recon_time->setValue(static_cast<double>(value)/SLIDER_MULTIPLIER);
}

void GPlatesQtWidgets::HellingerDialog::set_up_connections()
{
	QObject::connect(button_close, SIGNAL(rejected()), this, SLOT(handle_close()));

	// Connections related to file io
	QObject::connect(button_import_file, SIGNAL(clicked()), this, SLOT(handle_import_hellinger_file()));
	QObject::connect(button_export_pick_file, SIGNAL(clicked()), this, SLOT(handle_export_pick_file()));
	QObject::connect(button_export_com_file, SIGNAL(clicked()), this, SLOT(handle_export_com_file()));
	QObject::connect(button_output_path,SIGNAL(clicked()),this,SLOT(handle_output_path_button_clicked()));
	QObject::connect(line_edit_output_path,SIGNAL(editingFinished()),this,SLOT(handle_output_path_editing_finished()));
	QObject::connect(line_edit_output_path,SIGNAL(textChanged(const QString&)),this,SLOT(handle_output_path_changed()));

	// Connections related to fit widget
	QObject::connect(d_fit_widget,SIGNAL(pole_estimate_12_changed(double, double)),this,SLOT(handle_pole_estimate_12_changed(double, double)));
	QObject::connect(d_fit_widget,SIGNAL(pole_estimate_13_changed(double, double)),this,SLOT(handle_pole_estimate_13_changed(double, double)));
	QObject::connect(d_fit_widget,SIGNAL(pole_estimate_12_angle_changed(double)),this,SLOT(handle_pole_estimate_12_angle_changed(double)));
	QObject::connect(d_fit_widget,SIGNAL(pole_estimate_13_angle_changed(double)),this,SLOT(handle_pole_estimate_13_angle_changed(double)));
	QObject::connect(d_fit_widget,SIGNAL(show_result_checkboxes_clicked()),this,SLOT(handle_show_result_checkboxes_clicked()));
	QObject::connect(d_fit_widget,SIGNAL(show_estimate_checkboxes_clicked()),this,SLOT(handle_show_estimate_checkboxes_clicked()));

	// Connections related to pick wiget
	QObject::connect(d_pick_widget,SIGNAL(edit_pick_signal()),this,SLOT(handle_edit_pick()));
	QObject::connect(d_pick_widget,SIGNAL(add_new_pick_signal()),this,SLOT(handle_add_new_pick()));
	QObject::connect(d_pick_widget,SIGNAL(edit_segment_signal()),this,SLOT(handle_edit_segment()));
	QObject::connect(d_pick_widget,SIGNAL(add_new_segment_signal()),this,SLOT(handle_add_new_segment()));
	QObject::connect(d_pick_widget,SIGNAL(tree_updated_signal()),this,SLOT(handle_pick_dialog_updated()));


	// Connections related to the resultant pole.
	QObject::connect(spinbox_chron, SIGNAL(valueChanged(double)), this, SLOT(handle_chron_time_changed(double)));
	QObject::connect(spinbox_recon_time, SIGNAL(valueChanged(double)), this, SLOT(handle_recon_time_spinbox_changed(double)));
	QObject::connect(slider_recon_time, SIGNAL(valueChanged(int)), this, SLOT(handle_recon_time_slider_changed(int)));


	// Connections related to the python threads.
	QObject::connect(d_hellinger_thread, SIGNAL(finished()),this, SLOT(handle_thread_finished()));

	// Connections related to child dialogs.
	QObject::connect(d_hellinger_edit_point_dialog,SIGNAL(finished_editing()),this,SLOT(handle_finished_editing()));
	QObject::connect(d_hellinger_new_point_dialog,SIGNAL(finished_editing()),this,SLOT(handle_finished_editing()));

	QObject::connect(d_hellinger_edit_point_dialog,SIGNAL(update_editing()),this,SLOT(handle_update_point_editing()));
	QObject::connect(d_hellinger_new_point_dialog,SIGNAL(update_editing()),this,SLOT(handle_update_point_editing()));

	QObject::connect(d_hellinger_edit_segment_dialog,SIGNAL(finished_editing()),this,SLOT(handle_finished_editing()));
	QObject::connect(d_hellinger_new_segment_dialog,SIGNAL(finished_editing()),this,SLOT(handle_finished_editing()));

	// Connection to the app-state's AgeModelCollection, so that we are notified of changes to the active model.
	QObject::connect(&d_view_state.get_application_state().get_age_model_collection(),
					 SIGNAL(active_age_model_changed()),
					 this,
					 SLOT(handle_active_age_model_changed()));

	// Connection to the configuration dialog
	QObject::connect(button_settings,SIGNAL(clicked()),this,SLOT(handle_settings_clicked()));
	QObject::connect(d_configuration_dialog,SIGNAL(configuration_changed()),this,SLOT(handle_configuration_changed()));

	// Connections relating to the tab widget
	QObject::connect(tab_widget,SIGNAL(currentChanged(int)),this,SLOT(handle_tab_changed(int)));

}

void
GPlatesQtWidgets::HellingerDialog::set_up_child_layers()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Create a rendered layer to draw the picks.
	d_pick_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw resultant pole and reconstructed picks
	d_result_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);


	// Create a rendered layer to draw selected geometries
	d_selection_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw highlighted geometries
	d_hover_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw geometries undergoing editing.
	d_editing_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to highlight feature geometries which can be selected.
	d_feature_highlight_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw the pole estimate(s).
	d_pole_estimate_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	activate_layers(true);
}

void
GPlatesQtWidgets::HellingerDialog::activate_layers(bool activate)
{
	d_pick_layer_ptr->set_active(activate);
	d_hover_layer_ptr->set_active(activate);
	d_result_layer_ptr->set_active(activate);
	d_selection_layer_ptr->set_active(activate);
	d_pole_estimate_layer_ptr->set_active(activate);
}

void
GPlatesQtWidgets::HellingerDialog::clear_rendered_geometries()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_pick_layer_ptr->clear_rendered_geometries();
	d_hover_layer_ptr->clear_rendered_geometries();
	d_result_layer_ptr->clear_rendered_geometries();
	d_selection_layer_ptr->clear_rendered_geometries();
	d_editing_layer_ptr->clear_rendered_geometries();
	d_pole_estimate_layer_ptr->clear_rendered_geometries();
}

void GPlatesQtWidgets::HellingerDialog::clear_pick_geometries()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_pick_layer_ptr->clear_rendered_geometries();
	d_hover_layer_ptr->clear_rendered_geometries();
	d_selection_layer_ptr->clear_rendered_geometries();
	d_editing_layer_ptr->clear_rendered_geometries();
}

void
GPlatesQtWidgets::HellingerDialog::close()
{
	handle_close();
	GPlatesDialog::hide();
}

void
GPlatesQtWidgets::HellingerDialog::hide()
{
	hide_child_dialogs();
	GPlatesDialog::hide();
}

void
GPlatesQtWidgets::HellingerDialog::keyPressEvent(
		QKeyEvent *event_)
{
	// See comments in initialise_widgets() regarding presence (or not) of close button and
	// other behaviour of this dialog. This function was implemented as an empty function to prevent
	// the Escape key from closing the dialog, as we wanted the dialog behaviour to be closer to
	// the other workflow task panels. But right now we're allowing the dialog to close - either
	// via the close button or by the Escape key - hence we propagate this event -to the parent.
	GPlatesDialog::keyPressEvent(event_);
}

void
GPlatesQtWidgets::HellingerDialog::check_and_highlight_output_path()
{
	QString path = line_edit_output_path->text();
	QFileInfo file_info(path);
	if (file_info.exists() &&
			file_info.isDir() &&
			file_info.isWritable())
	{
		line_edit_output_path->setPalette(QPalette());
		d_output_file_path = path;
		d_output_path_is_valid = true;
	}
	else
	{
		static QPalette red_palette;
		red_palette.setColor(QPalette::Active, QPalette::Base, Qt::red);
		line_edit_output_path->setPalette(red_palette);
		d_output_path_is_valid = false;
	}
}

GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
GPlatesQtWidgets::HellingerDialog::get_pick_layer()
{
	return d_pick_layer_ptr;
}

GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
GPlatesQtWidgets::HellingerDialog::get_editing_layer()
{
	return d_editing_layer_ptr;
}

GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
GPlatesQtWidgets::HellingerDialog::get_feature_highlight_layer()
{
	return d_feature_highlight_layer_ptr;
}

GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
GPlatesQtWidgets::HellingerDialog::get_pole_estimate_layer()
{
	return d_pole_estimate_layer_ptr;
}

void GPlatesQtWidgets::HellingerDialog::set_hovered_pick(
		const unsigned int index)
{

	if (index > d_geometry_to_model_map.size())
	{
		return;
	}

	hellinger_model_type::const_iterator it = d_geometry_to_model_map[index];
	const HellingerPick &pick = it->second;

	d_hover_layer_ptr->clear_rendered_geometries();

	add_pick_geometry_to_layer(pick,d_hover_layer_ptr,GPlatesGui::Colour::get_silver());

	d_pick_widget->update_hovered_item(index,pick.d_is_enabled);
}

void GPlatesQtWidgets::HellingerDialog::set_selected_pick(
		const unsigned int index)
{
	d_pick_widget->set_selected_pick_from_geometry_index(index);
}

void GPlatesQtWidgets::HellingerDialog::clear_hovered_layer_and_table()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_hover_layer_ptr->clear_rendered_geometries();
	d_pick_widget->clear_hovered_item();
}

void GPlatesQtWidgets::HellingerDialog::clear_selection_layer()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_selection_layer_ptr->clear_rendered_geometries();
}

void GPlatesQtWidgets::HellingerDialog::clear_editing_layer()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_editing_layer_ptr->clear_rendered_geometries();
}

void GPlatesQtWidgets::HellingerDialog::clear_feature_highlight_layer()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_feature_highlight_layer_ptr->clear_rendered_geometries();
}

void GPlatesQtWidgets::HellingerDialog::edit_current_pick()
{
	handle_edit_pick();
}

