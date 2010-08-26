/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#include <boost/shared_ptr.hpp>
#include <QDebug>

#include "VisualLayersDelegate.h"

#include "VisualLayerWidget.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/VisualLayersListModel.h"
#include "gui/VisualLayersProxy.h"

#include "presentation/VisualLayer.h"


GPlatesQtWidgets::VisualLayersDelegate::VisualLayersDelegate(
		GPlatesGui::VisualLayersProxy &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QString &open_file_path,
		ReadErrorAccumulationDialog *read_errors_dialog,
		QObject *parent_) :
	QItemDelegate(parent_),
	d_visual_layers(visual_layers),
	d_application_state(application_state),
	d_view_state(view_state),
	d_open_file_path(open_file_path),
	d_read_errors_dialog(read_errors_dialog)
{  }


QSize
GPlatesQtWidgets::VisualLayersDelegate::sizeHint(
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const
{
	// Find the VisualLayer at the given index.
	boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer =
		d_visual_layers.visual_layer_at(index.row());

	// We want the height of the sizeHint to be the preferred height of the edit
	// widget for the given index.
	QSize result = QItemDelegate::sizeHint(option, index);
	editor_ptr_map_type::const_iterator iter = d_editor_ptrs.find(visual_layer);
	if (iter != d_editor_ptrs.end())
	{
		VisualLayerWidget *editor = iter->second;
		result.setHeight(editor->sizeHint().height());
	}

	return result;
}


QWidget *
GPlatesQtWidgets::VisualLayersDelegate::createEditor(
		QWidget *parent_,
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const
{
	return new VisualLayerWidget(
			d_visual_layers,
			d_application_state,
			d_view_state,
			d_open_file_path,
			d_read_errors_dialog,
			parent_);
}


void
GPlatesQtWidgets::VisualLayersDelegate::setEditorData(
		QWidget *editor,
		const QModelIndex &index) const
{
	// qDebug() << index.row() << d_visual_layers.size();
	if (!index.isValid())
	{
		return;
	}

	// The editor should be a VisualLayerWidget and nothing else.
	VisualLayerWidget *visual_layer_editor = dynamic_cast<VisualLayerWidget *>(editor);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			visual_layer_editor,
			GPLATES_ASSERTION_SOURCE);

	// Get the visual layer at the given index; the visual layer should be valid.
	boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer =
		d_visual_layers.visual_layer_at(index.row());
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!visual_layer.expired(),
			GPLATES_ASSERTION_SOURCE);

	// Remember that the visual_layer_editor is editing visual_layer.
	editor_ptr_map_type::iterator iter = d_editor_ptrs.find(visual_layer);
	if (iter == d_editor_ptrs.end())
	{
		d_editor_ptrs.insert(std::make_pair(visual_layer, visual_layer_editor));
	}
	else
	{
		iter->second = visual_layer_editor;
	}

	// Update the edit widget.
	visual_layer_editor->set_data(visual_layer, index.row());

	// The sizeHint of the editor widget may well have changed because we updated
	// the data displayed in it, so let's tell any views attached about this.
	emit_size_hint_changed(index);
}


void
GPlatesQtWidgets::VisualLayersDelegate::handle_layer_about_to_be_removed(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	d_editor_ptrs.erase(visual_layer);
}


void
GPlatesQtWidgets::VisualLayersDelegate::make_signal_slot_connections()
{
	// Listen in to when a layer gets removed.
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_about_to_be_removed(
					boost::weak_ptr<GPlatesPresentation::VisualLayer>)),
			this,
			SLOT(handle_layer_about_to_be_removed(
					boost::weak_ptr<GPlatesPresentation::VisualLayer>)));
}


void
GPlatesQtWidgets::VisualLayersDelegate::emit_size_hint_changed(
		const QModelIndex &index) const
{
	// This ugly hack is unfortunately necessary because the sizeHint of a row can
	// change after a call to setEditorData, which is const.
	const_cast<VisualLayersDelegate *>(this)->emit_size_hint_changed(index);
}


void
GPlatesQtWidgets::VisualLayersDelegate::emit_size_hint_changed(
		const QModelIndex &index)
{
	emit sizeHintChanged(index);
}

