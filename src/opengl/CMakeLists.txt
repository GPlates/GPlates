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
    GLArray.h
    GLBindTextureState.cc
    GLBindTextureState.h
    GLBlendState.h
    GLCache.h
    GLClearBuffers.h
    GLClearBuffersState.h
    GLCompositeDrawable.cc
    GLCompositeDrawable.h
    GLCompositeStateSet.cc
    GLCompositeStateSet.h
    GLContext.cc
    GLContext.h
    GLCullVisitor.cc
    GLCullVisitor.h
    GLDepthRangeState.h
    GLDrawable.h
    GLEnterOrLeaveStateSet.h
    GLFragmentTestStates.h
    GLIntersect.cc
    GLIntersect.h
    GLIntersectPrimitives.cc
    GLIntersectPrimitives.h
    GLMaskBuffersState.h
    GLMatrix.cc
    GLMatrix.h
    GLMultiResolutionRaster.cc
    GLMultiResolutionRaster.h
    GLMultiResolutionRasterNode.h
    GLPointLinePolygonState.cc
    GLPointLinePolygonState.h
    GLRasterProxy.cc
    GLRasterProxy.h
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
    GLRenderTarget.h
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
    GLText2DDrawable.h
    GLText3DNode.cc
    GLText3DNode.h
    GLTexture.cc
    GLTexture.h
    GLTextureCache.h
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
    GLVertexArray.cc
    GLVertexArray.h
    GLVertexArrayCache.h
    GLVertexArrayDrawable.h
    GLVertexElementArray.cc
    GLVertexElementArray.h
    GLViewport.h
    GLViewportNode.h
    GLViewportState.h
    GLVolatileObject.h
    OpenGL.h
    OpenGLBadAllocException.h
    OpenGLException.h
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
