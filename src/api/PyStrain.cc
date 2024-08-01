/**
 * Copyright (C) 2024 The University of Sydney, Australia
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

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "app-logic/DeformationStrainRate.h"

#include "global/python.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	// Zero strain rate.
	const GPlatesAppLogic::DeformationStrainRate zero_strain_rate;

	bp::tuple
	strain_rate_get_rate_of_deformation(
			const GPlatesAppLogic::DeformationStrainRate &strain_rate)
	{
		const GPlatesAppLogic::DeformationStrainRate::RateOfDeformation &rate_of_deformation =
				strain_rate.get_rate_of_deformation();

		return bp::make_tuple(
				rate_of_deformation.theta_theta,
				rate_of_deformation.theta_phi,
				rate_of_deformation.phi_theta,
				rate_of_deformation.phi_phi);
	}

	bp::tuple
	strain_rate_get_velocity_spatial_gradient(
			const GPlatesAppLogic::DeformationStrainRate &strain_rate)
	{
		const GPlatesAppLogic::DeformationStrainRate::VelocitySpatialGradient &velocity_spatial_gradient =
				strain_rate.get_velocity_spatial_gradient();

		return bp::make_tuple(
				velocity_spatial_gradient.theta_theta,
				velocity_spatial_gradient.theta_phi,
				velocity_spatial_gradient.phi_theta,
				velocity_spatial_gradient.phi_phi);
	}
}


void
export_strain()
{
	//
	// StrainRate - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesAppLogic::DeformationStrainRate>(
					"StrainRate",
					"Represents the surface (2D) strain rate (in units of :math:`second^{-1}`). Strain rates are equality (``==``, ``!=``) comparable "
					"(but not hashable - cannot be used as a key in a ``dict``).\n"
					"\n"
					"References:\n"
					"\n"
					"- Malvern, L. E. (1969). `Introduction to the mechanics of a continuous medium. <http://books.google.com/books?id=IIMpAQAAMAAJ>`_ Prentice-Hall.\n"
					"- Mase, G.T., Smelser, R.E., & Mase, G.E. (2010). `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_ (3rd ed.). CRC Press.\n"
					"\n"
					"Convenience class static data is available for the zero strain rate:\n"
					"\n"
					"* ``pygplates.StrainRate.zero``\n"
					"\n"
					"A *StrainRate* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.46\n",
					bp::init<>(
							"__init__()\n"
							"  Construct a zero strain rate (non-deforming).\n"
							"\n"
							"  ::\n"
							"\n"
							"    zero_strain_rate = pygplates.StrainRate()\n"
							"\n"
							"  .. note:: Alternatively you can use ``zero_strain_rate = pygplates.StrainRate.zero``.\n"))
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<boost::shared_ptr<GPlatesAppLogic::DeformationStrainRate>>())
		// Static property 'pygplates.StrainRate.zero'...
		.def_readonly("zero", GPlatesApi::zero_strain_rate)
		.def("get_dilatation_rate",
				&GPlatesAppLogic::DeformationStrainRate::get_strain_rate_dilatation,
				"get_dilatation_rate()\n"
				"  Return the dilatation rate (in units of :math:`second^{-1}`).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The dilatation rate is the rate of increase (if positive) or decrease (if negative) of crustal area per unit area at the current location "
				"(at which this strain rate was calculated).\n"
				"\n"
				"  It is defined as the `trace` of the :meth:`rate-of-deformation tensor<get_rate_of_deformation>` (sum of its diagonal elements). "
				"So if we define :math:`A` as the area of a parcel of crust at the current location, then the dilatation rate is the "
				"*rate of change of area per unit area*, and is given by:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{\\dot{A}}{A} = trace(\\boldsymbol D)\n"
				"\n"
				"  .. seealso::\n"
				"\n"
				"     Chapter 4.11 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_ for a derivation of the material derivative of volume. "
				"We ignore the radial dimension, hence volume becomes area.\n"
				"\n"
				"  .. note:: The dilatation rate is *invariant* with respect to the local coordinate axes (South and East) since :math:`trace(\\boldsymbol D)` is "
				"the *first invariant* of :math:`\\boldsymbol D` (see Chapter 3.6 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_).\n"
				"\n"
				"  .. note:: As shown for the :meth:`rate-of-deformation tensor<get_rate_of_deformation>` :math:`\\boldsymbol D`, the *rate of stretching per unit stretch* "
				"along the local coordinate South and East axes are the *diagonal* elements :math:`D_{\\theta\\theta}` and :math:`D_{\\phi\\phi}` "
				"(which are included in :math:`trace(\\boldsymbol D)`). And the *off-diagonal* elements determine the instantaneous shear rate which does not affect "
				"expansion/contraction (and is excluded from :math:`trace(\\boldsymbol D))`.\n")
		.def("get_total_strain_rate",
				&GPlatesAppLogic::DeformationStrainRate::get_strain_rate_second_invariant,
				"get_total_strain_rate()\n"
				"  Return the total strain rate (in units of :math:`second^{-1}`).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The total strain rate represents the magnitude, including both the normal (extension/compression) and shear components, of strain rate.\n"
				"\n"
				"  It is defined in terms of the :meth:`rate-of-deformation symmetric tensor<get_rate_of_deformation>` :math:`\\boldsymbol D` as:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\sqrt{trace(\\boldsymbol{D}^2)} = \\sqrt{D_{\\theta\\theta}^2 + D_{\\phi\\phi}^2 + 2 D_{\\phi\\theta}^2}\n"
				"\n"
				"  ...where :math:`D_{\\phi\\theta} = D_{\\theta\\phi}` since :math:`\\boldsymbol D` is symmetric\n"
				"\n"
				"  .. note:: The total strain rate is *invariant* with respect to the local coordinate axes (South and East) since :math:`\\sqrt{trace(\\boldsymbol{D}^2)}` "
				"is invariant. This is because it is a function of the *second invariant*: :math:`\\frac{1}{2} \\left[trace(\\boldsymbol{D})^2 - trace(\\boldsymbol{D}^2)\\right]` "
				"and the *first invariant*: :math:`trace(\\boldsymbol D)` (see Chapter 3.6 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_).\n")
		.def("get_strain_rate_style",
				&GPlatesAppLogic::DeformationStrainRate::get_strain_rate_style,
				"get_strain_rate_style()\n"
				"  Return a measure categorising the type of deformation (in units of :math:`second^{-1}`).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The strain rate *style* is a way to represent the type of deformation. The approach used here is from "
				"*Kreemer, C., G. Blewitt, and E. C. Klein (2014),* `A geodetic plate motion and Global Strain Rate Model <https://10.1002/2014GC005407>`_, "
				"which defines *strain rate style* as:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{\\dot{\\varepsilon}_{(1)} + \\dot{\\varepsilon}_{(2)}}{max(\\lvert \\dot{\\varepsilon}_{(1)} \\rvert, \\lvert \\dot{\\varepsilon}_{(2)} \\rvert)}\n"
				"\n"
				"  ...and is equivalent to:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{D_{(1)} + D_{(2)}}{max(\\lvert D_{(1)} \\rvert, \\lvert D_{(2)} \\rvert)}\n"
				"\n"
				"  ..where :math:`D_{(1)}` and :math:`D_{(2)}` are the principal values of the :meth:`rate-of-deformation tensor<get_rate_of_deformation>` :math:`\\boldsymbol D`.\n"
				"\n"
				"  A value of `-1` represents contraction (eg, pure reverse faulting), `0` represents pure strike-slip faulting and `1` represents extension (eg, pure normal faulting).\n"
				"\n"
				"  .. warning:: | If the :meth:`rate-of-deformation tensor<get_rate_of_deformation>` is zero (ie, no deformation) then `NaN` (zero divided by zero) will be returned.\n"
				"               | Also, the returned value is **not** clamped to the range `[-1, 1]` when *both* principal values are non-zero and have the same sign.\n"
				"\n"
				"  .. note:: The strain rate *style* is *invariant* with respect to the local coordinate axes (South and East) since it uses the principal values of :math:`\\boldsymbol D`.\n"
				"\n"
				"  .. note:: A similar measure for *style* is to divide :meth:`dilatation rate<get_dilatation_rate>` by :meth:`total strain rate<get_total_strain_rate>`. This is "
				"equivalent to :math:`\\frac{trace(\\boldsymbol D)}{\\sqrt{trace(\\boldsymbol{D}^2)}}` which is also *invariant* with respect to the local coordinate axes (South and East).\n")
		.def("get_rate_of_deformation",
				&GPlatesApi::strain_rate_get_rate_of_deformation,
				"get_rate_of_deformation()\n"
				"  Return the rate-of-deformation symmetric tensor :math:`\\boldsymbol D` (in units of :math:`second^{-1}`) in spherical polar coordinates (ignoring radial dimension).\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol D = \\begin{bmatrix} D_{\\theta\\theta} & D_{\\theta\\phi} \\\\ D_{\\phi\\theta} & D_{\\phi\\phi} \\end{bmatrix}\n"
				"\n"
				"  :returns: the tuple of :math:`(D_{\\theta\\theta}, D_{\\theta\\phi}, D_{\\phi\\theta}, D_{\\phi\\phi})`\n"
				"  :rtype: tuple (float, float, float, float)\n"
				"\n"
				"  .. note:: :math:`\\theta` is **co**-latitude and hence increases from North to South (and :math:`\\phi` increases from West to East, as expected).\n"
				"\n"
				"  The rate-of-deformation tensor (:math:`\\boldsymbol D`) is related to the spatial gradients of velocity tensor (:math:`\\boldsymbol L`):\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol D = \\frac{\\boldsymbol L + \\boldsymbol{L}^T}{2}\n"
				"\n"
				"  ...and therefore :math:`D_{\\theta\\phi} = D_{\\phi\\theta}` (since :math:`\\boldsymbol D` is symmetric).\n"
				"\n"
				"  If :math:`\\Lambda` is the stretch ratio along current direction :math:`\\hat{\\boldsymbol n}` then the "
				"*rate of stretching per unit stretch* is given by:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\frac{\\dot{\\Lambda}}{\\Lambda} = \\hat{\\boldsymbol n} \\cdot \\boldsymbol D \\cdot \\hat{\\boldsymbol n}\n"
				"\n"
				"  ...where :math:`\\hat{\\boldsymbol n}` is a 2D unit vector in the local South-East (:math:`\\theta, \\phi`) coordinate system "
				"at the current location (at which this strain rate was calculated).\n"
				"\n"
				"  So that means, in the local South direction (ie, :math:`\\hat{\\boldsymbol n} = (1,0)`) the *rate of stretching per unit stretch* is :math:`D_{\\theta\\theta}`, and "
				" in the local East direction (ie, :math:`\\hat{\\boldsymbol n} = (0,1)`) it is :math:`D_{\\phi\\phi}`. These are the *diagonal* elements of :math:`\\boldsymbol D`.\n"
				"\n"
				"  The *rate of change of angle* :math:`\\alpha` between two current directions :math:`\\hat{\\boldsymbol n_1}` and :math:`\\hat{\\boldsymbol n_2}` is given by:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     -\\dot{\\alpha} = \\hat{\\boldsymbol n_1} \\cdot 2 \\boldsymbol D \\cdot \\hat{\\boldsymbol n_2}\n"
				"\n"
				"  The *shear rate* is commonly defined as half the *rate of change of angle* between two directions that are currently *perpendicular*. "
				"And if those directions are aligned with the local coordinate system (ie, :math:`\\hat{\\boldsymbol n_1} = (1,0)` and :math:`\\hat{\\boldsymbol n_2} = (0,1)`) then "
				"the *shear rate* is :math:`D_{\\phi\\theta}` (which is the same as :math:`D_{\\theta\\phi}` since :math:`\\boldsymbol D` is symmetric). "
				"So the symmetric *off-diagonal* elements represent the shear rate between the local coordinate axes (between South and East).\n"
				"\n"
				"  .. seealso::\n"
				"\n"
				"     Chapter 4.10 in `Continuum Mechanics for Engineers <https://doi.org/10.1201/9781420085396>`_ for a derivation of these equations.\n")
		.def("get_velocity_spatial_gradient",
				&GPlatesApi::strain_rate_get_velocity_spatial_gradient,
				"get_velocity_spatial_gradient()\n"
				"  Return the spatial gradients of velocity tensor :math:`\\boldsymbol L` (in units of :math:`second^{-1}`) in spherical polar coordinates (ignoring radial dimension).\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol L = \\begin{bmatrix} L_{\\theta\\theta} & L_{\\theta\\phi} \\\\ L_{\\phi\\theta} & L_{\\phi\\phi} \\end{bmatrix}\n"
				"\n"
				"  :returns: the tuple of :math:`(L_{\\theta\\theta}, L_{\\theta\\phi}, L_{\\phi\\theta}, L_{\\phi\\phi})`\n"
				"  :rtype: tuple (float, float, float, float)\n"
				"\n"
				"  .. note:: :math:`\\theta` is **co**-latitude and hence increases from North to South (and :math:`\\phi` increases from West to East, as expected).\n"
				"\n"
				"  The spatial gradients of velocity tensor (:math:`\\boldsymbol L`) can be decomposed into the rate-of-deformation tensor (:math:`\\boldsymbol D`) "
				"and the vorticity (or spin) tensor (:math:`\\boldsymbol W`):\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol L &= \\boldsymbol D + \\boldsymbol W\\\\\n"
				"     \\boldsymbol D &= \\frac{\\boldsymbol L + \\boldsymbol{L}^T}{2}\\\\\n"
				"     \\boldsymbol W &= \\frac{\\boldsymbol L - \\boldsymbol{L}^T}{2}\n")
		// Comparisons...
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Enable boost::optional<StrainRate> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::DeformationStrainRate>();
}
