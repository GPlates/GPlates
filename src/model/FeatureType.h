/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATURETYPE_H
#define GPLATES_MODEL_FEATURETYPE_H

#include <QString>
#include <QStringList>

#include "QualifiedXmlName.h"

#include "utils/Parse.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/XmlNamespaces.h"

namespace GPlatesModel
{
	class FeatureTypeFactory
	{
	public:

		static
		GPlatesUtils::StringSet &
		instance()
		{
			return StringSetSingletons::feature_type_instance();
		}

	private:

		FeatureTypeFactory();
	};

	typedef QualifiedXmlName<FeatureTypeFactory> FeatureType;
}

namespace GPlatesUtils
{
	using GPlatesModel::FeatureType;

	// Specialisation of Parse for FeatureType.
	template<>
	struct Parse<FeatureType>
	{
		FeatureType
		operator()(
				const QString &s)
		{
			QStringList tokens = s.split(':');
			if (tokens.count() == 2)
			{
				return FeatureType(
						*GPlatesUtils::XmlNamespaces::get_namespace_for_standard_alias(
							GPlatesUtils::make_icu_string_from_qstring(tokens.at(0))),
						GPlatesUtils::make_icu_string_from_qstring(tokens.at(1)));
			}
			else
			{
				throw ParseError();
			}
		}
	};
}

#endif  // GPLATES_MODEL_FEATURETYPE_H
