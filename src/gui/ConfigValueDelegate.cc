/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <QWidget>
#include <QLineEdit>

#include "ConfigValueDelegate.h"


#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "gui/ConfigModel.h"	// for ROLE_RESET_TO_DEFAULT.

#include "qt-widgets/ConfigValueEditorWidget.h"


// Random thoughts:
// We can pass Schema information through a custom Qt::UserRole.



GPlatesGui::ConfigValueDelegate::ConfigValueDelegate(
		QObject *parent_):
	QItemDelegate(parent_)
{
}


QWidget *
GPlatesGui::ConfigValueDelegate::createEditor(
		QWidget *parent_widget,
		const QStyleOptionViewItem &option,
		const QModelIndex &idx) const
{
	GPlatesQtWidgets::ConfigValueEditorWidget *editor = 
			new GPlatesQtWidgets::ConfigValueEditorWidget(parent_widget);
	// We connect the editor's "I want to reset this value" signal to our superclass closeEditor()
	// slot. This will mean that setModelData() gets called with that editor widget as an argument;
	// we must inspect that widget for the "reset" flag there and send a message back to the model
	// via setData() that we want to reset the value to defaults. It's a bit of a roundabout way
	// to do things, I know, but pushing all these things through the existing Qt Model interface
	// is probably the least-sucky way to do things.
	connect(editor, SIGNAL(reset_requested(QWidget *)), this, SLOT(commit_and_close(QWidget *)));
	
	return editor;
}


void
GPlatesGui::ConfigValueDelegate::setEditorData(
		QWidget *editor,
		const QModelIndex &idx) const
{
	const QAbstractItemModel *model = idx.model();
	QLineEdit *actual_editor = editor->findChild<QLineEdit *>("editor");
	if (actual_editor) {
		actual_editor->setText(model->data(idx, Qt::DisplayRole).toString());
	}
}
		
		
void
GPlatesGui::ConfigValueDelegate::setModelData(
		QWidget *editor,
		QAbstractItemModel *model,
		const QModelIndex &idx) const
{
	// Cast the editor widget to what (we assume) it really is so we can do more clever stuff.
	GPlatesQtWidgets::ConfigValueEditorWidget *cfg_editor = 
			dynamic_cast<GPlatesQtWidgets::ConfigValueEditorWidget *>(editor);
	// (This delegate should ONLY be called with ConfigValueEditorWidget editors.)
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			cfg_editor != NULL, GPLATES_ASSERTION_SOURCE);
	
	// Before we set any value from this editor, we must check to see if the "reset to default"
	// button was pressed prior to this method being invoked by Qt's model/view system.
	if (cfg_editor->wants_reset()) {
		// Committing an invalid QVariant() with a special user ItemDataRole is being used to
		// indicate the user wants a reset.
		// In the far-off future, if we wanted to be able to send more complex messages,
		// we might register our own custom QVariant type - see the docs on QVariant for info.
		model->setData(idx, QVariant(), GPlatesGui::ConfigModel::ROLE_RESET_VALUE_TO_DEFAULT);
		return;
	}
	

	// FIXME: Assuming it's a QLineEdit editor for now:-
	QLineEdit *actual_editor = editor->findChild<QLineEdit *>("editor");
	if (actual_editor) {
		QString value = actual_editor->text();
		model->setData(idx, value);
	}
}


void
GPlatesGui::ConfigValueDelegate::updateEditorGeometry(
		QWidget *editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const
{
	// Do the bare minimum implementation.
	editor->setGeometry(option.rect);
}








