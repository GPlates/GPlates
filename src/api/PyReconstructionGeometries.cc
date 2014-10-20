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

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"

#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * A wrapper around GPlatesAppLogic::ReconstructedFeatureGeometry that keeps the referenced feature alive.
	 *
	 * Keeping the referenced feature alive (and the referenced property alive in case subsequently removed
	 * from feature) is important because the unwrapped GPlatesAppLogic::ReconstructedFeatureGeometry
	 * stores only weak references which will be invalid if the referenced features are no longer
	 * used (kept alive) in the user's python code.
	 *
	 * This is the type that gets stored in the python object.
	 */
	class ReconstructedFeatureGeometryWrapper
	{
	public:

		explicit
		ReconstructedFeatureGeometryWrapper(
				const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_feature_geometry) :
			d_reconstructed_feature_geometry(reconstructed_feature_geometry)
		{
			// The feature reference could be invalid. It should normally be valid though.
			GPlatesModel::FeatureHandle::weak_ref feature_ref = reconstructed_feature_geometry->get_feature_ref();
			if (feature_ref.is_valid())
			{
				d_feature = GPlatesModel::FeatureHandle::non_null_ptr_type(feature_ref.handle_ptr());
			}

			// The property iterator could be invalid. It should normally be valid though.
			GPlatesModel::FeatureHandle::iterator property_iter = reconstructed_feature_geometry->property();
			if (property_iter.is_still_valid())
			{
				d_property = *property_iter;
			}
		}

		/**
		 * Returns the wrapped GPlatesAppLogic::ReconstructedFeatureGeometry.
		 */
		GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type
		get_reconstructed_feature_geometry() const
		{
			return d_reconstructed_feature_geometry;
		}

		/**
		 * Returns the reconstructed geometry.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_reconstructed_geometry() const
		{
			return d_reconstructed_feature_geometry->reconstructed_geometry();
		}

		/**
		 * Returns the present day geometry.
		 *
		 * boost::none could be returned but it normally shouldn't so we don't document that Py_None
		 * could be returned to the caller.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_present_day_geometry() const
		{
			if (!d_property)
			{
				return boost::none;
			}

			// Call python since Property.get_value is implemented in python code...
			bp::object property_value_object = bp::object(d_property.get()).attr("get_value")();
			if (property_value_object == bp::object()/*Py_None*/)
			{
				return boost::none;
			}

			// Get the property value.
			GPlatesModel::PropertyValue::non_null_ptr_type property_value =
					bp::extract<GPlatesModel::PropertyValue::non_null_ptr_type>(
							property_value_object);

			// Extract the geometry from the property value.
			return GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(*property_value);
		}

		/**
		 * Returns the referenced feature.
		 *
		 * The feature reference could be invalid.
		 * It should normally be valid though so we don't document that Py_None could be returned
		 * to the caller.
		 */
		boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
		get_feature() const
		{
			return d_feature;
		}

		/**
		 * Returns the referenced feature property.
		 *
		 * The feature property reference could be invalid.
		 * It should normally be valid though so we don't document that Py_None could be returned
		 * to the caller.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
		get_property() const
		{
			return d_property;
		}

	private:

		//! The wrapped RFG itself.
		GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type d_reconstructed_feature_geometry;

		/**
		 * Keep the feature alive (by using intrusive pointer instead of weak ref)
		 * since GPlatesAppLogic::ReconstructedFeatureGeometry only stores weak reference.
		 */
		boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> d_feature;

		/**
		 * Keep the geometry feature property alive (by using intrusive pointer instead of weak ref)
		 * since GPlatesAppLogic::ReconstructedFeatureGeometry only stores an iterator and
		 * someone could remove the feature property in the meantime.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> d_property;

	};


	/**
	 * Python converter from a GPlatesAppLogic::ReconstructedFeatureGeometry to a
	 * ReconstructedFeatureGeometryWrapper (and vice versa).
	 */
	struct python_ReconstructedFeatureGeometry :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &rfg)
			{
				namespace bp = boost::python;

				// Convert to ReconstructedFeatureGeometryWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(ReconstructedFeatureGeometryWrapper(rfg)).ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// GPlatesAppLogic::ReconstructedFeatureGeometry is obtained from a
			// ReconstructedFeatureGeometryWrapper (which in turn is already convertible).
			return bp::extract<ReconstructedFeatureGeometryWrapper>(obj).check() ? obj : NULL;
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
							GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> *>(
									data)->storage.bytes;

			new (storage) GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type(
					bp::extract<ReconstructedFeatureGeometryWrapper>(obj)()
							.get_reconstructed_feature_geometry());

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a GPlatesAppLogic::ReconstructedFeatureGeometry to a
	 * ReconstructedFeatureGeometryWrapper (and vice versa).
	 */
	void
	register_reconstructed_feature_geometry_conversion()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type,
				python_ReconstructedFeatureGeometry::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_ReconstructedFeatureGeometry::convertible,
				&python_ReconstructedFeatureGeometry::construct,
				bp::type_id<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>());
	}
}


void
export_reconstructed_feature_geometry()
{
	//
	// ReconstructedFeatureGeometry - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ReconstructedFeatureGeometryWrapper>(
					"ReconstructedFeatureGeometry",
					"The geometry of a feature reconstructed to a geological time.\n"
					"\n"
					"Note that a single feature can have multiple geometry properties, and hence multiple "
					"reconstructed feature geometries, associated with it. "
					"Therefore each :class:`ReconstructedFeatureGeometry` references a different property of "
					"the feature via :meth:`get_property`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::ReconstructedFeatureGeometryWrapper::get_feature,
				"get_feature() -> Feature\n"
				"  Returns the feature associated with this :class:`ReconstructedFeatureGeometry`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  Note that multiple :class:`reconstructed feature geometries<ReconstructedFeatureGeometry>` can "
				"be associated with the same :class:`feature<Feature>` if that feature has multiple geometry properties.\n")
		.def("get_property",
				&GPlatesApi::ReconstructedFeatureGeometryWrapper::get_property,
				"get_property() -> Property\n"
				"  Returns the feature property containing the present day (unreconstructed) geometry "
				"associated with this :class:`ReconstructedFeatureGeometry`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day geometry<get_present_day_geometry>` "
				"and the :meth:`reconstructed geometry<get_reconstructed_geometry>` are obtained from.\n")
		.def("get_present_day_geometry",
				&GPlatesApi::ReconstructedFeatureGeometryWrapper::get_present_day_geometry,
				"get_present_day_geometry() -> GeometryOnSphere\n"
				"  Returns the present day geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
		.def("get_reconstructed_geometry",
				&GPlatesApi::ReconstructedFeatureGeometryWrapper::get_reconstructed_geometry,
				"get_reconstructed_geometry() -> GeometryOnSphere\n"
				"  Returns the reconstructed geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Enable python-wrapped ReconstructedFeatureGeometryWrapper to be converted to
	// a GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstructed_feature_geometry_conversion();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedFeatureGeometry
	// (not ReconstructedFeatureGeometryWrapper).
	//

	// Enable boost::optional<ReconstructedFeatureGeometry::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type,
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>,
			boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type> >();
}


namespace GPlatesApi
{
	/**
	 * A wrapper around GPlatesAppLogic::ReconstructedMotionPath that keeps the referenced feature alive.
	 *
	 * Keeping the referenced feature alive (and the referenced property alive in case subsequently removed
	 * from feature) is important because the unwrapped GPlatesAppLogic::ReconstructedMotionPath
	 * stores only weak references which will be invalid if the referenced features are no longer
	 * used (kept alive) in the user's python code.
	 *
	 * This is the type that gets stored in the python object.
	 */
	class ReconstructedMotionPathWrapper
	{
	public:

		explicit
		ReconstructedMotionPathWrapper(
				const GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type &reconstructed_motion_path) :
			d_reconstructed_motion_path(reconstructed_motion_path)
		{
			// The feature reference could be invalid. It should normally be valid though.
			GPlatesModel::FeatureHandle::weak_ref feature_ref = reconstructed_motion_path->get_feature_ref();
			if (feature_ref.is_valid())
			{
				d_feature = GPlatesModel::FeatureHandle::non_null_ptr_type(feature_ref.handle_ptr());
			}

			// The property iterator could be invalid. It should normally be valid though.
			GPlatesModel::FeatureHandle::iterator property_iter = reconstructed_motion_path->property();
			if (property_iter.is_still_valid())
			{
				d_property = *property_iter;
			}
		}

		/**
		 * Returns the wrapped GPlatesAppLogic::ReconstructedMotionPath.
		 */
		GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type
		get_reconstructed_motion_path() const
		{
			return d_reconstructed_motion_path;
		}

		/**
		 * Returns the motion path points.
		 */
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		get_motion_path() const
		{
			return d_reconstructed_motion_path->motion_path_points();
		}

		/**
		 * Returns the reconstructed seed point.
		 */
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		get_reconstructed_seed_point() const
		{
			return d_reconstructed_motion_path->reconstructed_seed_point();
		}

		/**
		 * Returns the present day seed point.
		 */
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		get_present_day_seed_point() const
		{
			return d_reconstructed_motion_path->present_day_seed_point();
		}

		/**
		 * Returns the referenced feature.
		 *
		 * The feature reference could be invalid.
		 * It should normally be valid though so we don't document that Py_None could be returned
		 * to the caller.
		 */
		boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
		get_feature() const
		{
			return d_feature;
		}

		/**
		 * Returns the referenced feature property.
		 *
		 * The feature property reference could be invalid.
		 * It should normally be valid though so we don't document that Py_None could be returned
		 * to the caller.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
		get_property() const
		{
			return d_property;
		}

	private:

		//! The wrapped reconstructed motion path itself.
		GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type d_reconstructed_motion_path;

		/**
		 * Keep the feature alive (by using intrusive pointer instead of weak ref)
		 * since GPlatesAppLogic::ReconstructedMotionPath only stores weak reference.
		 */
		boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> d_feature;

		/**
		 * Keep the geometry feature property alive (by using intrusive pointer instead of weak ref)
		 * since GPlatesAppLogic::ReconstructedMotionPath only stores an iterator and
		 * someone could remove the feature property in the meantime.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> d_property;

	};


	/**
	 * Python converter from a GPlatesAppLogic::ReconstructedMotionPath to a
	 * ReconstructedMotionPathWrapper (and vice versa).
	 */
	struct python_ReconstructedMotionPath :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type &rfg)
			{
				namespace bp = boost::python;

				// Convert to ReconstructedMotionPathWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(ReconstructedMotionPathWrapper(rfg)).ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// GPlatesAppLogic::ReconstructedMotionPath is obtained from a
			// ReconstructedMotionPathWrapper (which in turn is already convertible).
			return bp::extract<ReconstructedMotionPathWrapper>(obj).check() ? obj : NULL;
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
							GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type> *>(
									data)->storage.bytes;

			new (storage) GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type(
					bp::extract<ReconstructedMotionPathWrapper>(obj)()
							.get_reconstructed_motion_path());

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a GPlatesAppLogic::ReconstructedMotionPath to a
	 * ReconstructedMotionPathWrapper (and vice versa).
	 */
	void
	register_reconstructed_motion_path()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type,
				python_ReconstructedMotionPath::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_ReconstructedMotionPath::convertible,
				&python_ReconstructedMotionPath::construct,
				bp::type_id<GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type>());
	}
}


void
export_reconstructed_motion_path()
{
	//
	// ReconstructedMotionPath - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ReconstructedMotionPathWrapper>(
					"ReconstructedMotionPath",
					"The reconstructed history of a plate's motion in the form of a path of points "
					"over geological time.\n"
					"\n"
					"Note that, although a single motion path :class:`feature<Feature>` has only a single "
					"seed geometry, that seed geometry can be either a :class:`PointOnSphere` or a "
					":class:`MultiPointOnSphere`. And since there is one "
					":class:`reconstructed motion path<ReconstructedMotionPath>` per seed point there can be, "
					"in the case of a :class:`MultiPointOnSphere`, multiple "
					":class:`reconstructed motion paths<ReconstructedMotionPath>` per motion path "
					":class:`feature<Feature>`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::ReconstructedMotionPathWrapper::get_feature,
				"get_feature() -> Feature\n"
				"  Returns the feature associated with this :class:`ReconstructedMotionPath`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  Note that multiple :class:`reconstructed motion paths<ReconstructedMotionPath>` "
				"can be associated with the same motion path :class:`feature<Feature>` if its seed geometry "
				"is a :class:`MultiPointOnSphere`.\n")
		.def("get_property",
				&GPlatesApi::ReconstructedMotionPathWrapper::get_property,
				"get_property() -> Property\n"
				"  Returns the feature property containing the seed point associated with this "
				":class:`ReconstructedMotionPath`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day seed point<get_present_day_seed_point>` "
				"and the :meth:`reconstructed seed point<get_reconstructed_seed_point>` are obtained from.\n")
		.def("get_present_day_seed_point",
				&GPlatesApi::ReconstructedMotionPathWrapper::get_present_day_seed_point,
				"get_present_day_seed_point() -> PointOnSphere\n"
				"  Returns the present day seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  Note that this is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedMotionPath` instances.\n")
		.def("get_reconstructed_seed_point",
				&GPlatesApi::ReconstructedMotionPathWrapper::get_reconstructed_seed_point,
				"get_reconstructed_seed_point() -> PointOnSphere\n"
				"  Returns the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  Note that this is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedMotionPath` instances.\n")
		.def("get_motion_path",
				&GPlatesApi::ReconstructedMotionPathWrapper::get_motion_path,
				"get_motion_path() -> PolylineOnSphere\n"
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
				"  Note that this is just one of the motion paths associated with this "
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

	// Enable python-wrapped ReconstructedMotionPathWrapper to be converted to
	// a GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstructed_motion_path();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedMotionPath
	// (not ReconstructedMotionPathWrapper).
	//

	// Enable boost::optional<ReconstructedMotionPath::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type,
			GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type>,
			boost::optional<GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_to_const_type> >();
}


namespace GPlatesApi
{
	/**
	 * A wrapper around GPlatesAppLogic::ReconstructedFlowline that keeps the referenced feature alive.
	 *
	 * Keeping the referenced feature alive (and the referenced property alive in case subsequently removed
	 * from feature) is important because the unwrapped GPlatesAppLogic::ReconstructedFlowline
	 * stores only weak references which will be invalid if the referenced features are no longer
	 * used (kept alive) in the user's python code.
	 *
	 * This is the type that gets stored in the python object.
	 */
	class ReconstructedFlowlineWrapper
	{
	public:

		explicit
		ReconstructedFlowlineWrapper(
				const GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type &reconstructed_flowline) :
			d_reconstructed_flowline(reconstructed_flowline)
		{
			// The feature reference could be invalid. It should normally be valid though.
			GPlatesModel::FeatureHandle::weak_ref feature_ref = reconstructed_flowline->get_feature_ref();
			if (feature_ref.is_valid())
			{
				d_feature = GPlatesModel::FeatureHandle::non_null_ptr_type(feature_ref.handle_ptr());
			}

			// The property iterator could be invalid. It should normally be valid though.
			GPlatesModel::FeatureHandle::iterator property_iter = reconstructed_flowline->property();
			if (property_iter.is_still_valid())
			{
				d_property = *property_iter;
			}
		}

		/**
		 * Returns the wrapped GPlatesAppLogic::ReconstructedFlowline.
		 */
		GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type
		get_reconstructed_flowline() const
		{
			return d_reconstructed_flowline;
		}

		/**
		 * Returns the left flowline points.
		 */
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		get_left_flowline() const
		{
			return d_reconstructed_flowline->left_flowline_points();
		}

		/**
		 * Returns the right flowline points.
		 */
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		get_right_flowline() const
		{
			return d_reconstructed_flowline->right_flowline_points();
		}

		/**
		 * Returns the reconstructed seed point.
		 */
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		get_reconstructed_seed_point() const
		{
			return d_reconstructed_flowline->reconstructed_seed_point();
		}

		/**
		 * Returns the present day seed point.
		 */
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		get_present_day_seed_point() const
		{
			return d_reconstructed_flowline->present_day_seed_point();
		}

		/**
		 * Returns the referenced feature.
		 *
		 * The feature reference could be invalid.
		 * It should normally be valid though so we don't document that Py_None could be returned
		 * to the caller.
		 */
		boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
		get_feature() const
		{
			return d_feature;
		}

		/**
		 * Returns the referenced feature property.
		 *
		 * The feature property reference could be invalid.
		 * It should normally be valid though so we don't document that Py_None could be returned
		 * to the caller.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
		get_property() const
		{
			return d_property;
		}

	private:

		//! The wrapped reconstructed flowline itself.
		GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type d_reconstructed_flowline;

		/**
		 * Keep the feature alive (by using intrusive pointer instead of weak ref)
		 * since GPlatesAppLogic::ReconstructedFlowline only stores weak reference.
		 */
		boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> d_feature;

		/**
		 * Keep the geometry feature property alive (by using intrusive pointer instead of weak ref)
		 * since GPlatesAppLogic::ReconstructedFlowline only stores an iterator and
		 * someone could remove the feature property in the meantime.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> d_property;

	};


	/**
	 * Python converter from a GPlatesAppLogic::ReconstructedFlowline to a
	 * ReconstructedFlowlineWrapper (and vice versa).
	 */
	struct python_ReconstructedFlowline :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type &rfg)
			{
				namespace bp = boost::python;

				// Convert to ReconstructedFlowlineWrapper first.
				// Then it'll get converted to python.
				return bp::incref(bp::object(ReconstructedFlowlineWrapper(rfg)).ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// GPlatesAppLogic::ReconstructedFlowline is obtained from a
			// ReconstructedFlowlineWrapper (which in turn is already convertible).
			return bp::extract<ReconstructedFlowlineWrapper>(obj).check() ? obj : NULL;
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
							GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type> *>(
									data)->storage.bytes;

			new (storage) GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type(
					bp::extract<ReconstructedFlowlineWrapper>(obj)()
							.get_reconstructed_flowline());

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a GPlatesAppLogic::ReconstructedFlowline to a
	 * ReconstructedFlowlineWrapper (and vice versa).
	 */
	void
	register_reconstructed_flowline_conversion()
	{
		// To python conversion.
		bp::to_python_converter<
				GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type,
				python_ReconstructedFlowline::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_ReconstructedFlowline::convertible,
				&python_ReconstructedFlowline::construct,
				bp::type_id<GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type>());
	}
}


void
export_reconstructed_flowline()
{
	//
	// ReconstructedFlowline - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ReconstructedFlowlineWrapper>(
					"ReconstructedFlowline",
					"The reconstructed history of plate motion away from a spreading ridge in the form of "
					"a path of points over geological time.\n"
					"\n"
					"Note that, although a single flowline :class:`feature<Feature>` has only a single "
					"seed geometry, that seed geometry can be either a :class:`PointOnSphere` or a "
					":class:`MultiPointOnSphere`. And since there is one "
					":class:`reconstructed flowline<ReconstructedFlowline>` per seed point there can be, "
					"in the case of a :class:`MultiPointOnSphere`, multiple "
					":class:`reconstructed flowlines<ReconstructedFlowline>` per flowline "
					":class:`feature<Feature>`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::ReconstructedFlowlineWrapper::get_feature,
				"get_feature() -> Feature\n"
				"  Returns the feature associated with this :class:`ReconstructedFlowline`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  Note that multiple :class:`reconstructed flowlines<ReconstructedFlowline>` "
				"can be associated with the same flowline :class:`feature<Feature>` if its seed geometry "
				"is a :class:`MultiPointOnSphere`.\n")
		.def("get_property",
				&GPlatesApi::ReconstructedFlowlineWrapper::get_property,
				"get_property() -> Property\n"
				"  Returns the feature property containing the seed point associated with this "
				":class:`ReconstructedFlowline`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This is the :class:`Property` that the :meth:`present day seed point<get_present_day_seed_point>` "
				"and the :meth:`reconstructed seed point<get_reconstructed_seed_point>` are obtained from.\n")
		.def("get_present_day_seed_point",
				&GPlatesApi::ReconstructedFlowlineWrapper::get_present_day_seed_point,
				"get_present_day_seed_point() -> PointOnSphere\n"
				"  Returns the present day seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  Note that this is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedFlowline` instances.\n")
		.def("get_reconstructed_seed_point",
				&GPlatesApi::ReconstructedFlowlineWrapper::get_reconstructed_seed_point,
				"get_reconstructed_seed_point() -> PointOnSphere\n"
				"  Returns the reconstructed seed point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  Note that this is just one of the seed points in this :meth:`feature's<get_feature>` "
				"seed geometry if that seed geometry is a :class:`MultiPointOnSphere`. The remaining "
				"seed points are associated with other :class:`ReconstructedFlowline` instances.\n")
		.def("get_left_flowline",
				&GPlatesApi::ReconstructedFlowlineWrapper::get_left_flowline,
				"get_left_flowline() -> PolylineOnSphere\n"
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
				"  Note that this is just one of the *left* flowlines associated with this "
				":meth:`feature's<get_feature>` seed geometry if that seed geometry is a "
				":class:`MultiPointOnSphere`.\n"
				"\n"
				"  Iterate over the left flowline points:\n"
				"  ::\n"
				"\n"
				"    for left_point in reconstructed_flowline.get_left_flowline():\n"
				"      ...\n")
		.def("get_right_flowline",
				&GPlatesApi::ReconstructedFlowlineWrapper::get_right_flowline,
				"get_right_flowline() -> PolylineOnSphere\n"
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
				"  Note that this is just one of the *right* flowlines associated with this "
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

	// Enable python-wrapped ReconstructedFlowlineWrapper to be converted to
	// a GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type (and vice versa).
	GPlatesApi::register_reconstructed_flowline_conversion();

	//
	// Now for the conversions that only involve GPlatesAppLogic::ReconstructedFlowline
	// (not ReconstructedFlowlineWrapper).
	//

	// Enable boost::optional<ReconstructedFlowline::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type,
			GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type>,
			boost::optional<GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_to_const_type> >();
}


void
export_reconstruction_geometries()
{
	export_reconstructed_feature_geometry();
	export_reconstructed_motion_path();
	export_reconstructed_flowline();
}

#endif // GPLATES_NO_PYTHON
