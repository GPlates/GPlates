/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include <functional>
#include <QHeaderView>
#include <QTableWidget>
#include <QMap>
#include <QMessageBox>
#include <QVariant>
#include <QMetaType>
#include <QDebug>

#include "TotalReconstructionPolesDialog.h"

#include "QtWidgetUtils.h"
#include "VisualLayersComboBox.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/LayerTaskType.h"
#include "app-logic/ReconstructionLayerProxy.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeEdge.h"

#include "gui/CsvExport.h"

#include "maths/MathsUtils.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayerRegistry.h"
#include "presentation/VisualLayers.h"

#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

Q_DECLARE_METATYPE( boost::weak_ptr<GPlatesPresentation::VisualLayer> )


namespace ColumnNames
{
	/**
	 * These should match the columns set up in the designer.
	 */
	enum ColumnName
	{
		PLATEID, LATITUDE, LONGITUDE, ANGLE, FIXED, INTERPOLATED
	};
}

namespace {

	/**
	 * Struct to build the following table of file dialog filters / options.
	 * Typedef for the resulting QMap.
	 */
	struct FileDialogFilterOption
	{
		const char *text;
		const GPlatesGui::CsvExport::ExportOptions options;
	};
	typedef QMap<QString, GPlatesGui::CsvExport::ExportOptions> FileDialogFilterMapType;
	
	/**
	 * Table of filter options to present to the user when exporting CSV.
	 */
	static const FileDialogFilterOption file_dialog_filter_table[] = {
		{ QT_TRANSLATE_NOOP("TotalReconstructionPolesDialog",
				"CSV file, comma-delimited"),
			{ ',' } },
		{ QT_TRANSLATE_NOOP("TotalReconstructionPolesDialog",
				"CSV file, semicolon-delimited"),
			{ ';' } },
		{ QT_TRANSLATE_NOOP("TotalReconstructionPolesDialog",
				"CSV file, tab-delimited"),
			{ '\t' } },
	};

	/**
	 * This map is built for a quick, easy way to get back the CSV options
	 * based on what filter the QFileDialog says was selected.
	 */
	const FileDialogFilterMapType &
	build_export_filter_map()
	{
		static FileDialogFilterMapType map;
		const FileDialogFilterOption *begin = file_dialog_filter_table;
		const FileDialogFilterOption *end = begin + NUM_ELEMS(file_dialog_filter_table);
		for (; begin != end; ++begin)
		{
			map.insert(
					GPlatesQtWidgets::TotalReconstructionPolesDialog::tr(begin->text) + " (*.csv)",
					begin->options);
		}
		return map;
	}

	/**
	 * Construct filters to give to SaveFileDialog.
	 */
	GPlatesQtWidgets::SaveFileDialog::filter_list_type
	build_save_file_dialog_filters()
	{
		GPlatesQtWidgets::SaveFileDialog::filter_list_type result;
		const FileDialogFilterOption *begin = file_dialog_filter_table;
		const FileDialogFilterOption *end = begin + NUM_ELEMS(file_dialog_filter_table);
		for (; begin != end; ++begin)
		{
			result.push_back(GPlatesQtWidgets::FileDialogFilter(begin->text, "csv"));
		}

		return result;
	}


	const QString
	make_string_from_rotation(
			const GPlatesMaths::FiniteRotation &rotation)
	{
		using namespace GPlatesMaths;

		const UnitQuaternion3D &uq = rotation.unit_quat();
		const boost::optional<UnitVector3D> &axis_hint = rotation.axis_hint();

		if (represents_identity_rotation(uq)) {
			// Assume that this string won't change after the first time this function
			// is called, so we can keep the QString in a static local var.
			static QString indeterm_pole_tr_str =
					GPlatesQtWidgets::TotalReconstructionPolesDialog::tr(
							"(indeterminate pole)\t  angle: 0.00");
			return indeterm_pole_tr_str;
		} else {
			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(axis_hint);

			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);

			QLocale locale_;

			QString lat_val_string = locale_.toString(llp.latitude(),'f',2);
			QString lon_val_string = locale_.toString(llp.longitude(),'f',2);

			double angle = GPlatesMaths::convert_rad_to_deg(params.angle).dval();
			QString angle_val_string = locale_.toString(angle,'f',2);

			return GPlatesQtWidgets::TotalReconstructionPolesDialog::tr(
					"lat: %1\tlon: %2\t  angle: %3")
					.arg(lat_val_string)
					.arg(lon_val_string)
					.arg(angle_val_string);
		}
	}


	void
	fill_tree_item(
			QTreeWidgetItem* item,
			GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type edge)
	{
		int column = item->columnCount();
		QString moving_string;
		moving_string.setNum(edge->moving_plate());
		item->setText(column++,moving_string);

		QString fixed_string;
		fixed_string.setNum(edge->fixed_plate());

		QString edge_string;

		edge_string.append(fixed_string);

		GPlatesMaths::FiniteRotation relative_rotation = edge->relative_rotation();
		QString relative_rotation_string = make_string_from_rotation(relative_rotation);

		const GPlatesMaths::FiniteRotation &composed_rotation = edge->composed_absolute_rotation();
		QString composed_rotation_string = make_string_from_rotation(composed_rotation);

		item->setText(column++,edge_string);
		item->setText(column++,relative_rotation_string);
		item->setText(column,composed_rotation_string);
	}


	void
	add_children_of_edge_to_tree_item(
			GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type edge,
			QTreeWidgetItem *item)
	{
		std::vector<GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::iterator 
				it = edge->children_in_built_tree().begin();
		std::vector<GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::iterator 
				end = edge->children_in_built_tree().end();

		for( ; it != end ; it++)
		{
			QTreeWidgetItem* child_item = new QTreeWidgetItem(item,0);
			fill_tree_item(child_item,*it);
			add_children_of_edge_to_tree_item(*it,child_item);
		}
	}


	void
	populate_rotation_table_row(
			QTableWidget *table,
			int row_num,
			GPlatesModel::integer_plate_id_type plate_id,
			const GPlatesMaths::FiniteRotation &fr)
	{
		table->insertRow(row_num);

		// Insert the plate ID into the first column of the table.
		QString plate_id_as_string;
		plate_id_as_string.setNum(plate_id);
		QTableWidgetItem* plate_id_item = new QTableWidgetItem(plate_id_as_string);
		plate_id_item->setFlags(Qt::ItemIsEnabled);
		table->setItem(row_num, ColumnNames::PLATEID, plate_id_item);

		using namespace GPlatesMaths;

		// Now handle the finite rotation.
		const UnitQuaternion3D &uq = fr.unit_quat();
		if (represents_identity_rotation(uq)) {
			// Assume that this string won't change after the first time this function
			// is called, so we can keep the QString in a static local var.
			static QString indeterm_tr_str =
					GPlatesQtWidgets::TotalReconstructionPolesDialog::tr("Indeterminate");

			QTableWidgetItem*  latitude_item = new QTableWidgetItem(indeterm_tr_str);
			latitude_item->setFlags(Qt::ItemIsEnabled);
			table->setItem(row_num, ColumnNames::LATITUDE, latitude_item);

			QTableWidgetItem*  longitude_item = new QTableWidgetItem(indeterm_tr_str);
			longitude_item->setFlags(Qt::ItemIsEnabled);
			table->setItem(row_num, ColumnNames::LONGITUDE, longitude_item);

			QTableWidgetItem* angle_item = new QTableWidgetItem();
			angle_item->setFlags(Qt::ItemIsEnabled);
			angle_item->setData(Qt::DisplayRole, QVariant(0.0));
			table->setItem(row_num, ColumnNames::ANGLE, angle_item);
		} else {
			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());
			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);

			QLocale locale_;
			QString euler_pole_lat = locale_.toString(llp.latitude());
			QString euler_pole_lon = locale_.toString(llp.longitude());

			QTableWidgetItem* latitude_item = new QTableWidgetItem(euler_pole_lat);
			table->setItem(row_num, ColumnNames::LATITUDE, latitude_item);

			QTableWidgetItem* longitude_item = new QTableWidgetItem(euler_pole_lon);
			longitude_item->setFlags(Qt::ItemIsEnabled);
			table->setItem(row_num, ColumnNames::LONGITUDE, longitude_item);

			double angle = convert_rad_to_deg(params.angle).dval();
			
			QString angle_string = locale_.toString(angle);
			QTableWidgetItem* angle_item = new QTableWidgetItem(angle_string);
			angle_item->setFlags(Qt::ItemIsEnabled);
			table->setItem(row_num, ColumnNames::ANGLE, angle_item);
		}
	}

} // anonymous namespace


GPlatesQtWidgets::TotalReconstructionPolesDialog::TotalReconstructionPolesDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_view_state(view_state),
	d_application_state(view_state.get_application_state()),
	d_plate(0),
	d_time(0.0),
	d_save_file_dialog(
			this,
			tr("Export Tabular Data"),
			build_save_file_dialog_filters(),
			view_state),
	d_visual_layers_combobox(
			new VisualLayersComboBox(
				view_state.get_visual_layers(),
				view_state.get_visual_layer_registry(),
				std::bind1st(std::equal_to<GPlatesPresentation::VisualLayerType::Type>(),
					static_cast<GPlatesPresentation::VisualLayerType::Type>(
						GPlatesAppLogic::LayerTaskType::RECONSTRUCTION)),
				this))
{
	setupUi(this);
	QtWidgetUtils::add_widget_to_placeholder(
			d_visual_layers_combobox,
			visual_layers_combobox_placeholder_widget);
	label_reconstruction_tree_layer->setBuddy(d_visual_layers_combobox);

	QHeaderView *equivalent_header = table_equivalent->horizontalHeader();

	equivalent_header->setResizeMode(ColumnNames::PLATEID, QHeaderView::Fixed);
	equivalent_header->setResizeMode(ColumnNames::LONGITUDE, QHeaderView::Fixed);
	equivalent_header->setResizeMode(ColumnNames::LATITUDE, QHeaderView::Fixed);
	equivalent_header->setResizeMode(ColumnNames::ANGLE, QHeaderView::Fixed);

	QHeaderView *equivalent_vertical = table_equivalent->verticalHeader();
	equivalent_vertical->hide();

	QHeaderView *relative_header = table_relative->horizontalHeader();

	relative_header->setResizeMode(ColumnNames::PLATEID, QHeaderView::Fixed);
	relative_header->setResizeMode(ColumnNames::LONGITUDE, QHeaderView::Fixed);
	relative_header->setResizeMode(ColumnNames::LATITUDE, QHeaderView::Fixed);
	relative_header->setResizeMode(ColumnNames::ANGLE, QHeaderView::Fixed);
	relative_header->setResizeMode(ColumnNames::FIXED, QHeaderView::Fixed);

	QHeaderView *relative_vertical = table_relative->verticalHeader();
	relative_vertical->hide();

	QHeaderView *tree_reconstruction_header = tree_reconstruction->header();
	tree_reconstruction_header->setResizeMode(0,QHeaderView::ResizeToContents);
	tree_reconstruction_header->setResizeMode(1,QHeaderView::Fixed);
	tree_reconstruction_header->setResizeMode(2,QHeaderView::Fixed);
	tree_reconstruction_header->setResizeMode(3,QHeaderView::Fixed);
	tree_reconstruction_header->setMovable(false);

	tree_reconstruction_header->resizeSection(1,100);
	tree_reconstruction_header->resizeSection(2,270);
	tree_reconstruction_header->resizeSection(3,270);


	QHeaderView *tree_circuit_header = tree_circuit->header();
	tree_circuit_header->setResizeMode(0,QHeaderView::ResizeToContents);
	tree_circuit_header->setResizeMode(1,QHeaderView::Fixed);
	tree_circuit_header->setResizeMode(2,QHeaderView::Fixed);
	tree_circuit_header->setResizeMode(3,QHeaderView::Fixed);
	tree_circuit_header->setMovable(false);

	tree_circuit_header->resizeSection(1,100);
	tree_circuit_header->resizeSection(2,270);
	tree_circuit_header->resizeSection(3,270);

	set_time(d_application_state.get_current_reconstruction_time());
	set_plate(d_application_state.get_current_anchored_plate_id());

	make_signal_slot_connections();
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::set_plate(
		unsigned long plate)
{
	d_plate = plate;
	QString s;
	s.setNum(plate);
	field_reference_plate->setText(s);
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::set_time(
		double time)
{
	d_time = time;
	QString s;
	s.setNum(time);
	field_time->setText(s);
}




/**
 * Fill the equivalent rotations QTableWidget with a list of Plate-ids and their corresponding
 * composed absolute rotations.
 */
void
GPlatesQtWidgets::TotalReconstructionPolesDialog::fill_equivalent_table(
		const GPlatesAppLogic::ReconstructionTree &reconstruction_tree)
{
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it = 
			reconstruction_tree.edge_map_begin();
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator end = 
			reconstruction_tree.edge_map_end();

	for( ; it != end ; ++it)
	{
		// Fill in a row of the table.
		int num_row = table_equivalent->rowCount();
		populate_rotation_table_row(table_equivalent, num_row, it->first,
				it->second->composed_absolute_rotation());
	}

}


/**
 * Fill the relative rotations QTableWidget with a list of Plate-ids and their corresponding
 * relative rotations.
 */
void
GPlatesQtWidgets::TotalReconstructionPolesDialog::fill_relative_table(
		const GPlatesAppLogic::ReconstructionTree &reconstruction_tree)
{
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it = 
			reconstruction_tree.edge_map_begin();
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator end = 
			reconstruction_tree.edge_map_end();

	for( ; it != end ; ++it)
	{
		// Fill in a row of the table (or at least the first four fields).
		int num_row = table_relative->rowCount();
		populate_rotation_table_row(table_relative, num_row, it->first,
				it->second->relative_rotation());

		// Now insert the fixed plate ID into the second-last column of the table.
		GPlatesModel::integer_plate_id_type fixed_id = it->second->fixed_plate();
		QString fixed_string;
		fixed_string.setNum(fixed_id);
		QTableWidgetItem* fixed_item = new QTableWidgetItem(fixed_string);
		fixed_item->setFlags(Qt::ItemIsEnabled);
		table_relative->setItem(num_row, ColumnNames::FIXED, fixed_item);

		// Finally, state whether the pole was interpolated or not.

		// Assume that these strings won't change after the first time this function
		// is called, so we can keep the QStrings in static local vars.
		static QString interp_tr_str = tr("interp");
		static QString not_interp_tr_str = tr("not-interp");

		if (it->second->finite_rotation_was_interpolated()) {
			QTableWidgetItem* interp_item = new QTableWidgetItem(interp_tr_str);
			interp_item->setFlags(Qt::ItemIsEnabled);
			table_relative->setItem(num_row, ColumnNames::INTERPOLATED, interp_item);
		} else {
			QTableWidgetItem* not_interp_item = new QTableWidgetItem(not_interp_tr_str);
			not_interp_item->setFlags(Qt::ItemIsEnabled);
			table_relative->setItem(num_row, ColumnNames::INTERPOLATED, not_interp_item);
		}
	}

}


/**
 * Fill the reconstruction tree QTreeWidget with the Reconstruction Tree.
 */
void
GPlatesQtWidgets::TotalReconstructionPolesDialog::fill_reconstruction_tree(
		const GPlatesAppLogic::ReconstructionTree &reconstruction_tree)
{
	std::vector<GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it = 
			reconstruction_tree.rootmost_edges_begin();
	std::vector<GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator end = 
			reconstruction_tree.rootmost_edges_end();

	for( ; it != end; ++it)
	{
		//std::cerr << (*it)->moving_plate() << std::endl;
		
		// Create a QTreeWidgetItem for each of the rootmost edges, and recursively
		// add its children to the tree 


		QTreeWidgetItem *item = new QTreeWidgetItem(tree_reconstruction,0);
		fill_tree_item(item,*it);
		add_children_of_edge_to_tree_item(*it,item);

	}
}


/**
 * Fill the plate circuit QTreeWidget with the circuit-to-stationary-plate for each plate-id. 
 */
void
GPlatesQtWidgets::TotalReconstructionPolesDialog::fill_circuit_tree(
		const GPlatesAppLogic::ReconstructionTree &reconstruction_tree)
{
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it = 
			reconstruction_tree.edge_map_begin();
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator end = 
			reconstruction_tree.edge_map_end();


	for( ; it != end ; ++it)
	{
		// get plate id and add it to the top level of the tree
		QTreeWidgetItem *item = new QTreeWidgetItem(tree_circuit,0);
		QString id_as_string;
		id_as_string.setNum(it->first);
		item->setText(0,id_as_string);

		//std::cerr << it->first << std::endl;

		// go up the rotation tree using the parent, until 
		// we come to the stationary plate.
	
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type edge = it->second;

		while (edge->parent_edge()) {
			QTreeWidgetItem *child = new QTreeWidgetItem(item,0);
			fill_tree_item(child,edge);

			edge = GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type(
					edge->parent_edge(),
					GPlatesUtils::NullIntrusivePointerHandler());
		}
		// and finally add the edge from the last plate to the stationary plate
		QTreeWidgetItem *child = new QTreeWidgetItem(item,0);
		fill_tree_item(child,edge);		

	}


///////////////////////////////////////////////////////////////////////////////


}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::showEvent(
		QShowEvent *event_)
{
	update_if_layer_changed();
}	


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::update_if_visible()
{
	if (isVisible())
	{
		update();
	}
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::update_if_layer_changed()
{
	boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer =
		d_visual_layers_combobox->get_selected_visual_layer();
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_curr_visual_layer = d_curr_visual_layer.lock())
		{
			if (locked_curr_visual_layer != locked_visual_layer)
			{
				update();
			}
		}
	}
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::update()
{
	reset_everything();

	// Extract the reconstruction tree from the currently selected layer.
	boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer =
		d_visual_layers_combobox->get_selected_visual_layer();

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		typedef GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree_ptr_type;
		boost::optional<GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type>
				reconstruction_tree_layer_proxy = layer.get_layer_output<
						GPlatesAppLogic::ReconstructionLayerProxy>();
		if (reconstruction_tree_layer_proxy)
		{
			reconstruction_tree_ptr_type reconstruction_tree =
					reconstruction_tree_layer_proxy.get()->get_reconstruction_tree();

			fill_equivalent_table(*reconstruction_tree);
			fill_relative_table(*reconstruction_tree);
			fill_reconstruction_tree(*reconstruction_tree);
			fill_circuit_tree(*reconstruction_tree);
		}

		d_curr_visual_layer = visual_layer;
	}
	else
	{
		d_curr_visual_layer = boost::weak_ptr<GPlatesPresentation::VisualLayer>();
	}
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::reset_everything()
{
	set_time(d_application_state.get_current_reconstruction_time());
	set_plate(d_application_state.get_current_anchored_plate_id());
	table_equivalent->clearContents();
	table_equivalent->setRowCount(0);
	table_relative->clearContents();
	table_relative->setRowCount(0);
	tree_reconstruction->clear();
	tree_circuit->clear();
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::update(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	// By moving the combobox to the correct index, the dialog should be updated
	// via a signal emitted by the combobox.
	d_visual_layers_combobox->set_selected_visual_layer(visual_layer);
}

void
GPlatesQtWidgets::TotalReconstructionPolesDialog::export_relative()
{
	handle_export(*table_relative);
#if 0
	QString filename = QFileDialog::getSaveFileName(this,
			tr("Save As"), "", tr("CSV file (*.csv)"));

	if (filename.isEmpty()){
		return;
	}

	GPlatesGui::CsvExport::export_table(filename,table_relative);

	return;
#endif
}

void
GPlatesQtWidgets::TotalReconstructionPolesDialog::export_equivalent()
{
	handle_export(*table_equivalent);
#if 0
	QString filename = QFileDialog::getSaveFileName(this,
			tr("Save As"), "", tr("CSV file (*.csv)"));

	if (filename.isEmpty()){
		return;
	}

	GPlatesGui::CsvExport::export_table(filename,table_equivalent);

	return;
#endif
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::handle_export(
		const QTableWidget &table)
{
	// Build a map to let us look up the options the user wants based on what
	// file filter was selected in the dialog.
	static const FileDialogFilterMapType &filter_map = build_export_filter_map();

	// Pop up and ask for file.
	QString filter;
	boost::optional<QString> filename = d_save_file_dialog.get_file_name(&filter);
	if (filename)
	{
		if (filter_map.contains(filter))
		{
			GPlatesGui::CsvExport::ExportOptions options = filter_map.value(filter);
			GPlatesGui::CsvExport::export_table(*filename, options, table);
		}
		else
		{
			// Somehow, user chose filter that we didn't put in there.
			QMessageBox::critical(this, tr("Invalid export filter"), tr("Please specify a CSV file format variant in the save dialog."));
		}
	}
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::make_signal_slot_connections()
{
	// Export buttons.
	QObject::connect(
			button_export_relative_rotations,
			SIGNAL(clicked()),
			this,
			SLOT(export_relative()));
	QObject::connect(
			button_export_equiv_rotations,
			SIGNAL(clicked()),
			this,
			SLOT(export_equivalent()));

	// Layers combobox.
	QObject::connect(
			d_visual_layers_combobox,
			SIGNAL(selected_visual_layer_changed(
					boost::weak_ptr<GPlatesPresentation::VisualLayer>)),
			this,
			SLOT(update_if_layer_changed()));

	// Reconstructed.
	QObject::connect(
			&d_application_state,
			SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
			this,
			SLOT(update_if_visible()));
}

