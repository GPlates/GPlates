# Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
# This is because the default '__hash__'is based on 'id()' which is not compatible and
# would cause errors when used as key in a dictionary.
# In python 3 fixes this by automatically making unhashable if define '__eq__' only.
def qualified_xml_name_hash(qualified_xml_name):
    # Hash the internal string.
    return hash(qualified_xml_name.to_qualified_string())

# Add the module function as a class method.
EnumerationType.__hash__ = qualified_xml_name_hash

# Add the module function as a class method.
FeatureType.__hash__ = qualified_xml_name_hash

# Add the module function as a class method.
PropertyName.__hash__ = qualified_xml_name_hash

# Add the module function as a class method.
StructuralType.__hash__ = qualified_xml_name_hash

# Delete the module reference to the function - we only keep the class methods.
del qualified_xml_name_hash
