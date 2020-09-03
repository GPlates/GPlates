#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# You can either explicitly add/remove source files here, or
# run 'python cmake/add_sources.py' to automatically list them (here).
#
# Note that CMake discourages use of the 'file(GLOB)' CMake command to automatically collect source files.
# One of the reasons is if source files are added or removed, CMake is not automatically re-run,
# so the build is unaware of the change.
#
set(srcs
    GL.cc
    GL.h
    GLAgeGridMaskSource.cc
    GLAgeGridMaskSource.h
    GLBuffer.cc
    GLBuffer.h
    GLCapabilities.cc
    GLCapabilities.h
    GLCompiledDrawState.cc
    GLCompiledDrawState.h
    GLContext.cc
    GLContext.h
    GLContextImpl.h
    GLCubeMeshGenerator.cc
    GLCubeMeshGenerator.h
    GLCubeSubdivision.cc
    GLCubeSubdivision.h
    GLCubeSubdivisionCache.h
    GLDataRasterSource.cc
    GLDataRasterSource.h
    GLFilledPolygonsGlobeView.cc
    GLFilledPolygonsGlobeView.h
    GLFilledPolygonsMapView.cc
    GLFilledPolygonsMapView.h
    GLFramebuffer.cc
    GLFramebuffer.h
    GLFrustum.cc
    GLFrustum.h
    GLImageUtils.cc
    GLImageUtils.h
    GLIntersect.cc
    GLIntersect.h
    GLIntersectPrimitives.cc
    GLIntersectPrimitives.h
    GLLight.cc
    GLLight.h
    GLMapCubeMeshGenerator.cc
    GLMapCubeMeshGenerator.h
    GLMatrix.cc
    GLMatrix.h
    GLMultiResolutionCubeMesh.cc
    GLMultiResolutionCubeMesh.h
    GLMultiResolutionCubeRaster.cc
    GLMultiResolutionCubeRaster.h
    GLMultiResolutionCubeRasterInterface.h
    GLMultiResolutionCubeReconstructedRaster.cc
    GLMultiResolutionCubeReconstructedRaster.h
    GLMultiResolutionMapCubeMesh.cc
    GLMultiResolutionMapCubeMesh.h
    GLMultiResolutionRaster.cc
    GLMultiResolutionRaster.h
    GLMultiResolutionRasterInterface.cc
    GLMultiResolutionRasterInterface.h
    GLMultiResolutionRasterMapView.cc
    GLMultiResolutionRasterMapView.h
    GLMultiResolutionRasterSource.h
    GLMultiResolutionStaticPolygonReconstructedRaster.cc
    GLMultiResolutionStaticPolygonReconstructedRaster.h
    GLNormalMapSource.cc
    GLNormalMapSource.h
    GLObject.h
    GLObjectResource.h
    GLObjectResourceManager.h
    GLProgram.cc
    GLProgram.h
    GLRasterCoRegistration.cc
    GLRasterCoRegistration.h
    GLReconstructedStaticPolygonMeshes.cc
    GLReconstructedStaticPolygonMeshes.h
    GLRenderbuffer.cc
    GLRenderbuffer.h
    GLRenderer.cc
    GLRenderer.h
    GLRendererImpl.h
    GLSampler.cc
    GLSampler.h
    GLSaveRestoreFrameBuffer.cc
    GLSaveRestoreFrameBuffer.h
    GLScalarField3D.cc
    GLScalarField3D.h
    GLScalarField3DGenerator.cc
    GLScalarField3DGenerator.h
    GLScalarFieldDepthLayersSource.cc
    GLScalarFieldDepthLayersSource.h
    GLShader.cc
    GLShader.h
    GLShaderSource.cc
    GLShaderSource.h
    GLState.cc
    GLState.h
    GLStateSet.h
    GLStateSetKeys.cc
    GLStateSetKeys.h
    GLStateSets.cc
    GLStateSets.h
    GLStateSetStore.h
    GLStateStore.h
    GLStreamBuffer.cc
    GLStreamBuffer.h
    GLStreamPrimitives.h
    GLStreamPrimitiveWriters.h
    GLText.cc
    GLText.h
    GLTexture.cc
    GLTexture.h
    GLTextureUtils.cc
    GLTextureUtils.h
    GLTileRender.cc
    GLTileRender.h
    GLTransform.h
    GLUtils.cc
    GLUtils.h
    GLVertexArray.cc
    GLVertexArray.h
    GLVertexUtils.cc
    GLVertexUtils.h
    GLViewport.h
    GLViewProjection.cc
    GLViewProjection.h
    GLVisualLayers.cc
    GLVisualLayers.h
    GLVisualRasterSource.cc
    GLVisualRasterSource.h
    OpenGL1.h
    OpenGL3.h
    OpenGLException.h
)

# Add the source files to the gplates static library.
target_sources_util(gplates-lib PRIVATE ${srcs})

# Add the source files to the pygplates extension module.
#
# Note this is the external Python library that is not embedded inside GPlates.
# All sources files compiled in pygplates need to be position independent and hence
# compiled differently (POSITION_INDEPENDENT_CODE target property is set on pygplates).
set(pygplates_srcs ${srcs})
target_sources_util(pygplates PRIVATE ${pygplates_srcs})
