<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:gpgim="http://www.gplates.org/gpgim"
	>
	<!--
		This is an XSLT stylesheet that is applied to the gpgim.xml document to transform
		it into an HTML reference document viewable in any web browser. It can be used
		with a stand-alone XSLT tool to make an HTML output file, but the recommended way
		is to simply open up the gpgim.xml file in a web browser, providing that this
		file is also available to it from the same path.

		It is the hope that having the GPGIM XML definition file viewable in a slightly
		more readable style will make the process of editing the XML more pleasant.
	-->


	<!-- To enable slightly less horribly slow lookups by id when coming up with the nested stuff -->
	<xsl:key name="object" match="gpgim:Enumeration" use="gpgim:Type" />
	<xsl:key name="object" match="gpgim:NativeProperty" use="gpgim:Type" />
	<xsl:key name="object" match="gpgim:Property" use="gpgim:Name" />
	<xsl:key name="object" match="gpgim:FeatureClass" use="gpgim:Name" />

	<xsl:key name="property" match="gpgim:Property" use="gpgim:Name" />
	<xsl:key name="feature" match="gpgim:FeatureClass" use="gpgim:Name" />


	<!-- The main GPGIM element of our XML doc gets transformed into the root of our HTML. -->
	<xsl:template match="gpgim:GPGIM">
		<html>
			<head>
				<style>	<!-- YES! XML, XSLT and CSS - together at last! MUAHAHAHAHA -->
					h1.title, h2.title { text-align: center; }
					h2.section_title { border-bottom: 3px solid black; }
					p.preamble { margin-left: 5em; margin-right: 5em; }
					ul.toc { margin: 0em; list-style-type: circle; padding-left: 1em; }
					ul.toc li { margin-top: 0.5em; }
					ul.toc li > a { font-size: larger; font-weight: bold; }
					.toc_object_list { font-size: small; }
					
					a.objlink { text-decoration: none; font-family: "DejaVu Sans Mono", "monospace"; }
					a.objlink:hover { text-decoration: underline; }
					.broken { color: #CC0000; }
					
					.object { margin: 1em; padding: 0.5em; background: #F1F1F1; border-left: solid black 3px; border-top: solid black 3px; }
					.enumeration { background: #FFF6EB; }
					.nativeproperty { background: #D1DFD1; }
					.property { background: #FFFFF1; }
					.featureclass_abstract { background: #F1F1F1; }
					.featureclass_concrete { background: #F1F1FF; }
					.object_id { padding-left: 1em; font-family: "DejaVu Sans Mono", "monospace"; }
					.header_annotation { font-size: smaller; float: right; }
					.object_content { margin: 1em; padding: 0.5em; background: #FFFFFF; }
					.object_description {  }
					
					.section_heading { font-size: smaller; font-style: italic; margin-top: 1em; color: #555588 }
					ul.enum_values { margin: 0em; list-style-type: square; }
					.enum_value_name { font-weight: bold; }
					.comment { margin-top: 0em; font-style: italic; color: #558855; }
					ul.prop_attrs { margin: 0em; list-style-type: none; padding-left: 0em; }
					.prop_attr_name { font-weight: bold; color: #555588; }
					ul.feat_attrs { margin: 0em; list-style-type: none; padding-left: 0em; }
					.feat_attr_name { font-weight: bold; color: #555588; }
					ul.feat_props { margin: 0em; list-style-type: none; padding-left: 1em; }
					.propref_multiplicity { font-family: "DejaVu Sans Mono", "monospace"; color: #555588 }
					.propref_description {  }
				</style>
			</head>
			<body>
				<h1 class="title">The GPlates Geological Information Model (GPGIM)</h1>
				<h2 class="title">Technical Reference Document</h2>
				<div class="version_info">
					GPGIM Version: <xsl:value-of select="@gpgim:version" /><br />
				</div>
				<p class="preamble">
					The document you are reading is a technical reference for how GPlates uses and stores data
					according to its information model. It is a direct transliteration of the XML file that
					GPlates uses internally to describe that information model. It is intended to be an aid
					for those editing the GPGIM XML file and anyone creating software that produces or
					consumes GPML or other GPlates-compatible data.
				</p>
				<span class="section_heading">Table of Contents:-</span>
				<ul class="toc">
					<li>
						<a href="#PropertyTypeList">Property Type List</a>
						<div class="toc_object_list">
							<xsl:for-each select="//gpgim:PropertyTypeList/*">
								<xsl:if test="position() != 1"><xsl:text>, </xsl:text></xsl:if>
								<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="gpgim:Type" /></xsl:call-template>
							</xsl:for-each>
						</div>
					</li>
					<li>
						<a href="#PropertyList">Property List</a>
						<div class="toc_object_list">
							<xsl:for-each select="//gpgim:PropertyList/*">
								<xsl:if test="position() != 1"><xsl:text>, </xsl:text></xsl:if>
								<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="gpgim:Name" /></xsl:call-template>
							</xsl:for-each>
						</div>
					</li>
					<li>
						<a href="#FeatureClassList">Feature Class List</a>
						<div class="toc_object_list">
							<xsl:for-each select="//gpgim:FeatureClassList/*">
								<xsl:if test="position() != 1"><xsl:text>, </xsl:text></xsl:if>
								<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="gpgim:Name" /></xsl:call-template>
							</xsl:for-each>
						</div>
					</li>
				</ul>
				<xsl:apply-templates />
			</body>
		</html>
	</xsl:template>


	<xsl:template match="gpgim:PropertyTypeList">
		<a name="PropertyTypeList" />
		<h2 class="section_title">Property Type List</h2>
		<p class="preamble">
			A list of all the types of property value that GPlates understands, including Enumerations
			(a set of string values defining categorical values), and Native Properties (types of complex
			or "structural" values that are built in to GPlates).
		</p>
		<xsl:apply-templates />
	</xsl:template>


	<xsl:template match="gpgim:PropertyList">
		<a name="PropertyList" />
		<h2 class="section_title">Property List</h2>
		<p class="preamble">
			The definition of every possible property that can be included by features - what the property
			name is, how often it can appear in a single feature, and what type of value is supported.
		</p>
		<xsl:apply-templates />
	</xsl:template>


	<xsl:template match="gpgim:FeatureClassList">
		<a name="FeatureClassList" />
		<h2 class="section_title">Feature Class List</h2>
		<p class="preamble">
			The definition of every possible class of Feature that can be used by GPlates - what the
			name of the feature is, and what properties it supports. To avoid redundancy in definitions,
			Features are able to 'inherit' properties from a parent Feature type, and some of the
			feature definitions are purely abstract features for the purpose of grouping commonly-used
			properties together.
		</p>
		<p class="preamble">
			Don't get too hung up on names of things. Remember that this is a technical document describing
			to GPlates how the <b>data</b> is structured. If feature type X and feature type Y have the
			exact same properties, and should be treated the exact same way by the code, then there's no
			real reason to duplicate their definitions.
		</p>
		<xsl:apply-templates />
	</xsl:template>


	<xsl:template match="gpgim:Enumeration">
		<div class="object enumeration">
			<a><xsl:attribute name="name"><xsl:value-of select="gpgim:Type" /></xsl:attribute></a>
			<h3 class="object_id"> <xsl:value-of select="gpgim:Type" /> </h3>

			<div class="object_content">
				<div class="section_heading">Description:-</div>
				<div class="object_description"> <xsl:value-of select="gpgim:Description" /> </div>

				<div class="section_heading">Possible values:-</div>
				<ul class="enum_values">
					<xsl:for-each select="gpgim:Content">
						<li>
							<span class="enum_value_name"> <xsl:value-of select="gpgim:Value" /> </span> :
							<span class="enum_value_description"> <xsl:value-of select="gpgim:Description" /> </span>
						</li>
					</xsl:for-each>
				</ul>

				<xsl:if test="gpgim:Comment">
					<div class="section_heading">Comments:-</div>
					<xsl:for-each select="gpgim:Comment">
						<p class="comment">
							<xsl:value-of select="text()" />
						</p>
					</xsl:for-each>
				</xsl:if>

				<div class="section_heading">Used by properties:-</div>
				<xsl:variable name="current_type_name"><xsl:value-of select="gpgim:Type" /></xsl:variable>
				<xsl:for-each select="//gpgim:Property[gpgim:Type=$current_type_name]">
					<xsl:if test="position() != 1"><xsl:text>, </xsl:text></xsl:if>
					<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="gpgim:Name" /></xsl:call-template>
				</xsl:for-each>

			</div>
		</div>
	</xsl:template>


	<xsl:template match="gpgim:NativeProperty">
		<div class="object nativeproperty">
			<a><xsl:attribute name="name"><xsl:value-of select="gpgim:Type" /></xsl:attribute></a>
			<h3 class="object_id">
				<xsl:value-of select="gpgim:Type" />
				<span class="header_annotation"> (Native Property Type) </span>
			</h3>

			<div class="object_content">
				<div class="section_heading">Description:-</div>
				<div class="object_description"> <xsl:value-of select="gpgim:Description" /> </div>

				<xsl:if test="gpgim:Comment">
					<div class="section_heading">Comments:-</div>
					<xsl:for-each select="gpgim:Comment">
						<p class="comment">
							<xsl:value-of select="text()" />
						</p>
					</xsl:for-each>
				</xsl:if>

				<div class="section_heading">Used by properties:-</div>
				<xsl:variable name="current_type_name"><xsl:value-of select="gpgim:Type" /></xsl:variable>
				<xsl:for-each select="//gpgim:Property[gpgim:Type=$current_type_name]">
					<xsl:if test="position() != 1"><xsl:text>, </xsl:text></xsl:if>
					<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="gpgim:Name" /></xsl:call-template>
				</xsl:for-each>

			</div>
		</div>
	</xsl:template>


	<xsl:template match="gpgim:Property">
		<div class="object property">
			<a><xsl:attribute name="name"><xsl:value-of select="gpgim:Name" /></xsl:attribute></a>
			<h3 class="object_id">
				<xsl:value-of select="gpgim:Name" />
				<xsl:if test="gpgim:UserFriendlyName">
				 - <xsl:value-of select="gpgim:UserFriendlyName" />
				</xsl:if>
			</h3>

			<div class="object_content">
				<ul class="prop_attrs">
					<xsl:if test="@gpgim:defaultType">
						<li>
							<span class="prop_attr_name">Default Value Type: </span>
							<span class="prop_attr_val">
								<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="@gpgim:defaultType" /></xsl:call-template>
							</span>
						</li>
					</xsl:if>
					<li>
						<span class="prop_attr_name">Value Type: </span>
						<span class="prop_attr_val">
							<xsl:for-each select="gpgim:Type">
								<xsl:if test="position() != 1"><xsl:text> | </xsl:text></xsl:if>
								<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="text()" /></xsl:call-template>
							</xsl:for-each>
						</span>
					</li>
					<li>
						<span class="prop_attr_name">Multiplicity: </span>
						<span class="prop_attr_val"> <xsl:value-of select="gpgim:Multiplicity" /> </span>
					</li>
					<xsl:for-each select="gpgim:TimeDependent">
						<li>
							<span class="prop_attr_name">Time Dependency: </span>
							<span class="prop_attr_val"> <xsl:value-of select="text()" /> </span>
						</li>
					</xsl:for-each>
					<xsl:for-each select="gpgim:SeeAlso">
						<li>
							<span class="prop_attr_name">See Also: </span>
							<span class="prop_attr_val">
								<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="text()" /></xsl:call-template>
							</span>
						</li>
					</xsl:for-each>
				</ul>

				<div class="section_heading">Description:-</div>
				<div class="object_description"> <xsl:value-of select="gpgim:Description" /> </div>

				<xsl:if test="gpgim:Comment">
					<div class="section_heading">Comments:-</div>
					<xsl:for-each select="gpgim:Comment">
						<p class="comment">
							<xsl:value-of select="text()" />
						</p>
					</xsl:for-each>
				</xsl:if>

				<div class="section_heading">Used by features:-</div>
				<xsl:variable name="current_property_name"><xsl:value-of select="gpgim:Name" /></xsl:variable>
				<xsl:for-each select="//gpgim:FeatureClass[gpgim:Property=$current_property_name]">
					<xsl:if test="position() != 1"><xsl:text>, </xsl:text></xsl:if>
					<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="gpgim:Name" /></xsl:call-template>
				</xsl:for-each>

			</div>
		</div>
	</xsl:template>


	<xsl:template match="gpgim:FeatureClass">
		<div >
			<!-- Add the CSS 'class' to the div based on feature abstractness -->
			<xsl:attribute name="class">object featureclass_<xsl:value-of select="gpgim:ClassType" /></xsl:attribute>
			
			<a><xsl:attribute name="name"><xsl:value-of select="gpgim:Name" /></xsl:attribute></a>
			<h3 class="object_id">
				<xsl:value-of select="gpgim:Name" />
				<xsl:if test="gpgim:UserFriendlyName">
				 - <xsl:value-of select="gpgim:UserFriendlyName" />
				</xsl:if>
			</h3>

			<div class="object_content">
				<ul class="feat_attrs">
					<li>
						<span class="feat_attr_name">Class Type: </span>
						<span class="feat_attr_val"> <xsl:value-of select="gpgim:ClassType" /> </span>
					</li>
					<xsl:if test="@gpgim:defaultGeometryProperty">
						<li>
							<span class="feat_attr_name">Default Geometry Property: </span>
							<span class="feat_attr_val">
								<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="@gpgim:defaultGeometryProperty" /></xsl:call-template>
							</span>
						</li>
					</xsl:if>
					<li>
						<span class="feat_attr_name">Inherits From: </span>
						<span class="feat_attr_val">
							<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="gpgim:Inherits" /></xsl:call-template>
						</span>
					</li>
					<xsl:for-each select="gpgim:SeeAlso">
						<li>
							<span class="feat_attr_name">See Also: </span>
							<span class="feat_attr_val">
								<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="text()" /></xsl:call-template>
							</span>
						</li>
					</xsl:for-each>
				</ul>

				<div class="section_heading">Description:-</div>
				<div class="object_description"> <xsl:value-of select="gpgim:Description" /> </div>

				<xsl:if test="gpgim:Comment">
					<div class="section_heading">Comments:-</div>
					<xsl:for-each select="gpgim:Comment">
						<p class="comment">
							<xsl:value-of select="text()" />
						</p>
					</xsl:for-each>
				</xsl:if>

				<!-- Recursively walk up the inheritance hierarchy and list all the properties we get. -->
				<xsl:call-template name="PropertyReferenceList">
					<xsl:with-param name="class" select="gpgim:Name" />
					<xsl:with-param name="heading" select="'Properties defined by '" />
				</xsl:call-template>

				<!-- List what features subclass this one, if any -->
				<xsl:variable name="current_feature_name"><xsl:value-of select="gpgim:Name" /></xsl:variable>
				<xsl:if test="//gpgim:FeatureClass[gpgim:Inherits=$current_feature_name]">
					<div class="section_heading">Inherited by features:-</div>
					<xsl:for-each select="//gpgim:FeatureClass[gpgim:Inherits=$current_feature_name]">
						<xsl:if test="position() != 1"><xsl:text>, </xsl:text></xsl:if>
						<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="gpgim:Name" /></xsl:call-template>
					</xsl:for-each>
				</xsl:if>

			</div>
		</div>
	</xsl:template>


	<!-- Named Template for the Property List of features, called during the gpgim:FeatureClass template and from itself recursively -->
	<xsl:template name="PropertyReferenceList">
		<xsl:param name="class" />
		<xsl:param name="heading" />

		<xsl:if test="key('feature', $class)/gpgim:Property">	<!-- If there are any properties defined for this FeatureClass, -->
			<div class="section_heading"> <xsl:value-of select="$heading" /> <xsl:value-of select="$class" /> :-</div>
			<ul class="feat_props">
				<xsl:for-each select="key('feature', $class)/gpgim:Property">
					<li>
						<xsl:call-template name="PropertyReference"><xsl:with-param name="property" select="text()" /></xsl:call-template>
					</li>
				</xsl:for-each>
			</ul>
		</xsl:if>

		<!-- Recursion OMG! -->
		<xsl:if test="key('feature', $class)/gpgim:Inherits">	<!-- If this FeatureClass Inherits from another, -->
			<xsl:call-template name="PropertyReferenceList">
				<xsl:with-param name="class" select="key('feature', $class)/gpgim:Inherits" />
				<xsl:with-param name="heading" select="'Properties inherited from '" />
			</xsl:call-template>
		</xsl:if>
	</xsl:template>


	<!-- Named Template to fetch details of a property by name and make a nice 1-line summary. Called by PropertyReferenceList. -->
	<xsl:template name="PropertyReference">
		<xsl:param name="property" />
		<!-- let's format it as e.g. [0..1] gpml:propertyName (gpml:ValueType) -->
		<span class="propref_multiplicity"> [<xsl:value-of select="key('property', $property)/gpgim:Multiplicity" />] </span>

		<span class="propref_name"> <xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="$property" /></xsl:call-template> </span>

		<span class="propref_valuetype">
			(
			<xsl:choose>
				<xsl:when test="key('property', $property)/@gpgim:defaultType">
					<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="key('property', $property)/@gpgim:defaultType" /></xsl:call-template>, ...
				</xsl:when>
				<xsl:otherwise>
					<xsl:call-template name="ObjectLink"><xsl:with-param name="href" select="key('property', $property)/gpgim:Type" /></xsl:call-template>
				</xsl:otherwise>
			</xsl:choose>
			)
		</span>

		<!-- <span class="propref_description"> : <xsl:value-of select="key('property', $property)/gpgim:Description" /> </span> -->
	</xsl:template>


	<!-- Named Template to wrap up a given gpml:SomethingSomething (given with param 'href') in a suitable <a> element. -->
	<xsl:template name="ObjectLink">
		<xsl:param name="href" />
		<a>
			<!-- Object links get their href attribute set to "#gpml:yourFeatureOrPropertyEtc" -->
			<xsl:attribute name="href">#<xsl:value-of select="$href" /></xsl:attribute>

			<xsl:choose>
				<!-- Does the $href appear to point to a valid object defined in the XML somewhere? -->
				<xsl:when test="key('object', $href)">
					<xsl:attribute name="class">objlink</xsl:attribute>
					<!-- Good Object links get their title attribute sourced by searching for features and properties with a Name equal to the given $href, or NativeProperties and Enumerations that have a Type equal to the given $href. It'd have been easier if we'd used Name for everything but oh well, the 'xsl:key' feature makes things a bit faster ;) -->
					<xsl:attribute name="title"><xsl:value-of select="key('object', $href)/gpgim:Description" /></xsl:attribute>
				</xsl:when>

				<!-- If there doesn't seem to be anything with that Name or Type, format the link as 'broken' -->
				<xsl:otherwise>
					<xsl:attribute name="class">broken objlink</xsl:attribute>
					<xsl:attribute name="title">Broken link : Nothing is defined with this name. Possible typo?</xsl:attribute>
				</xsl:otherwise>

			</xsl:choose>

			<!-- The text of the object link -->
			<xsl:choose>
				<xsl:when test="$href"> <xsl:value-of select="$href" /> </xsl:when>
				<xsl:otherwise>(none)</xsl:otherwise>\
			</xsl:choose>
		</a>
	</xsl:template>


	<!-- do nothing with text elements by default, overriding the built-in template. This prevents you from getting source document text spewed everywhere. -->
	<xsl:template match="text()" />

</xsl:stylesheet>
