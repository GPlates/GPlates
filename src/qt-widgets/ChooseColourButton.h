/* $Id$ */

/**
 * \file
 *
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
 
#ifndef GPLATES_QTWIDGETS_CHOOSECOLOURBUTTON_H
#define GPLATES_QTWIDGETS_CHOOSECOLOURBUTTON_H

#include <QToolButton>

#include "gui/Colour.h"


namespace GPlatesQtWidgets
{
	class ChooseColourButton :
			public QToolButton
	{
		Q_OBJECT

	public:

		explicit
		ChooseColourButton(
				QWidget *parent_ = NULL);

		void
		set_colour(
				const GPlatesGui::Colour &colour);

		const GPlatesGui::Colour &
		get_colour() const
		{
			return d_colour;
		}

	private Q_SLOTS:

		void
		handle_clicked();

	private:

		GPlatesGui::Colour d_colour;
	};
}

#endif  // GPLATES_QTWIDGETS_CHOOSECOLOURBUTTON_H
