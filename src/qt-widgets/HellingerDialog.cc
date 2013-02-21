/* $Id: HellingerDialog.cc 260 2012-05-30 13:47:23Z robin.watson@ngu.no $ */

/**
 * \file 
 * $Revision: 260 $
 * $Date: 2012-05-30 15:47:23 +0200 (Wed, 30 May 2012) $ 
 * 
 * Copyright (C) 2011, 2012, 2013 Geological Survey of Norway
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
#include <vector>

#include <QDebug>
#include <QProgressBar>
#include <QTextStream>


#include "api/PythonInterpreterLocker.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "file-io/HellingerReader.h"
#include "file-io/HellingerWriter.h"
#include "global/CompilerWarnings.h"
#include "maths/PointOnSphere.h"
#include "presentation/ViewState.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "HellingerDialog.h"
#include "HellingerEditPoint.h"
#include "HellingerEditSegment.h"
#include "HellingerErrorOrder.h"
#include "HellingerErrorLatLonRho.h"
#include "HellingerErrorPythonFile.h"
#include "HellingerNewPoint.h"
#include "HellingerNewSegment.h"
#include "HellingerRemoveError.h"
#include "HellingerStatsDialog.h"
#include "HellingerThread.h"
#include "ReadErrorAccumulationDialog.h"

#include "QtWidgetUtils.h"


// FIXME: is this required here?
DISABLE_GCC_WARNING("-Wwrite-strings")

const double slider_multiplier = -10000.;

namespace{

	enum PickColumns
	{
		SEGMENT_NUMBER,
		SEGMENT_TYPE,
		LAT,
		LON,
		UNCERTAINTY
	};

	void
	add_pick_to_segment(
		QTreeWidgetItem *parent_item,
		const int &segment_number,
		const GPlatesQtWidgets::Pick &pick)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(SEGMENT_NUMBER, QString::number(segment_number));
		item->setText(SEGMENT_TYPE, QString::number(pick.segment_type));
		item->setText(LAT, QString::number(pick.lat));
		item->setText(LON, QString::number(pick.lon));
		item->setText(UNCERTAINTY, QString::number(pick.uncert));
		parent_item->addChild(item);

		if (!pick.is_enabled)
		{
			item->setTextColor(SEGMENT_NUMBER,Qt::gray);
			item->setTextColor(SEGMENT_TYPE,Qt::gray);
			item->setTextColor(LAT,Qt::gray);
			item->setTextColor(LON,Qt::gray);
			item->setTextColor(UNCERTAINTY,Qt::gray);
		}
	}

	void
	add_pick_to_tree(
		const int &segment_number,
		const GPlatesQtWidgets::Pick &pick,
		QTreeWidget *tree)
	{
		QString segment_as_string = QString::number(segment_number);
		QList<QTreeWidgetItem*> items = tree->findItems(
			segment_as_string, Qt::MatchExactly, 0);
		QTreeWidgetItem *item;
		if (items.isEmpty())
		{
			item = new QTreeWidgetItem(tree);
			item->setText(0, segment_as_string);

		}
		else
		{
			item = items.at(0);
		}
		add_pick_to_segment(item, segment_number, pick);
	}
}

GPlatesQtWidgets::HellingerDialog::HellingerDialog(
        GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ReadErrorAccumulationDialog &read_error_accumulation_dialog,
		QWidget *parent_):
	GPlatesDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    d_view_state(view_state),
    d_hellinger_layer(*view_state.get_rendered_geometry_collection().get_main_rendered_layer(
                          GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_TOOL_LAYER)),
	d_read_error_accumulation_dialog(read_error_accumulation_dialog),
    d_hellinger_model(0),
    d_hellinger_stats_dialog(0),
    d_hellinger_new_point(0),
    d_hellinger_edit_point(0),
    d_hellinger_new_segment(0),
    d_hellinger_edit_segment(0),
    d_hellinger_error_order(0),
    d_hellinger_error_lat_lon_rho(0),
    d_hellinger_error_python_file(0),
    d_hellinger_remove_error(0),
    d_hellinger_thread(0),
    d_moving_plate_id(0),
    d_fixed_plate_id(0),
    d_recon_time(0.),
    d_chron_time(0.),
    d_moving_symbol(GPlatesGui::Symbol::CROSS, 2, true),
    d_fixed_symbol(GPlatesGui::Symbol::SQUARE,2, false),
    d_thread_type(0)
{
	setupUi(this);

    QObject::connect(button_apply_fit, SIGNAL(clicked()),this, SLOT(handle_calculate()));
        QObject::connect(button_import_file, SIGNAL(clicked()), this, SLOT(import_pick_file()));
        QObject::connect(button_details, SIGNAL(clicked()), this, SLOT(show_stat_details()));
        QObject::connect(button_new_point, SIGNAL(clicked()), this, SLOT(handle_add_new_point()));
        QObject::connect(button_export_file, SIGNAL(clicked()), this, SLOT(handle_export_file()));
        QObject::connect(button_expand_all, SIGNAL(clicked()), this, SLOT(handle_expand_all()));
        QObject::connect(button_collapse_all, SIGNAL(clicked()), this, SLOT(handle_collapse_all()));
        QObject::connect(button_edit_point, SIGNAL(clicked()), this, SLOT(handle_edit_point()));
        QObject::connect(button_remove_point, SIGNAL(clicked()), this, SLOT(handle_remove_point()));
        QObject::connect(button_remove_segment, SIGNAL(clicked()), this, SLOT(handle_remove_segment()));
        QObject::connect(button_new_segment, SIGNAL(clicked()), this, SLOT(handle_add_new_segment()));
        QObject::connect(button_edit_segment, SIGNAL(clicked()), this, SLOT(handle_edit_segment()));
        QObject::connect(button_stats, SIGNAL(clicked()), this, SLOT(handle_calculate_stats()));
        QObject::connect(button_activate_pick, SIGNAL(clicked()), this, SLOT(handle_pick_state_changed()));
        QObject::connect(button_deactivate_pick, SIGNAL(clicked()), this, SLOT(handle_pick_state_changed()));
        QObject::connect(button_renumber, SIGNAL(clicked()), this, SLOT(renumber_segments()));
        QObject::connect(button_close, SIGNAL(rejected()), this, SLOT(close_dialog()));
        QObject::connect(spinbox_chron, SIGNAL(valueChanged(double)), this, SLOT(handle_chron_time_changed(double)));
        QObject::connect(spinbox_recon_time, SIGNAL(valueChanged(double)), this, SLOT(handle_recon_time_spinbox_changed(double)));
        QObject::connect(slider_recon_time, SIGNAL(valueChanged(int)), this, SLOT(handle_recon_time_slider_changed(int)));
        QObject::connect(spinbox_result_lat, SIGNAL(valueChanged(double)), this, SLOT(handle_fit_spinboxes_changed()));
        QObject::connect(spinbox_result_lon, SIGNAL(valueChanged(double)), this, SLOT(handle_fit_spinboxes_changed()));
        QObject::connect(spinbox_result_angle, SIGNAL(valueChanged(double)), this, SLOT(handle_fit_spinboxes_changed()));
        QObject::connect(spinbox_radius, SIGNAL(valueChanged(double)), this, SLOT(handle_spinbox_radius_changed()));
        QObject::connect(checkbox_grid_search, SIGNAL(clicked()), this, SLOT(handle_checkbox_grid_search_changed()));
        QObject::connect(treeWidget,SIGNAL(collapsed(QModelIndex)),this,SLOT(update_expanded_status()));

		QObject::connect(treeWidget->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
             this, SLOT(handle_selection_changed(const QItemSelection &, const QItemSelection &)));
        d_hellinger_thread = new HellingerThread(this, d_hellinger_model);
		QObject::connect(d_hellinger_thread, SIGNAL(finished()),this, SLOT(handle_thread_finished()));


        //FIXME: think about when we should deactivate this layer....and/or do we make it an orthogonal layer?
        d_view_state.get_rendered_geometry_collection().set_main_layer_active(
                    GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_TOOL_LAYER);
        d_hellinger_layer.set_active();

		// Look in system-specific locations for supplied sample scripts, site-specific scripts, etc.
		// The default location will be platform-dependent and is currently set up in UserPreferences.cc.
		d_python_path = d_view_state.get_application_state().get_user_preferences().get_value("paths/python_system_script_dir").toString();
		qDebug() << "python path: " << d_python_path;

        //d_python_path = QString("scripts");
		d_python_path.append(QDir::separator());
        d_python_file = d_python_path + "py_hellinger.py";
        d_temporary_folder = d_python_path;
        d_temporary_folder.append(QDir::separator());
        d_temp_pick_file = QString("temp_file");
        d_temp_result = QString("temp_file_temp_result");
        d_temp_par = QString("temp_file_par");
        d_temp_res = QString("temp_file_res");

		d_hellinger_model = new HellingerModel(d_python_path);

		progress_bar->setEnabled(false);
        progress_bar->setMinimum(0.);
        progress_bar->setMaximum(1.);
        progress_bar->setValue(0.);
        update();
        groupbox_rotation->hide();

}

 void
 GPlatesQtWidgets::HellingerDialog::handle_selection_changed(
         const QItemSelection & new_selection,
         const QItemSelection & old_selection)
 {

     const QModelIndex index = treeWidget->selectionModel()->currentIndex();
     QString segment = treeWidget->currentItem()->text(0);
     int row = index.row();
     int segment_int = segment.toInt();
     bool state = d_hellinger_model->get_state(segment_int, row);
     if (state == 1)
     {
      button_activate_pick->setEnabled(false);
      button_deactivate_pick->setEnabled(true);
     }
     else if (state == 0)
     {
         button_activate_pick->setEnabled(true);
         button_deactivate_pick->setEnabled(false);
     }

     if (treeWidget->currentItem()->text(1) == 0)
     {
       button_edit_point -> setEnabled(false);
       button_edit_segment -> setEnabled(true);
       button_remove_segment ->setEnabled(true);
       button_remove_point -> setEnabled(false);
       button_activate_pick->setEnabled(false);
       button_deactivate_pick->setEnabled(false);
     }
	 else
     {
       button_edit_point -> setEnabled(true);
       button_edit_segment -> setEnabled(false);
       button_remove_segment ->setEnabled(false);
       button_remove_point -> setEnabled(true);
     }

     d_hellinger_layer.clear_rendered_geometries();
     update_canvas();

     if (treeWidget->currentItem()->text(1) != 0)
     {
         double lat = treeWidget->currentItem()->text(2).toDouble();
         double lon = treeWidget->currentItem()->text(3).toDouble();
         int type_segment = treeWidget->currentItem()->text(1).toInt();
         show_point_globe(lat, lon, type_segment);

	 }
 }

void
GPlatesQtWidgets::HellingerDialog::show_point_globe(
        const double &lat,
        const double &lon,
        const int &type_segment)
{

    GPlatesGui::Symbol moving_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, 2, true);
    GPlatesGui::Symbol fixed_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::SQUARE, 2, false);
    GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
                GPlatesMaths::LatLonPoint(lat,lon));
    GPlatesViewOperations::RenderedGeometry pick_geometry =
            GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
                point.get_non_null_pointer(),
                GPlatesGui::Colour::get_yellow(),
                2, /* point thickness */
                2, /* line thickness */
                false /* fill */,
                type_segment == MOVING_SEGMENT_TYPE ? moving_symbol : fixed_symbol);

    d_hellinger_layer.add_rendered_geometry(pick_geometry);

}

void
GPlatesQtWidgets::HellingerDialog::handle_pick_state_changed()
{
    const QModelIndex index = treeWidget->selectionModel()->currentIndex();
    QString segment = treeWidget->currentItem()->text(0);
    int row = index.row();
    int segment_int = segment.toInt();
    QStringList get_data_line = d_hellinger_model->get_line(segment_int, row);
    QStringList data_to_model;
    QString segment_str = get_data_line.at(0);
    QString move_fix = get_data_line.at(1);
    QString lat = get_data_line.at(2);
    QString lon = get_data_line.at(3);
    QString uncert = get_data_line.at(4);

    bool state = d_hellinger_model->get_state(segment_int, row);
    if (state)  // 1 = true
    {
        if (move_fix == "1")
        {
            move_fix = QString("%1").arg(COMMENT_MOVING_SEGMENT_TYPE);
            data_to_model << move_fix << segment_str << lat << lon << uncert;
            d_hellinger_model->remove_line(segment_int,row);
            d_hellinger_model->add_pick(data_to_model);
            update();
        }
        else if (move_fix == "2")
        {
            move_fix = QString("%1").arg(COMMENT_FIXED_SEGMENT_TYPE);
            data_to_model << move_fix << segment_str << lat << lon << uncert;
            d_hellinger_model->remove_line(segment_int,row);
            d_hellinger_model->add_pick(data_to_model);
            update();
        }

    }
    else if (!state)   // 2 = false
    {
        QString change_state = "0";
        data_to_model << move_fix << segment_str << lat << lon << uncert;
//        qDebug()<<data_to_model<<change_state;
        d_hellinger_model->remove_line(segment_int,row);
        d_hellinger_model->add_pick(data_to_model);
        update();
    }
	reset_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::reset_picks_globe()
{
    d_hellinger_layer.clear_rendered_geometries();
    update_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::handle_edit_point()
{
    const QModelIndex index = treeWidget->selectionModel()->currentIndex();
	if (!d_hellinger_edit_point)
	{
		d_hellinger_edit_point = new GPlatesQtWidgets::HellingerEditPoint(this, d_hellinger_model);
	}
    QString segment = treeWidget->currentItem()->text(0);
    int row = index.row();
    int segment_int = segment.toInt();
    //d_hellinger_model ->get_line(segment_int, row);
    d_hellinger_edit_point->initialization(segment_int, row);
    d_hellinger_edit_point->exec();
    reset_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_edit_segment()
{
    if (!d_hellinger_edit_segment)
    {
        d_hellinger_edit_segment = new GPlatesQtWidgets::HellingerEditSegment(this,d_hellinger_model);
    }
    QString segment = treeWidget->currentItem()->text(0);
    int segment_int = segment.toInt();
    d_hellinger_edit_segment->initialization(segment_int);
    d_hellinger_edit_segment->exec();
    reset_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_remove_point()
{
	if (!d_hellinger_remove_error)
	{
		d_hellinger_remove_error = new GPlatesQtWidgets::HellingerRemoveError(this);
	}
    d_hellinger_remove_error->exec();
    bool remove_ok = d_hellinger_remove_error->get_status();
    if (remove_ok)
    {
    const QModelIndex index = treeWidget->selectionModel()->currentIndex();
    QString segment = treeWidget->currentItem()->text(0);
    int row = index.row();
    int segment_int = segment.toInt();
    d_hellinger_model->remove_line(segment_int, row);
    update();
    reset_expanded_status();
    }
}

void
GPlatesQtWidgets::HellingerDialog::handle_remove_segment()
{
	if (!d_hellinger_remove_error)
	{
		d_hellinger_remove_error = new GPlatesQtWidgets::HellingerRemoveError(this);
	}
    d_hellinger_remove_error->exec();
    bool remove_ok = d_hellinger_remove_error->get_status();
    if (remove_ok)
    {
    QString segment = treeWidget->currentItem()->text(0);
    int segment_int = segment.toInt();
    d_hellinger_model ->remove_segment(segment_int);
    button_renumber->setEnabled(true);
    update();
    reset_expanded_status();
    }
}

void
GPlatesQtWidgets::HellingerDialog::initialise()
{
    update_buttons();    
    update();
    reset_expanded_status();

}

void
GPlatesQtWidgets::HellingerDialog::import_pick_file()
{
    QString filters;
    filters = QObject::tr("Hellinger Pick file (*.pick)");
    filters += ";;";
    filters += QObject::tr("Hellinger com file (*.com)");
//    filters += ";;";
//    filters += QObject::tr("All files (*)");

    QString path = QFileDialog::getOpenFileName(
        this,
        QObject::tr("Open Hellinger Pick or Com file"),
        d_view_state.get_last_open_directory(),
        filters);

    if (path.isEmpty())
    {
        return;
    }
    QFile file(path);
    QFileInfo file_info(file.fileName());
    QStringList file_name = file_info.fileName().split(".", QString::SkipEmptyParts);
    QString type_file = file_name.at(1);

    d_hellinger_model->reset_model();



	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_error_accumulation_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();
    if (type_file == "pick")
    {
        GPlatesFileIO::HellingerReader::read_pick_file(path,*d_hellinger_model,read_errors);
    }
    else if (type_file == "com")
    {
         GPlatesFileIO::HellingerReader::read_com_file(path,*d_hellinger_model,read_errors);
    }

    
	d_read_error_accumulation_dialog.update();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors)
	{
		d_read_error_accumulation_dialog.show();
	}

	line_import_file -> setText(path);
    d_hellinger_layer.clear_rendered_geometries();
    update_buttons();
    update();
    initialise();
    handle_expand_all();

}

void
GPlatesQtWidgets::HellingerDialog::handle_spinbox_radius_changed()
{
    if (spinbox_radius->value()>0.0)
    {
        button_apply_fit->setEnabled(true);
    }
    else
    {
        button_apply_fit->setEnabled(false);
    }
}

void
GPlatesQtWidgets::HellingerDialog::update_initial_guess()
{
   boost::optional<GPlatesQtWidgets::com_file_struct> com_file_data = d_hellinger_model->get_initialization_guess();

   if (com_file_data)
   {
	   spinbox_lat->setValue(com_file_data.get().lat);
	   spinbox_lon->setValue(com_file_data.get().lon);
	   spinbox_rho->setValue(com_file_data.get().rho);
	   spinbox_radius->setValue(com_file_data.get().search_radius);
	   checkbox_grid_search->setChecked(com_file_data.get().grid_search);
	   spinbox_sig_level->setValue(com_file_data.get().significance_level);
	   if (com_file_data.get().estimated_kappa)
	   {
		   checkbox_kappa->setChecked(true);
	   }
	   else
	   {
		   checkbox_kappa->setChecked(false);
	   }

	   if (com_file_data.get().output_files)
	   {
		   checkbox_graphics->setChecked(true);
	   }
	   else
	   {
		   checkbox_graphics->setChecked(false);
	   }
	   d_filename_dat = com_file_data.get().dat_file;
	   d_filename_up = com_file_data.get().up_file;
	   d_filename_do = com_file_data.get().do_file;
   }

}

void
GPlatesQtWidgets::HellingerDialog::handle_calculate_stats()
{
    int script_part_stats = STATS_PART_SCRIPT;
    d_thread_type = STATS_PART_SCRIPT;
	update_buttons_statistics(false);
    d_hellinger_thread->initialization_stats_part(d_path,d_file_name,d_filename_dat,d_filename_up,d_filename_do, d_python_file, d_temporary_folder, d_temp_pick_file, d_temp_result, d_temp_par, d_temp_res);
    d_hellinger_thread->set_python_script(script_part_stats);
	progress_bar->setEnabled(true);
    progress_bar->setMaximum(0.);
    d_hellinger_thread->start();
}

void
GPlatesQtWidgets::HellingerDialog::handle_export_file()
{
	QString file_name = QFileDialog::getSaveFileName(this, tr("Save File"), "",
		tr("Text Files (*.txt);"));
	
	if (file_name != "")
	{
		GPlatesFileIO::HellingerWriter::write_pick_file(file_name,*d_hellinger_model);
	}
}

void
GPlatesQtWidgets::HellingerDialog::show_stat_details()
{
	if (!d_hellinger_stats_dialog)
	{
		d_hellinger_stats_dialog = new GPlatesQtWidgets::HellingerStatsDialog(d_python_path,this);
	}
	d_hellinger_stats_dialog->update();
    d_hellinger_stats_dialog->show();
}

void
GPlatesQtWidgets::HellingerDialog::handle_add_new_point()
{    
	if (!d_hellinger_new_point)
	{
		d_hellinger_new_point = new GPlatesQtWidgets::HellingerNewPoint(this, d_hellinger_model);
	}
    d_hellinger_new_point->exec();
    reset_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_add_new_segment()
{
    if (!d_hellinger_new_segment)
	{
		d_hellinger_new_segment = new GPlatesQtWidgets::HellingerNewSegment(this,d_hellinger_model);
	}
    d_hellinger_new_segment->exec();
    reset_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_calculate()
{        
	if (!d_hellinger_error_order)
	{
		d_hellinger_error_order = new GPlatesQtWidgets::HellingerErrorOrder(this,d_hellinger_model);
	}
	if (!d_hellinger_error_lat_lon_rho)
	{
		d_hellinger_error_lat_lon_rho = new GPlatesQtWidgets::HellingerErrorLatLonRho(this,d_hellinger_model);
	}
	if (!d_hellinger_error_python_file)
	{
		d_hellinger_error_python_file = new GPlatesQtWidgets::HellingerErrorPythonFile(this);
	}

    if (button_renumber->isEnabled())
    {        
        d_hellinger_error_order->exec();
    }

    if (!((spinbox_lat->value()>0) || (spinbox_lon->value()>0) || (spinbox_rho->value()>0)))
    {
        d_hellinger_error_lat_lon_rho->exec();
    }
    bool continue_ok = d_hellinger_model->get_error_order();
    bool continue_lat_lon_rho = d_hellinger_model->get_error_lat_lon_rho();
    QFile python_code(d_python_file);
    if (python_code.exists())
    {
        if (continue_ok)
        {
            if (continue_lat_lon_rho)
            {
                QString path = d_python_path + d_temp_pick_file;
				GPlatesFileIO::HellingerWriter::write_pick_file(path,*d_hellinger_model);
                QString import_file_line = QString("%1").arg(line_import_file->text());
                update_buttons();
				std::vector<double> input_data;
				std::vector<int>bool_data;
                input_data.push_back(spinbox_lat->value());
                input_data.push_back(spinbox_lon->value());
                input_data.push_back(spinbox_rho->value());
                input_data.push_back(spinbox_radius->value());
                input_data.push_back(spinbox_sig_level->value());
                int iteration;
                if (checkbox_grid_search->isChecked())
                {
                    int grid_search = 1;
                    iteration = spinbox_iteration->value();
                    bool_data.push_back(grid_search);

                }
                else
                {
                    int grid_search = 0;
                    iteration = 0;
                    bool_data.push_back(grid_search);

                }
                if (checkbox_kappa->isChecked())
                {
                    int kappa = 1;
                    bool_data.push_back(kappa);
                }
                else
                {
                    int kappa = 0;
                    bool_data.push_back(kappa);
                }
                if (checkbox_graphics->isChecked())
                {
                    int graphics = 1;
                    bool_data.push_back(graphics);
                }
                else
                {
                    int graphics = 0;
                    bool_data.push_back(graphics);
                }

                d_hellinger_thread->initialization_pole_part(import_file_line, input_data, bool_data, iteration, d_python_file, d_temporary_folder, d_temp_pick_file, d_temp_result, d_temp_par, d_temp_res);
                int script_part = POLE_PART_SCRIPT;
                d_thread_type = POLE_PART_SCRIPT;
                d_hellinger_model->reset_data_file();
                reset_picks_globe();
                d_hellinger_thread->set_python_script(script_part);
				
				progress_bar->setEnabled(true);
                progress_bar->setMaximum(0.);
                d_hellinger_thread->start();
            }
        }
    }
    else
    {
        d_hellinger_error_python_file->exec();

    }
}

void
GPlatesQtWidgets::HellingerDialog::handle_thread_finished()
{
	progress_bar->setEnabled(false);
    progress_bar->setMaximum(1.);
    if (d_thread_type == POLE_PART_SCRIPT)
    {
        QString path = d_python_path + "temp_file_temp_result";
        QFile data_file(path);
        if (data_file.open(QFile::ReadOnly))
        {
            QTextStream in(&data_file);
            QString line = in.readLine();
            QStringList fields = line.split(" ",QString::SkipEmptyParts);
            d_hellinger_model->add_results(fields);
            data_file.close();
            bool result_info = true;
            bool continue_info = true;
            results_python();
            update_buttons_statistics(result_info);
            update_continue_button(continue_info);
        }
    }
    else if(d_thread_type == STATS_PART_SCRIPT)
    {
        bool result_info = true;
        bool continue_info = false;
        update_buttons_statistics(result_info);
        update_continue_button(continue_info);
        d_hellinger_model->add_dat_file();
        show_data_points();
    }
}

void
GPlatesQtWidgets::HellingerDialog::update_buttons()
{

 if (treeWidget->topLevelItemCount() == 0)
 {
     button_expand_all -> setEnabled(false);
     button_collapse_all -> setEnabled(false);
     button_export_file -> setEnabled(false);
     button_apply_fit -> setEnabled(false);
     button_details -> setEnabled(false);
     button_apply_pole -> setEnabled(false);
     button_remove_segment -> setEnabled(false);
     button_remove_point -> setEnabled(false);
     button_stats -> setEnabled(false);

 }else{
     button_expand_all -> setEnabled(true);
     button_collapse_all -> setEnabled(true);
     button_export_file -> setEnabled(true);
     if (spinbox_radius->value() > 0.0)
     {
         button_apply_fit -> setEnabled(true);
     }
     else
     {
         button_apply_fit->setEnabled(false);
     }
     button_apply_pole -> setEnabled(true);
     button_remove_segment -> setEnabled(true);
     button_remove_point -> setEnabled(true);     
 }

 button_stats->setEnabled(false);
 button_details->setEnabled(false);


}

void
GPlatesQtWidgets::HellingerDialog::update()
{
    treeWidget -> clear();
    d_hellinger_model->reset_fit_struct();
    d_hellinger_model->reset_data_file();
    load_data_from_model();
    update_canvas();
	update_buttons();
}


void
GPlatesQtWidgets::HellingerDialog::update_buttons_statistics(bool info)
{
    if (info)
    {
        spinbox_result_lat -> setEnabled(true);
        spinbox_result_lon -> setEnabled(true);
        spinbox_result_angle -> setEnabled(true);
//        spinbox_result_eps -> setEnabled(true);
        button_details->setEnabled(true);

    }
    else
    {
        button_details->setEnabled(false);
    }
}

void
GPlatesQtWidgets::HellingerDialog::update_continue_button(
        bool info)
{
    if (!info)
    {
        button_stats -> setEnabled(false);
    }
    else
    {
       button_stats->setEnabled(true);
    }
}

void
GPlatesQtWidgets::HellingerDialog::show_results(
        const double &lat,
        const double &lon)
{
    GPlatesGui::Symbol results_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CIRCLE, 2, true);
    GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
                GPlatesMaths::LatLonPoint(lat,lon));
    GPlatesViewOperations::RenderedGeometry pick_results =
            GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
                point.get_non_null_pointer(),
                GPlatesGui::Colour::get_red(),
                2, /* point thickness */
                2, /* line thickness */
                false /* fill */,
                results_symbol);

    d_hellinger_layer.add_rendered_geometry(pick_results);
}


void
GPlatesQtWidgets::HellingerDialog::results_python()
{
    boost::optional<GPlatesQtWidgets::fit_struct> data_fit_struct = d_hellinger_model->get_results();

	if (data_fit_struct)
	{
		spinbox_result_lat->setValue(data_fit_struct.get().lat);
		spinbox_result_lon->setValue(data_fit_struct.get().lon);
		spinbox_result_angle->setValue(data_fit_struct.get().angle);
		show_results(data_fit_struct.get().lat, data_fit_struct.get().lon);
	}

}

void
GPlatesQtWidgets::HellingerDialog::show_data_points()
{
    std::vector<GPlatesMaths::LatLonPoint> data_points = d_hellinger_model->get_data_file();
    if (!data_points.empty())
    {
        std::vector<GPlatesMaths::LatLonPoint>::const_iterator iter;

        for (iter = data_points.begin(); iter != data_points.end(); ++iter)
        {
            GPlatesMaths::LatLonPoint llp = *iter;
            double lat = llp.latitude();
            double lon = llp.longitude();
            GPlatesGui::Symbol results_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, 2, true);
            GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
                        GPlatesMaths::LatLonPoint(lat,lon));
            GPlatesViewOperations::RenderedGeometry pick_results =
                    GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
                        point.get_non_null_pointer(),
                        GPlatesGui::Colour::get_red(),
                        2, /* point thickness */
                        2, /* line thickness */
                        false /* fill */,
                        results_symbol);
            d_hellinger_layer.add_rendered_geometry(pick_results);
        }
    }

}


void
GPlatesQtWidgets::HellingerDialog::load_data_from_model()
{    

	model_type::const_iterator 
		iter = d_hellinger_model->begin(),
		end = d_hellinger_model->end();

	for (; iter != end ; ++iter)
	{
		add_pick_to_tree(iter->first,iter->second,treeWidget);
	}
	update_buttons();
	update_initial_guess();
}

void
GPlatesQtWidgets::HellingerDialog::create_feature_collection()
{

}

void
GPlatesQtWidgets::HellingerDialog::close_dialog()
{
    d_hellinger_layer.clear_rendered_geometries();
}

void
GPlatesQtWidgets::HellingerDialog::update_canvas()
{

        draw_fixed_picks();
        draw_moving_picks();
        results_python();
        show_data_points();

}


void
GPlatesQtWidgets::HellingerDialog::set_segment_colour(
        int &num_colour)
{
	num_colour = num_colour%7;
	switch (num_colour)
	{
	case 0:
        d_segment_colour = GPlatesGui::Colour::get_green();
		break;
	case 1:
		d_segment_colour = GPlatesGui::Colour::get_blue();
		break;
	case 2:
		d_segment_colour = GPlatesGui::Colour::get_maroon();
		break;
	case 3:
		d_segment_colour = GPlatesGui::Colour::get_purple();
		break;
	case 4:
		d_segment_colour = GPlatesGui::Colour::get_fuchsia();
		break;
	case 5:
		d_segment_colour = GPlatesGui::Colour::get_olive();
		break;
	case 6:
		d_segment_colour = GPlatesGui::Colour::get_navy();
		break;
	default:
		d_segment_colour = GPlatesGui::Colour::get_navy();
	}
}

void
GPlatesQtWidgets::HellingerDialog::draw_fixed_picks()
{
    model_type::const_iterator it = d_hellinger_model->begin();

    int num_segment = 0;
    int num_colour = 0;

    for (; it != d_hellinger_model->end(); ++it)
    {
        if (it->second.is_enabled)
        {
            if (num_segment != it->first)
            {
                ++num_colour;
                ++num_segment;
            }

            set_segment_colour(num_colour);

            if (it->second.segment_type == FIXED_SEGMENT_TYPE)
            {
                        GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
                                    GPlatesMaths::LatLonPoint(it->second.lat,it->second.lon));

                        GPlatesViewOperations::RenderedGeometry pick_geometry =
                                GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
                                    point.get_non_null_pointer(),
                                    d_segment_colour,
                                    2, /* point thickness */
                                    2, /* line thickness */
                                    false /* fill */,
                                    it->second.segment_type == MOVING_SEGMENT_TYPE ? d_moving_symbol : d_fixed_symbol);

                        d_hellinger_layer.add_rendered_geometry(pick_geometry);
            }
        }
    }
}

void
GPlatesQtWidgets::HellingerDialog::draw_moving_picks()
{
    model_type::const_iterator it = d_hellinger_model->begin();
    int num_segment = 0;
    int num_colour = 0;
    for (; it != d_hellinger_model->end(); ++it)
    {
        if (it->second.is_enabled)
        {
            if (num_segment != it->first)
            {
                ++num_colour;
                ++num_segment;
            }

            set_segment_colour(num_colour);

            if (it->second.segment_type == MOVING_SEGMENT_TYPE)
            {
                        GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
                                    GPlatesMaths::LatLonPoint(it->second.lat,it->second.lon));

                        GPlatesViewOperations::RenderedGeometry pick_geometry =
                                GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
                                    point.get_non_null_pointer(),
                                    d_segment_colour,
                                    2, /* point thickness */
                                    2, /* line thickness */
                                    false /* fill */,
                                    it->second.segment_type == MOVING_SEGMENT_TYPE ? d_moving_symbol : d_fixed_symbol);

                        d_hellinger_layer.add_rendered_geometry(pick_geometry);
            }
        }
    }

}

void
GPlatesQtWidgets::HellingerDialog::reconstruct_picks()
{

    d_hellinger_layer.clear_rendered_geometries();
    draw_fixed_picks();
    results_python();
    boost::optional<GPlatesQtWidgets::fit_struct> data_fit_struct = d_hellinger_model->get_results();

    double recon_time = spinbox_recon_time->value();

    double lat = data_fit_struct.get().lat;
    double lon = data_fit_struct.get().lon;
    double angle;
    if (recon_time > 0 )
     {
         angle = (recon_time/spinbox_chron->value())*data_fit_struct.get().angle;
         double convert_angle = GPlatesMaths::convert_deg_to_rad(angle);
         model_type::const_iterator it = d_hellinger_model->begin();

         GPlatesMaths::LatLonPoint llp(lat,lon);
         GPlatesMaths::PointOnSphere point = make_point_on_sphere(llp);

         GPlatesMaths::FiniteRotation rotation =
         GPlatesMaths::FiniteRotation::create(point,convert_angle);
         int num_segment = 0;
         int num_colour = 0;
         for (; it != d_hellinger_model->end(); ++it)
         {
             if (it->second.is_enabled)
             {
                 if (num_segment != it->first)
                 {
                     ++num_colour;
                     ++num_segment;
                 }

                 set_segment_colour(num_colour);

                 if (it->second.segment_type == MOVING_SEGMENT_TYPE)
                 {

                             GPlatesMaths::LatLonPoint llp_move(it->second.lat,it->second.lon);
                             GPlatesMaths::PointOnSphere point_move = make_point_on_sphere(llp_move);
                             GPlatesMaths::PointOnSphere rotated_point = rotation*point_move;
                             GPlatesMaths::LatLonPoint transform_llp = make_lat_lon_point(rotated_point);

                             GPlatesMaths::PointOnSphere point_sphere = GPlatesMaths::make_point_on_sphere(
                                         GPlatesMaths::LatLonPoint(transform_llp.latitude(),transform_llp.longitude()));

                             GPlatesViewOperations::RenderedGeometry pick_geometry =
                                     GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
                                         point_sphere.get_non_null_pointer(),
                                         d_segment_colour,
                                         2, /* point thickness */
                                         2, /* line thickness */
                                         false /* fill */,
                                         it->second.segment_type == MOVING_SEGMENT_TYPE ? d_moving_symbol : d_fixed_symbol);

                             d_hellinger_layer.add_rendered_geometry(pick_geometry);
                 }
             }
         }
     }
    else
     {
        draw_moving_picks();
     }


}

void
GPlatesQtWidgets::HellingerDialog::handle_chron_time_changed(
        const double &time)
{
    d_chron_time = spinbox_chron->value();
    slider_recon_time->setMinimum(d_chron_time*slider_multiplier);
    slider_recon_time->setMaximum(0.);

    spinbox_recon_time->setMaximum(time);

    if (d_recon_time > d_chron_time)
    {
        d_recon_time = d_chron_time;

    }
    d_hellinger_layer.clear_rendered_geometries();

    draw_fixed_picks();
    draw_moving_picks();
    results_python();

}

void
GPlatesQtWidgets::HellingerDialog::handle_recon_time_spinbox_changed(
        const double &time)
{
    slider_recon_time->setValue(slider_multiplier*time);

    reconstruct_picks();
}

void
GPlatesQtWidgets::HellingerDialog::handle_recon_time_slider_changed(
        const int &value)
{
    spinbox_recon_time->setValue(static_cast<double>(value)/slider_multiplier);
}

void
GPlatesQtWidgets::HellingerDialog::handle_fit_spinboxes_changed()
{
    QStringList fit_spinboxes;
    QString lat_str = QString("%1").arg(spinbox_result_lat->value());
    QString lon_str = QString("%1").arg(spinbox_result_lon->value());
    QString angle_str = QString("%1").arg(spinbox_result_angle->value());
//    QString eps_str = QString("%1").arg(spinbox_result_eps->value());
    fit_spinboxes<<lat_str<<lon_str<<angle_str;
    d_hellinger_model->reset_data_file();
    d_hellinger_model->add_results(fit_spinboxes);
    reset_picks_globe();
}

void
GPlatesQtWidgets::HellingerDialog::reordering_picks()
{

    d_hellinger_model->reordering_picks();
    treeWidget->clear();
    load_data_from_model();
    button_renumber->setEnabled(false);
    reset_expanded_status();

}

void
GPlatesQtWidgets::HellingerDialog::renumber_segments()
{
    reordering_picks();
}

void
GPlatesQtWidgets::HellingerDialog::handle_expand_all()
{
    treeWidget->expandAll();
    update_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_collapse_all()
{
    treeWidget->collapseAll();
    update_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_checkbox_grid_search_changed()
{
    if (checkbox_grid_search->isChecked())
    {
        spinbox_iteration->setEnabled(true);
    }
    else
    {
        spinbox_iteration->setEnabled(false);
    }
}

void
GPlatesQtWidgets::HellingerDialog::update_expanded_status()
{
    int amount = treeWidget->topLevelItemCount();
    d_expanded_segments.clear();
    for (int i = 0; i<amount; i++)
    {
        if (treeWidget->topLevelItem(i)->isExpanded())
        {
            QString position = treeWidget->topLevelItem(i)->text(0);
            d_expanded_segments.push_back(position);
        }
    }
    std::vector<QString>::const_iterator iter;
    int num = 0;
    for (iter = d_expanded_segments.begin(); iter != d_expanded_segments.end(); ++iter)
    {
        ++num;
    }
}

void
GPlatesQtWidgets::HellingerDialog::reset_expanded_status()
{
    std::vector<QString>::const_iterator iter;
    int num = 0;
    int expended_size = d_expanded_segments.size();
    if (!d_expanded_segments.empty())
    {
        int amount = treeWidget->topLevelItemCount();
        num = 0;
        for (int i = 0; i<amount;i++)
        {
            int widget_count = treeWidget->topLevelItem(i)->text(0).toInt();
            int vector_count = d_expanded_segments.at(num).toInt();
            if (num+1< expended_size)
            {
                if (widget_count == vector_count)
                {
                    treeWidget->topLevelItem(i)->setExpanded(true);
                    num++;
                }
                else if(widget_count>vector_count)
                {
                    num++;
                    vector_count = d_expanded_segments.at(num).toInt();
                    if (widget_count == vector_count)
                    {
                        treeWidget->topLevelItem(i)->setExpanded(true);
                        num++;
                    }

                }
            }
        }
    }
    else
    {
        handle_expand_all();
    }
}


ENABLE_GCC_WARNING("-Wwrite-strings")
