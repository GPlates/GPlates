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

#ifndef GPLATES_QT_WIDGETS_RESIZETOCONTENTSTEXTEDIT_H
#define GPLATES_QT_WIDGETS_RESIZETOCONTENTSTEXTEDIT_H

#include <boost/optional.hpp>
#include <QString>
#include <QTextEdit>
#include <QWidget>


namespace GPlatesQtWidgets
{
	/**
	 * A QTextEdit that resizes to its contents (via overriding sizeHints and minimumSizeHints).
	 */
	class ResizeToContentsTextEdit :
			public QTextEdit
	{
		Q_OBJECT
		
	public:

		/**
		 * Resized only to contents *height* by default.
		 */
		explicit
		ResizeToContentsTextEdit(
				QWidget* parent_ = NULL,
				bool resize_to_contents_width = false,
				bool resize_to_contents_height = true);

		/**
		 * Resized only to contents *height* by default.
		 */
		explicit
		ResizeToContentsTextEdit(
				const QString &text_,
				QWidget* parent_ = NULL,
				bool resize_to_contents_width = false,
				bool resize_to_contents_height = true);

		virtual
		QSize
		sizeHint() const;

		virtual
		QSize
		minimumSizeHint() const;

	public slots:

		void
		fit_to_document_width();

		void
		fit_to_document_height();

		void
		fit_to_document();

	private:

		boost::optional<int> d_fitted_width;
		boost::optional<int> d_fitted_height;


		void
		initialise(
				bool resize_to_contents_width,
				bool resize_to_contents_height);
	};
}

#endif // GPLATES_QT_WIDGETS_RESIZETOCONTENTSTEXTEDIT_H
