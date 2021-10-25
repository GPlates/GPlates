/* $Id:  $ */

/**
 * @file
 * Contains the definition of the class PlatesRotationFileProxy.
 *
 * Most recent change:
 *   $Date: 2011-08-18 22:01:47 +1000 (Thu, 18 Aug 2011) $
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

#ifndef GPLATES_FILEIO_ROTATIONATTRIBUTESREGISTRY_H
#define GPLATES_FILEIO_ROTATIONATTRIBUTESREGISTRY_H

#include <set>
#include <map>

#include <QStringList>

#include <boost/foreach.hpp>
#include "utils/Singleton.h"

namespace GPlatesFileIO
{
	namespace MetadataType
	{
		enum MetadataType
		{
			Invalid,
			DC = 0x00000001,
			HEADER = 0x00000002,
			MPRS = 0x00000004,
			POLE = 0x00000008,
			MANDATORY = 0x00000010,
			MULTI_OCCUR = 0x00000020,
			REFERENCE = 0x00000040
		};
	}
	
	struct MetadataAttribute
	{
		MetadataAttribute(
				quint64 flags = MetadataType::Invalid,
				const QString& ref_str = ""):
			type_flag(flags),
			ref_name(ref_str)
		{	}
		
		quint64 type_flag;
		QString ref_name;
	};

	//Rotation Attributes Registry
	class RotationMetadataRegistry:
		public GPlatesUtils::Singleton<RotationMetadataRegistry>
	{
		GPLATES_SINGLETON_CONSTRUCTOR_DECL(RotationMetadataRegistry)

	public:
		typedef std::map<QString,MetadataAttribute> MetadataAttrMap;

		void
		register_metadata(
				const QString& name,
				const MetadataAttribute& attr)
		{
			d_map.insert(MetadataAttrMap::value_type(name, attr));
		}

		const MetadataAttribute
		get(
				const QString& name) const
		{
			MetadataAttrMap::const_iterator it = d_map.find(name);
			if(it != d_map.end())
				return it->second;
			else
				return MetadataAttribute();
		}

		const MetadataAttrMap&
		get() const
		{
			return d_map;
		}

		const MetadataAttrMap
		get(
				quint64 flags) const
		{
			MetadataAttrMap ret;
			BOOST_FOREACH(const MetadataAttrMap::value_type v, d_map )
			{
				if((v.second.type_flag & flags) == flags)
				{
					ret.insert(v);
				}
			}
			return ret;
		}

	protected:
		MetadataAttrMap d_map;
	};
}
#endif //GPLATES_FILEIO_ROTATIONATTRIBUTESREGISTRY_H

