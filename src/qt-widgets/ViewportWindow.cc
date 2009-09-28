/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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
 
#include <iostream>
#include <iterator>
#include <memory>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

#include <QtGlobal>
#include <QFileDialog>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QHeaderView>
#include <QMessageBox>
#include <QColorDialog>
#include <QColor>
#include <QInputDialog>
#include <QProgressBar>
#include <QDockWidget>
#include <QActionGroup>
#include <QDebug>

#include "ViewportWindow.h"
#include "InformationDialog.h"
#include "ShapefilePropertyMapper.h"
#include "TaskPanel.h"
#include "ActionButtonBox.h"
#include "CreateFeatureDialog.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructTemplate.h"

#include "global/GPlatesException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"
#include "gui/AgeColourTable.h"
#include "gui/ChooseCanvasTool.h"
#include "gui/FeatureColourTable.h"
#include "gui/FeatureWeakRefSequence.h"
#include "gui/GlobeCanvasToolAdapter.h"
#include "gui/GlobeCanvasToolChoice.h"
#include "gui/MapCanvasToolAdapter.h"
#include "gui/MapCanvasToolChoice.h"
#include "gui/PlatesColourTable.h"
#include "gui/ProjectionException.h"
#include "gui/SingleColourTable.h"
#include "gui/SvgExport.h"
#include "gui/TopologySectionsContainer.h"
#include "gui/TopologySectionsTable.h"
#include "maths/PointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/InvalidLatLonException.h"
#include "maths/Real.h"
#include "model/Model.h"
#include "model/types.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/DummyTransactionHandle.h"
#include "model/ReconstructionGraph.h"
#include "model/ReconstructionTreePopulator.h"
#include "model/ReconstructionTree.h"

#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/PlatesLineFormatReader.h"
#include "file-io/PlatesRotationFormatReader.h"
#include "file-io/FileInfo.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/RasterReader.h"
#include "file-io/ShapefileReader.h"
#include "file-io/GpmlOnePointSixReader.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"
#include "view-operations/RenderReconstructionGeometries.h"
#include "view-operations/UndoRedo.h"
#include "feature-visitors/FeatureCollectionClassifier.h"
#include "feature-visitors/ComputationalMeshSolver.h"
#include "feature-visitors/TopologyResolver.h"
#include "qt-widgets/MapCanvas.h"

#include "canvas-tools/MeasureDistanceState.h"

void
GPlatesQtWidgets::ViewportWindow::save_file(
		const GPlatesFileIO::FileInfo &file_info,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	if ( ! GPlatesFileIO::is_writable(file_info)) {
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
				file_info.get_qfileinfo().filePath());
	}

	if ( ! file_info.get_feature_collection()) {
		throw GPlatesGlobal::UnexpectedEmptyFeatureCollectionException(GPLATES_EXCEPTION_SOURCE,
				"Attempted to write an empty feature collection.");
	}

	const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
			*file_info.get_feature_collection();

	if (!feature_collection.is_valid())
	{
		return;
	}

	boost::shared_ptr<GPlatesModel::ConstFeatureVisitor> feature_collection_writer =
			GPlatesFileIO::get_feature_collection_writer(
					file_info,
					feature_collection_write_format);

	// Write the feature collection.
	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection,*feature_collection_writer);

	feature_collection->set_contains_unsaved_changes(false);
}


void
GPlatesQtWidgets::ViewportWindow::save_file_as(
		const GPlatesFileIO::FileInfo &file_info,
		file_info_iterator features_to_save,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	GPlatesFileIO::FileInfo file_copy = save_file_copy(
		file_info,
		features_to_save,
		feature_collection_write_format);

	// Update iterator
	*features_to_save = file_copy;
}


GPlatesFileIO::FileInfo
GPlatesQtWidgets::ViewportWindow::save_file_copy(
		const GPlatesFileIO::FileInfo &file_info,
		file_info_iterator features_to_save,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	GPlatesFileIO::FileInfo file_copy(file_info.get_qfileinfo().filePath());
	if ( ! features_to_save->get_feature_collection()) {
		throw GPlatesGlobal::UnexpectedEmptyFeatureCollectionException(GPLATES_EXCEPTION_SOURCE,
				"Attempted to write an empty feature collection.");
	}
	file_copy.set_feature_collection(*(features_to_save->get_feature_collection()));
	save_file(file_copy, feature_collection_write_format);
	return file_copy;
}

void
GPlatesQtWidgets::ViewportWindow::load_files(
		const QStringList &file_names)
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	QStringList::const_iterator iter = file_names.begin();
	QStringList::const_iterator end = file_names.end();

	bool have_loaded_new_rotation_file = false;

	for ( ; iter != end; ++iter) {

		GPlatesFileIO::FileInfo file(*iter);

		try
		{

			switch ( GPlatesFileIO::get_feature_collection_file_format(*iter) )
			{
			case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
				GPlatesFileIO::ShapefileReader::set_property_mapper(
					boost::shared_ptr< ShapefilePropertyMapper >(
						new ShapefilePropertyMapper(this)));
				break;

			default:
				break;
			}
			// Read the feature collection from file.
			GPlatesFileIO::read_feature_collection_file(file, d_model, read_errors);

			switch ( GPlatesFileIO::get_feature_collection_file_format(file) )
			{
			case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
			case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
				{
					// All loaded files are added to the set of loaded files.
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

					// GPML format files can contain both reconstructable features and
					// reconstruction trees. This visitor lets us find out which.
					if (file.get_feature_collection()) {
						GPlatesFeatureVisitors::FeatureCollectionClassifier classifier;
						classifier.scan_feature_collection(
								GPlatesModel::FeatureCollectionHandle::get_const_weak_ref(
										*file.get_feature_collection())
								);
						// Check if the file contains reconstructable features.
						if (classifier.reconstructable_feature_count() > 0) {
							// I am very bad for putting this here - I'll clean it up when
							// ApplicationState notifies of load/unload events - John.
							GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
									*file.get_feature_collection();
							if (!d_plate_velocities.load_reconstructable_feature_collection(
									feature_collection, file, d_model))
							{
								// Only add the file if it's not a velocity cap file because
								// would like to render things its own way.
								// In the future this will be taken care of by Visual Layers.
								d_active_reconstructable_files.push_back(new_file);
							}

						}
						// Check if the file contains reconstruction features.
						if (classifier.reconstruction_feature_count() > 0) {
							// We only want to make the first rotation file active.
							if ( ! have_loaded_new_rotation_file) 
							{
								d_active_reconstruction_files.clear();
								d_active_reconstruction_files.push_back(new_file);

								// I am very bad for putting this here - I'll clean it up when
								// ApplicationState notifies of load/unload events - John.
								GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
										*file.get_feature_collection();
								d_plate_velocities.load_reconstruction_feature_collection(
										feature_collection);

								have_loaded_new_rotation_file = true;
							}
						}
					}
				}
				break;

			case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
				if (file.get_feature_collection())
				{
					// All loaded files are added to the set of loaded files.
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

					// Line format files are made active by default.
					d_active_reconstructable_files.push_back(new_file);
				}
				break;

			case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
				if (file.get_feature_collection())
				{
					// All loaded files are added to the set of loaded files.
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);				

					// We only want to make the first rotation file active.
					if ( ! have_loaded_new_rotation_file) 
					{
						d_active_reconstruction_files.clear();
						d_active_reconstruction_files.push_back(new_file);

						// I am very bad for putting this here - I'll clean it up when
						// ApplicationState notifies of load/unload events - John.
						GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
								*file.get_feature_collection();
						d_plate_velocities.load_reconstruction_feature_collection(
								feature_collection);

						have_loaded_new_rotation_file = true;
					}
				}
				break;

			case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
				if (file.get_feature_collection())
				{
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);
					d_active_reconstructable_files.push_back(new_file);
				}
				break;

			default:
				break;
			}
		}
		catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
		{
			// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
			// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
			// QFileDialog support.
			// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
			boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
					new GPlatesFileIO::LocalFileDataSource(e.filename(), GPlatesFileIO::DataFormats::Unspecified));
			boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
					new GPlatesFileIO::LineNumberInFile(0));
			read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
					e_source,
					e_location,
					GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
					GPlatesFileIO::ReadErrors::FileNotLoaded));
		}
		catch (GPlatesFileIO::ErrorOpeningPipeFromGzipException &e)
		{
			QString message = tr("GPlates was unable to use the '%1' program to read the file '%2'."
					" Please check that gzip is installed and in your PATH. You will still be able to open"
					" files which are not compressed.")
					.arg(e.command())
					.arg(e.filename());
			QMessageBox::critical(this, tr("Error Opening File"), message,
					QMessageBox::Ok, QMessageBox::Ok);
		}
		catch (GPlatesFileIO::FileFormatNotSupportedException &)
		{
			QString message = tr("Error: Loading files in this format is currently not supported.");
			QMessageBox::critical(this, tr("Error Opening File"), message,
					QMessageBox::Ok, QMessageBox::Ok);
		}
		catch (GPlatesGlobal::Exception &e)
		{
			std::cerr << "Caught exception: " << e << std::endl;
		}


	}

	// Internal state changed, make sure dialogs are up to date.
	d_read_errors_dialog.update();
	d_manage_feature_collections_dialog.update();
	d_shapefile_attribute_viewer_dialog.update();

	// Pop up errors only if appropriate.
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}

}


void
GPlatesQtWidgets::ViewportWindow::reload_file(
		file_info_iterator file_it)
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	// Now load the files in a similar way to 'load_files' above, but in this case
	// we don't need to worry about adding/removing from ApplicationState, or the
	// d_active_reconstructable_files and d_active_reconstruction_files lists.
	// The file should already belong to them.
	
	try
	{
		// FIXME: In fact, we are sharing plenty of exception-handling code with load_files as
		// well, though this might also change after the merge. A possible area for refactoring
		// if someone is bored?

		switch ( GPlatesFileIO::get_feature_collection_file_format(*file_it) )
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			GPlatesFileIO::ShapefileReader::set_property_mapper(
				boost::shared_ptr< ShapefilePropertyMapper >(new ShapefilePropertyMapper(this)));
			break;

		default:
			break;
		}

		// Read the feature collection from file.
		GPlatesFileIO::read_feature_collection_file(*file_it, d_model, read_errors);

	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
	{
		// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
		// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
		// QFileDialog support.
		// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
				new GPlatesFileIO::LocalFileDataSource(e.filename(), GPlatesFileIO::DataFormats::Unspecified));
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
				new GPlatesFileIO::LineNumberInFile(0));
		read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
				GPlatesFileIO::ReadErrors::FileNotLoaded));
	}
	catch (GPlatesFileIO::ErrorOpeningPipeFromGzipException &e)
	{
		QString message = tr("GPlates was unable to use the '%1' program to read the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to open"
				" files which are not compressed.")
				.arg(e.command())
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesGlobal::Exception &e)
	{
		std::cerr << "Caught exception: " << e << std::endl;
	}

	
	// Internal state changed, make sure dialogs are up to date.
	d_read_errors_dialog.update();
	// We should be able to get by with just updating the MFCD's state buttons,
	// not rebuild the whole table. This avoids an ugly table redraw.
	d_manage_feature_collections_dialog.update_state();

	// Pop up errors only if appropriate.
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}

	// Data may have changed, update the display.
	reconstruct();
}


GPlatesAppState::ApplicationState::file_info_iterator
GPlatesQtWidgets::ViewportWindow::create_empty_reconstructable_file()
{
	// Create an empty "file" - does not correspond to anything on disk yet.
	GPlatesFileIO::FileInfo file;
	file.set_feature_collection(d_model->create_feature_collection());

	GPlatesAppState::ApplicationState::file_info_iterator new_file =
	GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

	// Given this method's name, we are promised this new FeatureCollection will
	// be used for reconstructable data.
	d_active_reconstructable_files.push_back(new_file);

	// Internal state changed, make sure dialogs are up to date.
	d_manage_feature_collections_dialog.update();
	
	return new_file;
}


namespace
{
	void
	get_features_collection_from_file_info_collection(
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_files,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &features_collection) {

		GPlatesQtWidgets::ViewportWindow::active_files_collection_type::iterator
			iter = active_files.begin(),
			end = active_files.end();
		for ( ; iter != end; ++iter)
		{
			if ((*iter)->get_feature_collection())
			{
				features_collection.push_back(*((*iter)->get_feature_collection()));
			}
		}
	}


	/**
	 * Handles reconstruction, solving plate velocities and rendering RFGs.
	 */
	class ReconstructView :
			public GPlatesAppLogic::ReconstructTemplate
	{
	public:
		ReconstructView(
				GPlatesModel::ModelInterface &model,
				GPlatesAppLogic::PlateVelocities &plate_velocities,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesGui::ColourTable &colour_table) :
			GPlatesAppLogic::ReconstructTemplate(model),
			d_plate_velocities(plate_velocities),
			d_rendered_geom_collection(rendered_geom_collection),
			d_colour_table(colour_table)
		{  }

		virtual
		~ReconstructView()
		{	}


	private:
		GPlatesAppLogic::PlateVelocities &d_plate_velocities;
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;
		const GPlatesGui::ColourTable &d_colour_table;


		/**
		 * Called after a reconstruction is created.
		 */
		virtual
		void
		end_reconstruction(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::Reconstruction &reconstruction,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
				GPlatesFeatureVisitors::TopologyResolver &topology_resolver)
		{
			// Solve plate velocities.
			d_plate_velocities.solve_velocities(
					reconstruction,
					reconstruction_time,
					reconstruction_anchored_plate_id,
					topology_resolver);

			// Render all RFGs as rendered geometries.
			GPlatesViewOperations::render_reconstruction_geometries(
					reconstruction,
					d_rendered_geom_collection,
					d_colour_table);
		}
	};
} // namespace


void
GPlatesQtWidgets::ViewportWindow::reconstruct_view(
		double recon_time,
		GPlatesModel::integer_plate_id_type recon_root)
{
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
		reconstructable_features_collection,
		reconstruction_features_collection;

	get_features_collection_from_file_info_collection(
			d_active_reconstructable_files,
			reconstructable_features_collection);

	get_features_collection_from_file_info_collection(
			d_active_reconstruction_files,
			reconstruction_features_collection);

	// Create a reconstruction and carry out any extra operations
	// in the inherited ReconstructView class.
	ReconstructView reconstruct_view_obj(
			d_model,
			d_plate_velocities,
			d_rendered_geom_collection,
			*get_colour_table());
	d_reconstruction = reconstruct_view_obj.reconstruct(
			reconstructable_features_collection,
			reconstruction_features_collection,
			recon_time,
			recon_root);
}


GPlatesGui::ColourTable *
GPlatesQtWidgets::ViewportWindow::get_colour_table()
{
	GPlatesGui::ColourTable *value;
	if (d_colour_table_ptr == NULL) {
		value = GPlatesGui::PlatesColourTable::Instance();
	} else {
		value = d_colour_table_ptr;
	}
	return value;
}

GPlatesQtWidgets::ViewportWindow::ViewportWindow() :
	d_model(),
	d_feature_focus(*this),
	d_view_state(d_rendered_geom_collection, d_feature_focus),
	d_reconstruction(GPlatesAppLogic::Reconstruct::create_empty_reconstruction(0.0, 0)),
	d_comp_mesh_point_layer(
			d_rendered_geom_collection.create_child_rendered_layer_and_transfer_ownership(
					GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER,
					0.175f)),
	d_comp_mesh_arrow_layer(
			d_rendered_geom_collection.create_child_rendered_layer_and_transfer_ownership(
					GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER,
					0.175f)),
	d_plate_velocities(d_comp_mesh_point_layer, d_comp_mesh_arrow_layer),
	d_recon_time(0.0),
	d_recon_root(0),
	d_animation_controller(*this),
	d_full_screen_mode(*this),
	d_reconstruction_view_widget(d_rendered_geom_collection, d_animation_controller, *this, d_view_state, this),
	d_about_dialog(*this, this),
	d_animate_dialog(d_animation_controller, this),
	d_export_animation_dialog(d_animation_controller, *this, this),
	d_total_reconstruction_poles_dialog(*this, this),
	d_feature_properties_dialog(*this, d_feature_focus, this),
	d_license_dialog(&d_about_dialog),
	d_manage_feature_collections_dialog(*this, this),
	d_read_errors_dialog(this),
	d_set_camera_viewpoint_dialog(*this, this),
	d_set_raster_surface_extent_dialog(*this, this),
	d_specify_anchored_plate_id_dialog(d_recon_root, this),
	d_export_rfg_dialog(this),
	d_specify_time_increment_dialog(d_animation_controller, this),
	d_set_projection_dialog(*this,this),
	d_choose_canvas_tool(*this),
	d_geometry_operation_target(
			d_digitise_geometry_builder,
			d_focused_feature_geometry_builder,
			d_feature_focus,
			d_choose_canvas_tool),
	d_enable_canvas_tool(*this, d_feature_focus, d_geometry_operation_target),
	d_focused_feature_geom_manipulator(
			d_focused_feature_geometry_builder,
			d_feature_focus,
			*this),
	d_measure_distance_state_ptr(new GPlatesCanvasTools::MeasureDistanceState(
				d_rendered_geom_collection,
				d_geometry_operation_target)),
	d_task_panel_ptr(NULL),
	d_shapefile_attribute_viewer_dialog(*this,this),
	d_feature_table_model_ptr( new GPlatesGui::FeatureTableModel(d_feature_focus)),
	d_topology_sections_container_ptr( new GPlatesGui::TopologySectionsContainer()),
	d_open_file_path(""),
	d_colour_table_ptr(NULL)
{
	setupUi(this);

	d_globe_canvas_ptr = &(d_reconstruction_view_widget.globe_canvas());

	std::auto_ptr<TaskPanel> task_panel_auto_ptr(new TaskPanel(
			d_feature_focus,
			d_model,
			d_rendered_geom_collection,
			d_digitise_geometry_builder,
			d_geometry_operation_target,
			d_active_geometry_operation,
			*d_measure_distance_state_ptr,
			*this,
			d_choose_canvas_tool,
			this));
	d_task_panel_ptr = task_panel_auto_ptr.get();

	// Connect all the Signal/Slot relationships of ViewportWindow's
	// toolbar buttons and menu items.
	connect_menu_actions();
	
	// Duplicate the menu structure for the full-screen-mode GMenu.
	populate_gmenu_from_menubar();
	// Initialise various elements for full-screen-mode that must wait until after setupUi().
	d_full_screen_mode.init();
	
	// Set up an emergency context menu to control QDockWidgets even if
	// they're no longer behaving properly.
	set_up_dock_context_menus();
	
	// FIXME: Set up the Task Panel in a more detailed fashion here.
#if 1
	d_reconstruction_view_widget.insert_task_panel(task_panel_auto_ptr);
#else
	// Stretchable Task Panel hack for testing: Make the Task Panel
	// into a QDockWidget, undocked by default.
	QDockWidget *task_panel_dock = new QDockWidget(tr("Task Panel"), this);
	task_panel_dock->setWidget(task_panel_auto_ptr.release());
	task_panel_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, task_panel_dock);
	task_panel_dock->setFloating(true);
#endif
	set_up_task_panel_actions();

	// Enable/disable canvas tools according to the current state.
	d_enable_canvas_tool.initialise();
	// Disable the feature-specific Actions as there is no currently focused feature to act on.
	enable_or_disable_feature_actions(d_feature_focus.focused_feature());
	QObject::connect(&d_feature_focus, SIGNAL(focus_changed(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			this, SLOT(enable_or_disable_feature_actions(GPlatesModel::FeatureHandle::weak_ref)));

	// Set up the Specify Anchored Plate ID dialog.
	QObject::connect(&d_specify_anchored_plate_id_dialog, SIGNAL(value_changed(unsigned long)),
			this, SLOT(reconstruct_with_root(unsigned long)));

	// Set up the Reconstruction View widget.
	setCentralWidget(&d_reconstruction_view_widget);

	QObject::connect(d_globe_canvas_ptr, SIGNAL(mouse_pointer_position_changed(const GPlatesMaths::PointOnSphere &, bool)),
			&d_reconstruction_view_widget, SLOT(update_mouse_pointer_position(const GPlatesMaths::PointOnSphere &, bool)));
	QObject::connect(&(d_reconstruction_view_widget.map_view()), SIGNAL(mouse_pointer_position_changed(const boost::optional<GPlatesMaths::LatLonPoint>&, bool)),
			&d_reconstruction_view_widget, SLOT(update_mouse_pointer_position(const boost::optional<GPlatesMaths::LatLonPoint>&, bool)));

	// Connect the reconstruction pole widget to the feature focus.
	QObject::connect(&d_feature_focus, SIGNAL(focus_changed(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			&(d_task_panel_ptr->reconstruction_pole_widget()), SLOT(set_focus(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)));

	// The Reconstruction Pole widget needs to know when the reconstruction time changes.
	QObject::connect(this, SIGNAL(reconstruction_time_changed(
					double)),
			&(d_task_panel_ptr->reconstruction_pole_widget()), SLOT(handle_reconstruction_time_change(
					double)));

	// Setup RenderedGeometryCollection.
	setup_rendered_geom_collection();

	// Render everything on the screen in present-day positions.
	reconstruct_view(0.0/*reconstruction time*/, d_recon_root);


	// Set up the Clicked table.
	// FIXME: feature table model for this Qt widget and the Query Tool should be stored in ViewState.
	table_view_clicked_geometries->setModel(d_feature_table_model_ptr.get());
	table_view_clicked_geometries->verticalHeader()->hide();
	table_view_clicked_geometries->resizeColumnsToContents();
	GPlatesGui::FeatureTableModel::set_default_resize_modes(*table_view_clicked_geometries->horizontalHeader());
	table_view_clicked_geometries->horizontalHeader()->setMinimumSectionSize(60);
	table_view_clicked_geometries->horizontalHeader()->setMovable(true);
	table_view_clicked_geometries->horizontalHeader()->setHighlightSections(false);
	// When the user selects a row of the table, we should focus that feature.
	// RJW: This is what triggers the highlighting of the geometry on the canvas.
	QObject::connect(table_view_clicked_geometries->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			d_feature_table_model_ptr.get(),
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));

	// Set up the Topology Sections Table, now that the table widget has been created.
	d_topology_sections_table_ptr = new GPlatesGui::TopologySectionsTable(
			*table_widget_topology_sections, *d_topology_sections_container_ptr, d_feature_focus);

	// If the focused feature is modified, we may need to reconstruct to update the view.
	// FIXME:  If the FeatureFocus emits the 'focused_feature_modified' signal, the view will
	// be reconstructed twice -- once here, and once as a result of the 'set_focus' slot in the
	// GeometryFocusHighlight below.
	QObject::connect(&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			this, SLOT(reconstruct()));

	// If the focused feature is modified, we may need to update the ShapefileAttributeViewerDialog.
	QObject::connect(&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			&d_shapefile_attribute_viewer_dialog, SLOT(update()));

	// Set up the Map and Globe Canvas Tools Choices.
	d_map_canvas_tool_choice_ptr.reset(
			new GPlatesGui::MapCanvasToolChoice(
					d_rendered_geom_collection,
					d_geometry_operation_target,
					d_active_geometry_operation,
					d_choose_canvas_tool,
					d_reconstruction_view_widget.map_view(),
					d_reconstruction_view_widget.map_canvas(),
					d_reconstruction_view_widget.map_view(),
					*this,
					*d_feature_table_model_ptr,
					d_feature_properties_dialog,
					d_feature_focus,
					d_task_panel_ptr->reconstruction_pole_widget(),
					*d_topology_sections_container_ptr,
					d_task_panel_ptr->topology_tools_widget(),
					*d_measure_distance_state_ptr));



	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_globe_canvas_tool_choice_ptr.reset(
			new GPlatesGui::GlobeCanvasToolChoice(
					d_rendered_geom_collection,
					d_geometry_operation_target,
					d_active_geometry_operation,
					d_choose_canvas_tool,
					*d_globe_canvas_ptr,
					d_globe_canvas_ptr->globe(),
					*d_globe_canvas_ptr,
					*this,
					d_view_state,
					*d_feature_table_model_ptr,
					d_feature_properties_dialog,
					d_feature_focus,
					d_task_panel_ptr->reconstruction_pole_widget(),
					*d_topology_sections_container_ptr,
					d_task_panel_ptr->topology_tools_widget(),
					*d_measure_distance_state_ptr));



	// Set up the Map and Globe Canvas Tool Adapters for handling globe click and drag events.
	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_globe_canvas_tool_adapter_ptr.reset(new GPlatesGui::GlobeCanvasToolAdapter(
			*d_globe_canvas_tool_choice_ptr));
	d_map_canvas_tool_adapter_ptr.reset(new GPlatesGui::MapCanvasToolAdapter(
			*d_map_canvas_tool_choice_ptr));

	connect_canvas_tools();


	// If the user creates a new feature with the DigitisationWidget, we need to reconstruct to
	// make sure everything is displayed properly.
	QObject::connect(&(d_task_panel_ptr->digitisation_widget().get_create_feature_dialog()),
			SIGNAL(feature_created(GPlatesModel::FeatureHandle::weak_ref)),
			this,
			SLOT(reconstruct()));

	// Add a progress bar to the status bar (Hidden until needed).
	std::auto_ptr<QProgressBar> progress_bar(new QProgressBar(this));
	progress_bar->setMaximumWidth(100);
	progress_bar->hide();
	statusBar()->addPermanentWidget(progress_bar.release());

}


GPlatesQtWidgets::ViewportWindow::~ViewportWindow()
{
	// boost::scoped_ptr destructors needs complete type.
}


void	
GPlatesQtWidgets::ViewportWindow::connect_menu_actions()
{
	// If you want to add a new menu action, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	//    2a. Canvas Tools must have the 'checkable' property set.
	// 3. Drag this action to a menu.
	//    3a. If it's a canvas tool, drag it to the toolbar instead. It will appear on the
	//        Tools menu automatically.
	// 4. Add code for the triggered() signal your action generates here.
	//    Please keep this function sorted in the same order as menu items appear.

	// Canvas Tools:
	// The canvas tools are special; we only want one of them to be checked at any time.
	// We can do this quite easily by adding all the actions on the toolbar to a QActionGroup.
	QList<QAction *> canvas_tools_actions_list = toolbar_canvas_tools->actions();
	QActionGroup* canvas_tools_group = new QActionGroup(this);	// parented to ViewportWindow.
	Q_FOREACH(QAction *tool_action, canvas_tools_actions_list) {
		canvas_tools_group->addAction(tool_action);
	}
	// We also want to populate the Tools menu based on the actions added to the toolbar.
	Q_FOREACH(QAction *tool_action, canvas_tools_actions_list) {
		menu_Tools->addAction(tool_action);
	}

	// Canvas Tool connections:
	QObject::connect(action_Drag_Globe, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_drag_globe_tool()));
	QObject::connect(action_Zoom_Globe, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_zoom_globe_tool()));
	QObject::connect(action_Click_Geometry, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_click_geometry_tool()));
	QObject::connect(action_Digitise_New_Polyline, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_digitise_polyline_tool()));
	QObject::connect(action_Digitise_New_MultiPoint, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_digitise_multipoint_tool()));
	QObject::connect(action_Digitise_New_Polygon, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_digitise_polygon_tool()));
	QObject::connect(action_Move_Geometry, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_move_geometry_tool()));
	QObject::connect(action_Move_Vertex, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_move_vertex_tool()));
	QObject::connect(action_Delete_Vertex, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_delete_vertex_tool()));
	QObject::connect(action_Insert_Vertex, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_insert_vertex_tool()));
	// FIXME: The Move Geometry tool, although it has an awesome icon,
	// is to be disabled until it can be implemented.
	action_Move_Geometry->setVisible(false);
	QObject::connect(action_Manipulate_Pole, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_manipulate_pole_tool()));
	QObject::connect(action_Build_Topology, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_build_topology_tool()));
	QObject::connect(action_Edit_Topology, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_edit_topology_tool()));
	QObject::connect(action_Measure_Distance, SIGNAL(triggered()),
			&d_choose_canvas_tool, SLOT(choose_measure_distance_tool()));


	// File Menu:
	QObject::connect(action_Open_Feature_Collection, SIGNAL(triggered()),
			&d_manage_feature_collections_dialog, SLOT(open_file()));
	QObject::connect(action_Open_Raster, SIGNAL(triggered()),
			this, SLOT(open_raster()));
	QObject::connect(action_Open_Time_Dependent_Raster_Sequence, SIGNAL(triggered()),
			this, SLOT(open_time_dependent_raster_sequence()));
	QObject::connect(action_File_Errors, SIGNAL(triggered()),
			this, SLOT(pop_up_read_errors_dialog()));
	// ---
	QObject::connect(action_Manage_Feature_Collections, SIGNAL(triggered()),
			this, SLOT(pop_up_manage_feature_collections_dialog()));
	QObject::connect(action_View_Shapefile_Attributes, SIGNAL(triggered()),
			this, SLOT(pop_up_shapefile_attribute_viewer_dialog()));
	// ----
	QObject::connect(action_Quit, SIGNAL(triggered()),
			this, SLOT(close()));
	
	// Edit Menu:
	QObject::connect(action_Query_Feature, SIGNAL(triggered()),
			&d_feature_properties_dialog, SLOT(choose_query_widget_and_open()));
	QObject::connect(action_Edit_Feature, SIGNAL(triggered()),
			&d_feature_properties_dialog, SLOT(choose_edit_widget_and_open()));
	// ----
	// Unfortunately, the Undo and Redo actions cannot be added in the Designer,
	// or at least, not nicely. We need to ask the QUndoGroup to create some
	// QActions for us, and add them programmatically. To follow the principle
	// of least surprise, placeholder actions are set up in the designer, which
	// this code can use to insert the actions in the correct place with the
	// correct shortcut.
	// The new actions will be linked to the QUndoGroup appropriately.
	QAction *undo_action_ptr =
		GPlatesViewOperations::UndoRedo::instance().get_undo_group().createUndoAction(this, tr("&Undo"));
	QAction *redo_action_ptr =
		GPlatesViewOperations::UndoRedo::instance().get_undo_group().createRedoAction(this, tr("&Redo"));
	undo_action_ptr->setShortcut(action_Undo_Placeholder->shortcut());
	redo_action_ptr->setShortcut(action_Redo_Placeholder->shortcut());
	menu_Edit->insertAction(action_Undo_Placeholder, undo_action_ptr);
	menu_Edit->insertAction(action_Redo_Placeholder, redo_action_ptr);
	menu_Edit->removeAction(action_Undo_Placeholder);
	menu_Edit->removeAction(action_Redo_Placeholder);
	// ----
#if 0		// Delete Feature is nontrivial to implement (in the model) properly.
	QObject::connect(action_Delete_Feature, SIGNAL(triggered()),
			this, SLOT(delete_focused_feature()));
#else
	action_Delete_Feature->setVisible(false);
#endif
	// ----
	QObject::connect(action_Clear_Selection, SIGNAL(triggered()),
			&d_feature_focus, SLOT(unset_focus()));

	// Reconstruction Menu:
	QObject::connect(action_Reconstruct_to_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_time_spinbox()));
	QObject::connect(action_Increment_Animation_Time_Forwards, SIGNAL(triggered()),
			&d_animation_controller, SLOT(step_forward()));
	QObject::connect(action_Increment_Animation_Time_Backwards, SIGNAL(triggered()),
			&d_animation_controller, SLOT(step_back()));
	QObject::connect(action_Animate, SIGNAL(triggered()),
			this, SLOT(pop_up_animate_dialog()));
	QObject::connect(action_Reset_Animation, SIGNAL(triggered()),
			&d_animation_controller, SLOT(seek_beginning()));
	QObject::connect(action_Play, SIGNAL(triggered(bool)),
			&d_animation_controller, SLOT(set_play_or_pause(bool)));
	QObject::connect(&d_animation_controller, SIGNAL(animation_state_changed(bool)),
			action_Play, SLOT(setChecked(bool)));
	// ----
	QObject::connect(action_Specify_Anchored_Plate_ID, SIGNAL(triggered()),
			this, SLOT(pop_up_specify_anchored_plate_id_dialog()));
	QObject::connect(action_View_Reconstruction_Poles, SIGNAL(triggered()),
			this, SLOT(pop_up_total_reconstruction_poles_dialog()));
	// ----
	QObject::connect(action_Export_Animation, SIGNAL(triggered()),
			this, SLOT(pop_up_export_animation_dialog()));
	QObject::connect(action_Export_Reconstruction, SIGNAL(triggered()),
			this, SLOT(pop_up_export_reconstruction_dialog()));
	
	// View Menu:
	QObject::connect(action_Show_Raster, SIGNAL(triggered()),
			this, SLOT(enable_raster_display()));
	QObject::connect(action_Set_Raster_Surface_Extent, SIGNAL(triggered()),
			this, SLOT(pop_up_set_raster_surface_extent_dialog()));

	QObject::connect(action_Show_Point_Features, SIGNAL(triggered()),
			this, SLOT(enable_point_display()));
	QObject::connect(action_Show_Line_Features, SIGNAL(triggered()),
			this, SLOT(enable_line_display()));
	QObject::connect(action_Show_Polygon_Features, SIGNAL(triggered()),
			this, SLOT(enable_polygon_display()));
	QObject::connect(action_Show_Multipoint_Features, SIGNAL(triggered()),
			this, SLOT(enable_multipoint_display()));
	QObject::connect(action_Show_Arrow_Decorations, SIGNAL(triggered()),
			this, SLOT(enable_arrows_display()));
	// ----
	QObject::connect(action_Colour_By_Plate_ID, SIGNAL(triggered()), 
			this, SLOT(choose_colour_by_plate_id()));
	QObject::connect(action_Colour_By_Single_Colour, SIGNAL(triggered()), 
			this, SLOT(choose_colour_by_single_colour()));
	QObject::connect(action_Colour_By_Feature_Type, SIGNAL(triggered()), 
			this, SLOT(choose_colour_by_feature_type()));
	QObject::connect(action_Colour_By_Age, SIGNAL(triggered()), 
			this, SLOT(choose_colour_by_age()));
	// ----
	QObject::connect(action_Set_Projection, SIGNAL(triggered()),
			this, SLOT(pop_up_set_projection_dialog()));
	QObject::connect(action_Full_Screen, SIGNAL(triggered(bool)),
			&d_full_screen_mode, SLOT(toggle_full_screen(bool)));
	// ----
	QObject::connect(action_Set_Camera_Viewpoint, SIGNAL(triggered()),
			this, SLOT(pop_up_set_camera_viewpoint_dialog()));
 
	QObject::connect(action_Move_Camera_Up, SIGNAL(triggered()),
			this, SLOT(handle_move_camera_up()));
	QObject::connect(action_Move_Camera_Down, SIGNAL(triggered()),
			this, SLOT(handle_move_camera_down()));
	QObject::connect(action_Move_Camera_Left, SIGNAL(triggered()),
			this, SLOT(handle_move_camera_left()));
	QObject::connect(action_Move_Camera_Right, SIGNAL(triggered()),
			this, SLOT(handle_move_camera_right()));

	QObject::connect(action_Rotate_Camera_Clockwise, SIGNAL(triggered()),
			this, SLOT(handle_rotate_camera_clockwise()));
	QObject::connect(action_Rotate_Camera_Anticlockwise, SIGNAL(triggered()),
			this, SLOT(handle_rotate_camera_anticlockwise()));
	QObject::connect(action_Reset_Camera_Orientation, SIGNAL(triggered()),
			this, SLOT(handle_reset_camera_orientation()));

	// ----
	QObject::connect(action_Set_Zoom, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_zoom_spinbox()));
	QObject::connect(action_Zoom_In, SIGNAL(triggered()),
			&d_view_state.get_viewport_zoom(), SLOT(zoom_in()));
	QObject::connect(action_Zoom_Out, SIGNAL(triggered()),
			&d_view_state.get_viewport_zoom(), SLOT(zoom_out()));
	QObject::connect(action_Reset_Zoom_Level, SIGNAL(triggered()),
			&d_view_state.get_viewport_zoom(), SLOT(reset_zoom()));
	// ----
	QObject::connect(action_Export_Geometry_Snapshot, SIGNAL(triggered()),
			this, SLOT(pop_up_export_geometry_snapshot_dialog()));
	
	// Help Menu:
	QObject::connect(action_About, SIGNAL(triggered()),
			this, SLOT(pop_up_about_dialog()));

	// This action is for GUI debugging purposes, to help developers trigger
	// some arbitrary code while debugging GUI problems:
#ifndef GPLATES_PUBLIC_RELEASE
	menu_View->addAction(action_Gui_Debug_Action);
#endif
	QObject::connect(action_Gui_Debug_Action, SIGNAL(triggered()),
			this, SLOT(handle_gui_debug_action()));
}

void
GPlatesQtWidgets::ViewportWindow::populate_gmenu_from_menubar()
{
	// Populate the GMenu with the menubar's menu actions.
	// For now, this has to be done here in ViewportWindow so we can get at 'menubar'.
	// It is also difficult to do this in GMenu's constructor, where I'd prefer to put
	// it, because at that time @a ViewportWindow::setupUi() hasn't been called yet,
	// so the menu structure does not exist.
	
	// Find the GMenu by Qt object name. This is a lot more convenient for this kind
	// of one-off setup than going through ReconstructionViewWidget, etc.
	QMenu *gmenu = findChild<QMenu *>("GMenu");
	if (gmenu) {
		// Add each of the top-level menu items from the main menu bar.
		QList<QAction *> main_menubar_actions = menubar->actions();
		Q_FOREACH(QAction *action, main_menubar_actions) {
			gmenu->addAction(action);
		}
	}
}

void
GPlatesQtWidgets::ViewportWindow::connect_canvas_tools()
{
	QObject::connect(d_globe_canvas_ptr, SIGNAL(mouse_clicked(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
					Qt::KeyboardModifiers)),
			d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_click(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(d_globe_canvas_ptr, SIGNAL(mouse_dragged(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)));

	QObject::connect(d_globe_canvas_ptr, SIGNAL(mouse_released_after_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_release_after_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)));

	QObject::connect(d_globe_canvas_ptr, SIGNAL(mouse_moved_without_drag(
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &)),
					d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_move_without_drag(
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &)));


	QObject::connect(&(d_reconstruction_view_widget.map_view()), SIGNAL(mouse_clicked(const QPointF &,
					 bool, Qt::MouseButton,
					Qt::KeyboardModifiers)),
			d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_click(const QPointF &,
				    bool, Qt::MouseButton,
					Qt::KeyboardModifiers)));


	QObject::connect(&(d_reconstruction_view_widget.map_view()), SIGNAL(mouse_dragged(const QPointF &,
					 bool, const QPointF &, bool,
					Qt::MouseButton, Qt::KeyboardModifiers, const QPointF &)),
			d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_drag(const QPointF &,
					 bool, const QPointF &, bool,
					Qt::MouseButton, Qt::KeyboardModifiers, const QPointF &)));

	QObject::connect(&(d_reconstruction_view_widget.map_view()), SIGNAL(mouse_released_after_drag(const QPointF &,
					 bool, const QPointF &, bool, const QPointF &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_release_after_drag(const QPointF &,
					 bool, const QPointF &, bool, const QPointF &,
					Qt::MouseButton, Qt::KeyboardModifiers)));


	QObject::connect(&(d_reconstruction_view_widget.map_view()), SIGNAL(mouse_moved_without_drag(
					const QPointF &,
					bool, const QPointF &)),
					d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_move_without_drag(
					const QPointF &,
					bool, const QPointF &)));

}


void	
GPlatesQtWidgets::ViewportWindow::set_up_task_panel_actions()
{
	ActionButtonBox &feature_actions = d_task_panel_ptr->feature_action_button_box();

	// If you want to add a new action button, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	// 3. Drag this action to a menu (optional).
	// 4. Add code for the triggered() signal your action generates,
	//    see ViewportWindow::connect_menu_actions().
	// 5. Add a new line of code here adding the QAction to the ActionButtonBox.

	feature_actions.add_action(action_Query_Feature);
	feature_actions.add_action(action_Edit_Feature);
#if 0
	// Doesn't work - hidden for release.
	feature_actions.add_action(action_Delete_Feature);
#endif
	feature_actions.add_action(action_Clear_Selection);
}


void
GPlatesQtWidgets::ViewportWindow::set_up_dock_context_menus()
{
	// Search Results Dock:
	dock_search_results->addAction(action_Information_Dock_At_Top);
	dock_search_results->addAction(action_Information_Dock_At_Bottom);
	QObject::connect(action_Information_Dock_At_Top, SIGNAL(triggered()),
			this, SLOT(dock_search_results_at_top()));
	QObject::connect(action_Information_Dock_At_Bottom, SIGNAL(triggered()),
			this, SLOT(dock_search_results_at_bottom()));
}


void
GPlatesQtWidgets::ViewportWindow::dock_search_results_at_top()
{
	dock_search_results->setFloating(false);
	addDockWidget(Qt::TopDockWidgetArea, dock_search_results);
}

void
GPlatesQtWidgets::ViewportWindow::dock_search_results_at_bottom()
{
	dock_search_results->setFloating(false);
	addDockWidget(Qt::BottomDockWidgetArea, dock_search_results);
}


void
GPlatesQtWidgets::ViewportWindow::highlight_first_clicked_feature_table_row() const
{
	QModelIndex idx = d_feature_table_model_ptr->index(0, 0);
	
	if (idx.isValid()) {
		table_view_clicked_geometries->selectionModel()->clear();
		table_view_clicked_geometries->selectionModel()->select(idx,
				QItemSelectionModel::Select |
				QItemSelectionModel::Current |
				QItemSelectionModel::Rows);
	}
	table_view_clicked_geometries->scrollToTop();
}



void
GPlatesQtWidgets::ViewportWindow::reconstruct_to_time(
		double new_recon_time)
{
	// != does not work with doubles, so we must wrap them in Real.
	GPlatesMaths::Real original_recon_time(d_recon_time);
	if (original_recon_time != GPlatesMaths::Real(new_recon_time)) {
		d_recon_time = new_recon_time;

	}
	// Reconstruct before we tell everyone that we've reconstructed!
	reconstruct();
	emit reconstruction_time_changed(d_recon_time);
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct_with_root(
		unsigned long new_recon_root)
{
	if (d_recon_root != new_recon_root) {
		d_recon_root = new_recon_root;
		// Does anyone care if the reconstruction root changed?
	}
	reconstruct();

	// The reconstruction time hasn't really changed, but emitting this signal will 
	// make sure that other parts of GPlates which are dependent on new geometry values 
	// will get updated.
	// FIXME: Create a suitable new slot, or maybe just rename the slot to 
	// something like "reconstruction_time_or_root_changed"
	emit reconstruction_time_changed(d_recon_time);
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct_to_time_with_root(
		double new_recon_time,
		unsigned long new_recon_root)
{
	// FIXME: This function is only called once, on application startup, for root=0 and time=0;
	// if we ever need to call this for other reasons, then we should be careful about the relative 
	// order of the reconstruction, and the emit signal. 

	// != does not work with doubles, so we must wrap them in Real.
	GPlatesMaths::Real original_recon_time(d_recon_time);
	if (original_recon_time != GPlatesMaths::Real(new_recon_time)) {
		d_recon_time = new_recon_time;
		emit reconstruction_time_changed(d_recon_time);
	}
	if (d_recon_root != new_recon_root) {
		d_recon_root = new_recon_root;
		// Does anyone care if the reconstruction root changed?
	}
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct()
{
	reconstruct_view(d_recon_time, d_recon_root);

	if (d_total_reconstruction_poles_dialog.isVisible()) {
		d_total_reconstruction_poles_dialog.update();
	}
	if (action_Show_Raster->isChecked() && ! d_time_dependent_raster_map.isEmpty()) {	
		update_time_dependent_raster();
	}
	if (d_feature_focus.is_valid()) {
		// There's a focused feature.
		// We need to update the associated RFG for the new reconstruction.
		d_feature_focus.find_new_associated_reconstruction_geometry(reconstruction());
	}
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_specify_anchored_plate_id_dialog()
{
	d_specify_anchored_plate_id_dialog.show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_specify_anchored_plate_id_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_specify_anchored_plate_id_dialog.raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_set_camera_viewpoint_dialog()
{

	GPlatesMaths::LatLonPoint cur_llp = d_reconstruction_view_widget.camera_llp();

	d_set_camera_viewpoint_dialog.set_lat_lon(cur_llp.latitude(), cur_llp.longitude());
	if (d_set_camera_viewpoint_dialog.exec())
	{
		// Note: d_set_camera_viewpoint_dialog is modal and should not need the 'raise' hacks
		// that the other dialogs use.
		try {
			GPlatesMaths::LatLonPoint desired_centre(
					d_set_camera_viewpoint_dialog.latitude(),
					d_set_camera_viewpoint_dialog.longitude());
			
			d_reconstruction_view_widget.active_view().set_camera_viewpoint(desired_centre);

		} catch (GPlatesMaths::InvalidLatLonException &) {
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.

			// User somehow managed to specify an invalid lat,lon. Pretend it didn't happen.
		}
	}
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_total_reconstruction_poles_dialog()
{
	d_total_reconstruction_poles_dialog.update();

	d_total_reconstruction_poles_dialog.show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_total_reconstruction_poles_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_total_reconstruction_poles_dialog.raise();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_animate_dialog()
{
	d_animate_dialog.show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_animate_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_animate_dialog.raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_about_dialog()
{
	d_about_dialog.show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_about_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_about_dialog.raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_license_dialog()
{
	d_license_dialog.show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_license_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_license_dialog.raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_export_animation_dialog()
{
	// FIXME: Should Export Animation be modal?
	d_export_animation_dialog.show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_export_animation_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_export_animation_dialog.raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_export_reconstruction_dialog()
{
	GPlatesViewOperations::ExportReconstructedFeatureGeometries::active_files_collection_type
			active_reconstructable_geometry_files(
					active_reconstructable_files().begin(),
					active_reconstructable_files().end());

	d_export_rfg_dialog.export_visible_reconstructed_feature_geometries(
			reconstruction(),
			rendered_geometry_collection(),
			active_reconstructable_geometry_files,
			d_recon_root,
			d_recon_time);
}


void
GPlatesQtWidgets::ViewportWindow::enable_drag_globe_tool(
		bool enable)
{
	action_Drag_Globe->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_zoom_globe_tool(
		bool enable)
{
	action_Zoom_Globe->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_click_geometry_tool(
		bool enable)
{
	action_Click_Geometry->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_digitise_polyline_tool(
		bool enable)
{
	action_Digitise_New_Polyline->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_digitise_multipoint_tool(
		bool enable)
{
	action_Digitise_New_MultiPoint->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_digitise_polygon_tool(
		bool enable)
{
	action_Digitise_New_Polygon->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_move_geometry_tool(
		bool enable)
{
	action_Move_Geometry->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_move_vertex_tool(
		bool enable)
{
	action_Move_Vertex->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_delete_vertex_tool(
		bool enable)
{
	action_Delete_Vertex->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_insert_vertex_tool(
		bool enable)
{
	action_Insert_Vertex->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_manipulate_pole_tool(
		bool enable)
{
	action_Manipulate_Pole->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::enable_build_topology_tool(
		bool enable)
{
	action_Build_Topology->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::enable_edit_topology_tool(
		bool enable)
{
	action_Edit_Topology->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::enable_measure_distance_tool(
		bool enable)
{
	action_Measure_Distance->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::choose_drag_globe_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_reorient_globe_tool();
	d_map_canvas_tool_choice_ptr->choose_pan_map_tool();
}


void
GPlatesQtWidgets::ViewportWindow::choose_zoom_globe_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_zoom_globe_tool();

	d_map_canvas_tool_choice_ptr->choose_zoom_map_tool();
}


void
GPlatesQtWidgets::ViewportWindow::choose_click_geometry_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_click_geometry_tool();
	d_map_canvas_tool_choice_ptr->choose_click_geometry_tool();

	d_task_panel_ptr->choose_feature_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_polyline_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_digitise_polyline_tool();
	d_map_canvas_tool_choice_ptr->choose_digitise_polyline_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_multipoint_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_digitise_multipoint_tool();
	d_map_canvas_tool_choice_ptr->choose_digitise_multipoint_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_polygon_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_digitise_polygon_tool();
	d_map_canvas_tool_choice_ptr->choose_digitise_polygon_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_move_geometry_tool()
{
	// The MoveGeometry tool is not yet implemented.
#if 0
	d_globe_canvas_tool_choice_ptr->choose_move_geometry_tool();
	d_task_panel_ptr->choose_feature_tab();
#endif
}


void
GPlatesQtWidgets::ViewportWindow::choose_move_vertex_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_move_vertex_tool();
	d_map_canvas_tool_choice_ptr->choose_move_vertex_tool();
	d_task_panel_ptr->choose_modify_geometry_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_delete_vertex_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_delete_vertex_tool();
	d_map_canvas_tool_choice_ptr->choose_delete_vertex_tool();

	d_task_panel_ptr->choose_modify_geometry_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_insert_vertex_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_insert_vertex_tool();
	d_map_canvas_tool_choice_ptr->choose_insert_vertex_tool();

	d_task_panel_ptr->choose_modify_geometry_tab();
}

void
GPlatesQtWidgets::ViewportWindow::choose_measure_distance_tool()
{
	action_Measure_Distance->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_measure_distance_tool();
	d_map_canvas_tool_choice_ptr->choose_measure_distance_tool();

	d_task_panel_ptr->choose_measure_distance_tab();
}

void
GPlatesQtWidgets::ViewportWindow::choose_manipulate_pole_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_manipulate_pole_tool();

// The map's manipulate pole tool doesn't yet do anything. 
	d_map_canvas_tool_choice_ptr->choose_manipulate_pole_tool();
	d_task_panel_ptr->choose_modify_pole_tab();
}

void
GPlatesQtWidgets::ViewportWindow::choose_build_topology_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_build_topology_tool();
	// FIXME: There is no MapCanvasToolChoice equivalent yet.
	
	if (d_reconstruction_view_widget.map_is_active())
	{
		status_message(QObject::tr(
			"Build topology tool is not yet available on the map. Use the globe projection to build a topology."
			" Ctrl+drag to pan the map."));		
	}
	d_task_panel_ptr->choose_topology_tools_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_edit_topology_tool()
{
	d_globe_canvas_tool_choice_ptr->choose_edit_topology_tool();
	// FIXME: There is no MapCanvasToolChoice equivalent yet.
	if(d_reconstruction_view_widget.map_is_active())
	{
		status_message(QObject::tr(
			"Edit topology tool is not yet available on the map. Use the globe projection to edit a topology."
			" Ctrl+drag to pan the map."));			
	}
	d_task_panel_ptr->choose_topology_tools_tab();
}



void
GPlatesQtWidgets::ViewportWindow::enable_or_disable_feature_actions(
		GPlatesModel::FeatureHandle::weak_ref focused_feature)
{
	// Note: enabling/disabling canvas tools is now done in class 'EnableCanvasTool'.
	bool enable_canvas_tool_actions = focused_feature.is_valid();
	
	action_Query_Feature->setEnabled(enable_canvas_tool_actions);
	action_Edit_Feature->setEnabled(enable_canvas_tool_actions);
	action_Manipulate_Pole->setEnabled(enable_canvas_tool_actions);
	
#if 0		// Delete Feature is nontrivial to implement (in the model) properly.
	action_Delete_Feature->setEnabled(enable_canvas_tool_actions);
#else
	action_Delete_Feature->setDisabled(true);
#endif
	action_Clear_Selection->setEnabled(true);

#if 0
	// FIXME: Add to Selection is unimplemented and should stay disabled for now.
	// FIXME: To handle the "Remove from Selection", "Clear Selection" actions,
	// we may want to modify this method to also test for a nonempty selection of features.
	action_Add_Feature_To_Selection->setEnabled(enable);
#endif
}


void 
GPlatesQtWidgets::ViewportWindow::uncheck_all_colouring_tools() {
	action_Colour_By_Plate_ID->setChecked(false);
	action_Colour_By_Single_Colour->setChecked(false);
	action_Colour_By_Feature_Type->setChecked(false);
	action_Colour_By_Age->setChecked(false);
}

void		
GPlatesQtWidgets::ViewportWindow::choose_colour_by_plate_id() {
	d_colour_table_ptr = GPlatesGui::PlatesColourTable::Instance();
	uncheck_all_colouring_tools();
	action_Colour_By_Plate_ID->setChecked(true);
	reconstruct();
}

void
GPlatesQtWidgets::ViewportWindow::choose_colour_by_single_colour() {	
	// Pop up Qt's colour-choosing dialog.
	// Specifying the default isn't absolutely necessary, but does allow us to pass 'this'
	// as the parent, solving some dialog placement problems.
	static QColor default_qcolor = Qt::white;
	QColor qcolor = QColorDialog::getColor(default_qcolor, this);
	
	// Cancelling the dialog returns an invalid colour.
	if (qcolor.isValid()) {
		// To be nice, present the user with the colour they chose last time.
		default_qcolor = qcolor;

		// Assign this new colour to our SingleColourTable.
		GPlatesGui::Colour colour(qcolor.redF(), qcolor.greenF(), qcolor.blueF(), qcolor.alphaF());
		GPlatesGui::SingleColourTable::Instance()->set_colour(colour);
	}
	
	// Regardless of whether the user cancels the colour selection dialog, the
	// colouring mode will change to 'single colour', as it is nontrivial to revert
	// the selection back to whatever prior colouring method may have been selected.
	d_colour_table_ptr = GPlatesGui::SingleColourTable::Instance();

	uncheck_all_colouring_tools();
	action_Colour_By_Single_Colour->setChecked(true);
	reconstruct();
}

void	
GPlatesQtWidgets::ViewportWindow::choose_colour_by_feature_type() {
	d_colour_table_ptr = GPlatesGui::FeatureColourTable::Instance();

	uncheck_all_colouring_tools();
	action_Colour_By_Feature_Type->setChecked(true);
	reconstruct();
}

void
GPlatesQtWidgets::ViewportWindow::choose_colour_by_age() {
	GPlatesGui::AgeColourTable::Instance()->set_viewport_window(*this);
	d_colour_table_ptr = GPlatesGui::AgeColourTable::Instance();

	uncheck_all_colouring_tools();
	action_Colour_By_Age->setChecked(true);
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_read_errors_dialog()
{
	d_read_errors_dialog.show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_read_errors_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_read_errors_dialog.raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_manage_feature_collections_dialog()
{
	d_manage_feature_collections_dialog.show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_manage_feature_collections_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_manage_feature_collections_dialog.raise();
}


void
GPlatesQtWidgets::ViewportWindow::deactivate_loaded_file(
		file_info_iterator loaded_file)
{
	// Don't bother checking whether 'loaded_file' is actually an element of
	// 'd_active_reconstructable_files' and/or 'd_active_reconstruction_files' -- just tell the
	// lists to remove the value if it *is* an element.

	// list<T>::remove(const T &val) -- remove all elements with value 'val'.
	// Will not throw (unless element comparisons can throw).
	// See Josuttis section 6.10.7 "Inserting and Removing Elements".
	d_active_reconstructable_files.remove(loaded_file);
	d_active_reconstruction_files.remove(loaded_file);

	// Before we delete the feature collection let the plate velocities hook know this.
	// This will need to be refactored to listen to ApplicationState for load/unload events.
	if (loaded_file->get_feature_collection())
	{
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				*loaded_file->get_feature_collection();
		d_plate_velocities.unload_feature_collection(feature_collection);
	}

	// FIXME: This is a temporary hack to stop highlighting the focused feature if
	// it's in the feature collection we're about to unload.
	if (d_feature_focus.is_valid() && loaded_file->get_feature_collection())
	{
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				*loaded_file->get_feature_collection();

		GPlatesModel::FeatureHandle::weak_ref focused_feature = d_feature_focus.focused_feature();

		GPlatesModel::FeatureCollectionHandle::features_iterator feature_iter;
		for (feature_iter = feature_collection->features_begin();
			feature_iter != feature_collection->features_end();
			++feature_iter)
		{
			if (feature_iter.is_valid() &&
				(*feature_iter).get() == focused_feature.handle_ptr())
			{
				d_feature_focus.unset_focus();
				break;
			}
		}
	}

	// FIXME:  This should not happen here -- in fact, it should be removal of the loaded file
	// (using 'remove_loaded_file' in ApplicationState) which triggers *this*! -- but until we
	// have multiple view windows, it doesn't matter.
	GPlatesAppState::ApplicationState::instance()->remove_loaded_file(loaded_file);

	// Update the shapefile-attribute viewer dialog, which needs to know which files are loaded.
	d_shapefile_attribute_viewer_dialog.update();

	// FIXME: Force a reconstruction so that any unloaded features will be refreshed
	// and will disappear if they've been unloaded.
	reconstruct();
}


bool
GPlatesQtWidgets::ViewportWindow::is_file_active(
		file_info_iterator loaded_file)
{
	return is_file_active_reconstructable(loaded_file) ||
			is_file_active_reconstruction(loaded_file);
}


bool
GPlatesQtWidgets::ViewportWindow::is_file_active_reconstructable(
		file_info_iterator loaded_file)
{
	active_files_iterator reconstructable_it = d_active_reconstructable_files.begin();
	active_files_iterator reconstructable_end = d_active_reconstructable_files.end();
	for (; reconstructable_it != reconstructable_end; ++reconstructable_it) {
		if (*reconstructable_it == loaded_file) {
			return true;
		}
	}
	return false;
}


bool
GPlatesQtWidgets::ViewportWindow::is_file_active_reconstruction(
		file_info_iterator loaded_file)
{
	active_files_iterator reconstruction_it = d_active_reconstruction_files.begin();
	active_files_iterator reconstruction_end = d_active_reconstruction_files.end();
	for (; reconstruction_it != reconstruction_end; ++reconstruction_it) {
		if (*reconstruction_it == loaded_file) {
			return true;
		}
	}
	return false;
}


void
GPlatesQtWidgets::ViewportWindow::set_file_active_reconstructable(
		file_info_iterator file_it,
		bool activate)
{
	if (activate) {
		// Add it to the list, if it's not there already.
		if ( ! is_file_active_reconstructable(file_it)) {
			d_active_reconstructable_files.push_back(file_it);
		}
	} else {
		// Don't bother checking whether 'loaded_file' is actually an element of
		// 'd_active_reconstructable_files' and/or 'd_active_reconstruction_files' -- just tell the
		// lists to remove the value if it *is* an element.
		
		// list<T>::remove(const T &val) -- remove all elements with value 'val'.
		// Will not throw (unless element comparisons can throw).
		// See Josuttis section 6.10.7 "Inserting and Removing Elements".
		d_active_reconstructable_files.remove(file_it);
	}
	// Active features changed, will need to reconstruct() to make RFGs for them.
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::set_file_active_reconstruction(
		file_info_iterator file_it,
		bool activate)
{
	if (activate) {
		// At the moment, we only want one active reconstruction tree
		// at a time. Deactivate the others, and update ManageFeatureCollectionsDialog
		// so that the other buttons get deselected appropriately.
		d_active_reconstruction_files.clear();
		d_active_reconstruction_files.push_back(file_it);
		// NOTE: in the current setup, the only place this set_file_active_xxxx()
		// method is called is by ManageFeatureCollectionsDialog itself, in response
		// to a button press. Therefore we can assume it is up-to-date already, except
		// for this one case where we have cleared all the other reconstruction files.
		// If this situation changes and other code will also be calling
		// set_file_active_xxxx() or otherwise messing with file 'active' status, you
		// will need to call ManageFeatureCollectionsDialog::update_state() at the end
		// of both these methods.
		d_manage_feature_collections_dialog.update_state();
	} else {
		// Don't bother checking whether 'loaded_file' is actually an element of
		// 'd_active_reconstructable_files' and/or 'd_active_reconstruction_files' -- just tell the
		// lists to remove the value if it *is* an element.
		
		// list<T>::remove(const T &val) -- remove all elements with value 'val'.
		// Will not throw (unless element comparisons can throw).
		// See Josuttis section 6.10.7 "Inserting and Removing Elements".
		d_active_reconstruction_files.remove(file_it);
	}
	// Active rotation changed, will need to reconstruct() to make make use of it.
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::create_svg_file()
{
	QString filename = QFileDialog::getSaveFileName(this,
			tr("Save As"), "", tr("SVG file (*.svg)"));

	if (filename.isEmpty()){
		return;
	}
	create_svg_file(filename);
}

void
GPlatesQtWidgets::ViewportWindow::create_svg_file(
		const QString &filename)
{
#if 0
	bool result = GPlatesGui::SvgExport::create_svg_output(filename,d_globe_canvas_ptr);
	if (!result){
		std::cerr << "Error creating SVG output.." << std::endl;
	}
#endif
	try{
		d_reconstruction_view_widget.active_view().create_svg_output(filename);
	}
	catch(...)
	{
		std::cerr << "Caught exception creating SVG output." << std::endl;
	}
}


void
GPlatesQtWidgets::ViewportWindow::close_all_dialogs()
{
	d_about_dialog.reject();
	d_animate_dialog.reject();
	d_total_reconstruction_poles_dialog.reject();
	d_feature_properties_dialog.reject();
	d_license_dialog.reject();
	d_manage_feature_collections_dialog.reject();
	d_read_errors_dialog.reject();
	d_set_camera_viewpoint_dialog.reject();
	d_set_projection_dialog.reject();
	d_set_raster_surface_extent_dialog.reject();
	d_specify_anchored_plate_id_dialog.reject();
	d_shapefile_attribute_viewer_dialog.reject();
	d_export_animation_dialog.reject();
}

void
GPlatesQtWidgets::ViewportWindow::closeEvent(QCloseEvent *close_event)
{
	// For now, always accept the close event.
	// In the future, ->reject() can be used to postpone closure in the event of
	// unsaved files, etc.
	close_event->accept();
	// If we decide to accept the close event, we should also tidy up after ourselves.
	close_all_dialogs();
}

void
GPlatesQtWidgets::ViewportWindow::remap_shapefile_attributes(
	GPlatesFileIO::FileInfo &file_info)
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	GPlatesFileIO::ShapefileReader::remap_shapefile_attributes(file_info, d_model, read_errors);

	d_read_errors_dialog.update();

	// Pop up errors only if appropriate.
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}

	// Plate-ids may have changed, so update the reconstruction. 
	reconstruct();
}



void
GPlatesQtWidgets::ViewportWindow::enable_point_display()
{
	if (action_Show_Point_Features->isChecked())
	{
		d_reconstruction_view_widget.enable_point_display();
	}
	else
	{
		d_reconstruction_view_widget.disable_point_display();
	}
}


void
GPlatesQtWidgets::ViewportWindow::enable_line_display()
{
	if (action_Show_Line_Features->isChecked())
	{
		d_reconstruction_view_widget.enable_line_display();
	}
	else
	{
		d_reconstruction_view_widget.disable_line_display();
	}
}


void
GPlatesQtWidgets::ViewportWindow::enable_polygon_display()
{
	if (action_Show_Polygon_Features->isChecked())
	{
		d_reconstruction_view_widget.enable_polygon_display();
	}
	else
	{
		d_reconstruction_view_widget.disable_polygon_display();
	}
}


void
GPlatesQtWidgets::ViewportWindow::enable_multipoint_display()
{
	if (action_Show_Multipoint_Features->isChecked())
	{
		d_reconstruction_view_widget.enable_multipoint_display();
	}
	else
	{
		d_reconstruction_view_widget.disable_multipoint_display();
	}
}

void
GPlatesQtWidgets::ViewportWindow::enable_arrows_display()
{
	if (action_Show_Arrow_Decorations->isChecked())
	{
		d_reconstruction_view_widget.enable_arrows_display();
	}
	else
	{
		d_reconstruction_view_widget.disable_arrows_display();
	}
}

void
GPlatesQtWidgets::ViewportWindow::enable_raster_display()
{
	if (action_Show_Raster->isChecked())
	{
		d_reconstruction_view_widget.enable_raster_display();
	}
	else
	{
		d_reconstruction_view_widget.disable_raster_display();
	}
}

void
GPlatesQtWidgets::ViewportWindow::open_raster()
{

	QString filename = QFileDialog::getOpenFileName(this,
		QObject::tr("Open File"), d_open_file_path, QObject::tr("Raster files (*.jpg *.jpeg)") );

	if ( filename.isEmpty()){
		return;
	}

	if (load_raster(filename))
	{
		// If we've successfully loaded a single raster, clear the raster_map.
		d_time_dependent_raster_map.clear();
	}
	QFileInfo last_opened_file(filename);
	d_open_file_path = last_opened_file.path();
}

bool
GPlatesQtWidgets::ViewportWindow::load_raster(
	QString filename)
{
	bool result = false;
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	
	GPlatesFileIO::FileInfo file_info(filename);

	try{

		GPlatesFileIO::RasterReader::read_file(file_info, d_globe_canvas_ptr->globe().texture(), read_errors);
		action_Show_Raster->setChecked(true);
		result = true;
	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
	{
		// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
		// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
		// QFileDialog support.
		// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
			new GPlatesFileIO::LocalFileDataSource(e.filename(), GPlatesFileIO::DataFormats::Unspecified));
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
			new GPlatesFileIO::LineNumberInFile(0));
		read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
			e_source,
			e_location,
			GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
			GPlatesFileIO::ReadErrors::FileNotLoaded));
	}
	catch (GPlatesGlobal::Exception &e)
	{
		std::cerr << "Caught GPlates exception: " << e << std::endl;
	}
	catch (...)
	{
		std::cerr << "Caught exception loading raster file." << std::endl;
	}

	d_globe_canvas_ptr->update_canvas();
	d_read_errors_dialog.update();

	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}
	return result;
}

void
GPlatesQtWidgets::ViewportWindow::open_time_dependent_raster_sequence()
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	QFileDialog file_dialog(this,QObject::tr("Choose Folder Containing Time-dependent Rasters"),d_open_file_path,NULL);
	file_dialog.setFileMode(QFileDialog::DirectoryOnly);

	if (file_dialog.exec())
	{
		
		QStringList directory_list = file_dialog.selectedFiles();
		QString directory = directory_list.at(0);

		GPlatesFileIO::RasterReader::populate_time_dependent_raster_map(d_time_dependent_raster_map,directory,read_errors);
		
		QFileInfo last_opened_file(file_dialog.directory().absoluteFilePath(directory_list.last()));
		d_open_file_path = last_opened_file.path();

		d_read_errors_dialog.update();
	
		// Pop up errors only if appropriate.
		GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
		if (num_initial_errors != num_final_errors) {
			d_read_errors_dialog.show();
		}
	}

	if (!d_time_dependent_raster_map.isEmpty())
	{
		action_Show_Raster->setChecked(true);
		update_time_dependent_raster();
	}



}

void
GPlatesQtWidgets::ViewportWindow::update_time_dependent_raster()
{
	QString filename = GPlatesFileIO::RasterReader::get_nearest_raster_filename(d_time_dependent_raster_map,d_recon_time);
	load_raster(filename);
}

// FIXME: Should be a ViewState operation, or /somewhere/ better than this.
void
GPlatesQtWidgets::ViewportWindow::delete_focused_feature()
{
	if (d_feature_focus.is_valid()) {
		GPlatesModel::FeatureHandle::weak_ref feature_ref = d_feature_focus.focused_feature();
#if 0		// Cannot call ModelInterface::remove_feature() as it is #if0'd out and not implemented in Model!
		// FIXME: figure out FeatureCollectionHandle::weak_ref that feature_ref belongs to.
		// Possibly implement that as part of ModelUtils.
		d_model->remove_feature(feature_ref, collection_ref);
#endif
		d_feature_focus.announce_deletion_of_focused_feature();
	}
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_set_raster_surface_extent_dialog()
{
	d_set_raster_surface_extent_dialog.exec();
	// d_set_raster_surface_extent_dialog is modal and should not need the 'raise' hack
	// other dialogs use.
}

void
GPlatesQtWidgets::ViewportWindow::update_tools_and_status_message()
{
	// These calls ensure that the correct status message is displayed. 
	d_map_canvas_tool_choice_ptr->tool_choice().handle_activation();
	d_globe_canvas_tool_choice_ptr->tool_choice().handle_activation();
	
	// Only enable raster-related menu items when the globe is active. 
	bool globe_is_active = d_reconstruction_view_widget.globe_is_active();
	action_Open_Raster->setEnabled(globe_is_active);
	action_Open_Time_Dependent_Raster_Sequence->setEnabled(globe_is_active);
	action_Show_Raster->setEnabled(globe_is_active);	
	action_Set_Raster_Surface_Extent->setEnabled(globe_is_active);
	action_Show_Arrow_Decorations->setEnabled(globe_is_active);
	
	// Grey-out the modify pole tab when in map mode. 
	d_task_panel_ptr->enable_modify_pole_tab(globe_is_active);
	d_task_panel_ptr->enable_topology_tab(d_reconstruction_view_widget.globe_is_active());
	
	// Display appropriate status bar message for tools which are not available on the map.
	if (action_Build_Topology->isChecked() && d_reconstruction_view_widget.map_is_active())
	{
		status_message(QObject::tr(
			"Build topology tool is not yet available on the map. Use the globe projection to build a topology."
			" Ctrl+drag to pan the map."));		
	}
	else if(action_Edit_Topology->isChecked() && d_reconstruction_view_widget.map_is_active())
	{
		status_message(QObject::tr(
			"Edit topology tool is not yet available on the map. Use the globe projection to edit a topology."
			" Ctrl+drag to pan the map."));			
	}
}


void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_up()
{
	d_reconstruction_view_widget.active_view().move_camera_up();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_down()
{
	d_reconstruction_view_widget.active_view().move_camera_down();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_left()
{
	d_reconstruction_view_widget.active_view().move_camera_left();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_right()
{
	d_reconstruction_view_widget.active_view().move_camera_right();
}

void
GPlatesQtWidgets::ViewportWindow::handle_rotate_camera_clockwise()
{
	d_reconstruction_view_widget.active_view().rotate_camera_clockwise();
}

void
GPlatesQtWidgets::ViewportWindow::handle_rotate_camera_anticlockwise()
{
	d_reconstruction_view_widget.active_view().rotate_camera_anticlockwise();
}

void
GPlatesQtWidgets::ViewportWindow::handle_reset_camera_orientation()
{
	d_reconstruction_view_widget.active_view().reset_camera_orientation();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_set_projection_dialog()
{

	d_set_projection_dialog.setup();

	if (d_set_projection_dialog.exec())
	{
		try {
			// Notify the view state of the projection change.
			// It will handle the rest.
			GPlatesViewOperations::ViewportProjection &viewport_projection =
					d_view_state.get_viewport_projection();
			viewport_projection.set_projection_type(
					d_set_projection_dialog.get_projection_type());
			viewport_projection.set_central_meridian(
					d_set_projection_dialog.central_meridian());
		}
		catch(GPlatesGui::ProjectionException &e)
		{
			std::cerr << e << std::endl;
		}
	}
}


void
GPlatesQtWidgets::ViewportWindow::handle_gui_debug_action()
{
	// Some handy information that may aid debugging:
#if 0
	// "Where the hell did my keyboard focus go?"
	qDebug() << "Current focus:" << QApplication::focusWidget();
#endif
	// "What's the name of the current style so I can test against it?"
	qDebug() << "Current style:" << style()->objectName();

	// "What's this thing doing there?"
	QWidget *cursor_widget = QApplication::widgetAt(QCursor::pos());
	qDebug() << "Current widget under cursor:" << cursor_widget;
	while (cursor_widget && cursor_widget->parentWidget()) {
		cursor_widget = cursor_widget->parentWidget();
		qDebug() << "\twhich is inside:" << cursor_widget;
	}
}


void
GPlatesQtWidgets::ViewportWindow::setup_rendered_geom_collection()
{
	// Reconstruction rendered layer is always active.
	d_rendered_geom_collection.set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	d_rendered_geom_collection.set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER);

	// Activate the main rendered layer.
	// Specify which main rendered layers are orthogonal to each other - when
	// one is activated the others are automatically deactivated.
	GPlatesViewOperations::RenderedGeometryCollection::orthogonal_main_layers_type orthogonal_main_layers;
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::MEASURE_DISTANCE_LAYER);

	d_rendered_geom_collection.set_orthogonal_main_layers(orthogonal_main_layers);
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_shapefile_attribute_viewer_dialog()
{
	d_shapefile_attribute_viewer_dialog.show();
	d_shapefile_attribute_viewer_dialog.update();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_shapefile_attribute_viewer_dialog.activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_shapefile_attribute_viewer_dialog.raise();
}

