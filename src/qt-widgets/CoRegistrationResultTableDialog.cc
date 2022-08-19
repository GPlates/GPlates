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

#include <boost/optional.hpp> 
#include <QDebug>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>

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

#include "model/ModelUtils.h"

#include "opengl/GLContext.h"
#include "opengl/OpenGL.h"  // For Class GL and the OpenGL constants/typedefs

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
	table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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
	pushButton_close->setText(QApplication::translate("CoRegistrationResultTableDialog", "close", 0));
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
			d_viewport_window->reconstruction_view_widget().globe_and_map_widget().get_gl_context();

	// Make sure the context is currently active.
	gl_context->make_current();

	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = gl_context->create_gl();
	GPlatesOpenGL::GL::RenderScope render_scope(*gl);

	//
	// Get the co-registration results (perform the co-registration).
	//

	// Get the co-registration result data for the current reconstruction time.
	GPlatesAppLogic::CoRegistrationData::non_null_ptr_type coregistration_data =
			layer_proxy.get()->get_coregistration_data(*gl);

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


// For a problem in the ResultTableModel::data function.
DISABLE_GCC_WARNING("-Wshadow")

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
	QString id = (idx.column()==0) ? idx.data().toString() : 
		idx.sibling(idx.row(),0).data().toString();

	try
	{
		d_view_state.get_feature_focus().set_focus(
					GPlatesModel::ModelUtils::find_feature(id));
	}
	//TODO: Create new exception for this.
	catch(GPlatesGlobal::LogException& ex)
	{
		std::ostringstream ostr;
		ex.write(ostr);
		qDebug() << ostr.str().c_str();
	}
}


