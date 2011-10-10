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
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QIcon>

#include "ConfigValueDelegate.h"


// Random thoughts:
// We can pass Schema information through a custom Qt::UserRole.

// wrt Reset button,
// Subclass QWidget for ConfigValueEditorWidget or somesuch.
// This merely takes the clicked() from the reset button and connects to it:s own
// handle_reset() which sets a flag and emits a reset_requested(QWidget *editor)
// which is connected to the ConfigValueDelegate::closeEditor(QWidget *editor)
// which is connected to the thigh-bone.
// When setModelData() is then called by Qt, ConfigValueDelegate can check for this
// 'reset to default' flag and commit an invalid QVariant() to the ConfigModel which
// will interpret *that* as a request to revert to the default.
// Swings and roundabouts really.



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
	static const QIcon reset_icon(":/tango_undo_16.png");

	QWidget *editor = new QWidget(parent_widget);
	QHBoxLayout *hbox = new QHBoxLayout(editor);
	hbox->setContentsMargins(0, 0, 0, 0);
	hbox->setSpacing(1);
	editor->setLayout(hbox);
	QLineEdit *line_edit = new QLineEdit(editor);
	line_edit->setObjectName("editor");
	hbox->addWidget(line_edit);
	QToolButton *reset_button = new QToolButton(editor);
	reset_button->setObjectName("reset");
	reset_button->setIcon(reset_icon);
	reset_button->setIconSize(QSize(16, 16));
	reset_button->setToolTip(tr("Reset to default value"));
	
	hbox->addWidget(reset_button);
	editor->setFocusProxy(line_edit);
	
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








