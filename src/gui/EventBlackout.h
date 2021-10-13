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
 
#ifndef GPLATES_GUI_EVENTBLACKOUT_H
#define GPLATES_GUI_EVENTBLACKOUT_H

#include <set>
#include <QWidget>


namespace GPlatesGui
{
	/**
	 * EventBlackout, when enabled, discards all events other than those necessary
	 * for refreshing the user interface. As a special exception, it also lets
	 * through the Ctrl+C key combination.
	 *
	 * This is used during Python execution to prevent th user from interacting
	 * with GPlates while maintaining a responsive user interface. The rationale is
	 * that the GPlates model is single-threaded and so we should not allow the
	 * user to interact with GPlates and potentially modify the model.
	 */
	class EventBlackout :
			public QObject
	{
	public:

		explicit
		EventBlackout();

		/**
		 * Begins the event blackout.
		 */
		void
		start();

		/**
		 * Ends the event blackout.
		 */
		void
		stop();

		/**
		 * Exempt @a widget from the event blackout. All events will be delivered to
		 * @a widget and its children as usual.
		 */
		void
		add_blackout_exemption(
				QWidget *widget);

		/**
		 * Removes @a widget from event blackout exemption. Only certain events will
		 * now be delivered to @a widget and its children.
		 */
		void
		remove_blackout_exemption(
				QWidget *widget);

		/**
		 * Returns whether the event blackout is in force.
		 */
		bool
		has_started() const
		{
			return d_has_started;
		}

	protected:

		virtual
		bool
		eventFilter(
				QObject *obj,
				QEvent *ev);

	private:

		bool d_has_started;
		std::set<QWidget *> d_exempt_widgets;
	};
}

#endif	// GPLATES_GUI_EVENTBLACKOUT_H
