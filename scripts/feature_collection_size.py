import pygplates

class FeatureCollectionSizeUtility:
	def __init__(self):
		self.category = "Examples"
		self.name = "Count Features in Loaded Files"

	def __call__(self):
		loaded_files = pygplates.Application().get_loaded_files()
		print "There are %d loaded files" % len(loaded_files)
		for f in loaded_files:
			print "%s %d" % (f, pygplates.Application().get_feature_collection_from_loaded_file(f).size)

def register():
	pygplates.Application().register_utility(FeatureCollectionSizeUtility())

