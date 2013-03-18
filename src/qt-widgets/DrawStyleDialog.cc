/* $Id: ColouringDialog.cc 10521 2010-12-11 06:33:37Z elau $ */

/**
 * \file 
 * $Revision: 10521 $
 * $Date: 2010-12-11 17:33:37 +1100 (Sat, 11 Dec 2010) $ 
 * 
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
#include "global/python.h"
#include <boost/optional.hpp>
#include <QBuffer>
#include <QFileDialog>
#include <QColorDialog>
#include <QHeaderView>

#include "SceneView.h"
#include "ReconstructionViewWidget.h"
#include "DrawStyleDialog.h"

#include "PythonArgumentWidget.h"
#include "QtWidgetUtils.h"
#include "VisualLayersComboBox.h"
#include "app-logic/PropertyExtractors.h"
#include "file-io/CptReader.h"
#include "file-io/ReadErrorAccumulation.h"

#include "gui/DrawStyleAdapters.h"
#include "gui/DrawStyleManager.h"
#include "gui/HTMLColourNames.h"
#include "gui/PlateIdColourPalettes.h"
#include "gui/PythonConfiguration.h"
#include "gui/ViewportProjection.h"

#include "presentation/Application.h"
#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"

#include "GlobeAndMapWidget.h"
#include "GlobeCanvas.h"
#include "MapCanvas.h"
#include "MapView.h"


GPlatesQtWidgets::DrawStyleDialog::DrawStyleDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget* parent_) :
	GPlatesDialog(parent_),
	d_show_thumbnails(true),
	d_view_state(view_state),
	d_combo_box(NULL),
	d_style_of_all(NULL),
	d_dirty(false)
{
	init_dlg();
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refresh_preview_icons()));
	timer->start(1000);
}


void
GPlatesQtWidgets::DrawStyleDialog::reset(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> layer)
{
	//qDebug() << "reseting draw style dialog...";
	d_combo_box->set_selected_visual_layer(layer);
	d_visual_layer = layer;
	init_category_table();
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_visual_layer.lock())
	{
		focus_style(locked_visual_layer->get_visual_layer_params()->style_adapter());
	}
}


namespace
{
	bool pred(GPlatesPresentation::VisualLayerType::Type t)
	{
		const GPlatesPresentation::VisualLayerType::Type t1 = 
			static_cast<GPlatesPresentation::VisualLayerType::Type>(
					GPlatesAppLogic::LayerTaskType::RECONSTRUCT);
		const GPlatesPresentation::VisualLayerType::Type t2 = 
			static_cast<GPlatesPresentation::VisualLayerType::Type>(
			GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER);
		const GPlatesPresentation::VisualLayerType::Type t3 = 
			static_cast<GPlatesPresentation::VisualLayerType::Type>(
			GPlatesAppLogic::LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER);
		return (t == t1 || t == t2 || t == t3);
	}

	inline
	QPixmap
	to_QPixmap(
			const QImage& img)
	{
#ifdef GPLATES_USE_VGL
		//workaround for using VirtualGL.
		//With VirtualGL, the QPixmap::fromImage() funtion returns a corrupted QPixmap object.
		//use "cmake . -DCMAKE_CXX_FLAGS=-DGPLATES_USE_VGL" to define "GPLATES_USE_VGL"
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		img.save(&buffer, "BMP");

		QPixmap qp;
		qp.loadFromData(ba, "BMP");
		return qp;
#else
		return QPixmap::fromImage(img);
#endif
	}

}


void
GPlatesQtWidgets::DrawStyleDialog::handle_layer_changed(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> layer)
{
	if(!isVisible() || (layer.lock().get() == d_visual_layer.lock().get()))
		return;

	d_visual_layer = layer;
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_visual_layer.lock())
	{
		const GPlatesGui::StyleAdapter* t_style = 
			locked_visual_layer->get_visual_layer_params()->style_adapter();
		
		if(t_style)
		{
			focus_style(t_style);
		}
	}
	else
	{
		if(d_style_of_all)
		{
			focus_style(d_style_of_all);
		}
		else
		{
			categories_table->clearSelection();
			categories_table->setCurrentIndex(QModelIndex());
		
			style_list->clearSelection();
			style_list->clear();
		}
	}
}


Q_DECLARE_METATYPE( boost::weak_ptr<GPlatesPresentation::VisualLayer> )

void
GPlatesQtWidgets::LayerGroupComboBox::insert_all()
{
	QVariant qv; qv.setValue(boost::weak_ptr<GPlatesPresentation::VisualLayer>());
	static const QIcon empty_icon(QPixmap(":/gnome_stock_color_16.png"));
	insertItem(0, empty_icon, "(All)", qv);
	setCurrentIndex(0);
}


void
GPlatesQtWidgets::DrawStyleDialog::apply_style_to_all_layers()
{
	GPlatesPresentation::VisualLayers& layers = d_view_state.get_visual_layers();
	for(size_t i=0; i<layers.size(); i++)
	{
		if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_layer = layers.visual_layer_at(i).lock())
		{
			locked_layer->get_visual_layer_params()->set_style_adaper(d_style_of_all);
		}
	}
}


void
GPlatesQtWidgets::DrawStyleDialog::focus_style(
		const GPlatesGui::StyleAdapter* style_)
{
	if(NULL == style_)
	{
		const GPlatesGui::StyleAdapter* default_style = d_style_mgr->default_style();
		if(default_style)
		{
			focus_style(default_style);
		}
		return;
	}

	const GPlatesGui::StyleCategory* cata = &style_->catagory();
	int row_num = categories_table->rowCount();
	for(int i=0; i<row_num; i++)
	{
		QTableWidgetItem* item = categories_table->item(i,0);
		GPlatesGui::StyleCategory* tmp_cat = static_cast<GPlatesGui::StyleCategory*>(item->data(Qt::UserRole).value<void*>());
		if(tmp_cat == cata)
		{
			categories_table->setCurrentItem(item, QItemSelectionModel::SelectCurrent);
			break;
		}
	}

	int style_num = style_list->count();
	for(int i=0; i<style_num; i++)
	{
		QListWidgetItem* item = style_list->item(i);
		GPlatesGui::StyleAdapter* tmp_style = 
			static_cast<GPlatesGui::StyleAdapter*>(item->data(Qt::UserRole).value<void*>());
		if(tmp_style == style_)
		{
			style_list->setCurrentItem(item, QItemSelectionModel::SelectCurrent);
			break;
		}
	}
}


void
GPlatesQtWidgets::DrawStyleDialog::init_category_table()
{
	categories_table->clear();
	GPlatesGui::DrawStyleManager::CatagoryContainer& catas = d_style_mgr->all_catagories();
	std::size_t n_size = catas.size();
	categories_table->setRowCount(n_size);
	for(std::size_t row=0; row < n_size; row++)
	{
		QTableWidgetItem *item = new QTableWidgetItem(catas[row]->name());
		item->setToolTip(catas[row]->desc());
		QVariant qv;
		qv.setValue(static_cast<void*>(catas[row])); 
		item->setData(Qt::UserRole, qv);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		categories_table->setItem(row, 0, item);
	}
}

void
GPlatesQtWidgets::DrawStyleDialog::make_signal_slot_connections()
{
	// Close button.
	QObject::connect(
			close_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_close_button_clicked()));

	// Remove button.
	QObject::connect(
			remove_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_remove_button_clicked()));

	// Add button.
	QObject::connect(
			add_button,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_add_button_clicked(bool)));

	// Categories table.
	QObject::connect(
			categories_table,
			SIGNAL(currentCellChanged(int, int, int, int)),
			this,
			SLOT(handle_categories_table_cell_changed(int, int, int, int)));

	QObject::connect(
			style_list,
			SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
			this,
			SLOT(handle_style_selection_changed(QListWidgetItem*,QListWidgetItem*)));

	QObject::connect(
			&GPlatesPresentation::Application::instance().get_main_window().reconstruction_view_widget().globe_and_map_widget(),
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_main_repaint(bool)));

	QObject::connect(
			show_thumbnails_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_show_thumbnails_changed(int)));

	QObject::connect(
			cfg_name_line_edit,
			SIGNAL(textChanged(const QString&)),
			this,
			SLOT(handle_cfg_name_changed(const QString&)));

	QObject::connect(
			&GPlatesPresentation::Application::instance().get_main_window().globe_canvas(),
			SIGNAL(mouse_released_after_drag(
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			this, 
			SLOT(handle_release_after_drag()));

	QObject::connect(
			&GPlatesPresentation::Application::instance().get_main_window().map_view(),
			SIGNAL(mouse_released_after_drag(const QPointF &,
					bool, const QPointF &, bool, const QPointF &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			this, 
			SLOT(handle_release_after_drag()));

	QObject::connect(
			&d_view_state.get_viewport_projection(),
			SIGNAL(projection_type_changed(const GPlatesGui::ViewportProjection &)),
			this,
			SLOT(handle_change_projection()));

	QObject::connect(
			&GPlatesPresentation::Application::instance().get_application_state(),
			SIGNAL(reconstruction_time_changed(GPlatesAppLogic::ApplicationState &, const double &)),
			this,
			SLOT(handle_view_time_changed(GPlatesAppLogic::ApplicationState &)));

	QObject::connect(
			&d_view_state.get_viewport_zoom(),
			SIGNAL(zoom_changed()),
			this,
			SLOT(handle_zoom_change()));
}

void
GPlatesQtWidgets::DrawStyleDialog::handle_close_button_clicked()
{
	hide();
}


void
GPlatesQtWidgets::DrawStyleDialog::handle_main_repaint(
		bool mouse_down)
{
	if(!d_dirty && !mouse_down)
	{
		d_dirty = is_dirty();
	}
}


void
GPlatesQtWidgets::DrawStyleDialog::handle_remove_button_clicked()
{
	QTableWidgetItem* cur_item = categories_table->currentItem();
	if(!cur_item)
		return;

	GPlatesGui::DrawStyleManager* mgr = GPlatesGui::DrawStyleManager::instance();
	QListWidgetItem* item = style_list->currentItem();
	if(item)
	{
		QVariant qv = item->data(Qt::UserRole);
		GPlatesGui::StyleAdapter* sa = static_cast<GPlatesGui::StyleAdapter*>(qv.value<void*>());
		mgr->remove_style(sa);
		delete style_list->takeItem(style_list->currentRow());
	}
}


void
GPlatesQtWidgets::DrawStyleDialog::handle_configuration_changed()
{
	GPlatesGui::StyleAdapter* current_style = get_current_style();
	
	if(!current_style)
	{
		qDebug() << "DrawStyleDialog::handle_configuration_changed(): Cannot find current style setting.";
		return;
	}

	current_style->set_dirty_flag(true);
	set_style(current_style);
	refresh_current_icon();
}


void
GPlatesQtWidgets::DrawStyleDialog::set_style(
		GPlatesGui::StyleAdapter* _style)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_visual_layer.lock())
	{
		if(locked_visual_layer->get_visual_layer_params()->style_adapter() == _style)
			return;
		else
		{
			locked_visual_layer->get_visual_layer_params()->set_style_adaper(_style);
			d_style_of_all = NULL;
		}
	}
	else
	{
		d_style_of_all = _style;
		apply_style_to_all_layers();
	}
	GPlatesGui::DrawStyleManager::instance()->emit_style_changed();
}


GPlatesQtWidgets::DrawStyleDialog::~DrawStyleDialog()
{
	if(GPlatesGui::DrawStyleManager::is_alive())
	{
		d_style_mgr->decrease_ref(*get_style(style_list->currentItem()));
		d_style_mgr->save_user_defined_styles();
	}
}


void
GPlatesQtWidgets::DrawStyleDialog::set_style()
{
	QListWidgetItem* item = style_list->currentItem();
	if(item)
	{
		QVariant qv = item->data(Qt::UserRole);
		GPlatesGui::StyleAdapter* sa = static_cast<GPlatesGui::StyleAdapter*>(qv.value<void*>());
		set_style(sa);
	}
}


void
GPlatesQtWidgets::DrawStyleDialog::init_dlg()
{
	setupUi(this);
	
	d_globe_and_map_widget_ptr = 
		GPlatesPresentation::Application::instance().get_main_window().reconstruction_view_widget() \
		.globe_and_map_widget().clone_with_shared_opengl_context(style_list);

	categories_table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	categories_table->horizontalHeader()->hide();
	categories_table->verticalHeader()->hide();
	categories_table->resizeColumnsToContents();
	categories_table->resize(categories_table->horizontalHeader()->length(),0);

	// Set up the list of colour schemes.
	style_list->setViewMode(QListWidget::IconMode);
	style_list->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	//	style_list->setSpacing(SPACING); //Due to a qt bug, the setSpacing doesn't work well in IconMode.
	style_list->setMovement(QListView::Static);
	style_list->setWrapping(true);
	style_list->setResizeMode(QListView::Adjust);
	style_list->setUniformItemSizes(true);
	style_list->setWordWrap(true);

	QPixmap blank_pixmap(ICON_SIZE,ICON_SIZE);
	blank_pixmap.load(":/preview_not_available.png","PNG");
	//blank_pixmap.fill(*GPlatesGui::HTMLColourNames::instance().get_colour("slategray"));
	d_blank_icon = QIcon(blank_pixmap);
	d_style_mgr = GPlatesGui::DrawStyleManager::instance();
	
	//init_category_table();
	make_signal_slot_connections();
	
	open_button->hide();
	edit_button->hide();

	add_button->show();
	remove_button->show();

	// Set up our GlobeAndMapWidget that we use for rendering.
	d_globe_and_map_widget_ptr->resize(ICON_SIZE, ICON_SIZE);
	d_globe_and_map_widget_ptr->hide();

#if defined(Q_OS_MAC)
	if(QT_VERSION >= 0x040600)
		d_globe_and_map_widget_ptr->move(style_list->spacing()+4, style_list->spacing()+3); 
#else
		d_globe_and_map_widget_ptr->move(1- ICON_SIZE, 1- ICON_SIZE);
#endif
	splitter->setStretchFactor(splitter->indexOf(categories_table),1);
	splitter->setStretchFactor(splitter->indexOf(right_side_frame),4);

	d_combo_box = new LayerGroupComboBox(
			d_view_state.get_visual_layers(),
			d_view_state.get_visual_layer_registry(),
			pred,
			this);

	QObject::connect(
			d_combo_box,
			SIGNAL(selected_visual_layer_changed(
					boost::weak_ptr<GPlatesPresentation::VisualLayer>)),
			this,
			SLOT(handle_layer_changed(
				boost::weak_ptr<GPlatesPresentation::VisualLayer>)));
	if(d_combo_box->count() !=0 )
		d_combo_box->setCurrentIndex(0);

	QtWidgetUtils::add_widget_to_placeholder(d_combo_box, select_layer_widget);
}

void
GPlatesQtWidgets::DrawStyleDialog::handle_categories_table_cell_changed(
		int current_row,
		int current_column,
		int previous_row,
		int previous_column)
{
	if(current_row < 0)
	{
		qDebug() << "The index of current row is negative number. Do nothing and return.";
		return;
	}
	QTableWidgetItem* item = categories_table->currentItem();
	if(item)
	{
		GPlatesGui::StyleCategory* cata = get_catagory(*item);
		if(cata)
		{
			load_category(*cata);
		}
	}
}

void
GPlatesQtWidgets::DrawStyleDialog::load_category(
		const GPlatesGui::StyleCategory& cata)
{
	using namespace GPlatesGui;

	DrawStyleManager::StyleContainer styles = d_style_mgr->get_styles(cata);
	style_list->clear();
	
	BOOST_FOREACH(StyleAdapter* sa, styles)
	{
		QListWidgetItem *item = new QListWidgetItem(d_blank_icon, sa->name(), style_list);

		QVariant qv;
		qv.setValue(static_cast<void*>(sa)); 
		item->setData(Qt::UserRole, qv);
		style_list->addItem(item);
	}

	// Set the rendering chain in motion.
	if (d_show_thumbnails)
	{
		show_preview_icon();
	}
}

void
GPlatesQtWidgets::DrawStyleDialog::handle_style_selection_changed(
		QListWidgetItem* current,
		QListWidgetItem* previous)
{
	using namespace GPlatesGui;
	if(current == previous)
		return;
	
	DrawStyleManager* mgr = DrawStyleManager::instance();
	if(previous)
	{
		QVariant qv = previous->data(Qt::UserRole);
		StyleAdapter* pre_style = static_cast<StyleAdapter*>(qv.value<void*>());
		mgr->decrease_ref(*pre_style);
	}
	if(current)
	{
		QVariant qv = current->data(Qt::UserRole);
		StyleAdapter* current_style = static_cast<StyleAdapter*>(qv.value<void*>());
		set_style(current_style);
		mgr->increase_ref(*current_style);
		cfg_name_line_edit->setText(current_style->name());

		const Configuration& cfg = current_style->configuration();
		build_config_panel(cfg);
		
		if(!mgr->is_built_in_style(*current_style))
		{
			if(mgr->get_ref_number(*current_style) == 1 && mgr->can_be_removed(*current_style))
				remove_button->setEnabled(true);
			else
				remove_button->setEnabled(false);
			
			enable_config_panel(true);
		}
		else
		{
			remove_button->setEnabled(false);
			enable_config_panel(false);
		}
	}
}

void
GPlatesQtWidgets::DrawStyleDialog::show_preview_icon()
{
	//qDebug() << "show_preview_icon()";
	{
		//sync the camera point
		GPlatesQtWidgets::ReconstructionViewWidget & view_widget = 
			GPlatesPresentation::Application::instance().get_main_window().reconstruction_view_widget();
		boost::optional<GPlatesMaths::LatLonPoint> camera_point = view_widget.camera_llp();
	
		if(camera_point)
		{
			d_previous_camera_point = camera_point;
			d_globe_and_map_widget_ptr->get_active_view().set_camera_viewpoint(*camera_point);
		}

		if(view_widget.globe_is_active())
		{
			d_globe_and_map_widget_ptr->get_globe_canvas().set_orientation(
				*view_widget.globe_canvas().orientation());
		}
		
		PreviewGuard guard(*this);
		
		int len = style_list->count();
    
		d_globe_and_map_widget_ptr->show();
	
		for(int i = 0; i < len; i++)
		{
			 QListWidgetItem* current_item = style_list->item(i);
		 
			 if(!current_item)
				 continue;

			QVariant qv = current_item->data(Qt::UserRole);
			GPlatesGui::StyleAdapter* sa = static_cast<GPlatesGui::StyleAdapter*>(qv.value<void*>());
			set_style(sa);

			// Render the preview icon image.
			QImage image = d_globe_and_map_widget_ptr->render_to_qimage();

			current_item->setIcon(QIcon(to_QPixmap(image)));
		}

		d_globe_and_map_widget_ptr->hide();
		d_dirty = false;
	}
}


void
GPlatesQtWidgets::DrawStyleDialog::refresh_current_icon()
{
	d_globe_and_map_widget_ptr->show();

	QListWidgetItem* current_item = style_list->currentItem();

	if(!current_item)
		return;

	if (d_show_thumbnails)
	{
		QVariant qv = current_item->data(Qt::UserRole);
		GPlatesGui::StyleAdapter* sa = static_cast<GPlatesGui::StyleAdapter*>(qv.value<void*>());
		set_style(sa);

		// Render the preview icon image.
		QImage image = d_globe_and_map_widget_ptr->render_to_qimage();

		current_item->setIcon(QIcon(to_QPixmap(image)));
	}
	d_globe_and_map_widget_ptr->hide();
}


void
GPlatesQtWidgets::DrawStyleDialog::handle_add_button_clicked(bool )
{
	using namespace GPlatesGui;


	 QTableWidgetItem* cur_cata_item = categories_table->currentItem();
	 if(!cur_cata_item)
		 return;

	StyleCategory* current_cata = get_catagory(*cur_cata_item);
	if(current_cata)
	{
		const StyleAdapter* style_temp = d_style_mgr->get_template_style(*current_cata);
		if(style_temp)
		{
			StyleAdapter* new_style = style_temp->deep_clone();
			if(new_style)
			{
				QString new_name("Unnamed");
				if(!is_style_name_valid(*current_cata,new_name))
				{
					new_name = generate_new_valid_style_name(*current_cata,new_name);
				}
				new_style->set_name(new_name);
				d_style_mgr->register_style(new_style);
				load_category(*current_cata);
				focus_style(new_style);
			}
		}
	}
};


void
GPlatesQtWidgets::DrawStyleDialog::build_config_panel(const GPlatesGui::Configuration& cfg)
{
#if !defined(GPLATES_NO_PYTHON)
	//clear old gui widget in the panel
	BOOST_FOREACH(QWidget* old_widget, d_cfg_widgets)
	{
		QObject::disconnect(
				old_widget,
				SIGNAL(configuration_changed()),
				this,
				SLOT(handle_configuration_changed()));
	}
	d_cfg_widgets.clear();
				
	QLayoutItem* row = NULL;
	while((formLayout->count() > 2) && (row = formLayout->takeAt(2)) != 0)
	{
		delete row->layout();
		delete row->widget();
	}
		
	BOOST_FOREACH(const QString& item_name, cfg.all_cfg_item_names())
	{
		const GPlatesGui::PythonCfgItem* item = dynamic_cast<const GPlatesGui::PythonCfgItem*>(cfg.get(item_name));
		QWidget* cfg_widget = create_cfg_widget(const_cast<GPlatesGui::PythonCfgItem*>(item));
		if(cfg_widget)
		{
			QObject::connect(
					cfg_widget,
					SIGNAL(configuration_changed()),
					this,
					SLOT(handle_configuration_changed()));
			formLayout->addRow(item_name + ":" , cfg_widget);
			d_cfg_widgets.push_back(cfg_widget);//save the pointer so that we can disconnect them later.
		}
	}
#endif
}


void
GPlatesQtWidgets::DrawStyleDialog::handle_cfg_name_changed(const QString& cfg_name)
{
	QListWidgetItem * item = style_list->currentItem();
	if(item)
	{
		GPlatesGui::StyleAdapter* style_adpter = get_style(item);
		if(style_adpter)
		{
			QString new_cfg_name = cfg_name;
			new_cfg_name.remove(QChar('/'));
			if(style_adpter->name() == new_cfg_name)
				return;
			
			if(!is_style_name_valid(style_adpter->catagory(), new_cfg_name))
			{
				new_cfg_name = generate_new_valid_style_name(style_adpter->catagory(), new_cfg_name);
			}
			style_adpter->set_name(new_cfg_name);
			item->setText(new_cfg_name);
		}
	}
}


bool
GPlatesQtWidgets::DrawStyleDialog::is_style_name_valid(
		const GPlatesGui::StyleCategory& catagory,
		const QString& cfg_name)
{
	if(cfg_name.contains('/'))// '/' cannot be in style name.
		return false;

	//check duplicated name.
	GPlatesGui::DrawStyleManager::StyleContainer styles = d_style_mgr->get_styles(catagory);
	BOOST_FOREACH(const GPlatesGui::StyleAdapter* _style, styles)
	{
		if(_style->name() == cfg_name)
			return false;
	}
	return true;
}


const QString
GPlatesQtWidgets::DrawStyleDialog::generate_new_valid_style_name(
		const GPlatesGui::StyleCategory& catagory,
		const QString& cfg_name)
{
	QString new_name_base = cfg_name;
	new_name_base.remove("/");
	if(is_style_name_valid(catagory, new_name_base))
	{
		return new_name_base;
	}
	else
	{
		int c = 1;
		while(1)
		{
			QString new_name = QString(new_name_base + "_%1").arg(c);
			if(is_style_name_valid(catagory, new_name))
				return new_name;
			else
				c++;
		}
	}
}


void
GPlatesQtWidgets::DrawStyleDialog::focus_style()
{
	if(!isVisible())
		return;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_visual_layer.lock())
	{
		focus_style(locked_visual_layer->get_visual_layer_params()->style_adapter());
	}
	else
	{
		focus_style(d_style_of_all);
	}
}


GPlatesQtWidgets::PreviewGuard::PreviewGuard(
		DrawStyleDialog& dlg) :
	d_dlg(dlg),
	d_current_idx(0)
{
	dlg.d_combo_box->setDisabled(true);
	dlg.categories_table->setDisabled(true);

	GPlatesQtWidgets::ReconstructionViewWidget & view_widget = 
		GPlatesPresentation::Application::instance().get_main_window().reconstruction_view_widget();
	if(view_widget.globe_is_active())
	{
		view_widget.globe_canvas().set_disable_update(true);
	}
	else if(view_widget.map_is_active())
	{
		view_widget.map_view().set_disable_update(true);
	}
	d_current_idx = dlg.style_list->currentRow();
}

GPlatesQtWidgets::PreviewGuard::~PreviewGuard()
{
	d_dlg.d_combo_box->setDisabled(false);
	d_dlg.categories_table->setDisabled(false);
			
	GPlatesQtWidgets::ReconstructionViewWidget & view_widget = 
		GPlatesPresentation::Application::instance().get_main_window().reconstruction_view_widget();
			
	if(view_widget.globe_is_active())
	{
		view_widget.globe_canvas().set_disable_update(false);
	}
	else if(view_widget.map_is_active())
	{
		view_widget.map_view().set_disable_update(false);
	}
	 d_dlg.style_list->setCurrentRow(d_current_idx);
	 d_dlg.set_style();
}


bool
operator==(
		const GPlatesMaths::LatLonPoint& p1,
		const GPlatesMaths::LatLonPoint& p2)
{
	return GPlatesMaths::are_almost_exactly_equal(p1.latitude(), p2.latitude()) && 
		GPlatesMaths::are_almost_exactly_equal(p1.longitude(), p2.longitude());
}


bool
GPlatesQtWidgets::DrawStyleDialog::is_dirty()
{
	boost::optional<GPlatesMaths::LatLonPoint> 
		main_camera_point = get_main_window_camera_point();

	if(main_camera_point && d_previous_camera_point && (operator==(*main_camera_point,*d_previous_camera_point)))
	{
		if(GPlatesPresentation::Application::instance().get_main_window().reconstruction_view_widget().globe_is_active())
		{
			if((*d_globe_and_map_widget_ptr->get_globe_canvas().orientation()).quat() !=
				(*get_main_orientation()).quat())
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::DrawStyleDialog::get_main_window_camera_point()
{
	GPlatesQtWidgets::ReconstructionViewWidget & view_widget = 
		GPlatesPresentation::Application::instance().get_main_window().reconstruction_view_widget();
	return view_widget.camera_llp();
}


boost::optional<GPlatesMaths::Rotation>
GPlatesQtWidgets::DrawStyleDialog::get_main_orientation()
{
	GPlatesQtWidgets::ReconstructionViewWidget & view_widget = 
		GPlatesPresentation::Application::instance().get_main_window().reconstruction_view_widget();
	return view_widget.globe_canvas().orientation();
}






















