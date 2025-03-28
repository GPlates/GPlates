/* $Id$ */

/**
 * \file 
 * Interface for visiting derived @a RenderedGeometryImpl classes.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYVISITOR_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYVISITOR_H

namespace GPlatesViewOperations
{
	class RenderedArrowedPolyline;
	class RenderedCircleSymbol;
	class RenderedColouredEdgeSurfaceMesh;
	class RenderedColouredMultiPointOnSphere;
	class RenderedColouredPolygonOnSphere;
	class RenderedColouredPolylineOnSphere;
	class RenderedColouredTriangleSurfaceMesh;
	class RenderedCrossSymbol;
	class RenderedEllipse;
	class RenderedMultiPointOnSphere;
	class RenderedMultiReconstructionGeometry;
	class RenderedPointOnSphere;
	class RenderedPolygonOnSphere;
	class RenderedPolylineOnSphere;
	class RenderedRadialArrow;
	class RenderedReconstructionGeometry;
	class RenderedResolvedRaster;
	class RenderedResolvedScalarField3D;
	class RenderedSmallCircle;
	class RenderedSmallCircleArc;
	class RenderedSquareSymbol;
	class RenderedStrainMarkerSymbol;
	class RenderedString;
	class RenderedSubductionTeethPolyline;
	class RenderedTangentialArrow;
	class RenderedTriangleSymbol;

	/**
	 * Interface for visiting a derived @a RenderedGeometryImpl object.
	 *
	 * This visits const objects only because @a RenderedGeometry objects
	 * are immutable so we should respect this when accessing their
	 * implementation types.
	 */
	class ConstRenderedGeometryVisitor
	{
	public:
		virtual
		~ConstRenderedGeometryVisitor()
		{  }

		virtual
		void
		visit_rendered_arrowed_polyline(
				const GPlatesViewOperations::RenderedArrowedPolyline &)
		{  }

		virtual
		void
		visit_rendered_circle_symbol(
			const RenderedCircleSymbol &)
		{  }

		virtual
		void
		visit_rendered_coloured_edge_surface_mesh(
			const RenderedColouredEdgeSurfaceMesh &)
		{  }

		virtual
		void
		visit_rendered_coloured_triangle_surface_mesh(
			const RenderedColouredTriangleSurfaceMesh &)
		{  }

		virtual
		void
		visit_rendered_cross_symbol(
				const GPlatesViewOperations::RenderedCrossSymbol &)
		{  }

		virtual
		void
		visit_rendered_ellipse(
				const GPlatesViewOperations::RenderedEllipse &)
		{  }

		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &)
		{  }

		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const RenderedMultiPointOnSphere &)
		{  }

		virtual
		void
		visit_rendered_coloured_multi_point_on_sphere(
				const RenderedColouredMultiPointOnSphere &)
		{  }

		virtual
		void
		visit_rendered_polyline_on_sphere(
				const RenderedPolylineOnSphere &)
		{  }

		virtual
		void
		visit_rendered_coloured_polyline_on_sphere(
				const RenderedColouredPolylineOnSphere &)
		{  }

		virtual
		void
		visit_rendered_polygon_on_sphere(
				const RenderedPolygonOnSphere &)
		{  }

		virtual
		void
		visit_rendered_coloured_polygon_on_sphere(
				const RenderedColouredPolygonOnSphere &)
		{  }

		virtual
		void
		visit_rendered_radial_arrow(
				const RenderedRadialArrow &)
		{  }

		/**
		 * This rendered geometry is a composite object as opposed to the others.
		 *
		 * It wraps/associates a @a GPlatesAppLogic::ReconstructionGeometry object with a rendered geometry.
		 */
		virtual
		void
		visit_rendered_reconstruction_geometry(
				const RenderedReconstructionGeometry &)
		{  }

		/**
		 * This rendered geometry is a composite object as opposed to the others.
		 *
		 * It wraps/associates multiple @a GPlatesAppLogic::ReconstructionGeometry objects with a single rendered geometry.
		 */
		virtual
		void
		visit_rendered_multi_reconstruction_geometry(
				const RenderedMultiReconstructionGeometry &)
		{  }

		virtual
		void
		visit_rendered_resolved_raster(
				const RenderedResolvedRaster &)
		{  }
		
		virtual
		void
		visit_rendered_resolved_scalar_field_3d(
				const RenderedResolvedScalarField3D &)
		{  }
		
		virtual
		void
		visit_rendered_small_circle(
				const RenderedSmallCircle &)
		{  }
		
		virtual
		void
		visit_rendered_small_circle_arc(
			const RenderedSmallCircleArc &)
		{  }		

		virtual
		void
		visit_rendered_square_symbol(
			const RenderedSquareSymbol &)
		{  }

		virtual
		void
		visit_rendered_strain_marker_symbol(
				const GPlatesViewOperations::RenderedStrainMarkerSymbol &)
		{  }

		virtual
		void
		visit_rendered_string(
				const RenderedString &)
		{  }

		virtual
		void
		visit_rendered_subduction_teeth_polyline(
				const RenderedSubductionTeethPolyline &)
		{  }

		/**
		 * Note that this is not the same as a polyline with a symbol decoration.
		 *
		 * This is a single directional arrow tangential to the globe's surface.
		 */
		virtual
		void
		visit_rendered_tangential_arrow(
				const RenderedTangentialArrow &)
		{  }

		virtual
		void
		visit_rendered_triangle_symbol(
				const RenderedTriangleSymbol &)
		{  }
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYVISITOR_H
