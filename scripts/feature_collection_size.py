class FeatureCollectionSizeUtility:
	def __init__(self):
		self.category = "Examples"
		self.name = "Count Features in Loaded Files"

	def __call__(self):
		loaded_files = pygplates.instance.get_loaded_files()
		print "There are %d loaded files" % len(loaded_files)
		for f in loaded_files:
			print "%s %d" % (f, pygplates.instance.get_feature_collection_from_loaded_file(f).size)

# This needs to be run on the main thread because it causes an item to be added to the menus.
pygplates.instance.exec_string_on_main_thread("pygplates.instance.register_utility(FeatureCollectionSizeUtility())")

