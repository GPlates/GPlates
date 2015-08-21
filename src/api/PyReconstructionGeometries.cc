/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/type_traits/remove_const.hpp>

#include "PyReconstructionGeometries.h"

#include "PyRotationModel.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionGeometryVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/PolylineOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * Convert a GeometryOnSphere to a PolylineOnSphere (even if they are just single points - just duplicate).
		 *
		 * This will make it easier for the user who will expect sub-segments to be polylines, or
		 * RFG topological sections to be polylines (instead, eg, polygons).
		 */
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		convert_geometry_to_polyline(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &sub_segment_geometry)
		{
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> sub_segment_polyline =
					GPlatesAppLogic::GeometryUtils::convert_geometry_to_polyline(*sub_segment_geometry);
			if (!sub_segment_polyline)
			{
				// There were less than two points.
				// 
				// Retrieve the point.
				std::vector<GPlatesMaths::PointOnSphere> geometry_points;
				GPlatesAppLogic::GeometryUtils::get_geometry_points(*sub_segment_geometry, geometry_points);

				// There should be a single point.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						!geometry_points.empty(),
						GPLATES_ASSERTION_SOURCE);

				// Duplicate the point.
				geometry_points.push_back(geometry_points.back());

				sub_segment_polyline = GPlatesMaths::PolylineOnSphere::create_on_heap(geometry_points);
			}

			return sub_segment_polyline.get();
		}

		/**
		 * Get the polyline from the reconstruction geometry of a topological section feature.
		 *
		 * This can be either a reconstructed feature geometry or a resolved topological *line*,
		 * otherwise returns none.
		 */
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		get_topological_section_geometry(
				const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &topological_section)
		{
			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> topological_section_geometry =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_resolved_topological_boundary_section_geometry(
							topological_section);
			if (!topological_section_geometry)
			{
				return boost::none;
			}

			return convert_geometry_to_polyline(topological_section_geometry.get());
		}
	}


	/**
	 * Returns the referenced feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned
	 * to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	reconstruction_geometry_get_feature(
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry)
	{
		// The feature reference could be invalid. It should normally be valid though.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(&reconstruction_geometry);
		if (!feature_ref ||
			!feature_ref->is_valid())
		{
			return boost::none;
		}

		return GPlatesModel::FeatureHandle::non_null_ptr_type(feature_ref->handle_ptr());
	}

	/**
	 * Returns the referenced feature property.
	 *
	 * The feature property reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned
	 * to the caller.
	 */
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
	reconstruction_geometry_get_property(
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry)
	{
		// The property iterator could be invalid. It should normally be valid though.
		boost::optional<GPlatesModel::FeatureHandle::iterator> property_iter =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
						&reconstruction_geometry);
		if (!property_iter ||
			!property_iter->is_still_valid())
		{
			return boost::none;
		}

		return *property_iter.get();
	}

	/**
	 * ReconstructionGeometry visitor to create a derived reconstruction geometry type wrapper.
	 */
	class WrapReconstructionGeometryTypeVisitor :
			public GPlatesAppLogic::ConstReconstructionGeometryVisitor
	{
	public:
		// Bring base class visit methods into scope of current class.
		using GPlatesAppLogic::ConstReconstructionGeometryVisitor::visit;


		const boost::any &
		get_reconstruction_geometry_type_wrapper() const
		{
			return d_reconstruction_geometry_type_wrapper;
		}


		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<
							boost::remove_const<reconstructed_feature_geometry_type>::type>(rfg));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_motion_path_type> &rmp)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<
							boost::remove_const<reconstructed_motion_path_type>::type>(rmp));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_flowline_type> &rf)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<
							boost::remove_const<reconstructed_flowline_type>::type>(rf));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_line_type> &rtl)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<
							boost::remove_const<resolved_topological_line_type>::type>(rtl));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<
							boost::remove_const<resolved_topological_boundary_type>::type>(rtb));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<
							boost::remove_const<resolved_topological_network_type>::type>(rtn));
		}

	private:
		// We just need to store the wrapper - we don't need to access it.
		boost::any d_reconstruction_geometry_type_wrapper;
	};


	KeepReconstructionGeometryFeatureAndPropertyAlive::KeepReconstructionGeometryFeatureAndPropertyAlive(
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) :
		d_feature(reconstruction_geometry_get_feature(reconstruction_geometry)),
		d_property(reconstruction_geometry_get_property(reconstruction_geometry))
	{  }


	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry) :
		d_reconstruction_geometry(
				// Boost-python currently does not compile when wrapping *const* objects
				// (eg, 'ReconstructionGeometry::non_null_ptr_to_const_type') - see:
				//   https://svn.boost.org/trac/boost/ticket/857
				//   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
				//
				// ...so the current solution is to wrap *non-const* objects (to keep boost-python happy)
				// and cast away const (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ReconstructionGeometry>(reconstruction_geometry)),
		d_reconstruction_geometry_type_wrapper(
				create_reconstruction_geometry_type_wrapper(reconstruction_geometry))
	{  }


	boost::any
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>::create_reconstruction_geometry_type_wrapper(
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry)
	{
		WrapReconstructionGeometryTypeVisitor visitor;
		reconstruction_geometry->accept_visitor(visitor);
		return visitor.get_reconstruction_geometry_type_wrapper();
	}


	/**
	 * Python converter from a derived 'ReconstructionGeometryType' to a
	 * 'ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>' (and vice versa).
	 */
	template <class ReconstructionGeometryType>
	struct python_ReconstructionGeometryType :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const typename ReconstructionGeometryType::non_null_ptr_type &rg)
			{
				// Convert to ReconstructionGeometryTypeWrapper<> first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(
						ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>(rg)).ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			// 'ReconstructionGeometryType' is obtained from a
			// ReconstructionGeometryTypeWrapper<> (which in turn is already convertible).
			return bp::extract< ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> >(obj).check()
					? obj
					: NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				bp::converter::rvalue_from_python_stage1_data *data)
		{
			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							typename ReconstructionGeometryType::non_null_ptr_type> *>(
									data)->storage.bytes;

			new (storage) typename ReconstructionGeometryType::non_null_ptr_type(
					bp::extract< ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> >(obj)()
							.get_reconstruction_geometry_type());

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a derived 'ReconstructionGeometryType' to a
	 * 'ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>' (and vice versa).
	 */
	template <class ReconstructionGeometryType>
	void
	register_reconstruction_geometry_type_conversion()
	{
		// To python conversion.
		bp::to_python_converter<
				typename ReconstructionGeometryType::non_null_ptr_type,
				typename python_ReconstructionGeometryType<ReconstructionGeometryType>::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_ReconstructionGeometryType<ReconstructionGeometryType>::convertible,
				&python_ReconstructionGeometryType<ReconstructionGeometryType>::construct,
				bp::type_id<typename ReconstructionGeometryType::non_null_ptr_type>());
	}
}


void
export_reconstruction_geometry()
{
	//
	// ReconstructionGeometry - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructionGeometry,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ReconstructionGeometry",
					"The base class inherited by all derived reconstruction geometry classes.\n"
					"\n"
					"The list of derived classes is:\n"
					"\n"
					"* :class:`ReconstructedFeatureGeometry`\n"
					"* :class:`ReconstructedMotionPath`\n"
					"* :class:`ReconstructedFlowline`\n"
					"* :class:`ResolvedTopologicalLine`\n"
					"* :class:`ResolvedTopologicalBoundary`\n"
					"* :class:`ResolvedTopologicalNetwork`\n",
					bp::no_init)
		.def("get_reconstruction_time",
				&GPlatesAppLogic::ReconstructionGeometry::get_reconstruction_time,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_reconstruction_time()\n"
				"  Returns the reconstruction time that this instance was created at.\n"
				"\n"
				"  :rtype: float\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ReconstructionGeometryTypeWrapper<> to be converted to
	// a GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstruction_geometry_type_conversion<GPlatesAppLogic::ReconstructionGeometry>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructionGeometry
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Enable boost::optional<ReconstructionGeometry::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	bp::implicitly_convertible<
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type,
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>();
	bp::implicitly_convertible<
			boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>,
			boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> >();
}


namespace GPlatesApi
{
	/**
	 * Returns the present day geometry.
	 *
	 * boost::none could be returned but it normally shouldn't so we don't document that Py_None
	 * could be returned to the caller.
	 */
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	reconstructed_feature_geometry_get_present_day_geometry(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry)
	{
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> property =
				reconstruction_geometry_get_property(reconstructed_feature_geometry);
		if (!property)
		{
			return boost::none;
		}

		// Call python since Property.get_value is implemented in python code...
		bp::object property_value_object = bp::object(property.get()).attr("get_value")();
		if (property_value_object == bp::object()/*Py_None*/)
		{
			return boost::none;
		}

		// Get the property value.
		GPlatesModel::PropertyValue::non_null_ptr_type property_value =
				bp::extract<GPlatesModel::PropertyValue::non_null_ptr_type>(property_value_object);

		// Extract the geometry from the property value.
		return GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(*property_value);
	}

	/**
	 * Returns the reconstructed geometry.
	 */
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
	reconstructed_feature_geometry_get_reconstructed_geometry(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry)
	{
		return reconstructed_feature_geometry.reconstructed_geometry();
	}
}


void
export_reconstructed_feature_geometry()
{
	//
	// ReconstructedFeatureGeometry - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructedFeatureGeometry,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedFeatureGeometry>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ReconstructedFeatureGeometry",
					"The geometry of a feature reconstructed to a geological time.\n"
					"\n"
					"The :func:`reconstruct` function can be used to generate *ReconstructedFeatureGeometry* instances.\n"
					"\n"
					".. note:: | A single feature can have multiple geometry properties, and hence multiple "
					"reconstructed feature geometries, associated with it.\n"
					"          | Therefore each :class:`ReconstructedFeatureGeometry` references a different property of "
					"the feature via :meth:`get_property`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ReconstructedFeatureGeometry`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: Multiple :class:`reconstructed feature geometries<ReconstructedFeatureGeometry>` can "
				"be associated with the same :class:`feature<Feature>` if that feature has multiple geometry properties.\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the present day (unreconstructed) geometry "
				"associated with this :class:`ReconstructedFeatureGeometry`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day geometry<get_present_day_geometry>` "
				"and the :meth:`reconstructed geometry<get_reconstructed_geometry>` are obtained from.\n")
		.def("get_present_day_geometry",
				&GPlatesApi::reconstructed_feature_geometry_get_present_day_geometry,
				"get_present_day_geometry()\n"
				"  Returns the present day geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
		.def("get_reconstructed_geometry",
				&GPlatesApi::reconstructed_feature_geometry_get_reconstructed_geometry,
				"get_reconstructed_geometry()\n"
				"  Returns the reconstructed geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ReconstructionGeometryTypeWrapper<> to be converted to
	// a GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstruction_geometry_type_conversion<GPlatesAppLogic::ReconstructedFeatureGeometry>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedFeatureGeometry
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class ReconstructionGeometry.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesAppLogic::ReconstructedFeatureGeometry,
			GPlatesAppLogic::ReconstructionGeometry>();
}


namespace GPlatesApi
{
	/**
	 * Returns the motion path points.
	 */
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	reconstructed_motion_path_get_motion_path(
			const GPlatesAppLogic::ReconstructedMotionPath &reconstructed_motion_path)
	{
		return reconstructed_motion_path.motion_path_points();
	}

	/**
	 * Returns the reconstructed seed point.
	 */
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	reconstructed_motion_path_get_reconstructed_seed_point(
			const GPlatesAppLogic::ReconstructedMotionPath &reconstructed_motion_path)
	{
		return reconstructed_motion_path.reconstructed_seed_point();
	}

	/**
	 * Returns the present day seed point.
	 */
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	reconstructed_motion_path_get_present_day_seed_point(
			const GPlatesAppLogic::ReconstructedMotionPath &reconstructed_motion_path)
	{
		return reconstructed_motion_path.present_day_seed_point();
	}
}


void
export_reconstructed_motion_path()
{
	//
	// ReconstructedMotionPath - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructedMotionPath,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedMotionPath>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ReconstructedMotionPath",
					"The reconstructed history of a plate's motion in the form of a path of points "
					"over geological time.\n"
					"\n"
					"The :func:`reconstruct` function can be used to generate *ReconstructedMotionPath* instances.\n"
					"\n"
					".. note:: | Although a single motion path :class:`feature<Feature>` has only a single "
					"seed geometry that seed geometry can be either a :class:`PointOnSphere` or a "
					":class:`MultiPointOnSphere`.\n"
					"          | And since there is one :class:`reconstructed motion path<ReconstructedMotionPath>` "
					"per seed point there can be, in the case of a :class:`MultiPointOnSphere`, multiple "
					":class:`reconstructed motion paths<ReconstructedMotionPath>` per motion path "
					":class:`feature<Feature>`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ReconstructedMotionPath`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: Multiple :class:`reconstructed motion paths<ReconstructedMotionPath>` "
				"can be associated with the same motion path :class:`feature<Feature>` if its seed geometry "
				"is a :class:`MultiPointOnSphere`.\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the seed point associated with this "
				":class:`ReconstructedMotionPath`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day seed point<get_present_day_seed_point>` "
				"and the :meth:`reconstructed seed point<get_reconstructed_seed_point>` are obtained from.\n")
		.def("get_present_day_seed_point",
				&GPlatesApi::reconstructed_motion_path_get_present_day_seed_point,
				"get_present_day_seed_point()\n"
				"  Returns the present day seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  .. note:: This is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedMotionPath` instances.\n")
		.def("get_reconstructed_seed_point",
				&GPlatesApi::reconstructed_motion_path_get_reconstructed_seed_point,
				"get_reconstructed_seed_point()\n"
				"  Returns the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  .. note:: This is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedMotionPath` instances.\n")
		.def("get_motion_path",
				&GPlatesApi::reconstructed_motion_path_get_motion_path,
				"get_motion_path()\n"
				"  Returns the motion path.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  The returned points plot the history of motion of the "
				":meth:`seed point<get_present_day_seed_point>` on the plate associated with "
				"``get_feature().get_reconstruction_plate_id()`` relative to the plate associated "
				"with ``get_feature().get_relative_plate()``.\n"
				"\n"
				"  The first point in the returned :class:`PolylineOnSphere` is the furthest in the "
				"geological past and subsequent points are progressively more recent with the last "
				"point being the :meth:`reconstructed seed point<get_reconstructed_seed_point>`.\n"
				"\n"
				"  .. note:: This is just one of the motion paths associated with this "
				":meth:`feature's<get_feature>` seed geometry if that seed geometry is a "
				":class:`MultiPointOnSphere`.\n"
				"\n"
				"  Iterate over the motion path points:\n"
				"  ::\n"
				"\n"
				"    for point in reconstructed_motion_path.get_motion_path():\n"
				"      ...\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ReconstructionGeometryTypeWrapper<> to be converted to
	// a GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstruction_geometry_type_conversion<GPlatesAppLogic::ReconstructedMotionPath>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedMotionPath
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class ReconstructionGeometry.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesAppLogic::ReconstructedMotionPath,
			GPlatesAppLogic::ReconstructionGeometry>();
}


namespace GPlatesApi
{
	/**
	 * Returns the left flowline points.
	 */
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	reconstructed_flowline_get_left_flowline(
			const GPlatesAppLogic::ReconstructedFlowline &reconstructed_flowline)
	{
		return reconstructed_flowline.left_flowline_points();
	}

	/**
	 * Returns the right flowline points.
	 */
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	reconstructed_flowline_get_right_flowline(
			const GPlatesAppLogic::ReconstructedFlowline &reconstructed_flowline)
	{
		return reconstructed_flowline.right_flowline_points();
	}

	/**
	 * Returns the reconstructed seed point.
	 */
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	reconstructed_flowline_get_reconstructed_seed_point(
			const GPlatesAppLogic::ReconstructedFlowline &reconstructed_flowline)
	{
		return reconstructed_flowline.reconstructed_seed_point();
	}

	/**
	 * Returns the present day seed point.
	 */
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	reconstructed_flowline_get_present_day_seed_point(
			const GPlatesAppLogic::ReconstructedFlowline &reconstructed_flowline)
	{
		return reconstructed_flowline.present_day_seed_point();
	}
}


void
export_reconstructed_flowline()
{
	//
	// ReconstructedFlowline - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructedFlowline,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructedFlowline>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ReconstructedFlowline",
					"The reconstructed history of plate motion away from a spreading ridge in the form of "
					"a path of points over geological time.\n"
					"\n"
					"The :func:`reconstruct` function can be used to generate *ReconstructedFlowline* instances.\n"
					"\n"
					".. note:: | Although a single flowline :class:`feature<Feature>` has only a single "
					"seed geometry that seed geometry can be either a :class:`PointOnSphere` or a "
					":class:`MultiPointOnSphere`.\n"
					"          | And since there is one :class:`reconstructed flowline<ReconstructedFlowline>` "
					"per seed point there can be, in the case of a :class:`MultiPointOnSphere`, multiple "
					":class:`reconstructed flowlines<ReconstructedFlowline>` per flowline "
					":class:`feature<Feature>`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ReconstructedFlowline`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: Multiple :class:`reconstructed flowlines<ReconstructedFlowline>` "
				"can be associated with the same flowline :class:`feature<Feature>` if its seed geometry "
				"is a :class:`MultiPointOnSphere`.\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the seed point associated with this "
				":class:`ReconstructedFlowline`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day seed point<get_present_day_seed_point>` "
				"and the :meth:`reconstructed seed point<get_reconstructed_seed_point>` are obtained from.\n")
		.def("get_present_day_seed_point",
				&GPlatesApi::reconstructed_flowline_get_present_day_seed_point,
				"get_present_day_seed_point()\n"
				"  Returns the present day seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  .. note:: This is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedFlowline` instances.\n")
		.def("get_reconstructed_seed_point",
				&GPlatesApi::reconstructed_flowline_get_reconstructed_seed_point,
				"get_reconstructed_seed_point()\n"
				"  Returns the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  .. note:: This is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedFlowline` instances.\n")
		.def("get_left_flowline",
				&GPlatesApi::reconstructed_flowline_get_left_flowline,
				"get_left_flowline()\n"
				"  Returns the flowline spread along the *left* plate from the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  The returned points are on the *left* plate associated with "
				"``get_feature().get_left_plate()``\n"
				"\n"
				"  The first point in the returned :class:`PolylineOnSphere` is the "
				":meth:`reconstructed seed point<get_reconstructed_seed_point>` and subsequent points "
				"are progressively further in the geological past.\n"
				"\n"
				"  .. note:: This is just one of the *left* flowlines associated with this "
				":meth:`feature's<get_feature>` seed geometry if that seed geometry is a "
				":class:`MultiPointOnSphere`.\n"
				"\n"
				"  Iterate over the left flowline points:\n"
				"  ::\n"
				"\n"
				"    for left_point in reconstructed_flowline.get_left_flowline():\n"
				"      ...\n")
		.def("get_right_flowline",
				&GPlatesApi::reconstructed_flowline_get_right_flowline,
				"get_right_flowline()\n"
				"  Returns the flowline spread along the *right* plate from the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  The returned points are on the *right* plate associated with "
				"``get_feature().get_right_plate()``\n"
				"\n"
				"  The first point in the returned :class:`PolylineOnSphere` is the "
				":meth:`reconstructed seed point<get_reconstructed_seed_point>` and subsequent points "
				"are progressively further in the geological past.\n"
				"\n"
				"  .. note:: This is just one of the *right* flowlines associated with this "
				":meth:`feature's<get_feature>` seed geometry if that seed geometry is a "
				":class:`MultiPointOnSphere`.\n"
				"\n"
				"  Iterate over the right flowline points:\n"
				"  ::\n"
				"\n"
				"    for right_point in reconstructed_flowline.get_right_flowline():\n"
				"      ...\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ReconstructionGeometryTypeWrapper<> to be converted to
	// a GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstruction_geometry_type_conversion<GPlatesAppLogic::ReconstructedFlowline>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedFlowline
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class ReconstructionGeometry.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesAppLogic::ReconstructedFlowline,
			GPlatesAppLogic::ReconstructionGeometry>();
}


namespace GPlatesApi
{
	/**
	 * Returns the resolved line geometry.
	 */
	GPlatesAppLogic::ResolvedTopologicalLine::resolved_topology_line_ptr_type
	resolved_topological_line_get_resolved_line(
			const GPlatesAppLogic::ResolvedTopologicalLine &resolved_topological_line)
	{
		return resolved_topological_line.resolved_topology_line();
	}

	bp::list
	resolved_topological_line_get_line_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalLine &resolved_topological_line)
	{
		bp::list line_sub_segments_list;

		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_line.get_sub_segment_sequence();
		BOOST_FOREACH(const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment, sub_segments)
		{
			line_sub_segments_list.append(sub_segment);
		}

		return line_sub_segments_list;
	}

	// The derived reconstruction geometry type wrapper might be the wrong type.
	// It normally should be the right type though so we don't document that Py_None could be returned to the caller.
	bp::object
	resolved_topological_line_get_resolved_feature(
			const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &reconstruction_geometry)
	{
		// Extract the 'GPlatesAppLogic::ResolvedTopologicalLine' wrapper.
		boost::optional<const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine> &> wrapper =
				reconstruction_geometry.get_reconstruction_geometry_type_wrapper<GPlatesAppLogic::ResolvedTopologicalLine>();
		if (!wrapper)
		{
			return bp::object()/*Py_None*/;
		}

		return wrapper->get_resolved_feature();
	}

	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_to_const_type resolved_topological_line) :
		d_resolved_topological_line(
				// Boost-python currently does not compile when wrapping *const* objects
				// (eg, 'ReconstructionGeometry::non_null_ptr_to_const_type') - see:
				//   https://svn.boost.org/trac/boost/ticket/857
				//   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
				//
				// ...so the current solution is to wrap *non-const* objects (to keep boost-python happy)
				// and cast away const (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalLine>(resolved_topological_line)),
		d_keep_feature_property_alive(*resolved_topological_line)
	{
		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_line->get_sub_segment_sequence();

		d_sub_segments.reserve(sub_segments.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment,
				sub_segments)
		{
			d_sub_segments.push_back(
					ResolvedTopologicalGeometrySubSegmentWrapper(sub_segment));
		}
	}

	bp::object
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> feature =
					reconstruction_geometry_get_feature(*d_resolved_topological_line);
			if (!feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type resolved_geometry =
					d_resolved_topological_line->resolved_topology_line();

			// Clone the feature.
			bp::object resolved_feature_object = bp::object(feature.get()).attr("clone")();

			// Set the resolved line geometry.
			resolved_feature_object.attr("set_geometry")(resolved_geometry);

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}
}


void
export_resolved_topological_line()
{
	//
	// ResolvedTopologicalLine - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalLine,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ResolvedTopologicalLine",
					"The geometry of a topological *line* feature resolved to a geological time.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalLine* instances.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ResolvedTopologicalLine`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The returned feature is what was used to generate this :class:`ResolvedTopologicalLine` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the topological line property associated with "
				"this :class:`ResolvedTopologicalLine`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`get_resolved_line` and "
				":meth:`get_resolved_geometry` are obtained from.\n")
		.def("get_resolved_line",
				&GPlatesApi::resolved_topological_line_get_resolved_line,
				"get_resolved_line()\n"
				"  Returns the resolved line geometry.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_line_get_resolved_line,
				"get_resolved_geometry()\n"
				"  Same as :meth:`get_resolved_line`.\n")
		.def("get_resolved_feature",
				&GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>::get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved line geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the static :meth:`resolved geometry<get_resolved_geometry>`. "
				"Unlike :meth:`get_feature` it cannot be used to generate a :class:`ResolvedTopologicalLine` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_feature`\n")
		// This overload of 'get_resolved_feature' works with
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>.
		// This happens when a GPlatesAppLogic::ResolvedTopologicalLine is passed to Python as a
		// GPlatesAppLogic::ReconstructionGeometry. Coming from Python, boost-python knows the
		// GPlatesAppLogic::ReconstructionGeometry is a GPlatesAppLogic::ResolvedTopologicalLine
		// (which is how it gets here) but cannot convert from a
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> to a
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>
		// so we have to do that ourselves...
		.def("get_resolved_feature",
				&GPlatesApi::resolved_topological_line_get_resolved_feature,
				"\n") // A non-empty string otherwise boost-python will double docstring of other overload.
		.def("get_line_sub_segments",
				&GPlatesApi::resolved_topological_line_get_line_sub_segments,
				"get_line_sub_segments()\n"
				"  Returns the :class:`sub-segments<ResolvedTopologicalSubSegment>` that make up the "
				"line of this resolved topological line.\n"
				"\n"
				"  :rtype: list of :class:`ResolvedTopologicalSubSegment`\n"
				"\n"
				"  To get a list of the *unreversed* sub-segment geometries:\n"
				"  ::\n"
				"\n"
				"    sub_segment_geometries = []\n"
				"    for sub_segment in resolved_topological_line.get_line_sub_segments():\n"
				"        sub_segment_geometries.append(sub_segment.get_resolved_geometry())\n")
		.def("get_geometry_sub_segments",
				&GPlatesApi::resolved_topological_line_get_line_sub_segments,
				"get_geometry_sub_segments()\n"
				"  Same as :meth:`get_line_sub_segments`.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ReconstructionGeometryTypeWrapper<> to be converted to
	// a GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstruction_geometry_type_conversion<GPlatesAppLogic::ResolvedTopologicalLine>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalLine
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class ReconstructionGeometry.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesAppLogic::ResolvedTopologicalLine,
			GPlatesAppLogic::ReconstructionGeometry>();
}


namespace GPlatesApi
{
	/**
	 * Returns the resolved boundary geometry.
	 */
	GPlatesAppLogic::ResolvedTopologicalBoundary::resolved_topology_boundary_ptr_type
	resolved_topological_boundary_get_resolved_boundary(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_topological_boundary)
	{
		return resolved_topological_boundary.resolved_topology_boundary();
	}

	bp::list
	resolved_topological_boundary_get_boundary_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_topological_boundary)
	{
		bp::list boundary_sub_segments_list;

		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_boundary.get_sub_segment_sequence();
		BOOST_FOREACH(const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment, sub_segments)
		{
			boundary_sub_segments_list.append(sub_segment);
		}

		return boundary_sub_segments_list;
	}

	// The derived reconstruction geometry type wrapper might be the wrong type.
	// It normally should be the right type though so we don't document that Py_None could be returned to the caller.
	bp::object
	resolved_topological_boundary_get_resolved_feature(
			const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &reconstruction_geometry)
	{
		// Extract the 'GPlatesAppLogic::ResolvedTopologicalBoundary' wrapper.
		boost::optional<const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary> &> wrapper =
				reconstruction_geometry.get_reconstruction_geometry_type_wrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>();
		if (!wrapper)
		{
			return bp::object()/*Py_None*/;
		}

		return wrapper->get_resolved_feature();
	}

	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type resolved_topological_boundary) :
		d_resolved_topological_boundary(
				// Boost-python currently does not compile when wrapping *const* objects
				// (eg, 'ReconstructionGeometry::non_null_ptr_to_const_type') - see:
				//   https://svn.boost.org/trac/boost/ticket/857
				//   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
				//
				// ...so the current solution is to wrap *non-const* objects (to keep boost-python happy)
				// and cast away const (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalBoundary>(resolved_topological_boundary)),
		d_keep_feature_property_alive(*resolved_topological_boundary)
	{
		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_boundary->get_sub_segment_sequence();

		d_sub_segments.reserve(sub_segments.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment,
				sub_segments)
		{
			d_sub_segments.push_back(
					ResolvedTopologicalGeometrySubSegmentWrapper(sub_segment));
		}
	}

	bp::object
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> feature =
					reconstruction_geometry_get_feature(*d_resolved_topological_boundary);
			if (!feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_geometry =
					d_resolved_topological_boundary->resolved_topology_boundary();

			// Clone the feature.
			bp::object resolved_feature_object = bp::object(feature.get()).attr("clone")();

			// Set the resolved boundary geometry.
			resolved_feature_object.attr("set_geometry")(resolved_geometry);

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}
}


void
export_resolved_topological_boundary()
{
	//
	// ResolvedTopologicalBoundary - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalBoundary,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ResolvedTopologicalBoundary",
					"The geometry of a topological *boundary* feature resolved to a geological time.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalBoundary* instances.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ResolvedTopologicalBoundary`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The returned feature is what was used to generate this :class:`ResolvedTopologicalBoundary` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the topological boundary property associated with "
				"this :class:`ResolvedTopologicalBoundary`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`get_resolved_boundary` and "
				":meth:`get_resolved_geometry` are obtained from.\n")
		.def("get_resolved_boundary",
				&GPlatesApi::resolved_topological_boundary_get_resolved_boundary,
				"get_resolved_boundary()\n"
				"  Returns the resolved boundary geometry.\n"
				"\n"
				"  :rtype: :class:`PolygonOnSphere`\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_boundary_get_resolved_boundary,
				"get_resolved_geometry()\n"
				"  Same as :meth:`get_resolved_boundary`.\n")
		.def("get_resolved_feature",
				&GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>::get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved boundary geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the static :meth:`resolved geometry<get_resolved_geometry>`. "
				"Unlike :meth:`get_feature` it cannot be used to generate a :class:`ResolvedTopologicalBoundary` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_feature`\n")
		// This overload of 'get_resolved_feature' works with
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>.
		// This happens when a GPlatesAppLogic::ResolvedTopologicalBoundary is passed to Python as a
		// GPlatesAppLogic::ReconstructionGeometry. Coming from Python, boost-python knows the
		// GPlatesAppLogic::ReconstructionGeometry is a GPlatesAppLogic::ResolvedTopologicalBoundary
		// (which is how it gets here) but cannot convert from a
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> to a
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>
		// so we have to do that ourselves...
		.def("get_resolved_feature",
				&GPlatesApi::resolved_topological_boundary_get_resolved_feature,
				"\n") // A non-empty string otherwise boost-python will double docstring of other overload.
		.def("get_boundary_sub_segments",
				&GPlatesApi::resolved_topological_boundary_get_boundary_sub_segments,
				"get_boundary_sub_segments()\n"
				"  Returns the :class:`sub-segments<ResolvedTopologicalSubSegment>` that make up the "
				"boundary of this resolved topological boundary.\n"
				"\n"
				"  :rtype: list of :class:`ResolvedTopologicalSubSegment`\n"
				"\n"
				"  To get a list of the *unreversed* boundary sub-segment geometries:\n"
				"  ::\n"
				"\n"
				"    sub_segment_geometries = []\n"
				"    for sub_segment in resolved_topological_boundary.get_boundary_sub_segments():\n"
				"        sub_segment_geometries.append(sub_segment.get_resolved_geometry())\n")
		.def("get_geometry_sub_segments",
				&GPlatesApi::resolved_topological_boundary_get_boundary_sub_segments,
				"get_geometry_sub_segments()\n"
				"  Same as :meth:`get_boundary_sub_segments`.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ReconstructionGeometryTypeWrapper<> to be converted to
	// a GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstruction_geometry_type_conversion<GPlatesAppLogic::ResolvedTopologicalBoundary>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalBoundary
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class ReconstructionGeometry.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesAppLogic::ResolvedTopologicalBoundary,
			GPlatesAppLogic::ReconstructionGeometry>();
}


namespace GPlatesApi
{
	/**
	 * Returns the resolved boundary of this network.
	 */
	GPlatesAppLogic::ResolvedTopologicalNetwork::boundary_polygon_ptr_type
	resolved_topological_network_get_resolved_boundary(
			const GPlatesAppLogic::ResolvedTopologicalNetwork &resolved_topological_network)
	{
		return resolved_topological_network.boundary_polygon();
	}

	bp::list
	resolved_topological_network_get_boundary_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalNetwork &resolved_topological_network)
	{
		bp::list boundary_sub_segments_list;

		const GPlatesAppLogic::sub_segment_seq_type &sub_segments =
				resolved_topological_network.get_boundary_sub_segment_sequence();
		BOOST_FOREACH(const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment, sub_segments)
		{
			boundary_sub_segments_list.append(sub_segment);
		}

		return boundary_sub_segments_list;
	}

	// The derived reconstruction geometry type wrapper might be the wrong type.
	// It normally should be the right type though so we don't document that Py_None could be returned to the caller.
	bp::object
	resolved_topological_network_get_resolved_feature(
			const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &reconstruction_geometry)
	{
		// Extract the 'GPlatesAppLogic::ResolvedTopologicalNetwork' wrapper.
		boost::optional<const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork> &> wrapper =
				reconstruction_geometry.get_reconstruction_geometry_type_wrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>();
		if (!wrapper)
		{
			return bp::object()/*Py_None*/;
		}

		return wrapper->get_resolved_feature();
	}

	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_topological_network) :
		d_resolved_topological_network(
				// Boost-python currently does not compile when wrapping *const* objects
				// (eg, 'ReconstructionGeometry::non_null_ptr_to_const_type') - see:
				//   https://svn.boost.org/trac/boost/ticket/857
				//   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
				//
				// ...so the current solution is to wrap *non-const* objects (to keep boost-python happy)
				// and cast away const (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalNetwork>(resolved_topological_network)),
		d_keep_feature_property_alive(*resolved_topological_network)
	{
		const GPlatesAppLogic::sub_segment_seq_type &boundary_sub_segments =
				resolved_topological_network->get_boundary_sub_segment_sequence();

		d_boundary_sub_segments.reserve(boundary_sub_segments.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &boundary_sub_segment,
				boundary_sub_segments)
		{
			d_boundary_sub_segments.push_back(
					ResolvedTopologicalGeometrySubSegmentWrapper(boundary_sub_segment));
		}
	}

	bp::object
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> feature =
					reconstruction_geometry_get_feature(*d_resolved_topological_network);
			if (!feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_geometry =
					d_resolved_topological_network->boundary_polygon();

			// Clone the feature.
			bp::object resolved_feature_object = bp::object(feature.get()).attr("clone")();

			// Set the resolved boundary geometry.
			resolved_feature_object.attr("set_geometry")(resolved_geometry);

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}
}


void
export_resolved_topological_network()
{
	//
	// ResolvedTopologicalNetwork - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalNetwork,
			GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>,
			bp::bases<GPlatesAppLogic::ReconstructionGeometry>,
			boost::noncopyable>(
					"ResolvedTopologicalNetwork",
					"The geometry of a topological *network* feature resolved to a geological time.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalNetwork* instances.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstruction_geometry_get_feature,
				"get_feature()\n"
				"  Returns the feature associated with this :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The returned feature is what was used to generate this :class:`ResolvedTopologicalNetwork` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("get_property",
				&GPlatesApi::reconstruction_geometry_get_property,
				"get_property()\n"
				"  Returns the feature property containing the topological network property associated with "
				"this :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`get_resolved_boundary` is obtained from.\n")
		.def("get_resolved_boundary",
				&GPlatesApi::resolved_topological_network_get_resolved_boundary,
				"get_resolved_boundary()\n"
				"  Returns the resolved boundary of this network.\n"
				"\n"
				"  :rtype: :class:`PolygonOnSphere`\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_network_get_resolved_boundary,
				"get_resolved_geometry()\n"
				"  Same as :meth:`get_resolved_boundary`.\n")
		.def("get_resolved_feature",
				&GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>::get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved boundary geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the static :meth:`resolved geometry<get_resolved_geometry>`. "
				"Unlike :meth:`get_feature` it cannot be used to generate a :class:`ResolvedTopologicalNetwork` "
				"via :func:`resolve_topologies`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_feature`\n")
		// This overload of 'get_resolved_feature' works with
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>.
		// This happens when a GPlatesAppLogic::ResolvedTopologicalNetwork is passed to Python as a
		// GPlatesAppLogic::ReconstructionGeometry. Coming from Python, boost-python knows the
		// GPlatesAppLogic::ReconstructionGeometry is a GPlatesAppLogic::ResolvedTopologicalNetwork
		// (which is how it gets here) but cannot convert from a
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> to a
		// GPlatesApi::ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>
		// so we have to do that ourselves...
		.def("get_resolved_feature",
				&GPlatesApi::resolved_topological_network_get_resolved_feature,
				"\n") // A non-empty string otherwise boost-python will double docstring of other overload.
		.def("get_boundary_sub_segments",
				&GPlatesApi::resolved_topological_network_get_boundary_sub_segments,
				"get_boundary_sub_segments()\n"
				"  Returns the :class:`sub-segments<ResolvedTopologicalSubSegment>` that make up the "
				"boundary of this resolved topological network.\n"
				"\n"
				"  :rtype: list of :class:`ResolvedTopologicalSubSegment`\n"
				"\n"
				"  To get a list of the *unreversed* boundary sub-segment geometries:\n"
				"  ::\n"
				"\n"
				"    sub_segment_geometries = []\n"
				"    for sub_segment in resolved_topological_network.get_boundary_sub_segments():\n"
				"        sub_segment_geometries.append(sub_segment.get_resolved_geometry())\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ReconstructionGeometryTypeWrapper<> to be converted to
	// a GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstruction_geometry_type_conversion<GPlatesAppLogic::ResolvedTopologicalNetwork>();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalNetwork
	// (not ReconstructionGeometryTypeWrapper<>).
	//

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class ReconstructionGeometry.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesAppLogic::ResolvedTopologicalNetwork,
			GPlatesAppLogic::ReconstructionGeometry>();
}


namespace GPlatesApi
{
	/**
	 * Returns the referenced feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned
	 * to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	resolved_topological_geometry_sub_segment_get_topological_section_feature(
			const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &resolved_topological_geometry_sub_segment)
	{
		return reconstruction_geometry_get_feature(
				*resolved_topological_geometry_sub_segment.get_reconstruction_geometry());
	}

	// Convert sub-segment geometries to polylines (even if they are just single points - just duplicate).
	// This will make it easier for the user who will expect sub-segments to be polylines.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	resolved_topological_geometry_sub_segment_get_resolved_geometry(
			const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &resolved_topological_geometry_sub_segment)
	{
		return convert_geometry_to_polyline(
				resolved_topological_geometry_sub_segment.get_geometry());
	}

	// The topological section might not be a reconstructed feature geometry or a resolved topological *line*.
	// It should normally be one though so we don't document that Py_None could be returned to the caller.
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	resolved_topological_geometry_sub_segment_get_topological_section_geometry(
			const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &resolved_topological_geometry_sub_segment)
	{
		return get_topological_section_geometry(
				resolved_topological_geometry_sub_segment.get_reconstruction_geometry());
	}

	bp::object
	ResolvedTopologicalGeometrySubSegmentWrapper::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> topological_section_feature =
					resolved_topological_geometry_sub_segment_get_topological_section_feature(d_resolved_topological_geometry_sub_segment);
			if (!topological_section_feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type resolved_sub_segment_geometry =
					resolved_topological_geometry_sub_segment_get_resolved_geometry(d_resolved_topological_geometry_sub_segment);

			// Clone the topological section feature.
			bp::object resolved_feature_object = bp::object(topological_section_feature.get()).attr("clone")();

			// Set the resolved sub-segment geometry.
			resolved_feature_object.attr("set_geometry")(resolved_sub_segment_geometry);

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}


	/**
	 * Python converter from a 'ResolvedTopologicalGeometrySubSegment' to a
	 * 'ResolvedTopologicalGeometrySubSegmentWrapper' (and vice versa).
	 */
	struct python_ResolvedTopologicalGeometrySubSegment :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment)
			{
				// Convert to ResolvedTopologicalGeometrySubSegmentWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(
						ResolvedTopologicalGeometrySubSegmentWrapper(sub_segment)).ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			// 'ResolvedTopologicalGeometrySubSegment' is obtained from a
			// ResolvedTopologicalGeometrySubSegmentWrapper (which in turn is already convertible).
			return bp::extract<const ResolvedTopologicalGeometrySubSegmentWrapper &>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				bp::converter::rvalue_from_python_stage1_data *data)
		{
			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment> *>(
									data)->storage.bytes;

			new (storage) GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment(
					bp::extract<const ResolvedTopologicalGeometrySubSegmentWrapper &>(obj)()
							.get_resolved_topological_geometry_sub_segment());

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a 'ResolvedTopologicalGeometrySubSegment' to a
	 * 'ResolvedTopologicalGeometrySubSegmentWrapper' (and vice versa).
	 */
	void
	register_resolved_topological_geometry_sub_segment_conversion()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment,
				python_ResolvedTopologicalGeometrySubSegment::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_ResolvedTopologicalGeometrySubSegment::convertible,
				&python_ResolvedTopologicalGeometrySubSegment::construct,
				bp::type_id<GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment>());
	}
}


void
export_resolved_topological_sub_segment()
{
	//
	// ResolvedTopologicalSubSegment - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment,
			GPlatesApi::ResolvedTopologicalGeometrySubSegmentWrapper,
			// NOTE: We must specify 'boost::noncopyable' otherwise boost-python will expect our wrapper
			// to have a constructor accepting a 'pointer to non-const' and our wrapper will be
			// responsible for deleting the pointer when it's done...
			boost::noncopyable>(
					"ResolvedTopologicalSubSegment",
					"The subset of vertices of a reconstructed topological section that contribute to the "
					"geometry of a resolved topology.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate resolved topologies "
					"(such as :class:`ResolvedTopologicalLine`, :class:`ResolvedTopologicalBoundary` and "
					":class:`ResolvedTopologicalNetwork`) which, in turn, reference these "
					"*ResolvedTopologicalSubSegment* instances.\n"
					"\n"
					".. note:: | Each *ResolvedTopologicalSubSegment* instance belongs to a *single* resolved topology.\n"
					"          | In contrast, a :class:`ResolvedTopologicalSharedSubSegment` instance can be shared "
					"by one or more resolved topologies.\n",
					// Don't allow creation from python side...
					bp::no_init)
		.def("get_resolved_feature",
				&GPlatesApi::ResolvedTopologicalGeometrySubSegmentWrapper::get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved sub-segment geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the :meth:`resolved geometry<get_resolved_geometry>`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because the topological section might be a :class:`ResolvedTopologicalLine` "
				"which is a topology and topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section_feature`\n")
		// To be deprecated in favour of 'get_topological_section_feature()'...
		.def("get_feature",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_topological_section_feature,
				"get_feature()\n"
				"  To be **deprecated** - use :meth:`get_topological_section_feature` instead.\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_resolved_geometry,
				"get_resolved_geometry()\n"
				"  Returns the geometry containing the sub-segment vertices.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: These are the *unreversed* vertices. They are in the same order as the "
				"geometry of :meth:`get_topological_section_geometry`.\n"
				"\n"
				"  .. seealso:: :meth:`was_geometry_reversed_in_topology`\n")
		// To be deprecated in favour of 'get_resolved_geometry()'...
		.def("get_geometry",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_resolved_geometry,
				"get_geometry()\n"
				"  To be **deprecated** - use :meth:`get_resolved_geometry` instead.\n")
		.def("get_topological_section",
				&GPlatesApi::ResolvedTopologicalGeometrySubSegmentWrapper::get_reconstruction_geometry,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_topological_section()\n"
				"  Returns the topological section that the sub-segment was obtained from.\n"
				"\n"
				"  :rtype: :class:`ReconstructionGeometry`\n"
				"\n"
				"  .. note:: This represents the **entire** geometry of the topological section, not just "
				"the part that contributes to the sub-segment.\n"
				"\n"
				"  .. note:: | If the resolved topology (that this sub-segment is a part of) is a "
				":class:`ResolvedTopologicalLine` then the topological section will be a "
				":class:`ReconstructedFeatureGeometry`.\n"
				"            | If the resolved topology (that this sub-segment is a part of) is a "
				":class:`ResolvedTopologicalBoundary` or a :class:`ResolvedTopologicalNetwork` then "
				"the topological section can be either a :class:`ReconstructedFeatureGeometry` or "
				"a :class:`ResolvedTopologicalLine`.\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section_feature`\n")
		.def("get_topological_section_geometry",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_topological_section_geometry,
				"get_topological_section_geometry()\n"
				"  Returns the topological section *geometry* that the sub-segment was obtained from.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: This is the **entire** geometry of the topological section, not just "
				"the part that contributes to the sub-segment.\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section`\n")
		.def("get_topological_section_feature",
				&GPlatesApi::resolved_topological_geometry_sub_segment_get_topological_section_feature,
				"get_topological_section_feature()\n"
				"  Returns the feature referenced by the topological section.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The geometry in the returned feature represents the **entire** "
				"geometry of the topological section, not just the "
				"part that contributes to the sub-segment.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		.def("was_geometry_reversed_in_topology",
				&GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment::get_use_reverse,
				"was_geometry_reversed_in_topology()\n"
				"  Whether a copy of the points in :meth:`get_resolved_geometry` were reversed in order to "
				"contribute to the resolved topology that this sub-segment is a part of.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  .. note:: A reversed version of the points of :meth:`get_resolved_geometry` is equivalent "
				"``sub_segment.get_resolved_geometry().get_points()[::-1]``.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_geometry`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ResolvedTopologicalGeometrySubSegmentWrapper to be converted to
	// a GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment (and vice versa).
	GPlatesApi::register_resolved_topological_geometry_sub_segment_conversion();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment
	// (not ResolvedTopologicalGeometrySubSegmentWrapper).
	//

	// Enable boost::optional<ResolvedTopologicalGeometrySubSegment> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment>();
}


namespace GPlatesApi
{
	/**
	 * Returns the referenced topological section feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	resolved_topological_shared_sub_segment_get_topological_section_feature(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment)
	{
		return reconstruction_geometry_get_feature(
				*resolved_topological_shared_sub_segment.get_reconstruction_geometry());
	}

	// Convert sub-segment geometries to polylines (even if they are just single points - just duplicate).
	// This will make it easier for the user who will expect sub-segments to be polylines.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	resolved_topological_shared_sub_segment_get_resolved_geometry(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment)
	{
		return convert_geometry_to_polyline(
				resolved_topological_shared_sub_segment.get_geometry());
	}

	// The topological section might not be a reconstructed feature geometry or a resolved topological *line*.
	// It should normally be one though so we don't document that Py_None could be returned to the caller.
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	resolved_topological_shared_sub_segment_get_topological_section_geometry(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment)
	{
		return get_topological_section_geometry(
				resolved_topological_shared_sub_segment.get_reconstruction_geometry());
	}

	/**
	 * Get the geometry reversal flags of the resolved topologies sharing the sub-segment (for passing to Python).
	 */
	bp::list
	resolved_topological_shared_sub_segment_get_sharing_resolved_topology_geometry_reversal_flags(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment)
	{
		bp::list geometry_reversal_flags_list;

		const std::vector<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> &
				sharing_resolved_topologies =
						resolved_topological_shared_sub_segment.get_sharing_resolved_topologies();

		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo &sharing_resolved_topology,
				sharing_resolved_topologies)
		{
			geometry_reversal_flags_list.append(
					sharing_resolved_topology.is_sub_segment_geometry_reversed);
		}

		return geometry_reversal_flags_list;
	}


	ResolvedTopologicalSharedSubSegmentWrapper::ResolvedTopologicalSharedSubSegmentWrapper(
			const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment) :
		d_resolved_topological_shared_sub_segment(resolved_topological_shared_sub_segment),
		d_reconstruction_geometry(resolved_topological_shared_sub_segment.get_reconstruction_geometry())
	{
		const std::vector<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo> &
				sharing_resolved_topologies =
						resolved_topological_shared_sub_segment.get_sharing_resolved_topologies();

		// Wrap the resolved topologies to keep their feature/property alive.
		d_sharing_resolved_topologies.reserve(sharing_resolved_topologies.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo &resolved_topology_info,
				sharing_resolved_topologies)
		{
			d_sharing_resolved_topologies.push_back(
					ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>(
							resolved_topology_info.resolved_topology));
		}
	}


	bp::object
	ResolvedTopologicalSharedSubSegmentWrapper::get_resolved_feature() const
	{
		if (!d_resolved_feature_object)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> topological_section_feature =
					resolved_topological_shared_sub_segment_get_topological_section_feature(d_resolved_topological_shared_sub_segment);
			if (!topological_section_feature)
			{
				return bp::object()/*Py_None*/;
			}

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type resolved_sub_segment_geometry =
					resolved_topological_shared_sub_segment_get_resolved_geometry(d_resolved_topological_shared_sub_segment);

			// Clone the topological section feature.
			bp::object resolved_feature_object = bp::object(topological_section_feature.get()).attr("clone")();

			// Set the resolved sub-segment geometry.
			resolved_feature_object.attr("set_geometry")(resolved_sub_segment_geometry);

			// No exceptions have been thrown so record our new resolved feature.
			d_resolved_feature_object = resolved_feature_object;
		}

		return d_resolved_feature_object;
	}


	bp::list
	ResolvedTopologicalSharedSubSegmentWrapper::get_sharing_resolved_topologies() const
	{
		bp::list sharing_resolved_topologies_list;

		BOOST_FOREACH(
				const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &resolved_topology,
				d_sharing_resolved_topologies)
		{
			sharing_resolved_topologies_list.append(resolved_topology);
		}

		return sharing_resolved_topologies_list;
	}


	/**
	 * Python converter from a 'ResolvedTopologicalSharedSubSegment' to a
	 * 'ResolvedTopologicalSharedSubSegmentWrapper' (and vice versa).
	 */
	struct python_ResolvedTopologicalSharedSubSegment :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &sub_segment)
			{
				// Convert to ResolvedTopologicalSharedSubSegmentWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(
						ResolvedTopologicalSharedSubSegmentWrapper(sub_segment)).ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			// 'ResolvedTopologicalSharedSubSegment' is obtained from a
			// ResolvedTopologicalSharedSubSegmentWrapper (which in turn is already convertible).
			return bp::extract<const ResolvedTopologicalSharedSubSegmentWrapper &>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				bp::converter::rvalue_from_python_stage1_data *data)
		{
			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							GPlatesAppLogic::ResolvedTopologicalSharedSubSegment> *>(
									data)->storage.bytes;

			new (storage) GPlatesAppLogic::ResolvedTopologicalSharedSubSegment(
					bp::extract<const ResolvedTopologicalSharedSubSegmentWrapper &>(obj)()
							.get_resolved_topological_shared_sub_segment());

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a 'ResolvedTopologicalSharedSubSegment' to a
	 * 'ResolvedTopologicalSharedSubSegmentWrapper' (and vice versa).
	 */
	void
	register_resolved_topological_shared_sub_segment_conversion()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ResolvedTopologicalSharedSubSegment,
				python_ResolvedTopologicalSharedSubSegment::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_ResolvedTopologicalSharedSubSegment::convertible,
				&python_ResolvedTopologicalSharedSubSegment::construct,
				bp::type_id<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment>());
	}
}


void
export_resolved_topological_shared_sub_segment()
{
	//
	// ResolvedTopologicalSharedSubSegment - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalSharedSubSegment,
			GPlatesApi::ResolvedTopologicalSharedSubSegmentWrapper,
			// NOTE: We must specify 'boost::noncopyable' otherwise boost-python will expect our wrapper
			// to have a constructor accepting a 'pointer to non-const' and our wrapper will be
			// responsible for deleting the pointer when it's done...
			boost::noncopyable>(
					"ResolvedTopologicalSharedSubSegment",
					"The shared subset of vertices of a reconstructed topological section that uniquely "
					"contribute to the *boundaries* of one or more resolved topologies.\n"
					"\n"
					".. note:: | Only :class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork` "
					"have *boundaries* and hence will share *ResolvedTopologicalSharedSubSegment* instances.\n"
					"          | :class:`ResolvedTopologicalLine` is excluded since it does not have a *boundary*.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate resolved topology *boundaries* "
					"(:class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`) and "
					"the shared *ResolvedTopologicalSharedSubSegment* instances.\n"
					"\n"
					".. note:: | Each *ResolvedTopologicalSharedSubSegment* instance can be shared "
					"by one or more resolved topologies.\n"
					"          | In contrast, a :class:`ResolvedTopologicalSubSegment` instance "
					"belongs to a *single* resolved topology.\n",
					// Don't allow creation from python side...
					bp::no_init)
		.def("get_resolved_feature",
				&GPlatesApi::ResolvedTopologicalSharedSubSegmentWrapper::get_resolved_feature,
				"get_resolved_feature()\n"
				"  Returns a feature containing the resolved sub-segment geometry.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  The returned feature contains the :meth:`resolved geometry<get_resolved_geometry>`.\n"
				"\n"
				"  .. note:: | The returned feature does **not** contain present-day geometry as is typical "
				"of most GPlates features.\n"
				"            | In this way the returned feature is similar to a GPlates reconstruction export.\n"
				"\n"
				"  .. note:: The returned feature should not be :func:`reverse reconstructed<reverse_reconstruct>` "
				"to present day because the topological section might be a :class:`ResolvedTopologicalLine` "
				"which is a topology and topologies are resolved (not reconstructed).\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section_feature`\n")
		.def("get_resolved_geometry",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_resolved_geometry,
				"get_resolved_geometry()\n"
				"  Returns the geometry containing the shared sub-segment vertices.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: These are the *unreversed* vertices. They are in the same order as the "
				"geometry of :meth:`get_topological_section_geometry`.\n")
		// To be deprecated in favour of 'get_resolved_geometry()'...
		.def("get_geometry",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_resolved_geometry,
				"get_geometry()\n"
				"  To be **deprecated** - use :meth:`get_resolved_geometry` instead.\n")
		.def("get_topological_section",
				&GPlatesApi::ResolvedTopologicalSharedSubSegmentWrapper::get_reconstruction_geometry,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_topological_section()\n"
				"  Returns the topological section that the shared sub-segment was obtained from.\n"
				"\n"
				"  :rtype: :class:`ReconstructionGeometry`\n"
				"\n"
				"  .. note:: This represents the **entire** geometry of the topological section, not just "
				"the part that contributes to the shared sub-segment.\n"
				"\n"
				"  .. note:: | This topological section can be either a "
				":class:`ReconstructedFeatureGeometry` or a :class:`ResolvedTopologicalLine`.\n"
				"            | The resolved topologies that share the topological section can be "
				":class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  .. seealso:: :class:`get_topological_section_feature`\n")
		.def("get_topological_section_geometry",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_topological_section_geometry,
				"get_topological_section_geometry()\n"
				"  Returns the topological section *geometry* that the shared sub-segment was obtained from.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: This is the **entire** geometry of the topological section, not just "
				"the part that contributes to the shared sub-segment.\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section`\n")
		.def("get_topological_section_feature",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_topological_section_feature,
				"get_topological_section_feature()\n"
				"  Returns the feature referenced by the topological section.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The geometry in the returned feature represents the **entire** "
				"geometry of the topological section, not just the "
				"part that contributes to the shared sub-segment.\n"
				"\n"
				"  .. seealso:: :meth:`get_resolved_feature`\n")
		// To be deprecated in favour of 'get_topological_section_feature()'...
		.def("get_feature",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_topological_section_feature,
				"get_feature()\n"
				"  To be **deprecated** - use :meth:`get_topological_section_feature` instead.\n")
		.def("get_sharing_resolved_topologies",
				&GPlatesApi::ResolvedTopologicalSharedSubSegmentWrapper::get_sharing_resolved_topologies,
				"get_sharing_resolved_topologies()\n"
				"  Returns a list of resolved topologies sharing this sub-segment.\n"
				"\n"
				"  :rtype: list of :class:`ReconstructionGeometry`\n"
				"\n"
				"  .. note:: | The resolved topologies (that share this sub-segment) can be "
				":class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`.\n"
				"            | :class:`ResolvedTopologicalLine` is excluded since it does not have a *boundary*.\n"
				"\n"
				"  .. seealso:: :meth:`get_sharing_resolved_topology_geometry_reversal_flags`\n")
		.def("get_sharing_resolved_topology_geometry_reversal_flags",
				&GPlatesApi::resolved_topological_shared_sub_segment_get_sharing_resolved_topology_geometry_reversal_flags,
				"get_sharing_resolved_topology_geometry_reversal_flags()\n"
				"  Returns a list of flags indicating whether a copy of the sub-segment geometry was reversed "
				"when contributing to each resolved topology sharing this sub-segment.\n"
				"\n"
				"  :rtype: list of bool\n"
				"\n"
				"  .. seealso:: :meth:`ResolvedTopologicalSubSegment.was_geometry_reversed_in_topology`\n"
				"  .. note::\n"
				"     The returned list is in the same order (and has the same number of elements) as the "
				"list of sharing resolved topologies returned in :meth:`get_sharing_resolved_topologies`.\n"
				"\n"
				"     ::\n"
				"\n"
				"       sharing_resolved_topologies = shared_sub_segment.get_sharing_resolved_topologies()\n"
				"       geometry_reversal_flags = shared_sub_segment.get_sharing_resolved_topology_geometry_reversal_flags()\n"
				"       for index in range(len(sharing_resolved_topologies)):\n"
				"           sharing_resolved_topology = sharing_resolved_topologies[index]\n"
				"           geometry_reversal_flag = geometry_reversal_flags[index]\n"
				"\n"
				"  .. seealso:: :meth:`get_sharing_resolved_topologies`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ResolvedTopologicalSharedSubSegmentWrapper to be converted to
	// a GPlatesAppLogic::ResolvedTopologicalSharedSubSegment (and vice versa).
	GPlatesApi::register_resolved_topological_shared_sub_segment_conversion();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalSharedSubSegment
	// (not ResolvedTopologicalSharedSubSegmentWrapper).
	//

	// Enable boost::optional<ResolvedTopologicalSharedSubSegment> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ResolvedTopologicalSharedSubSegment>();
}


namespace GPlatesApi
{
	/**
	 * Returns the referenced feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned
	 * to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	resolved_topological_section_get_topological_section_feature(
			const GPlatesAppLogic::ResolvedTopologicalSection &resolved_topological_section)
	{
		return reconstruction_geometry_get_feature(
				*resolved_topological_section.get_reconstruction_geometry());
	}

	// The topological section might not be a reconstructed feature geometry or a resolved topological *line*.
	// It should normally be one though so we don't document that Py_None could be returned to the caller.
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	resolved_topological_section_get_topological_section_geometry(
			const GPlatesAppLogic::ResolvedTopologicalSection &resolved_topological_section)
	{
		return get_topological_section_geometry(
				resolved_topological_section.get_reconstruction_geometry());
	}


	ResolvedTopologicalSectionWrapper::ResolvedTopologicalSectionWrapper(
			const GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_to_const_type &resolved_topological_section) :
		d_resolved_topological_section(
				// Boost-python currently does not compile when wrapping *const* objects
				// (eg, 'ReconstructionGeometry::non_null_ptr_to_const_type') - see:
				//   https://svn.boost.org/trac/boost/ticket/857
				//   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
				//
				// ...so the current solution is to wrap *non-const* objects (to keep boost-python happy)
				// and cast away const (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalSection>(resolved_topological_section)),
		d_reconstruction_geometry(resolved_topological_section->get_reconstruction_geometry())
	{
		const std::vector<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment> &shared_sub_segments =
				resolved_topological_section->get_shared_sub_segments();

		// Wrap the resolved topologies to keep their feature/property alive.
		d_shared_sub_segments.reserve(shared_sub_segments.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &shared_sub_segment,
				shared_sub_segments)
		{
			d_shared_sub_segments.push_back(
					ResolvedTopologicalSharedSubSegmentWrapper(shared_sub_segment));
		}
	}

	
	bp::list
	ResolvedTopologicalSectionWrapper::get_shared_sub_segments() const
	{
		bp::list shared_sub_segments_list;

		BOOST_FOREACH(
				const ResolvedTopologicalSharedSubSegmentWrapper &shared_sub_segment,
				d_shared_sub_segments)
		{
			shared_sub_segments_list.append(shared_sub_segment);
		}

		return shared_sub_segments_list;
	}


	/**
	 * Python converter from a 'ResolvedTopologicalSection' to a
	 * 'ResolvedTopologicalSectionWrapper' (and vice versa).
	 */
	struct python_ResolvedTopologicalSection :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type &section)
			{
				// Convert to ResolvedTopologicalSectionWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(
						ResolvedTopologicalSectionWrapper(section)).ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			// 'ResolvedTopologicalSection' is obtained from a
			// ResolvedTopologicalSectionWrapper (which in turn is already convertible).
			return bp::extract<const ResolvedTopologicalSectionWrapper &>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				bp::converter::rvalue_from_python_stage1_data *data)
		{
			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> *>(
									data)->storage.bytes;

			new (storage) GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type(
					bp::extract<const ResolvedTopologicalSectionWrapper &>(obj)()
							.get_resolved_topological_section());

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a 'ResolvedTopologicalSection' to a
	 * 'ResolvedTopologicalSectionWrapper' (and vice versa).
	 */
	void
	register_resolved_topological_section_conversion()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type,
				python_ResolvedTopologicalSection::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_ResolvedTopologicalSection::convertible,
				&python_ResolvedTopologicalSection::construct,
				bp::type_id<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>());
	}
}


void
export_resolved_topological_section()
{
	//
	// ResolvedTopologicalSection - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ResolvedTopologicalSection,
			GPlatesApi::ResolvedTopologicalSectionWrapper,
			boost::noncopyable>(
					"ResolvedTopologicalSection",
					"The sequence of shared sub-segments of a reconstructed topological section that uniquely "
					"contribute to the *boundaries* of one or more resolved topologies.\n"
					"\n"
					".. note:: | Only :class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork` "
					"have *boundaries* and hence will share sub-segments.\n"
					"          | :class:`ResolvedTopologicalLine` is excluded since it does not have a *boundary*.\n"
					"\n"
					"The :func:`resolve_topologies` function can be used to generate resolved topology *boundaries* "
					"(:class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`) and "
					"*ResolvedTopologicalSection* instances.\n",
					// Don't allow creation from python side...
					bp::no_init)
		.def("get_topological_section_feature",
				&GPlatesApi::resolved_topological_section_get_topological_section_feature,
				"get_topological_section_feature()\n"
				"  Returns the feature referenced by the topological section.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  .. note:: The geometry in the returned feature represents the **entire** geometry of the "
				"topological section, not just the parts that contribute to resolved topological boundaries.\n")
		// To be deprecated in favour of 'get_topological_section_feature()'...
		.def("get_feature",
				&GPlatesApi::resolved_topological_section_get_topological_section_feature,
				"get_feature()\n"
				"  To be **deprecated** - use :meth:`get_topological_section_feature` instead.\n")
		.def("get_topological_section",
				&GPlatesApi::ResolvedTopologicalSectionWrapper::get_reconstruction_geometry,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_topological_section()\n"
				"  Returns the topological section.\n"
				"\n"
				"  :rtype: :class:`ReconstructionGeometry`\n"
				"\n"
				"  .. note:: This represents the **entire** geometry of the topological section, not just "
				"the parts that contribute to resolved topological boundaries.\n"
				"\n"
				"  .. note:: | This topological section can be either a "
				":class:`ReconstructedFeatureGeometry` or a :class:`ResolvedTopologicalLine`.\n"
				"            | The resolved topologies that share the topological section can be "
				":class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  .. seealso:: :class:`get_topological_section_feature`\n")
		.def("get_topological_section_geometry",
				&GPlatesApi::resolved_topological_section_get_topological_section_geometry,
				"get_topological_section_geometry()\n"
				"  Returns the topological section *geometry*.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n"
				"\n"
				"  .. note:: This is the **entire** geometry of the topological section, not just "
				"the parts that contribute to resolved topological boundaries.\n"
				"\n"
				"  .. seealso:: :meth:`get_topological_section`\n")
		.def("get_shared_sub_segments",
				&GPlatesApi::ResolvedTopologicalSectionWrapper::get_shared_sub_segments,
				"get_shared_sub_segments()\n"
				"  Returns a list of sub-segments on this topological section that are *shared* by "
				"one or more resolved topologies.\n"
				"\n"
				"  :rtype: list of :class:`ResolvedTopologicalSharedSubSegment`\n"
				"\n"
				"  Get the length of the shared sub-segments of a resolved topological section:\n"
				"  ::\n"
				"\n"
				"    length = 0\n"
				"    for shared_sub_segment in resolved_topological_section.get_shared_sub_segments():\n"
				"        length += shared_sub_segment.get_resolved_geometry().get_arc_length()\n"
				"\n"
				"    length_in_kms = length * pygplates.Earth.mean_radius_in_kms\n"
				"\n"
				"  .. note:: | The returned objects are :class:`ResolvedTopologicalSharedSubSegment` instances, "
				"**not** :class:`ResolvedTopologicalSubSegment` instances.\n"
				"            | :class:`ResolvedTopologicalSubSegment` instances are *not* shared by  one or "
				"more resolved topologies - each instance is associated with a *single* resolved topology.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ResolvedTopologicalSectionWrapper to be converted to
	// a GPlatesAppLogic::ResolvedTopologicalSection (and vice versa).
	GPlatesApi::register_resolved_topological_section_conversion();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ResolvedTopologicalSection
	// (not ResolvedTopologicalSectionWrapper).
	//

	// Enable boost::optional<ResolvedTopologicalSection> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	bp::implicitly_convertible<
			GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type,
			GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_to_const_type>();
	bp::implicitly_convertible<
			boost::optional<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>,
			boost::optional<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_to_const_type> >();
}


void
export_reconstruction_geometries()
{
	export_reconstruction_geometry();

	export_reconstructed_feature_geometry();
	export_reconstructed_motion_path();
	export_reconstructed_flowline();

	export_resolved_topological_sub_segment();
	export_resolved_topological_shared_sub_segment();
	export_resolved_topological_section();

	export_resolved_topological_line();
	export_resolved_topological_boundary();
	export_resolved_topological_network();
}

#endif // GPLATES_NO_PYTHON
