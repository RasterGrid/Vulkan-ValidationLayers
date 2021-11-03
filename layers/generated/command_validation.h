// *** THIS FILE IS GENERATED - DO NOT EDIT ***
// See command_validation_generator.py for modifications


/***************************************************************************
 *
 * Copyright (c) 2021 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Spencer Fricke <s.fricke@samsung.com>
 *
 ****************************************************************************/

#pragma once
#include <array>

// Used as key for maps of all vkCmd* calls
// Does not include vkBeginCommandBuffer/vkEndCommandBuffer
typedef enum CMD_TYPE {
    CMD_NONE = 0,
    CMD_BEGINCONDITIONALRENDERINGEXT = 1,
    CMD_BEGINDEBUGUTILSLABELEXT = 2,
    CMD_BEGINQUERY = 3,
    CMD_BEGINQUERYINDEXEDEXT = 4,
    CMD_BEGINRENDERPASS = 5,
    CMD_BEGINRENDERPASS2 = 6,
    CMD_BEGINRENDERPASS2KHR = 7,
    CMD_BEGINRENDERINGKHR = 8,
    CMD_BEGINTRANSFORMFEEDBACKEXT = 9,
    CMD_BEGINVIDEOCODINGKHR = 10,
    CMD_BINDDESCRIPTORSETS = 11,
    CMD_BINDINDEXBUFFER = 12,
    CMD_BINDINVOCATIONMASKHUAWEI = 13,
    CMD_BINDPIPELINE = 14,
    CMD_BINDPIPELINESHADERGROUPNV = 15,
    CMD_BINDSHADINGRATEIMAGENV = 16,
    CMD_BINDTRANSFORMFEEDBACKBUFFERSEXT = 17,
    CMD_BINDVERTEXBUFFERS = 18,
    CMD_BINDVERTEXBUFFERS2EXT = 19,
    CMD_BLITIMAGE = 20,
    CMD_BLITIMAGE2KHR = 21,
    CMD_BUILDACCELERATIONSTRUCTURENV = 22,
    CMD_BUILDACCELERATIONSTRUCTURESINDIRECTKHR = 23,
    CMD_BUILDACCELERATIONSTRUCTURESKHR = 24,
    CMD_CLEARATTACHMENTS = 25,
    CMD_CLEARCOLORIMAGE = 26,
    CMD_CLEARDEPTHSTENCILIMAGE = 27,
    CMD_CONTROLVIDEOCODINGKHR = 28,
    CMD_COPYACCELERATIONSTRUCTUREKHR = 29,
    CMD_COPYACCELERATIONSTRUCTURENV = 30,
    CMD_COPYACCELERATIONSTRUCTURETOMEMORYKHR = 31,
    CMD_COPYBUFFER = 32,
    CMD_COPYBUFFER2KHR = 33,
    CMD_COPYBUFFERTOIMAGE = 34,
    CMD_COPYBUFFERTOIMAGE2KHR = 35,
    CMD_COPYIMAGE = 36,
    CMD_COPYIMAGE2KHR = 37,
    CMD_COPYIMAGETOBUFFER = 38,
    CMD_COPYIMAGETOBUFFER2KHR = 39,
    CMD_COPYMEMORYTOACCELERATIONSTRUCTUREKHR = 40,
    CMD_COPYQUERYPOOLRESULTS = 41,
    CMD_CULAUNCHKERNELNVX = 42,
    CMD_DEBUGMARKERBEGINEXT = 43,
    CMD_DEBUGMARKERENDEXT = 44,
    CMD_DEBUGMARKERINSERTEXT = 45,
    CMD_DECODEVIDEOKHR = 46,
    CMD_DISPATCH = 47,
    CMD_DISPATCHBASE = 48,
    CMD_DISPATCHBASEKHR = 49,
    CMD_DISPATCHINDIRECT = 50,
    CMD_DRAW = 51,
    CMD_DRAWINDEXED = 52,
    CMD_DRAWINDEXEDINDIRECT = 53,
    CMD_DRAWINDEXEDINDIRECTCOUNT = 54,
    CMD_DRAWINDEXEDINDIRECTCOUNTAMD = 55,
    CMD_DRAWINDEXEDINDIRECTCOUNTKHR = 56,
    CMD_DRAWINDIRECT = 57,
    CMD_DRAWINDIRECTBYTECOUNTEXT = 58,
    CMD_DRAWINDIRECTCOUNT = 59,
    CMD_DRAWINDIRECTCOUNTAMD = 60,
    CMD_DRAWINDIRECTCOUNTKHR = 61,
    CMD_DRAWMESHTASKSINDIRECTCOUNTNV = 62,
    CMD_DRAWMESHTASKSINDIRECTNV = 63,
    CMD_DRAWMESHTASKSNV = 64,
    CMD_DRAWMULTIEXT = 65,
    CMD_DRAWMULTIINDEXEDEXT = 66,
    CMD_ENCODEVIDEOKHR = 67,
    CMD_ENDCONDITIONALRENDERINGEXT = 68,
    CMD_ENDDEBUGUTILSLABELEXT = 69,
    CMD_ENDQUERY = 70,
    CMD_ENDQUERYINDEXEDEXT = 71,
    CMD_ENDRENDERPASS = 72,
    CMD_ENDRENDERPASS2 = 73,
    CMD_ENDRENDERPASS2KHR = 74,
    CMD_ENDRENDERINGKHR = 75,
    CMD_ENDTRANSFORMFEEDBACKEXT = 76,
    CMD_ENDVIDEOCODINGKHR = 77,
    CMD_EXECUTECOMMANDS = 78,
    CMD_EXECUTEGENERATEDCOMMANDSNV = 79,
    CMD_FILLBUFFER = 80,
    CMD_INSERTDEBUGUTILSLABELEXT = 81,
    CMD_NEXTSUBPASS = 82,
    CMD_NEXTSUBPASS2 = 83,
    CMD_NEXTSUBPASS2KHR = 84,
    CMD_PIPELINEBARRIER = 85,
    CMD_PIPELINEBARRIER2KHR = 86,
    CMD_PREPROCESSGENERATEDCOMMANDSNV = 87,
    CMD_PUSHCONSTANTS = 88,
    CMD_PUSHDESCRIPTORSETKHR = 89,
    CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR = 90,
    CMD_RESETEVENT = 91,
    CMD_RESETEVENT2KHR = 92,
    CMD_RESETQUERYPOOL = 93,
    CMD_RESOLVEIMAGE = 94,
    CMD_RESOLVEIMAGE2KHR = 95,
    CMD_SETBLENDCONSTANTS = 96,
    CMD_SETCHECKPOINTNV = 97,
    CMD_SETCOARSESAMPLEORDERNV = 98,
    CMD_SETCOLORWRITEENABLEEXT = 99,
    CMD_SETCULLMODEEXT = 100,
    CMD_SETDEPTHBIAS = 101,
    CMD_SETDEPTHBIASENABLEEXT = 102,
    CMD_SETDEPTHBOUNDS = 103,
    CMD_SETDEPTHBOUNDSTESTENABLEEXT = 104,
    CMD_SETDEPTHCOMPAREOPEXT = 105,
    CMD_SETDEPTHTESTENABLEEXT = 106,
    CMD_SETDEPTHWRITEENABLEEXT = 107,
    CMD_SETDEVICEMASK = 108,
    CMD_SETDEVICEMASKKHR = 109,
    CMD_SETDISCARDRECTANGLEEXT = 110,
    CMD_SETEVENT = 111,
    CMD_SETEVENT2KHR = 112,
    CMD_SETEXCLUSIVESCISSORNV = 113,
    CMD_SETFRAGMENTSHADINGRATEENUMNV = 114,
    CMD_SETFRAGMENTSHADINGRATEKHR = 115,
    CMD_SETFRONTFACEEXT = 116,
    CMD_SETLINESTIPPLEEXT = 117,
    CMD_SETLINEWIDTH = 118,
    CMD_SETLOGICOPEXT = 119,
    CMD_SETPATCHCONTROLPOINTSEXT = 120,
    CMD_SETPERFORMANCEMARKERINTEL = 121,
    CMD_SETPERFORMANCEOVERRIDEINTEL = 122,
    CMD_SETPERFORMANCESTREAMMARKERINTEL = 123,
    CMD_SETPRIMITIVERESTARTENABLEEXT = 124,
    CMD_SETPRIMITIVETOPOLOGYEXT = 125,
    CMD_SETRASTERIZERDISCARDENABLEEXT = 126,
    CMD_SETRAYTRACINGPIPELINESTACKSIZEKHR = 127,
    CMD_SETSAMPLELOCATIONSEXT = 128,
    CMD_SETSCISSOR = 129,
    CMD_SETSCISSORWITHCOUNTEXT = 130,
    CMD_SETSTENCILCOMPAREMASK = 131,
    CMD_SETSTENCILOPEXT = 132,
    CMD_SETSTENCILREFERENCE = 133,
    CMD_SETSTENCILTESTENABLEEXT = 134,
    CMD_SETSTENCILWRITEMASK = 135,
    CMD_SETVERTEXINPUTEXT = 136,
    CMD_SETVIEWPORT = 137,
    CMD_SETVIEWPORTSHADINGRATEPALETTENV = 138,
    CMD_SETVIEWPORTWSCALINGNV = 139,
    CMD_SETVIEWPORTWITHCOUNTEXT = 140,
    CMD_SUBPASSSHADINGHUAWEI = 141,
    CMD_TRACERAYSINDIRECTKHR = 142,
    CMD_TRACERAYSKHR = 143,
    CMD_TRACERAYSNV = 144,
    CMD_UPDATEBUFFER = 145,
    CMD_WAITEVENTS = 146,
    CMD_WAITEVENTS2KHR = 147,
    CMD_WRITEACCELERATIONSTRUCTURESPROPERTIESKHR = 148,
    CMD_WRITEACCELERATIONSTRUCTURESPROPERTIESNV = 149,
    CMD_WRITEBUFFERMARKER2AMD = 150,
    CMD_WRITEBUFFERMARKERAMD = 151,
    CMD_WRITETIMESTAMP = 152,
    CMD_WRITETIMESTAMP2KHR = 153,
    CMD_RANGE_SIZE = 154
} CMD_TYPE;

static const std::array<const char *, CMD_RANGE_SIZE> kGeneratedCommandNameList = {{
    "Command_Undefined",
    "vkCmdBeginConditionalRenderingEXT",
    "vkCmdBeginDebugUtilsLabelEXT",
    "vkCmdBeginQuery",
    "vkCmdBeginQueryIndexedEXT",
    "vkCmdBeginRenderPass",
    "vkCmdBeginRenderPass2",
    "vkCmdBeginRenderPass2KHR",
    "vkCmdBeginRenderingKHR",
    "vkCmdBeginTransformFeedbackEXT",
    "vkCmdBeginVideoCodingKHR",
    "vkCmdBindDescriptorSets",
    "vkCmdBindIndexBuffer",
    "vkCmdBindInvocationMaskHUAWEI",
    "vkCmdBindPipeline",
    "vkCmdBindPipelineShaderGroupNV",
    "vkCmdBindShadingRateImageNV",
    "vkCmdBindTransformFeedbackBuffersEXT",
    "vkCmdBindVertexBuffers",
    "vkCmdBindVertexBuffers2EXT",
    "vkCmdBlitImage",
    "vkCmdBlitImage2KHR",
    "vkCmdBuildAccelerationStructureNV",
    "vkCmdBuildAccelerationStructuresIndirectKHR",
    "vkCmdBuildAccelerationStructuresKHR",
    "vkCmdClearAttachments",
    "vkCmdClearColorImage",
    "vkCmdClearDepthStencilImage",
    "vkCmdControlVideoCodingKHR",
    "vkCmdCopyAccelerationStructureKHR",
    "vkCmdCopyAccelerationStructureNV",
    "vkCmdCopyAccelerationStructureToMemoryKHR",
    "vkCmdCopyBuffer",
    "vkCmdCopyBuffer2KHR",
    "vkCmdCopyBufferToImage",
    "vkCmdCopyBufferToImage2KHR",
    "vkCmdCopyImage",
    "vkCmdCopyImage2KHR",
    "vkCmdCopyImageToBuffer",
    "vkCmdCopyImageToBuffer2KHR",
    "vkCmdCopyMemoryToAccelerationStructureKHR",
    "vkCmdCopyQueryPoolResults",
    "vkCmdCuLaunchKernelNVX",
    "vkCmdDebugMarkerBeginEXT",
    "vkCmdDebugMarkerEndEXT",
    "vkCmdDebugMarkerInsertEXT",
    "vkCmdDecodeVideoKHR",
    "vkCmdDispatch",
    "vkCmdDispatchBase",
    "vkCmdDispatchBaseKHR",
    "vkCmdDispatchIndirect",
    "vkCmdDraw",
    "vkCmdDrawIndexed",
    "vkCmdDrawIndexedIndirect",
    "vkCmdDrawIndexedIndirectCount",
    "vkCmdDrawIndexedIndirectCountAMD",
    "vkCmdDrawIndexedIndirectCountKHR",
    "vkCmdDrawIndirect",
    "vkCmdDrawIndirectByteCountEXT",
    "vkCmdDrawIndirectCount",
    "vkCmdDrawIndirectCountAMD",
    "vkCmdDrawIndirectCountKHR",
    "vkCmdDrawMeshTasksIndirectCountNV",
    "vkCmdDrawMeshTasksIndirectNV",
    "vkCmdDrawMeshTasksNV",
    "vkCmdDrawMultiEXT",
    "vkCmdDrawMultiIndexedEXT",
    "vkCmdEncodeVideoKHR",
    "vkCmdEndConditionalRenderingEXT",
    "vkCmdEndDebugUtilsLabelEXT",
    "vkCmdEndQuery",
    "vkCmdEndQueryIndexedEXT",
    "vkCmdEndRenderPass",
    "vkCmdEndRenderPass2",
    "vkCmdEndRenderPass2KHR",
    "vkCmdEndRenderingKHR",
    "vkCmdEndTransformFeedbackEXT",
    "vkCmdEndVideoCodingKHR",
    "vkCmdExecuteCommands",
    "vkCmdExecuteGeneratedCommandsNV",
    "vkCmdFillBuffer",
    "vkCmdInsertDebugUtilsLabelEXT",
    "vkCmdNextSubpass",
    "vkCmdNextSubpass2",
    "vkCmdNextSubpass2KHR",
    "vkCmdPipelineBarrier",
    "vkCmdPipelineBarrier2KHR",
    "vkCmdPreprocessGeneratedCommandsNV",
    "vkCmdPushConstants",
    "vkCmdPushDescriptorSetKHR",
    "vkCmdPushDescriptorSetWithTemplateKHR",
    "vkCmdResetEvent",
    "vkCmdResetEvent2KHR",
    "vkCmdResetQueryPool",
    "vkCmdResolveImage",
    "vkCmdResolveImage2KHR",
    "vkCmdSetBlendConstants",
    "vkCmdSetCheckpointNV",
    "vkCmdSetCoarseSampleOrderNV",
    "vkCmdSetColorWriteEnableEXT",
    "vkCmdSetCullModeEXT",
    "vkCmdSetDepthBias",
    "vkCmdSetDepthBiasEnableEXT",
    "vkCmdSetDepthBounds",
    "vkCmdSetDepthBoundsTestEnableEXT",
    "vkCmdSetDepthCompareOpEXT",
    "vkCmdSetDepthTestEnableEXT",
    "vkCmdSetDepthWriteEnableEXT",
    "vkCmdSetDeviceMask",
    "vkCmdSetDeviceMaskKHR",
    "vkCmdSetDiscardRectangleEXT",
    "vkCmdSetEvent",
    "vkCmdSetEvent2KHR",
    "vkCmdSetExclusiveScissorNV",
    "vkCmdSetFragmentShadingRateEnumNV",
    "vkCmdSetFragmentShadingRateKHR",
    "vkCmdSetFrontFaceEXT",
    "vkCmdSetLineStippleEXT",
    "vkCmdSetLineWidth",
    "vkCmdSetLogicOpEXT",
    "vkCmdSetPatchControlPointsEXT",
    "vkCmdSetPerformanceMarkerINTEL",
    "vkCmdSetPerformanceOverrideINTEL",
    "vkCmdSetPerformanceStreamMarkerINTEL",
    "vkCmdSetPrimitiveRestartEnableEXT",
    "vkCmdSetPrimitiveTopologyEXT",
    "vkCmdSetRasterizerDiscardEnableEXT",
    "vkCmdSetRayTracingPipelineStackSizeKHR",
    "vkCmdSetSampleLocationsEXT",
    "vkCmdSetScissor",
    "vkCmdSetScissorWithCountEXT",
    "vkCmdSetStencilCompareMask",
    "vkCmdSetStencilOpEXT",
    "vkCmdSetStencilReference",
    "vkCmdSetStencilTestEnableEXT",
    "vkCmdSetStencilWriteMask",
    "vkCmdSetVertexInputEXT",
    "vkCmdSetViewport",
    "vkCmdSetViewportShadingRatePaletteNV",
    "vkCmdSetViewportWScalingNV",
    "vkCmdSetViewportWithCountEXT",
    "vkCmdSubpassShadingHUAWEI",
    "vkCmdTraceRaysIndirectKHR",
    "vkCmdTraceRaysKHR",
    "vkCmdTraceRaysNV",
    "vkCmdUpdateBuffer",
    "vkCmdWaitEvents",
    "vkCmdWaitEvents2KHR",
    "vkCmdWriteAccelerationStructuresPropertiesKHR",
    "vkCmdWriteAccelerationStructuresPropertiesNV",
    "vkCmdWriteBufferMarker2AMD",
    "vkCmdWriteBufferMarkerAMD",
    "vkCmdWriteTimestamp",
    "vkCmdWriteTimestamp2KHR",
}};
