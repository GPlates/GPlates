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
    GLAgeGridCoverageSource.cc
    GLAgeGridCoverageSource.h
    GLAgeGridMaskSource.cc
    GLAgeGridMaskSource.h
    GLArray.cc
    GLArray.h
    GLBindTextureState.cc
    GLBindTextureState.h
    GLBlendState.h
    GLClearBuffers.h
    GLClearBuffersState.h
    GLCompositeDrawable.cc
    GLCompositeDrawable.h
    GLCompositeStateSet.cc
    GLCompositeStateSet.h
    GLContext.cc
    GLContext.h
    GLCubeSubdivision.cc
    GLCubeSubdivision.h
    GLCubeSubdivisionCache.h
    GLCullVisitor.cc
    GLCullVisitor.h
    GLDepthRangeState.h
    GLDrawable.h
    GLEnterOrLeaveStateSet.h
    GLFragmentTestStates.h
    GLFrustum.cc
    GLFrustum.h
    GLIntersect.cc
    GLIntersect.h
    GLIntersectPrimitives.cc
    GLIntersectPrimitives.h
    GLMaskBuffersState.h
    GLMatrix.cc
    GLMatrix.h
    GLMultiResolutionCubeMesh.cc
    GLMultiResolutionCubeMesh.h
    GLMultiResolutionCubeRaster.cc
    GLMultiResolutionCubeRaster.h
    GLMultiResolutionFilledPolygons.cc
    GLMultiResolutionFilledPolygons.h
    GLMultiResolutionRaster.cc
    GLMultiResolutionRaster.h
    GLMultiResolutionRasterNode.h
    GLMultiResolutionRasterSource.h
    GLMultiResolutionReconstructedRasterNode.h
    GLMultiResolutionStaticPolygonReconstructedRaster.cc
    GLMultiResolutionStaticPolygonReconstructedRaster.h
    GLPointLinePolygonState.cc
    GLPointLinePolygonState.h
    GLProxiedRasterSource.cc
    GLProxiedRasterSource.h
    GLRasterProxy.cc
    GLRasterProxy.h
    GLReconstructedStaticPolygonMeshes.cc
    GLReconstructedStaticPolygonMeshes.h
    GLRenderer.cc
    GLRenderer.h
    GLRenderGraph.cc
    GLRenderGraph.h
    GLRenderGraphDrawableNode.cc
    GLRenderGraphDrawableNode.h
    GLRenderGraphInternalNode.cc
    GLRenderGraphInternalNode.h
    GLRenderGraphNode.h
    GLRenderGraphVisitor.h
    GLRenderOperation.h
    GLRenderOperationsTarget.cc
    GLRenderOperationsTarget.h
    GLRenderPass.cc
    GLRenderPass.h
    GLRenderQueue.cc
    GLRenderQueue.h
    GLRenderTarget.cc
    GLRenderTarget.h
    GLRenderTargetFactory.h
    GLRenderTargetManager.cc
    GLRenderTargetManager.h
    GLRenderTargetType.h
    GLResource.h
    GLResourceManager.h
    GLShadeModelState.h
    GLState.cc
    GLState.h
    GLStateGraph.cc
    GLStateGraph.h
    GLStateGraphBuilder.cc
    GLStateGraphBuilder.h
    GLStateGraphNode.cc
    GLStateGraphNode.h
    GLStateSet.h
    GLStreamPrimitives.h
    GLText2DDrawable.cc
    GLText2DDrawable.h
    GLText3DNode.cc
    GLText3DNode.h
    GLTexture.cc
    GLTexture.h
    GLTextureEnvironmentState.cc
    GLTextureEnvironmentState.h
    GLTextureResource.h
    GLTextureTransformState.cc
    GLTextureTransformState.h
    GLTextureUtils.cc
    GLTextureUtils.h
    GLTransform.cc
    GLTransform.h
    GLTransformState.cc
    GLTransformState.h
    GLUNurbsRenderer.cc
    GLUNurbsRenderer.h
    GLUNurbsRendererDrawable.cc
    GLUNurbsRendererDrawable.h
    GLUQuadric.cc
    GLUQuadric.h
    GLUQuadricDrawable.cc
    GLUQuadricDrawable.h
    GLUtils.cc
    GLUtils.h
    GLVertex.cc
    GLVertex.h
    GLVertexArray.cc
    GLVertexArray.h
    GLVertexArrayDrawable.h
    GLVertexBufferResource.cc
    GLVertexBufferResource.h
    GLVertexElementArray.cc
    GLVertexElementArray.h
    GLViewport.h
    GLViewportNode.h
    GLViewportState.h
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
