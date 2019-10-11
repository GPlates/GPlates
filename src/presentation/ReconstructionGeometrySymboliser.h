
/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_RECONSTRUCTIONGEOMETRYSYMBOLISER_H
#define GPLATES_PRESENTATION_RECONSTRUCTIONGEOMETRYSYMBOLISER_H

#include <QObject>

#include "utils/ReferenceCount.h"

#include "LineSymbol.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}


namespace GPlatesPresentation
{
	class ReconstructionGeometrySymboliser :
			public QObject,
			public GPlatesUtils::ReferenceCount<ReconstructionGeometrySymboliser>
	{
		Q_OBJECT

	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructionGeometrySymboliser.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometrySymboliser> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionGeometrySymboliser> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ReconstructionGeometrySymboliser());
		}


		LineSymbol::non_null_ptr_to_const_type
		symbolise(
				const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry) const;

	Q_SIGNALS:

		/**
		 * Emitted when any aspect of the symboliser has been modified.
		 */
		void
		modified();

	private:
		ReconstructionGeometrySymboliser()
		{  }
	};
}

#endif // GPLATES_PRESENTATION_RECONSTRUCTIONGEOMETRYSYMBOLISER_H
