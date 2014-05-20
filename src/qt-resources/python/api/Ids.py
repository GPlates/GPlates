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
