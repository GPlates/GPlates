/* $Id: $ */

/**
 * @file
 * Contains the definitions of the member functions of the class PlatesRotationFileProxy.
 *
 * Most recent change:
 *   $Date: 2011-10-05 16:09:50 +1100 (Wed, 05 Oct 2011) $
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

#include "RotationAttributesRegistry.h"

GPlatesFileIO::RotationMetadataRegistry::RotationMetadataRegistry()
{
	using namespace MetadataType;
	register_metadata("GPLATESROTATIONFILE:version", MetadataAttribute(HEADER | MANDATORY));
	register_metadata("GPLATESROTATIONFILE:documentation", MetadataAttribute(HEADER | MANDATORY));
	register_metadata("REVISIONHIST:id", MetadataAttribute(HEADER | MULTI_OCCUR));
	register_metadata("BIBINFO:blbiographyfile", MetadataAttribute(HEADER | MANDATORY));
	register_metadata("BIBINFO:doibase", MetadataAttribute(HEADER | MANDATORY));
	register_metadata("GPML:namespace", MetadataAttribute(HEADER | MANDATORY));
	register_metadata("GPML:MagneticAnomalyPickingScheme", MetadataAttribute(HEADER)); 
	register_metadata("GTS:info:ID", MetadataAttribute(HEADER | MANDATORY | MULTI_OCCUR));
	register_metadata("GTS:info:DOI_URL_ISSN", MetadataAttribute(HEADER | MANDATORY | MULTI_OCCUR));
	register_metadata("GTS:info:PlainText", MetadataAttribute(HEADER | MANDATORY | MULTI_OCCUR)); 

	register_metadata("DC:namespace", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:title", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:creator:name", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:creator:email", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:creator:url", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:creator:affiliation", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:rights:license", MetadataAttribute(DC | MANDATORY)); 
	register_metadata("DC:rights:url", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:date:created", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:date:modified", MetadataAttribute(DC | MANDATORY | MULTI_OCCUR));
	register_metadata("DC:coverage:temporal", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:bibliographicCitation", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:description", MetadataAttribute(DC | MANDATORY));
	register_metadata("DC:contributor:ID", MetadataAttribute(DC | MANDATORY | MULTI_OCCUR));
	register_metadata("DC:contributor:AU", MetadataAttribute(DC | MANDATORY | MULTI_OCCUR));
	register_metadata("DC:contributor:RealName", MetadataAttribute(DC | MANDATORY | MULTI_OCCUR));
	register_metadata("DC:contributor:Email", MetadataAttribute(DC | MANDATORY | MULTI_OCCUR));
	register_metadata("DC:contributor:URL", MetadataAttribute(DC | MANDATORY | MULTI_OCCUR));
	
	register_metadata("MPRS:pid", MetadataAttribute(MPRS | MANDATORY)); 
	register_metadata("MPRS:code", MetadataAttribute(MPRS | MANDATORY));
	register_metadata("MPRS:name", MetadataAttribute(MPRS | MANDATORY));
	
	register_metadata("PP", MetadataAttribute(POLE | MANDATORY));
	register_metadata("REF", MetadataAttribute(POLE | REFERENCE, "BIBINFO:bibliographyfile:citekey"));
	register_metadata("DOI", MetadataAttribute(POLE | REFERENCE, "BIBINFO:doibase:doi")); 
	register_metadata("AU", MetadataAttribute(POLE | REFERENCE, "DC:contributor:id"));
	register_metadata("T", MetadataAttribute(POLE | MULTI_OCCUR));
	register_metadata("C", MetadataAttribute(POLE | MULTI_OCCUR));
	register_metadata("GTS", MetadataAttribute(POLE | MANDATORY | REFERENCE, "GEOTIMESCALE:id"));
	register_metadata("CHRONID", MetadataAttribute(POLE | REFERENCE,"GPML:MagneticAnomalyIdentification:polarityChronID")); 
	
	register_metadata("HELL", MetadataAttribute(POLE));
	register_metadata("HELL:r", MetadataAttribute(POLE));
	register_metadata("HELL:Ns", MetadataAttribute(POLE));
	register_metadata("HELL:dF", MetadataAttribute(POLE));
	register_metadata("HELL:kappahat", MetadataAttribute(POLE));
	register_metadata("HELL:cov", MetadataAttribute(POLE));
	
}



