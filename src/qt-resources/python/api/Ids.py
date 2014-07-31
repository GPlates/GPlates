# Copyright (C) 2014 The University of Sydney, Australia
# 
# This file is part of GPlates.
# 
# GPlates is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation.
# 
# GPlates is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


# Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
# This is because the default '__hash__'is based on 'id()' which is not compatible and
# would cause errors when used as key in a dictionary.
# In python 3 fixes this by automatically making unhashable if define '__eq__' only.
def id_hash(id):
    # Hash the internal string.
    return hash(id.get_string())

# Add the module function as a class method.
FeatureId.__hash__ = id_hash

# Not including RevisionId yet since it is not really needed in the python API user (we can add it later though)...
## Add the module function as a class method.
#RevisionId.__hash__ = id_hash

# Delete the module reference to the function - we only keep the class methods.
del id_hash
