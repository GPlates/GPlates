# Current binary directory must be included first for g++ pre-compiled headers.
# There's a '_pch.h' file in current source and current binary directory however the '_pch.h.gch' pre-compiled header
# is only in current binary directory and it will only be used by the compiler if it's in the same directory as the found '_pch.h' file.
# So we need to ensure that we find the '_pch.h' file from the current binary directory first.
include_directories(${CMAKE_CURRENT_BINARY_DIR})
# Current source directory (directory that this 'CMakeLists.txt' file is in).
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# The variables used here are set in the parent 'CMakeList.txt' file.
include_directories(${SYSTEM_INCLUDE_FLAG} ${external_INCLUDE_DIRS})
include_directories(${internal_INCLUDE_DIRS})

set(srcs
    GLAgeGridMaskSource.cc
    GLAgeGridMaskSource.h
    GLBuffer.cc
    GLBuffer.h
    GLBufferImpl.cc
    GLBufferImpl.h
    GLBufferObject.cc
    GLBufferObject.h
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
    GLDepthRange.h
    GLFilledPolygonsGlobeView.cc
    GLFilledPolygonsGlobeView.h
    GLFilledPolygonsMapView.cc
    GLFilledPolygonsMapView.h
    GLFrameBufferObject.cc
    GLFrameBufferObject.h
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
    GLPixelBuffer.cc
    GLPixelBuffer.h
    GLPixelBufferImpl.cc
    GLPixelBufferImpl.h
    GLPixelBufferObject.cc
    GLPixelBufferObject.h
    GLProgramObject.cc
    GLProgramObject.h
    GLProjectionUtils.cc
    GLProjectionUtils.h
    GLRasterCoRegistration.cc
    GLRasterCoRegistration.h
    GLReconstructedStaticPolygonMeshes.cc
    GLReconstructedStaticPolygonMeshes.h
    GLRenderBufferObject.cc
    GLRenderBufferObject.h
    GLRenderer.cc
    GLRenderer.h
    GLRendererImpl.h
    GLRenderTarget.cc
    GLRenderTarget.h
    GLRenderTargetImpl.cc
    GLRenderTargetImpl.h
    GLSaveRestoreFrameBuffer.cc
    GLSaveRestoreFrameBuffer.h
    GLScalarField3D.cc
    GLScalarField3D.h
    GLScalarField3DGenerator.cc
    GLScalarField3DGenerator.h
    GLScalarFieldDepthLayersSource.cc
    GLScalarFieldDepthLayersSource.h
    GLScreenRenderTarget.cc
    GLScreenRenderTarget.h
    GLShaderObject.cc
    GLShaderObject.h
    GLShaderProgramUtils.cc
    GLShaderProgramUtils.h
    GLState.cc
    GLState.h
    GLStateSet.h
    GLStateSetKeys.cc
    GLStateSetKeys.h
    GLStateSets.cc
    GLStateSets.h
    GLStateSetStore.h
    GLStateStore.cc
    GLStateStore.h
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
    GLVertex.cc
    GLVertex.h
    GLVertexArray.cc
    GLVertexArray.h
    GLVertexArrayImpl.cc
    GLVertexArrayImpl.h
    GLVertexArrayObject.cc
    GLVertexArrayObject.h
    GLVertexBuffer.cc
    GLVertexBuffer.h
    GLVertexBufferImpl.cc
    GLVertexBufferImpl.h
    GLVertexBufferObject.cc
    GLVertexBufferObject.h
    GLVertexElementBuffer.cc
    GLVertexElementBuffer.h
    GLVertexElementBufferImpl.cc
    GLVertexElementBufferImpl.h
    GLVertexElementBufferObject.cc
    GLVertexElementBufferObject.h
    GLViewport.h
    GLVisualLayers.cc
    GLVisualLayers.h
    GLVisualRasterSource.cc
    GLVisualRasterSource.h
    OpenGL.h
    OpenGLBadAllocException.h
    OpenGLException.h
    OpenGLFwd.h
)





set(target_name opengl)

# Does the following:
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - adds dependencies on generated header files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} STATIC ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
