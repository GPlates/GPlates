/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "ResizeToContentsTextEdit.h"


GPlatesQtWidgets::ResizeToContentsTextEdit::ResizeToContentsTextEdit(
		QWidget* parent_,
		bool resize_to_contents_width,
		bool resize_to_contents_height) :
	QTextEdit(parent_)
{
	initialise(resize_to_contents_width, resize_to_contents_height);
}


GPlatesQtWidgets::ResizeToContentsTextEdit::ResizeToContentsTextEdit(
		const QString &text_,
		QWidget* parent_,
		bool resize_to_contents_width,
		bool resize_to_contents_height) :
	QTextEdit(text_, parent_)
{
	initialise(resize_to_contents_width, resize_to_contents_height);
}


void
GPlatesQtWidgets::ResizeToContentsTextEdit::initialise(
		bool resize_to_contents_width,
		bool resize_to_contents_height)
{
	if (resize_to_contents_width && resize_to_contents_height)
	{
		QObject::connect(this, SIGNAL(textChanged()), this, SLOT(fit_to_document()));
	}
	else if (resize_to_contents_width)
	{
		QObject::connect(this, SIGNAL(textChanged()), this, SLOT(fit_to_document_width()));
	}
	else if (resize_to_contents_height)
	{
		QObject::connect(this, SIGNAL(textChanged()), this, SLOT(fit_to_document_height()));
	}
}


QSize
GPlatesQtWidgets::ResizeToContentsTextEdit::sizeHint() const
{
	QSize size_hint = QTextEdit::sizeHint();
	if (d_fitted_width)
	{
		size_hint.setWidth(d_fitted_width.get() + (width() - viewport()->width()));
	}
	if (d_fitted_height)
	{
		size_hint.setHeight(d_fitted_height.get() + (height() - viewport()->height()));
	}
	return size_hint;
}


QSize
GPlatesQtWidgets::ResizeToContentsTextEdit::minimumSizeHint() const
{
	QSize size_hint = QTextEdit::minimumSizeHint();
	if (d_fitted_width)
	{
		size_hint.setWidth(d_fitted_width.get() + (width() - viewport()->width()));
	}
	if (d_fitted_height)
	{
		size_hint.setHeight(d_fitted_height.get() + (height() - viewport()->height()));
	}
	return size_hint;
}


void
GPlatesQtWidgets::ResizeToContentsTextEdit::fit_to_document_width()
{
	d_fitted_width = document()->size().toSize().width();
	d_fitted_height = boost::none;
	// 'sizeHint()' will now give a different value so get layout to recalculate.
	updateGeometry();
}


void
GPlatesQtWidgets::ResizeToContentsTextEdit::fit_to_document_height()
{
	d_fitted_width = boost::none;
	d_fitted_height = document()->size().toSize().height();
	// 'sizeHint()' will now give a different value so get layout to recalculate.
	updateGeometry();
}


void
GPlatesQtWidgets::ResizeToContentsTextEdit::fit_to_document()
{
	d_fitted_width = document()->size().toSize().width();
	d_fitted_height = document()->size().toSize().height();
	// 'sizeHint()' will now give a different value so get layout to recalculate.
	updateGeometry();
}
