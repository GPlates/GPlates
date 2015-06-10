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

#include <boost/optional.hpp>

#include "PyReconstructionGeometries.h"

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionGeometryVisitor.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalGeometrySubSegment.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTopologicalSharedSubSegment.h"

#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


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
			public GPlatesAppLogic::ReconstructionGeometryVisitor
	{
	public:
		// Bring base class visit methods into scope of current class.
		using GPlatesAppLogic::ReconstructionGeometryVisitor::visit;


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
					ReconstructionGeometryTypeWrapper<reconstructed_feature_geometry_type>(rfg));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_motion_path_type> &rmp)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<reconstructed_motion_path_type>(rmp));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_flowline_type> &rf)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<reconstructed_flowline_type>(rf));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_line_type> &rtl)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<resolved_topological_line_type>(rtl));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<resolved_topological_boundary_type>(rtb));
		}

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
		{
			d_reconstruction_geometry_type_wrapper = boost::any(
					ReconstructionGeometryTypeWrapper<resolved_topological_network_type>(rtn));
		}

	private:
		// We just need to store the wrapper - we don't need to access it.
		boost::any d_reconstruction_geometry_type_wrapper;
	};


	boost::any
	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>::create_reconstruction_geometry_type_wrapper(
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry)
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
					const typename ReconstructionGeometryType::non_null_ptr_type &rtb)
			{
				namespace bp = boost::python;

				// Convert to ReconstructionGeometryTypeWrapper<> first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(
						ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>(rtb)).ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

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
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

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
				python_ReconstructionGeometryType<ReconstructionGeometryType>::Conversion>();

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
					"The base class inherited by all derived reconstruction geometry classes..\n"
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
	boost::python::implicitly_convertible<
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type,
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
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
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalLine* instances.\n"
					"\n"
					".. note:: | Although uncommon, a single feature can have *multiple* topological geometry properties, "
					"and hence multiple resolved topological geometries, associated with it.\n"
					"          | Therefore each :class:`ResolvedTopologicalLine` references a different property of "
					"the feature via :meth:`get_property`.\n",
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
				"  .. note:: Multiple resolved topological geometries(:class:`ResolvedTopologicalLine` or "
				":class:`ResolvedTopologicalBoundary`) can be associated with the same :class:`feature<Feature>` "
				"if that feature has multiple topological geometry properties.\n")
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
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalBoundary* instances.\n"
					"\n"
					".. note:: | Although uncommon, a single feature can have *multiple* topological geometry properties, "
					"and hence multiple resolved topological geometries, associated with it.\n"
					"          | Therefore each :class:`ResolvedTopologicalBoundary` references a different property of "
					"the feature via :meth:`get_property`.\n",
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
				"  .. note:: Multiple resolved topological geometries(:class:`ResolvedTopologicalLine` or "
				":class:`ResolvedTopologicalBoundary`) can be associated with the same :class:`feature<Feature>` "
				"if that feature has multiple topological geometry properties.\n")
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
					"The :func:`resolve_topologies` function can be used to generate *ResolvedTopologicalNetwork* instances.\n"
					"\n"
					".. note:: | Although *very* uncommon, a single feature can have *multiple* topological network properties, "
					"and hence multiple resolved topological networks, associated with it.\n"
					"          | Therefore each :class:`ResolvedTopologicalNetwork` references a different property of "
					"the feature via :meth:`get_property`.\n",
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
				"  .. note:: Although *very* uncommon, multiple resolved topological networks can be associated with "
				"the same :class:`feature<Feature>` if that feature has multiple topological network properties.\n")
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


void
export_reconstruction_geometries()
{
	export_reconstruction_geometry();

	export_reconstructed_feature_geometry();
	export_reconstructed_motion_path();
	export_reconstructed_flowline();

	export_resolved_topological_line();
	export_resolved_topological_boundary();
	export_resolved_topological_network();
}

#endif // GPLATES_NO_PYTHON
