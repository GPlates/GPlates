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

#ifndef GPLATES_APP_LOGIC_SERIALIZATION_H
#define GPLATES_APP_LOGIC_SERIALIZATION_H

#include <boost/noncopyable.hpp>

// Not using Boost Serialization at the moment, it is teh suck.
#if 0
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_free.hpp>

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>

#include <iostream>
#endif

#include <QObject>
#include <QPointer>
#include <QString>
#include <QByteArray>

#include <string>

#include "Session.h"


namespace GPlatesAppLogic
{
	class ApplicationState;

	class Serialization :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:

		explicit
		Serialization(
				GPlatesAppLogic::ApplicationState &app_state);

		virtual
		~Serialization()
		{  }

		/**
		 * Convert current layers state to something that we can save via the Session system
		 * or some future Projects system.
		 */
		GPlatesAppLogic::Session::LayersStateType
		save_layers_state();

		/**
		 * Convert xml-domified layers state to actual connections in the ReconstructGraph.
		 */
		void
		load_layers_state(
				const GPlatesAppLogic::Session::LayersStateType &dom);

	public slots:

		/**
		 * For testing during serialisation development, do a test-run of XML serialisation and print output.
		 */
		void
		debug_serialise();

	private:
		/**
		 * Guarded pointer back to ApplicationState so we can interact with the rest
		 * of GPlates. Since ApplicationState is a QObject, we don't have to worry
		 * about a dangling pointer (even though ApplicationState should never be
		 * destroyed before we are)
		 */
		QPointer<GPlatesAppLogic::ApplicationState> d_app_state_ptr;
	};
}




// Not using Boost Serialization at the moment, it is teh suck.
#if 0

/**
 * Enable boost::serialization to handle external classes like QString.
 */
namespace boost {
	namespace serialization {

		////////////////////////////////////////
		//           QSTRING SUPPORT          //
		////////////////////////////////////////

		template<class Archive>
		void save(
				Archive & ar,
				const QString & str,
				const unsigned int version)
		{
			const std::string utf8(str.toUtf8().data());
			ar << BOOST_SERIALIZATION_NVP(utf8);	// Macro necessary even in this trivial case, yes.
		}

		template<class Archive>
		void load(
				Archive & ar,
				QString & str,
				const unsigned int version)
		{
			std::string utf8;
			ar >> BOOST_SERIALIZATION_NVP(utf8);	// Macro necessary even in this trivial case, yes.
			str = QString::fromUtf8(utf8.data());
		}

		/**
		 * Necessary to enable distinct save/load functions for this non-intrusive serialisation.
		 * http://www.boost.org/doc/libs/1_46_1/libs/serialization/doc/serialization.html#splitting
		 */
		template<class Archive>
		void serialize(
				Archive & ar,
				QString & str,
				const unsigned int version)
		{
			split_free(ar, str, version);
		}

	} // namespace serialization
} // namespace boost

#endif


#endif // GPLATES_APP_LOGIC_SERIALIZATION_H

