# How-to guides

Two walkthroughs for extending the GPGIM: adding a new feature type, and adding a new property
type. Between them they follow the whole path from a type's specification through to reading,
writing and displaying it, which nothing else in the repository does.

**Both are out of date, and are kept here unrevised.** They describe the tree as it stood around
2010, and were moved into a `deprecated/` directory in 2021 for that reason. Read them for the
shape of the task and the order of the steps, not for the identifiers — every class either guide
names as the place to edit has since gone:

- The property-type guide edits `PropertyCreationUtils`, `StructurePropertyCreatorMap` and
  `GpmlOnePointSixOutputVisitor`. Reading is now split between
  `file-io/GpmlStructuralTypeReaderUtils` and `file-io/GpmlPropertyStructuralTypeReaderUtils`,
  whose file headers still point back at this guide.
- The feature-type guide edits `FeaturePropertiesMap`, a hard-coded table of which properties each
  feature type may carry. Feature types are now data: `model/Gpgim` loads them at startup from the
  Qt resource `:/gpgim/gpgim.xml`, which holds a `<FeatureClass>` element per type.

They are preserved rather than deleted because the sequence they describe is still broadly right,
and rewriting them is a separate piece of work. Correct one in place when you next follow it.

- [add-support-for-a-new-feature.txt](add-support-for-a-new-feature.txt)
- [add-support-for-a-new-property-type.txt](add-support-for-a-new-property-type.txt)
