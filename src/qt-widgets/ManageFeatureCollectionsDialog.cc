/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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
#include <boost/bind.hpp>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>

#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "file-io/FileInfo.h"
#include "file-io/ExternalProgram.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/ErrorOpeningPipeToGzipException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/GpmlOnePointSixOutputVisitor.h"
#include "file-io/OgrException.h"
#include "global/InvalidFeatureCollectionException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"
#include "ManageFeatureCollectionsActionWidget.h"
#include "ManageFeatureCollectionsStateWidget.h"
#include "GMTHeaderFormatDialog.h"

#include "ManageFeatureCollectionsDialog.h"


namespace
{
	namespace ColumnNames
	{
		/**
		 * These should match the columns set up in the designer.
		 */
		enum ColumnName
		{
			FILENAME, FORMAT, LAYER_TYPES, ACTIONS
		};
	}
	
	const QString &
	get_format_for_file(
			const QFileInfo &qfileinfo)
	{
		static const QString format_line(QObject::tr("PLATES4 line"));
		static const QString format_rotation(QObject::tr("PLATES4 rotation"));
		static const QString format_shapefile(QObject::tr("ESRI shapefile"));
		static const QString format_gpml(QObject::tr("GPlates Markup Language"));
		static const QString format_gpml_gz(QObject::tr("Compressed GPML"));
		static const QString format_gmt(QObject::tr("GMT xy"));
		static const QString format_gmap(QObject::tr("GMAP VGP"));
		static const QString format_unknown(QObject::tr(""));
		
		switch ( GPlatesFileIO::get_feature_collection_file_format(qfileinfo) )
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
			return format_line;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
			return format_rotation;

		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			return format_shapefile;

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
			return format_gpml;

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
			return format_gpml_gz;

		case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
			return format_gmt;
			
		case GPlatesFileIO::FeatureCollectionFileFormat::GMAP:
			return format_gmap;

		case GPlatesFileIO::FeatureCollectionFileFormat::UNKNOWN:
		default:
			return format_unknown;
		}
	}
	
	std::vector<std::pair<QString, QString> >
	get_output_filters_for_file(
			GPlatesAppLogic::FeatureCollectionFileState &file_state,
			GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_iter,
			bool has_gzip)
	{
		using std::make_pair;

		// Appropriate filters for available output formats.
		static const QString filter_gmt(QObject::tr("GMT xy (*.xy)"));
		static const QString filter_gmt_ext(QObject::tr("xy"));
		static const QString filter_line(QObject::tr("PLATES4 line (*.dat *.pla)"));
		static const QString filter_line_ext(QObject::tr("dat"));
		static const QString filter_rotation(QObject::tr("PLATES4 rotation (*.rot)"));
		static const QString filter_rotation_ext(QObject::tr("rot"));
		static const QString filter_shapefile(QObject::tr("ESRI shapefile (*.shp)"));
		static const QString filter_shapefile_ext(QObject::tr("shp"));
		static const QString filter_gpml(QObject::tr("GPlates Markup Language (*.gpml)"));
		static const QString filter_gpml_ext(QObject::tr("gpml"));
		static const QString filter_gpml_gz(QObject::tr("Compressed GPML (*.gpml.gz)"));
		static const QString filter_gpml_gz_ext(QObject::tr("gpml.gz"));
		static const QString filter_all(QObject::tr("All files (*)"));
		static const QString filter_all_ext(QObject::tr(""));
		
		// Determine whether file contains reconstructable and/or reconstruction features.
		const GPlatesAppLogic::ClassifyFeatureCollection::classifications_type classification =
				file_state.get_feature_collection_classification(file_iter);
		const bool has_reconstructable_features = classification.test(
				GPlatesAppLogic::ClassifyFeatureCollection::RECONSTRUCTABLE);
		const bool has_reconstruction_features = classification.test(
				GPlatesAppLogic::ClassifyFeatureCollection::RECONSTRUCTION);

		std::vector<std::pair<QString, QString> > filters;
		
		// Build the list of filters depending on the original file format.
		switch ( file_iter->get_loaded_file_format() )
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
			{
				filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				filters.push_back(make_pair(filter_line, filter_line_ext));
				filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
			{
				filters.push_back(make_pair(filter_line, filter_line_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
				filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
			{
				filters.push_back(make_pair(filter_rotation, filter_rotation_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;
			
		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			{
				filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				filters.push_back(make_pair(filter_line, filter_line_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;
		case GPlatesFileIO::FeatureCollectionFileFormat::GMAP:
			{
#if 0			
				// Disable any kind of export from GMAP data. 
				break;
#else			
				// No GMAP writing yet. (Ever?).
				// Offer gmpl/gpml_gz export.
				// 
				// (Probably not useful to export these in shapefile or plates formats
				// for example - users would get the geometries but nothing else).
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext));
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext));
				}
				break;
#endif
			}
		case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
		case GPlatesFileIO::FeatureCollectionFileFormat::UNKNOWN:
		default: // If no file extension then use same options as GMPL.
			{
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext)); // Save uncompressed by default, same as original
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext)); // Option to change to compressed version.
				}
				if (has_reconstructable_features)
				{
					// Only offer to save in line-only formats if feature collection
					// actually has reconstructable features in it!
					filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
					filters.push_back(make_pair(filter_line, filter_line_ext));
					filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				}
				if (has_reconstruction_features)
				{
					// Only offer to save as PLATES4 .rot if feature collection
					// actually has rotations in it!
					filters.push_back(make_pair(filter_rotation, filter_rotation_ext));
				}
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
		case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
			{
				if (has_gzip)
				{
					filters.push_back(make_pair(filter_gpml_gz, filter_gpml_gz_ext)); // Save compressed by default, assuming we can.
				}
				filters.push_back(make_pair(filter_gpml, filter_gpml_ext)); // Option to change to uncompressed version.
				if (has_reconstructable_features)
				{
					// Only offer to save in line-only formats if feature collection
					// actually has reconstructable features in it!
					filters.push_back(make_pair(filter_gmt, filter_gmt_ext));
					filters.push_back(make_pair(filter_line, filter_line_ext));
					filters.push_back(make_pair(filter_shapefile, filter_shapefile_ext));
				}
				if (has_reconstruction_features)
				{
					// Only offer to save as PLATES4 .rot if feature collection
					// actually has rotations in it!
					filters.push_back(make_pair(filter_rotation, filter_rotation_ext));
				}
				filters.push_back(make_pair(filter_all, filter_all_ext));
			}
			break;
		}

		return filters;
	}


	GPlatesQtWidgets::ManageFeatureCollectionsStateWidget *
	get_state_widget(
			QTableWidget *qtable_widget,
			int row)
	{
		return dynamic_cast<GPlatesQtWidgets::ManageFeatureCollectionsStateWidget *>(
				qtable_widget->cellWidget(row, ColumnNames::LAYER_TYPES));
	}


	GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *
	get_action_widget(
			QTableWidget *qtable_widget,
			int row)
	{
		return dynamic_cast<GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *>(
				qtable_widget->cellWidget(row, ColumnNames::ACTIONS));
	}
}


GPlatesQtWidgets::ManageFeatureCollectionsDialog::ManageFeatureCollectionsDialog(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_file_state(file_state),
	d_feature_collection_file_io(&feature_collection_file_io),
	d_open_file_path(""),
	d_save_file_as_dialog_ptr(
			SaveFileDialog::get_save_file_dialog(
				parent_,
				"Save File As",
				std::vector<std::pair<QString, QString> >())),
	d_save_file_copy_dialog_ptr(
			SaveFileDialog::get_save_file_dialog(
				parent_,
				"Save a copy of the file with a different name",
				std::vector<std::pair<QString, QString> >())),
	d_gzip_available(false)
{
	setupUi(this);
	
	// Try to adjust column widths.
	QHeaderView *header = table_feature_collections->horizontalHeader();
	header->setResizeMode(ColumnNames::FILENAME, QHeaderView::Stretch);
	header->resizeSection(ColumnNames::FORMAT, 128);
	header->resizeSection(ColumnNames::LAYER_TYPES, 88);
	header->resizeSection(ColumnNames::ACTIONS, 216);

	// Enforce minimum row height for the Actions widget's sake.
	QHeaderView *sider = table_feature_collections->verticalHeader();
	sider->setResizeMode(QHeaderView::Fixed);
	sider->setDefaultSectionSize(34);
	
	// Set up slots for Open File and Save All
	QObject::connect(button_open_file, SIGNAL(clicked()), this, SLOT(open_files()));
	QObject::connect(button_save_all, SIGNAL(clicked()), this, SLOT(save_all()));
	
	// Set up slots for file load/unload notifications.
	connect_to_file_state_signals();


	// Test if we can offer on-the-fly gzip compression.
	// FIXME: Ideally we should let the user know WHY we're concealing this option.
	// The user will still be able to type in a .gpml.gz file name and activate the
	// gzipped saving code, however this will produce an exception which pops up
	// a suitable message box (See ViewportWindow.cc)
	d_gzip_available = GPlatesFileIO::GpmlOnePointSixOutputVisitor::gzip_program().test();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::update()
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range it_range =
			d_file_state.get_loaded_files();
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator it = it_range.begin;
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator end = it_range.end;
	
	clear_rows();
	for (; it != end; ++it) {
		add_row(it);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::edit_configuration(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	// The "edit configuration" method only makes sense for shapefiles 
	// (until there is some sort of equivalent requirement for other types of 
	// feature collection), and as such, only shapefiles have the "edit configuration" 
	// icon enabled in the ManageFeatureCollectionsActionWidget. 
	//  
	// For shapefiles, "edit configuration" translates to "re-map shapefile attributes to model properties". 
	// 
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it =
			action_widget_ptr->get_file_info_iterator();

	d_feature_collection_file_io->remap_shapefile_attributes(file_it);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator file =
			action_widget_ptr->get_file_info_iterator();

	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
		get_feature_collection_write_format(file->get_file_info());
	
	// Save the feature collection.
	save_file(
			file->get_file_info(),
			file->get_const_feature_collection(),
			feature_collection_write_format);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file_as(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator file =
			action_widget_ptr->get_file_info_iterator();

	d_save_file_as_dialog_ptr->set_filters(
			get_output_filters_for_file(
				d_file_state,
				file,
				d_gzip_available));
	QString file_path = file->get_file_info().get_qfileinfo().filePath();
	if (file_path != "")
	{
		d_save_file_as_dialog_ptr->select_file(file_path);
	}
	boost::optional<QString> filename_opt = d_save_file_as_dialog_ptr->get_file_name();

	if (!filename_opt)
	{
		return;
	}
	
	QString &filename = *filename_opt;

	// Make a new FileInfo object to tell save_file() what the new name should be.
	// This also copies any other info stored in the FileInfo.
	GPlatesFileIO::FileInfo new_fileinfo = GPlatesFileIO::create_copy_with_new_filename(
			filename, file->get_file_info());

	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
		get_feature_collection_write_format(new_fileinfo);

	// Save the feature collection.
	save_file(
			new_fileinfo,
			file->get_const_feature_collection(),
			feature_collection_write_format);

	// Wrap the same feature collection up with the new FileInfo.
	const GPlatesFileIO::File::shared_ref new_file = GPlatesFileIO::File::create_save_file(
			*file, new_fileinfo);

	// Update the name of the file by replacing with the new File object.
	// NOTE: this also removes the old file object thus making 'file'
	// a dangling reference.
	d_file_state.reset_file(file, new_file);

	update();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file_copy(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator file =
			action_widget_ptr->get_file_info_iterator();
	
	d_save_file_copy_dialog_ptr->set_filters(
			get_output_filters_for_file(
				d_file_state,
				file,
				d_gzip_available));
	QString file_path = file->get_file_info().get_qfileinfo().filePath();
	if (file_path != "")
	{
		d_save_file_copy_dialog_ptr->select_file(file_path);
	}
	boost::optional<QString> filename_opt = d_save_file_copy_dialog_ptr->get_file_name();

	if (!filename_opt)
	{
		return;
	}
	
	QString &filename = *filename_opt;

	// Make a new FileInfo object to tell save_file() what the copy name should be.
	// This also copies any other info stored in the FileInfo.
	GPlatesFileIO::FileInfo new_fileinfo = GPlatesFileIO::create_copy_with_new_filename(
			filename, file->get_file_info());

	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
			get_feature_collection_write_format(new_fileinfo);

	// Save the feature collection.
	save_file(
			new_fileinfo,
			file->get_const_feature_collection(),
			feature_collection_write_format);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::reload_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it =
			action_widget_ptr->get_file_info_iterator();
	
	reload_file(file_it);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::unload_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it =
			action_widget_ptr->get_file_info_iterator();
	
	d_feature_collection_file_io->unload_file(file_it);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::set_reconstructable_state_for_file(
		ManageFeatureCollectionsStateWidget *state_widget_ptr,
		bool activate)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it =
			state_widget_ptr->get_file_iterator();

#if 1
	// A temporary hack to hijack the active reconstructable GUI button to activate/deactivate
	// all the workflows (instead of activate/deactivate generation/rendering of RFGs).
	//
	// FIXME: When the GUI supports workflows (in FeatureCollectionFileState) then
	// remove this code.
	if (d_file_state.is_reconstructable_workflow_using_file(file_it))
	{
		d_file_state.set_file_active_reconstructable(file_it, activate);
	}
	typedef std::vector<GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type>
			workflow_seq_type;
	const workflow_seq_type workflow_tags = d_file_state.get_workflow_tags(file_it);
	// FIXME: HACK: We're hijacking the set reconstructable active button to set all
	// the workflows active instead - we're relying on the fact that the reconstructable
	// workflow will not be interested in this file if any other workflow is.
	workflow_seq_type::const_iterator tags_iter = workflow_tags.begin();
	const workflow_seq_type::const_iterator tag_end = workflow_tags.end();
	for ( ; tags_iter != tag_end; ++tags_iter )
	{
		d_file_state.set_file_active_workflow(file_it, *tags_iter, activate);
	}
#else
	// File will only be activated if it contains reconstructable features.
	d_file_state.set_file_active_reconstructable(file_it, activate);
#endif
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::set_reconstruction_state_for_file(
		ManageFeatureCollectionsStateWidget *state_widget_ptr,
		bool activate)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it =
			state_widget_ptr->get_file_iterator();

	// Activate the new reconstruction file.
	d_file_state.set_file_active_reconstruction(file_it, activate);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::open_files()
{
	static const QString filters = tr(
			"All loadable files (*.dat *.pla *.rot *.shp *.gpml *.gpml.gz *.vgp);;"
			"PLATES4 line (*.dat *.pla);;"
			"PLATES4 rotation (*.rot);;"
			"ESRI shapefile (*.shp);;"
			"GPlates Markup Language (*.gpml *.gpml.gz);;"
			"GMAP VGP file (*.vgp);;"
			"All files (*)" );
	
	QStringList filenames = QFileDialog::getOpenFileNames(parentWidget(),
			tr("Open Files"), d_open_file_path, filters);

	open_files(filenames);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::end_add_feature_collections(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_begin,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_end)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_file_iter = new_files_begin;
	for ( ; new_file_iter != new_files_end; ++new_file_iter)
	{
		add_row(new_file_iter);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::begin_remove_feature_collection(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator unload_file_it)
{
	const int row = find_row(unload_file_it);

	if (row != table_feature_collections->rowCount())
	{
		remove_row(row);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::reconstructable_file_activation(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it,
		bool activation)
{
	const int row = find_row(file_it);

	if (row != table_feature_collections->rowCount())
	{
		ManageFeatureCollectionsStateWidget *state_widget = get_state_widget(
				table_feature_collections, row);
		if (state_widget)
		{
			const bool enable_reconstructable =
					file_state.is_reconstructable_workflow_using_file(file_it);
			state_widget->update_reconstructable_state(activation, enable_reconstructable);
		}

		ManageFeatureCollectionsActionWidget *action_widget = get_action_widget(
				table_feature_collections, row);
		if (action_widget)
		{
			action_widget->update_state();
		}
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::reconstruction_file_activation(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it,
		bool activation)
{
	const int row = find_row(file_it);

	if (row != table_feature_collections->rowCount())
	{
		ManageFeatureCollectionsStateWidget *state_widget = get_state_widget(
				table_feature_collections, row);
		if (state_widget)
		{
			const bool enable_reconstruction =
					file_state.is_reconstruction_workflow_using_file(file_it);
			state_widget->update_reconstruction_state(activation, enable_reconstruction);
		}

		ManageFeatureCollectionsActionWidget *action_widget = get_action_widget(
				table_feature_collections, row);
		if (action_widget)
		{
			action_widget->update_state();
		}
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::workflow_file_activation(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it,
		const GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type &workflow_tag,
		bool activation)
{
	const int row = find_row(file_it);

	if (row != table_feature_collections->rowCount())
	{
		ManageFeatureCollectionsStateWidget *state_widget = get_state_widget(
				table_feature_collections, row);
		if (state_widget)
		{
#if 1
			// A temporary hack to hijack the active reconstructable GUI button to activate/deactivate
			// all the workflows (instead of activate/deactivate generation/rendering of RFGs).
			//
			// FIXME: When the GUI supports workflows (in FeatureCollectionFileState) then
			// remove this code.
			//
			// Enable reconstructable button if the reconstructable workflow is using the file *or*
			// if there are any other workflows using it.
			const bool enable_reconstructable =
					file_state.is_reconstructable_workflow_using_file(file_it) ||
					!file_state.get_workflow_tags(file_it).empty();
			state_widget->update_reconstructable_state(activation, enable_reconstructable);
#else
			const bool enable_workflow =
					file_state.is_file_using_workflow(file_it, workflow_tag);
			state_widget->update_workflow_state(activation, enable_workflow);
#endif
		}

		ManageFeatureCollectionsActionWidget *action_widget = get_action_widget(
				table_feature_collections, row);
		if (action_widget)
		{
			action_widget->update_state();
		}
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::connect_to_file_state_signals()
{
	QObject::connect(
			&d_file_state,
			SIGNAL(end_add_feature_collections(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(end_add_feature_collections(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));

	QObject::connect(
			&d_file_state,
			SIGNAL(begin_remove_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(begin_remove_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));

	QObject::connect(
			&d_file_state,
			SIGNAL(reconstructable_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
				bool)),
			this,
			SLOT(reconstructable_file_activation(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
					bool)));

	QObject::connect(
			&d_file_state,
			SIGNAL(reconstruction_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
				bool)),
			this,
			SLOT(reconstruction_file_activation(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
					bool)));

	QObject::connect(
			&d_file_state,
			SIGNAL(workflow_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
				const GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type &,
				bool)),
			this,
			SLOT(workflow_file_activation(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
					const GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type &,
					bool)));
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::clear_rows()
{
	table_feature_collections->clearContents();	// Do not clear the header items as well.
	table_feature_collections->setRowCount(0);	// Do remove the newly blanked rows.
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::add_row(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file)
{
	const GPlatesFileIO::FileInfo &file_info = file->get_file_info();

	// Obtain information from the FileInfo
	const QFileInfo &qfileinfo = file_info.get_qfileinfo();

	// Some files might not actually exist yet if the user created a new
	// feature collection internally and hasn't saved it to file yet.
	QString display_name;
	if (GPlatesFileIO::file_exists(file_info))
	{
		display_name = file_info.get_display_name(false);
	}
	else
	{
		// The file doesn't exist so give it a filename to indicate this.
		display_name = "New Feature Collection";
	}
	
	QString filename_str = qfileinfo.fileName();
	QString filepath_str = qfileinfo.path();
	QString format_str = get_format_for_file(qfileinfo);
#if 1
	// A temporary hack to hijack the active reconstructable GUI button to activate/deactivate
	// all the workflows (instead of activate/deactivate generation/rendering of RFGs).
	//
	// FIXME: When the GUI supports workflows (in FeatureCollectionFileState) then
	// remove this code.
	//
	// Check the reconstructable button if the file is activated in the reconstructable workflow
	// *or* if it is activated with any other workflow.
	bool active_reconstructable = d_file_state.is_reconstructable_workflow_using_file(file);
	typedef std::vector<GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type>
			workflow_tag_seq_type;
	const workflow_tag_seq_type workflow_tags = d_file_state.get_workflow_tags(file);
	workflow_tag_seq_type::const_iterator workflow_tags_iter = workflow_tags.begin();
	workflow_tag_seq_type::const_iterator workflow_tags_end = workflow_tags.end();
	for ( ; workflow_tags_iter != workflow_tags_end; ++workflow_tags_iter)
	{
		if (d_file_state.is_file_active_workflow(file, *workflow_tags_iter))
		{
			active_reconstructable = true;
		}
	}
	// Enable reconstructable button if the reconstructable workflow is using the file *or*
	// if there are any other workflows using it.
	const bool enable_reconstructable = d_file_state.is_reconstructable_workflow_using_file(file) ||
			!d_file_state.get_workflow_tags(file).empty();
#else
	const bool active_reconstructable = d_file_state.is_file_active_reconstructable(file);
	const bool enable_reconstructable = d_file_state.is_reconstructable_workflow_using_file(file);
#endif
	const bool active_reconstruction = d_file_state.is_file_active_reconstruction(file);
	const bool enable_reconstruction = d_file_state.is_reconstruction_workflow_using_file(file);
	
	// Add blank row.
	int row = table_feature_collections->rowCount();
	table_feature_collections->insertRow(row);
	
	// Add filename item.
	QTableWidgetItem *filename_item = new QTableWidgetItem(display_name);
	filename_item->setToolTip(tr("Location: %1").arg(filepath_str));
	filename_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, ColumnNames::FILENAME, filename_item);

	// Add file format item.
	QTableWidgetItem *format_item = new QTableWidgetItem(format_str);
	format_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, ColumnNames::FORMAT, format_item);

	// Add layer type / in use status.
	ManageFeatureCollectionsStateWidget *state_widget_ptr =
			new ManageFeatureCollectionsStateWidget(*this, file, 
					active_reconstructable, active_reconstruction,
					enable_reconstructable, enable_reconstruction,
					this);
	table_feature_collections->setCellWidget(row, ColumnNames::LAYER_TYPES, state_widget_ptr);
	
	// Add action buttons widget.
	ManageFeatureCollectionsActionWidget *action_widget_ptr =
			new ManageFeatureCollectionsActionWidget(*this, file, this);
	table_feature_collections->setCellWidget(row, ColumnNames::ACTIONS, action_widget_ptr);
	
	// Enable the edit_configuration button if we have a shapefile. 
	if (format_str == "ESRI shapefile")
	{
		action_widget_ptr->enable_edit_configuration_button();
	}
}


int
GPlatesQtWidgets::ManageFeatureCollectionsDialog::find_row(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	int row = 0;
	int end = table_feature_collections->rowCount();
	for (; row < end; ++row) {
		if (get_action_widget(table_feature_collections, row) == action_widget_ptr) {
			return row;
		}
	}
	return end;
}


int
GPlatesQtWidgets::ManageFeatureCollectionsDialog::find_row(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it)
{
	// Look for the action widget that references 'file_it'.
	int row = 0;
	int end = table_feature_collections->rowCount();
	for ( ; row < end; ++row)
	{
		ManageFeatureCollectionsActionWidget *action_widget = get_action_widget(
				table_feature_collections, row);
		if (action_widget &&
				action_widget->get_file_info_iterator() == file_it)
		{
			return row;
		}
	}

	return end;
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::remove_row(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	int row = find_row(action_widget_ptr);
	remove_row(row);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::remove_row(
		int row)
{
	if (row < table_feature_collections->rowCount()) {
		table_feature_collections->removeRow(row);
	}
}


GPlatesFileIO::FeatureCollectionWriteFormat::Format
GPlatesQtWidgets::ManageFeatureCollectionsDialog::get_feature_collection_write_format(
	const GPlatesFileIO::FileInfo& file_info)
{
	switch (get_feature_collection_file_format(file_info) )
	{
	case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
		{
			GMTHeaderFormatDialog gmt_header_format_dialog(parentWidget());
			gmt_header_format_dialog.exec();

			return gmt_header_format_dialog.get_header_format();
		}

	default:
		return GPlatesFileIO::FeatureCollectionWriteFormat::USE_FILE_EXTENSION;
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::open_files(
		const QStringList &filenames)
{
	try_catch_file_load(
			boost::bind(
					&GPlatesAppLogic::FeatureCollectionFileIO::load_files,
					d_feature_collection_file_io,
					filenames));

	// Set open file path to the last opened file.
	if ( ! filenames.isEmpty() )
	{
		QFileInfo last_opened_file(filenames.last());
		d_open_file_path = last_opened_file.path();
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::reload_file(
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator &file_it)
{
	try_catch_file_load(
			boost::bind(
					&GPlatesAppLogic::FeatureCollectionFileIO::reload_file,
					d_feature_collection_file_io,
					file_it));
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::try_catch_file_load(
		boost::function<void ()> file_load_func)
{
	try
	{
		file_load_func();
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
		QString message = tr("Error: Unexpected error loading file - ignoring file.");
		QMessageBox::critical(this, tr("Error Opening File"), message,
				QMessageBox::Ok, QMessageBox::Ok);

		std::cerr << "Caught exception: " << e << std::endl;
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file(
		const GPlatesFileIO::FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format)
{
	try
	{
		// Save the feature collection.
		GPlatesAppLogic::FeatureCollectionFileIO::save_file(
				file_info,
				feature_collection,
				feature_collection_write_format);
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &e)
	{
		QString message = tr("An error occurred while saving the file '%1'")
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesFileIO::ErrorOpeningPipeToGzipException &e)
	{
		QString message = tr("GPlates was unable to use the '%1' program to save the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to save"
				" files without compression.")
				.arg(e.command())
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesGlobal::InvalidFeatureCollectionException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Attempted to write an invalid feature collection.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesGlobal::UnexpectedEmptyFeatureCollectionException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Attempted to write an empty feature collection.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Writing files in this format is currently not supported.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesFileIO::OgrException &)
	{
		QString message = tr("An OGR error occurred.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
}
