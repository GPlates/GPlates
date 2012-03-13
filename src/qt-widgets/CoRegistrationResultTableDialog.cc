/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <QDebug>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <boost/optional.hpp> 

#include "CoRegistrationResultTableDialog.h"
#include "GlobeAndMapWidget.h"
#include "ReconstructionViewWidget.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/CoRegistrationLayerProxy.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Layer.h"

#include "file-io/ExportTemplateFilenameSequence.h"

#include "global/CompilerWarnings.h"

#include "gui/FeatureFocus.h"

#include "opengl/GLContext.h"
#include "opengl/GLRenderer.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::CoRegistrationResultTableDialog::CoRegistrationResultTableDialog(
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer,
		QWidget *parent_) :
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_visual_layer(visual_layer)
{
	setupUi(this);
	setModal(false);
	table_view = new ResultTableView(this);
	table_view->setObjectName(QString::fromUtf8("table_view"));
    table_view->setSelectionMode(QAbstractItemView::SingleSelection);
    table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_view->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    table_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	table_view->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	table_view->horizontalHeader()->setStretchLastSection(false);
	//table_view->setContextMenuPolicy(Qt::ActionsContextMenu);
	table_view->resizeColumnsToContents(); 
	vboxLayout->addWidget(table_view);

	QHBoxLayout* hboxLayout;
	QSpacerItem* spacerItem;
	hboxLayout = new QHBoxLayout();
	hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
	spacerItem = new QSpacerItem(91, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);
	hboxLayout->addItem(spacerItem);
	QPushButton* pushButton_close = new QPushButton(this);
	pushButton_close->setObjectName(QString::fromUtf8("pushButton_close"));
	hboxLayout->addWidget(pushButton_close);
	vboxLayout->addLayout(hboxLayout);
	pushButton_close->setText(QApplication::translate("CoRegistrationResultTableDialog", "close", 0, QApplication::UnicodeUTF8));
	QObject::connect(pushButton_close, SIGNAL(clicked()), this, SLOT(reject()));

	connect_application_state_signals(view_state.get_application_state());
}	


void	
GPlatesQtWidgets::CoRegistrationResultTableDialog::connect_application_state_signals(
		GPlatesAppLogic::ApplicationState &application_state)
{
	// Update whenever a new reconstruction happens (which in turn happens when the reconstruction
	// time changes or any layers/connections/inputs have been changed/modified).
	QObject::connect(
			&application_state,
			SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
			this,
			SLOT(update()));
}


void
GPlatesQtWidgets::CoRegistrationResultTableDialog::pop_up()
{
	QtWidgetUtils::pop_up_dialog(this);

	// Note: We update *after* popping up the dialog to ensure the *visible* table dialog
	// is correctly filled with the latest co-registration results.
	update();
}


void
GPlatesQtWidgets::CoRegistrationResultTableDialog::reject()
{
	done(QDialog::Rejected);
}

void
GPlatesQtWidgets::CoRegistrationResultTableDialog::update()
{
	// If result table is not visible then, as a client, we don't need to retrieve co-registration results.
	// Other clients (eg, co-registration export) can still retrieve co-registration results of course.
	if (!isVisible())
	{
		return;
	}

	boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_visual_layer.lock();
	if (!locked_visual_layer)
	{
		qWarning() << "CoRegistrationResultTableDialog: Unable to retrieve visual layer.";
		return;
	}

	GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();

	boost::optional<GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type> layer_proxy =
			layer.get_layer_output<GPlatesAppLogic::CoRegistrationLayerProxy>();
	if (!layer_proxy)
	{
		qWarning() << "CoRegistrationResultTableDialog: Expected a co-registration layer.";
		return;
	}

	//
	// Co-registration of rasters requires an OpenGL renderer (for co-registration of rasters).
	//

	// Get an OpenGL context for the (raster) co-registration since it accelerates it with OpenGL.
	GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
			d_viewport_window->reconstruction_view_widget().globe_and_map_widget().get_active_gl_context();

	// Make sure the context is currently active.
	gl_context->make_current();

	// Pass in the viewport of the window currently attached to the OpenGL context.
	const GPlatesOpenGL::GLViewport viewport(
			0, 0,
			d_view_state.get_main_viewport_dimensions().first/*width*/,
			d_view_state.get_main_viewport_dimensions().second/*height*/);

	// Start a begin_render/end_render scope.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = gl_context->create_renderer();
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, viewport);

	//
	// Get the co-registration results (perform the co-registration).
	//

	// Get the co-registration result data for the current reconstruction time.
	boost::optional<GPlatesAppLogic::CoRegistrationData::non_null_ptr_type> coregistration_data =
			layer_proxy.get()->get_coregistration_data(*renderer);

	// If there's no co-registration data then it means the user has not yet configured co-registration.
	if (!coregistration_data)
	{
		return;
	}

	// Update the co-registration data in the GUI.
	update_co_registration_data(coregistration_data.get()->data_table());
}


void
GPlatesQtWidgets::CoRegistrationResultTableDialog::update_co_registration_data(
		const GPlatesDataMining::DataTable &co_registration_data_table)
{
	d_table_model_prt.reset(new ResultTableModel(co_registration_data_table));
	table_view->setModel(d_table_model_prt.get());
	
	//table_view->setModel(new ResultTableModel(table));
	table_view->resizeColumnsToContents(); 
}


// For the nested BOOST_FOREACH below.
// Also for a problem in the ResultTableModel::data function.
DISABLE_GCC_WARNING("-Wshadow")


GPlatesModel::FeatureHandle::weak_ref
GPlatesQtWidgets::CoRegistrationResultTableDialog::find_feature_by_id(
		GPlatesAppLogic::FeatureCollectionFileState& state,
		const QString& id)
{
	using namespace GPlatesAppLogic;
	std::vector<FeatureCollectionFileState::file_reference> files = state.get_loaded_files();
	BOOST_FOREACH(FeatureCollectionFileState::file_reference& file_ref, files)
	{
		GPlatesModel::FeatureCollectionHandle::weak_ref fc = file_ref.get_file().get_feature_collection();
		BOOST_FOREACH(GPlatesModel::FeatureHandle::non_null_ptr_type feature, *fc)
		{
			if(feature->feature_id().get().qstring() == id)
			{
				return feature->reference();
			}
		}
	}
	throw QString("No feature found by this id: %1").arg(id);
}


QVariant
GPlatesQtWidgets::ResultTableModel::data(
		const QModelIndex &idx,
		int role) const
{
	if ( ! idx.isValid()) 
	{
		return QVariant();
	}

	if (idx.row() < 0 || idx.row() >= rowCount())	
	{
		return QVariant();
	}

	if (role == Qt::DisplayRole) 
	{
		GPlatesDataMining::OpaqueData o_data;
		d_table.at( idx.row() )->get_cell( idx.column(), o_data );

		return 
			boost::apply_visitor(
					GPlatesDataMining::ConvertOpaqueDataToString(),
					o_data);

	} 
	else if (role == Qt::TextAlignmentRole) 
	{
		return QVariant();
	}
	return QVariant();
}


void
GPlatesQtWidgets::CoRegistrationResultTableDialog::highlight_seed()
{
	QModelIndex idx = table_view->currentIndex();
	QString id;
	if(idx.column()==0)
		id = idx.data().toString();
	else
		id = idx.sibling(idx.row(),0).data().toString();

	GPlatesAppLogic::FeatureCollectionFileState& file_state = 
		d_view_state.get_application_state().get_feature_collection_file_state();
	try
	{
		GPlatesModel::FeatureHandle::weak_ref feature = 
			find_feature_by_id(file_state,id);
		d_view_state.get_feature_focus().set_focus(feature);
	}catch(QString err)
	{
		qDebug() << "exception happened!";
		qDebug() << err;
	}
}


