/* Copyright (c) 2015-2023 The Khronos Group Inc.
 * Copyright (c) 2015-2023 Valve Corporation
 * Copyright (c) 2015-2023 LunarG, Inc.
 * Copyright (C) 2015-2023 Google Inc.
 * Modifications Copyright (C) 2020-2022 Advanced Micro Devices, Inc. All rights reserved.
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
 * Author: Cody Northrop <cnorthrop@google.com>
 * Author: Michael Lentine <mlentine@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chia-I Wu <olv@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Ian Elliott <ianelliott@google.com>
 * Author: Dave Houlton <daveh@lunarg.com>
 * Author: Dustin Graves <dustin@lunarg.com>
 * Author: Jeremy Hayes <jeremy@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Karl Schultz <karl@lunarg.com>
 * Author: Mark Young <marky@lunarg.com>
 * Author: Mike Schuchardt <mikes@lunarg.com>
 * Author: Mike Weiblen <mikew@lunarg.com>
 * Author: Tony Barbour <tony@LunarG.com>
 * Author: John Zulauf <jzulauf@lunarg.com>
 * Author: Shannon McPherson <shannon@lunarg.com>
 * Author: Jeremy Kniager <jeremyk@lunarg.com>
 * Author: Tobias Hector <tobias.hector@amd.com>
 * Author: Jeremy Gebben <jeremyg@lunarg.com>
 */

#include <algorithm>
#include <assert.h>
#include <sstream>
#include <vector>

#include "vk_enum_string_helper.h"
#include "chassis.h"
#include "core_validation.h"
#include "sync_utils.h"
#include "convert_to_renderpass2.h"

bool CoreChecks::LogInvalidAttachmentMessage(const char *type1_string, const RENDER_PASS_STATE &rp1_state, const char *type2_string,
                                             const RENDER_PASS_STATE &rp2_state, uint32_t primary_attach, uint32_t secondary_attach,
                                             const char *msg, const char *caller, const char *error_code) const {
    const LogObjectList objlist(rp1_state.renderPass(), rp2_state.renderPass());
    return LogError(objlist, error_code,
                    "%s: RenderPasses incompatible between %s w/ %s and %s w/ %s Attachment %u is not "
                    "compatible with %u: %s.",
                    caller, type1_string, report_data->FormatHandle(rp1_state.renderPass()).c_str(), type2_string,
                    report_data->FormatHandle(rp2_state.renderPass()).c_str(), primary_attach, secondary_attach, msg);
}

bool CoreChecks::ValidateAttachmentCompatibility(const char *type1_string, const RENDER_PASS_STATE &rp1_state,
                                                 const char *type2_string, const RENDER_PASS_STATE &rp2_state,
                                                 uint32_t primary_attach, uint32_t secondary_attach, const char *caller,
                                                 const char *error_code) const {
    bool skip = false;
    const auto &primary_pass_ci = rp1_state.createInfo;
    const auto &secondary_pass_ci = rp2_state.createInfo;
    if (primary_pass_ci.attachmentCount <= primary_attach) {
        primary_attach = VK_ATTACHMENT_UNUSED;
    }
    if (secondary_pass_ci.attachmentCount <= secondary_attach) {
        secondary_attach = VK_ATTACHMENT_UNUSED;
    }
    if (primary_attach == VK_ATTACHMENT_UNUSED && secondary_attach == VK_ATTACHMENT_UNUSED) {
        return skip;
    }
    if (primary_attach == VK_ATTACHMENT_UNUSED) {
        skip |= LogInvalidAttachmentMessage(type1_string, rp1_state, type2_string, rp2_state, primary_attach, secondary_attach,
                                            "The first is unused while the second is not.", caller, error_code);
        return skip;
    }
    if (secondary_attach == VK_ATTACHMENT_UNUSED) {
        skip |= LogInvalidAttachmentMessage(type1_string, rp1_state, type2_string, rp2_state, primary_attach, secondary_attach,
                                            "The second is unused while the first is not.", caller, error_code);
        return skip;
    }
    if (primary_pass_ci.pAttachments[primary_attach].format != secondary_pass_ci.pAttachments[secondary_attach].format) {
        skip |= LogInvalidAttachmentMessage(type1_string, rp1_state, type2_string, rp2_state, primary_attach, secondary_attach,
                                            "They have different formats.", caller, error_code);
    }
    if (primary_pass_ci.pAttachments[primary_attach].samples != secondary_pass_ci.pAttachments[secondary_attach].samples) {
        skip |= LogInvalidAttachmentMessage(type1_string, rp1_state, type2_string, rp2_state, primary_attach, secondary_attach,
                                            "They have different samples.", caller, error_code);
    }
    if (primary_pass_ci.pAttachments[primary_attach].flags != secondary_pass_ci.pAttachments[secondary_attach].flags) {
        skip |= LogInvalidAttachmentMessage(type1_string, rp1_state, type2_string, rp2_state, primary_attach, secondary_attach,
                                            "They have different flags.", caller, error_code);
    }

    return skip;
}

bool CoreChecks::ValidateSubpassCompatibility(const char *type1_string, const RENDER_PASS_STATE &rp1_state,
                                              const char *type2_string, const RENDER_PASS_STATE &rp2_state, const int subpass,
                                              const char *caller, const char *error_code) const {
    bool skip = false;
    const auto &primary_desc = rp1_state.createInfo.pSubpasses[subpass];
    const auto &secondary_desc = rp2_state.createInfo.pSubpasses[subpass];
    uint32_t max_input_attachment_count = std::max(primary_desc.inputAttachmentCount, secondary_desc.inputAttachmentCount);
    for (uint32_t i = 0; i < max_input_attachment_count; ++i) {
        uint32_t primary_input_attach = VK_ATTACHMENT_UNUSED, secondary_input_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.inputAttachmentCount) {
            primary_input_attach = primary_desc.pInputAttachments[i].attachment;
        }
        if (i < secondary_desc.inputAttachmentCount) {
            secondary_input_attach = secondary_desc.pInputAttachments[i].attachment;
        }
        skip |= ValidateAttachmentCompatibility(type1_string, rp1_state, type2_string, rp2_state, primary_input_attach,
                                                secondary_input_attach, caller, error_code);
    }
    uint32_t max_color_attachment_count = std::max(primary_desc.colorAttachmentCount, secondary_desc.colorAttachmentCount);
    for (uint32_t i = 0; i < max_color_attachment_count; ++i) {
        uint32_t primary_color_attach = VK_ATTACHMENT_UNUSED, secondary_color_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.colorAttachmentCount) {
            primary_color_attach = primary_desc.pColorAttachments[i].attachment;
        }
        if (i < secondary_desc.colorAttachmentCount) {
            secondary_color_attach = secondary_desc.pColorAttachments[i].attachment;
        }
        skip |= ValidateAttachmentCompatibility(type1_string, rp1_state, type2_string, rp2_state, primary_color_attach,
                                                secondary_color_attach, caller, error_code);
        if (rp1_state.createInfo.subpassCount > 1) {
            uint32_t primary_resolve_attach = VK_ATTACHMENT_UNUSED, secondary_resolve_attach = VK_ATTACHMENT_UNUSED;
            if (i < primary_desc.colorAttachmentCount && primary_desc.pResolveAttachments) {
                primary_resolve_attach = primary_desc.pResolveAttachments[i].attachment;
            }
            if (i < secondary_desc.colorAttachmentCount && secondary_desc.pResolveAttachments) {
                secondary_resolve_attach = secondary_desc.pResolveAttachments[i].attachment;
            }
            skip |= ValidateAttachmentCompatibility(type1_string, rp1_state, type2_string, rp2_state, primary_resolve_attach,
                                                    secondary_resolve_attach, caller, error_code);
        }
    }
    uint32_t primary_depthstencil_attach = VK_ATTACHMENT_UNUSED, secondary_depthstencil_attach = VK_ATTACHMENT_UNUSED;
    if (primary_desc.pDepthStencilAttachment) {
        primary_depthstencil_attach = primary_desc.pDepthStencilAttachment[0].attachment;
    }
    if (secondary_desc.pDepthStencilAttachment) {
        secondary_depthstencil_attach = secondary_desc.pDepthStencilAttachment[0].attachment;
    }
    skip |= ValidateAttachmentCompatibility(type1_string, rp1_state, type2_string, rp2_state, primary_depthstencil_attach,
                                            secondary_depthstencil_attach, caller, error_code);

    // Both renderpasses must agree on Multiview usage
    if (primary_desc.viewMask && secondary_desc.viewMask) {
        if (primary_desc.viewMask != secondary_desc.viewMask) {
            std::stringstream ss;
            ss << "For subpass " << subpass << ", they have a different viewMask. The first has view mask " << primary_desc.viewMask
               << " while the second has view mask " << secondary_desc.viewMask << ".";
            skip |= LogInvalidPnextMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
        }
    } else if (primary_desc.viewMask) {
        skip |= LogInvalidPnextMessage(type1_string, rp1_state, type2_string, rp2_state,
                                       "The first uses Multiview (has non-zero viewMasks) while the second one does not.", caller,
                                       error_code);
    } else if (secondary_desc.viewMask) {
        skip |= LogInvalidPnextMessage(type1_string, rp1_state, type2_string, rp2_state,
                                       "The second uses Multiview (has non-zero viewMasks) while the first one does not.", caller,
                                       error_code);
    }

    // Find Fragment Shading Rate attachment entries in render passes if they
    // exist.
    const auto fsr1 = LvlFindInChain<VkFragmentShadingRateAttachmentInfoKHR>(primary_desc.pNext);
    const auto fsr2 = LvlFindInChain<VkFragmentShadingRateAttachmentInfoKHR>(secondary_desc.pNext);

    if (fsr1 && fsr2) {
        if ((fsr1->shadingRateAttachmentTexelSize.width != fsr2->shadingRateAttachmentTexelSize.width) ||
            (fsr1->shadingRateAttachmentTexelSize.height != fsr2->shadingRateAttachmentTexelSize.height)) {
            std::stringstream ss;
            ss << "Shading rate attachment texel sizes do not match (width is " << fsr1->shadingRateAttachmentTexelSize.width
               << " and " << fsr2->shadingRateAttachmentTexelSize.width << ", height is "
               << fsr1->shadingRateAttachmentTexelSize.height << " and " << fsr1->shadingRateAttachmentTexelSize.height << ".";
            skip |= LogInvalidPnextMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
        }
    } else if (fsr1) {
        skip |= LogInvalidPnextMessage(type1_string, rp1_state, type2_string, rp2_state,
                                       "The first uses a fragment "
                                       "shading rate attachment while "
                                       "the second one does not.",
                                       caller, error_code);
    } else if (fsr2) {
        skip |= LogInvalidPnextMessage(type1_string, rp1_state, type2_string, rp2_state,
                                       "The second uses a fragment "
                                       "shading rate attachment while "
                                       "the first one does not.",
                                       caller, error_code);
    }

    return skip;
}

bool CoreChecks::ValidateDependencyCompatibility(const char *type1_string, const RENDER_PASS_STATE &rp1_state,
                                                 const char *type2_string, const RENDER_PASS_STATE &rp2_state,
                                                 const uint32_t dependency, const char *caller, const char *error_code) const {
    bool skip = false;

    const auto &primary_dep = rp1_state.createInfo.pDependencies[dependency];
    const auto &secondary_dep = rp2_state.createInfo.pDependencies[dependency];

    if (primary_dep.srcSubpass != secondary_dep.srcSubpass) {
        std::stringstream ss;
        ss << "First srcSubpass is " << primary_dep.srcSubpass << ", but second srcSubpass is " << secondary_dep.srcSubpass << ".";
        skip |= LogInvalidDependencyMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
    }
    if (primary_dep.dstSubpass != secondary_dep.dstSubpass) {
        std::stringstream ss;
        ss << "First dstSubpass is " << primary_dep.dstSubpass << ", but second dstSubpass is " << secondary_dep.dstSubpass << ".";
        skip |= LogInvalidDependencyMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
    }
    if (primary_dep.srcStageMask != secondary_dep.srcStageMask) {
        std::stringstream ss;
        ss << "First srcStageMask is " << string_VkPipelineStageFlags(primary_dep.srcStageMask) << ", but second srcStageMask is "
           << string_VkPipelineStageFlags(secondary_dep.srcStageMask) << ".";
        skip |= LogInvalidDependencyMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
    }
    if (primary_dep.dstStageMask != secondary_dep.dstStageMask) {
        std::stringstream ss;
        ss << "First dstStageMask is " << string_VkPipelineStageFlags(primary_dep.dstStageMask) << ", but second dstStageMask is "
           << string_VkPipelineStageFlags(secondary_dep.dstStageMask) << ".";
        skip |= LogInvalidDependencyMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
    }
    if (primary_dep.srcAccessMask != secondary_dep.srcAccessMask) {
        std::stringstream ss;
        ss << "First srcAccessMask is " << string_VkAccessFlags(primary_dep.srcAccessMask) << ", but second srcAccessMask is "
           << string_VkAccessFlags(secondary_dep.srcAccessMask) << ".";
        skip |= LogInvalidDependencyMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
    }
    if (primary_dep.dstAccessMask != secondary_dep.dstAccessMask) {
        std::stringstream ss;
        ss << "First dstAccessMask is " << string_VkAccessFlags(primary_dep.dstAccessMask) << ", but second dstAccessMask is "
           << string_VkAccessFlags(secondary_dep.dstAccessMask) << ".";
        skip |= LogInvalidDependencyMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
    }
    if (primary_dep.dependencyFlags != secondary_dep.dependencyFlags) {
        std::stringstream ss;
        ss << "First dependencyFlags are " << string_VkDependencyFlags(primary_dep.dependencyFlags)
           << ", but second dependencyFlags are " << string_VkDependencyFlags(secondary_dep.dependencyFlags) << ".";
        skip |= LogInvalidDependencyMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
    }
    if (primary_dep.viewOffset != secondary_dep.viewOffset) {
        std::stringstream ss;
        ss << "First viewOffset are " << primary_dep.viewOffset << ", but second viewOffset are " << secondary_dep.viewOffset
           << ".";
        skip |= LogInvalidDependencyMessage(type1_string, rp1_state, type2_string, rp2_state, ss.str().c_str(), caller, error_code);
    }

    return skip;
}

bool CoreChecks::LogInvalidPnextMessage(const char *type1_string, const RENDER_PASS_STATE &rp1_state, const char *type2_string,
                                        const RENDER_PASS_STATE &rp2_state, const char *msg, const char *caller,
                                        const char *error_code) const {
    const LogObjectList objlist(rp1_state.renderPass(), rp2_state.renderPass());
    return LogError(objlist, error_code, "%s: RenderPasses incompatible between %s w/ %s and %s w/ %s: %s", caller, type1_string,
                    report_data->FormatHandle(rp1_state.renderPass()).c_str(), type2_string,
                    report_data->FormatHandle(rp2_state.renderPass()).c_str(), msg);
}

bool CoreChecks::LogInvalidDependencyMessage(const char *type1_string, const RENDER_PASS_STATE &rp1_state, const char *type2_string,
                                             const RENDER_PASS_STATE &rp2_state, const char *msg, const char *caller,
                                             const char *error_code) const {
    const LogObjectList objlist(rp1_state.renderPass(), rp2_state.renderPass());
    return LogError(objlist, error_code, "%s: RenderPasses incompatible between %s w/ %s and %s w/ %s: %s", caller, type1_string,
                    report_data->FormatHandle(rp1_state.renderPass()).c_str(), type2_string,
                    report_data->FormatHandle(rp2_state.renderPass()).c_str(), msg);
}

// Verify that given renderPass CreateInfo for primary and secondary command buffers are compatible.
//  This function deals directly with the CreateInfo, there are overloaded versions below that can take the renderPass handle and
//  will then feed into this function
bool CoreChecks::ValidateRenderPassCompatibility(const char *type1_string, const RENDER_PASS_STATE &rp1_state,
                                                 const char *type2_string, const RENDER_PASS_STATE &rp2_state, const char *caller,
                                                 const char *error_code) const {
    bool skip = false;

    // createInfo flags must be identical for the renderpasses to be compatible.
    if (rp1_state.createInfo.flags != rp2_state.createInfo.flags) {
        const LogObjectList objlist(rp1_state.renderPass(), rp2_state.renderPass());
        skip |=
            LogError(objlist, error_code,
                     "%s: RenderPasses incompatible between %s w/ %s with flags of %u and %s w/ "
                     "%s with a flags of %u.",
                     caller, type1_string, report_data->FormatHandle(rp1_state.renderPass()).c_str(), rp1_state.createInfo.flags,
                     type2_string, report_data->FormatHandle(rp2_state.renderPass()).c_str(), rp2_state.createInfo.flags);
    }

    if (rp1_state.createInfo.subpassCount != rp2_state.createInfo.subpassCount) {
        const LogObjectList objlist(rp1_state.renderPass(), rp2_state.renderPass());
        skip |= LogError(objlist, error_code,
                         "%s: RenderPasses incompatible between %s w/ %s with a subpassCount of %u and %s w/ "
                         "%s with a subpassCount of %u.",
                         caller, type1_string, report_data->FormatHandle(rp1_state.renderPass()).c_str(),
                         rp1_state.createInfo.subpassCount, type2_string, report_data->FormatHandle(rp2_state.renderPass()).c_str(),
                         rp2_state.createInfo.subpassCount);
    } else {
        for (uint32_t i = 0; i < rp1_state.createInfo.subpassCount; ++i) {
            skip |= ValidateSubpassCompatibility(type1_string, rp1_state, type2_string, rp2_state, i, caller, error_code);
        }
    }

    if (rp1_state.createInfo.dependencyCount != rp2_state.createInfo.dependencyCount) {
        const LogObjectList objlist(rp1_state.renderPass(), rp2_state.renderPass());
        skip |= LogError(objlist, error_code,
                         "%s: RenderPasses incompatible between %s w/ %s with a dependencyCount of %" PRIu32
                         " and %s w/ %s with a dependencyCount of %" PRIu32 ".",
                         caller, type1_string, report_data->FormatHandle(rp1_state.renderPass()).c_str(),
                         rp1_state.createInfo.dependencyCount, type2_string,
                         report_data->FormatHandle(rp2_state.renderPass()).c_str(), rp2_state.createInfo.dependencyCount);
    } else {
        for (uint32_t i = 0; i < rp1_state.createInfo.dependencyCount; ++i) {
            skip |= ValidateDependencyCompatibility(type1_string, rp1_state, type2_string, rp2_state, i, caller, error_code);
        }
    }
    if (rp1_state.createInfo.correlatedViewMaskCount != rp2_state.createInfo.correlatedViewMaskCount) {
        const LogObjectList objlist(rp1_state.renderPass(), rp2_state.renderPass());
        skip |= LogError(objlist, error_code,
                         "%s: RenderPasses incompatible between %s w/ %s with a correlatedViewMaskCount of %" PRIu32
                         " and %s w/ %s with a correlatedViewMaskCount of %" PRIu32 ".",
                         caller, type1_string, report_data->FormatHandle(rp1_state.renderPass()).c_str(),
                         rp1_state.createInfo.correlatedViewMaskCount, type2_string,
                         report_data->FormatHandle(rp2_state.renderPass()).c_str(), rp2_state.createInfo.correlatedViewMaskCount);
    } else {
        for (uint32_t i = 0; i < rp1_state.createInfo.correlatedViewMaskCount; ++i) {
            if (rp1_state.createInfo.pCorrelatedViewMasks[i] != rp2_state.createInfo.pCorrelatedViewMasks[i]) {
                const LogObjectList objlist(rp1_state.renderPass(), rp2_state.renderPass());
                skip |= LogError(objlist, error_code,
                                 "%s: RenderPasses incompatible between %s w/ %s with a pCorrelatedViewMasks[%" PRIu32
                                 "] of %" PRIu32 " and %s w/ %s with a pCorrelatedViewMasks[%" PRIu32 "] of %" PRIu32 ".",
                                 caller, type1_string, report_data->FormatHandle(rp1_state.renderPass()).c_str(), i,
                                 rp1_state.createInfo.pCorrelatedViewMasks[i], type2_string,
                                 report_data->FormatHandle(rp2_state.renderPass()).c_str(), i,
                                 rp1_state.createInfo.pCorrelatedViewMasks[i]);
            }
        }
    }

    // Find an entry of the Fragment Density Map type in the pNext chain, if it exists
    const auto fdm1 = LvlFindInChain<VkRenderPassFragmentDensityMapCreateInfoEXT>(rp1_state.createInfo.pNext);
    const auto fdm2 = LvlFindInChain<VkRenderPassFragmentDensityMapCreateInfoEXT>(rp2_state.createInfo.pNext);

    // Both renderpasses must agree on usage of a Fragment Density Map type
    if (fdm1 && fdm2) {
        uint32_t primary_input_attach = fdm1->fragmentDensityMapAttachment.attachment;
        uint32_t secondary_input_attach = fdm2->fragmentDensityMapAttachment.attachment;
        skip |= ValidateAttachmentCompatibility(type1_string, rp1_state, type2_string, rp2_state, primary_input_attach,
                                                secondary_input_attach, caller, error_code);
    } else if (fdm1) {
        skip |= LogInvalidPnextMessage(type1_string, rp1_state, type2_string, rp2_state,
                                       "The first uses a Fragment Density Map while the second one does not.", caller, error_code);
    } else if (fdm2) {
        skip |= LogInvalidPnextMessage(type1_string, rp1_state, type2_string, rp2_state,
                                       "The second uses a Fragment Density Map while the first one does not.", caller, error_code);
    }

    return skip;
}

bool CoreChecks::PreCallValidateDestroyRenderPass(VkDevice device, VkRenderPass renderPass,
                                                  const VkAllocationCallbacks *pAllocator) const {
    auto rp_state = Get<RENDER_PASS_STATE>(renderPass);
    bool skip = false;
    if (rp_state) {
        skip |= ValidateObjectNotInUse(rp_state.get(), "vkDestroyRenderPass", "VUID-vkDestroyRenderPass-renderPass-00873");
    }
    return skip;
}

// If this is a stencil format, make sure the stencil[Load|Store]Op flag is checked, while if it is a depth/color attachment the
// [load|store]Op flag must be checked
// TODO: The memory valid flag in DEVICE_MEMORY_STATE should probably be split to track the validity of stencil memory separately.
template <typename T>
static bool FormatSpecificLoadAndStoreOpSettings(VkFormat format, T color_depth_op, T stencil_op, T op) {
    if (color_depth_op != op && stencil_op != op) {
        return false;
    }
    const bool check_color_depth_load_op = !FormatIsStencilOnly(format);
    const bool check_stencil_load_op = FormatIsDepthAndStencil(format) || !check_color_depth_load_op;

    return ((check_color_depth_load_op && (color_depth_op == op)) || (check_stencil_load_op && (stencil_op == op)));
}

bool CoreChecks::ValidateCmdBeginRenderPass(VkCommandBuffer commandBuffer, RenderPassCreateVersion rp_version,
                                            const VkRenderPassBeginInfo *pRenderPassBegin, CMD_TYPE cmd_type) const {
    bool skip = false;
    auto cb_state = GetRead<CMD_BUFFER_STATE>(commandBuffer);
    const char *function_name = CommandTypeString(cmd_type);
    assert(cb_state);
    if (pRenderPassBegin) {
        auto rp_state = Get<RENDER_PASS_STATE>(pRenderPassBegin->renderPass);
        auto fb_state = Get<FRAMEBUFFER_STATE>(pRenderPassBegin->framebuffer);

        if (rp_state) {
            uint32_t clear_op_size = 0;  // Make sure pClearValues is at least as large as last LOAD_OP_CLEAR

            // Handle extension struct from EXT_sample_locations
            const VkRenderPassSampleLocationsBeginInfoEXT *sample_locations_begin_info =
                LvlFindInChain<VkRenderPassSampleLocationsBeginInfoEXT>(pRenderPassBegin->pNext);
            if (sample_locations_begin_info) {
                for (uint32_t i = 0; i < sample_locations_begin_info->attachmentInitialSampleLocationsCount; ++i) {
                    const VkAttachmentSampleLocationsEXT &sample_location =
                        sample_locations_begin_info->pAttachmentInitialSampleLocations[i];
                    skip |= ValidateSampleLocationsInfo(&sample_location.sampleLocationsInfo, function_name);
                    if (sample_location.attachmentIndex >= rp_state->createInfo.attachmentCount) {
                        skip |= LogError(device, "VUID-VkAttachmentSampleLocationsEXT-attachmentIndex-01531",
                                         "%s: Attachment index %u specified by attachment sample locations %u is greater than the "
                                         "attachment count of %u for the render pass being begun.",
                                         function_name, sample_location.attachmentIndex, i, rp_state->createInfo.attachmentCount);
                    }
                }

                for (uint32_t i = 0; i < sample_locations_begin_info->postSubpassSampleLocationsCount; ++i) {
                    const VkSubpassSampleLocationsEXT &sample_location =
                        sample_locations_begin_info->pPostSubpassSampleLocations[i];
                    skip |= ValidateSampleLocationsInfo(&sample_location.sampleLocationsInfo, function_name);
                    if (sample_location.subpassIndex >= rp_state->createInfo.subpassCount) {
                        skip |= LogError(
                            device, "VUID-VkSubpassSampleLocationsEXT-subpassIndex-01532",
                            "%s: Subpass index %u specified by subpass sample locations %u is greater than the subpass count "
                            "of %u for the render pass being begun.",
                            function_name, sample_location.subpassIndex, i, rp_state->createInfo.subpassCount);
                    }
                }
            }

            for (uint32_t i = 0; i < rp_state->createInfo.attachmentCount; ++i) {
                auto attachment = &rp_state->createInfo.pAttachments[i];
                if (FormatSpecificLoadAndStoreOpSettings(attachment->format, attachment->loadOp, attachment->stencilLoadOp,
                                                         VK_ATTACHMENT_LOAD_OP_CLEAR)) {
                    clear_op_size = static_cast<uint32_t>(i) + 1;

                    if (FormatHasDepth(attachment->format) && pRenderPassBegin->pClearValues) {
                        skip |= ValidateClearDepthStencilValue(commandBuffer, pRenderPassBegin->pClearValues[i].depthStencil,
                                                               function_name);
                    }
                }
            }

            if (clear_op_size > pRenderPassBegin->clearValueCount) {
                skip |=
                    LogError(rp_state->renderPass(), "VUID-VkRenderPassBeginInfo-clearValueCount-00902",
                             "In %s the VkRenderPassBeginInfo struct has a clearValueCount of %u but there "
                             "must be at least %u entries in pClearValues array to account for the highest index attachment in "
                             "%s that uses VK_ATTACHMENT_LOAD_OP_CLEAR is %u. Note that the pClearValues array is indexed by "
                             "attachment number so even if some pClearValues entries between 0 and %u correspond to attachments "
                             "that aren't cleared they will be ignored.",
                             function_name, pRenderPassBegin->clearValueCount, clear_op_size,
                             report_data->FormatHandle(rp_state->renderPass()).c_str(), clear_op_size, clear_op_size - 1);
            }
            skip |= VerifyFramebufferAndRenderPassImageViews(pRenderPassBegin, function_name);
            skip |= VerifyRenderAreaBounds(pRenderPassBegin, function_name);
            skip |= VerifyFramebufferAndRenderPassLayouts(rp_version, *cb_state, pRenderPassBegin, fb_state.get());
            if (fb_state->rp_state->renderPass() != rp_state->renderPass()) {
                skip |= ValidateRenderPassCompatibility("render pass", *rp_state.get(), "framebuffer", *fb_state->rp_state.get(),
                                                        function_name, "VUID-VkRenderPassBeginInfo-renderPass-00904");
            }

            skip |= ValidateDependencies(fb_state.get(), rp_state.get());

            skip |= ValidateCmd(*cb_state, cmd_type);
        }
    }

    auto chained_device_group_struct = LvlFindInChain<VkDeviceGroupRenderPassBeginInfo>(pRenderPassBegin->pNext);
    if (chained_device_group_struct) {
        const LogObjectList objlist(pRenderPassBegin->renderPass);
        skip |= ValidateDeviceMaskToPhysicalDeviceCount(chained_device_group_struct->deviceMask, objlist,
                                                        "VUID-VkDeviceGroupRenderPassBeginInfo-deviceMask-00905");
        skip |= ValidateDeviceMaskToZero(chained_device_group_struct->deviceMask, objlist,
                                         "VUID-VkDeviceGroupRenderPassBeginInfo-deviceMask-00906");
        skip |= ValidateDeviceMaskToCommandBuffer(*cb_state, chained_device_group_struct->deviceMask, objlist,
                                                  "VUID-VkDeviceGroupRenderPassBeginInfo-deviceMask-00907");

        if (chained_device_group_struct->deviceRenderAreaCount != 0 &&
            chained_device_group_struct->deviceRenderAreaCount != physical_device_count) {
            skip |= LogError(objlist, "VUID-VkDeviceGroupRenderPassBeginInfo-deviceRenderAreaCount-00908",
                             "%s: deviceRenderAreaCount[%" PRIu32 "] is invaild. Physical device count is %" PRIu32 ".",
                             function_name, chained_device_group_struct->deviceRenderAreaCount, physical_device_count);
        }
    }
    return skip;
}

bool CoreChecks::PreCallValidateCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                                   VkSubpassContents contents) const {
    bool skip = ValidateCmdBeginRenderPass(commandBuffer, RENDER_PASS_VERSION_1, pRenderPassBegin, CMD_BEGINRENDERPASS);
    return skip;
}

bool CoreChecks::PreCallValidateCmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                                       const VkSubpassBeginInfo *pSubpassBeginInfo) const {
    bool skip = ValidateCmdBeginRenderPass(commandBuffer, RENDER_PASS_VERSION_2, pRenderPassBegin, CMD_BEGINRENDERPASS2KHR);
    return skip;
}

bool CoreChecks::PreCallValidateCmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                                    const VkSubpassBeginInfo *pSubpassBeginInfo) const {
    bool skip = ValidateCmdBeginRenderPass(commandBuffer, RENDER_PASS_VERSION_2, pRenderPassBegin, CMD_BEGINRENDERPASS2);
    return skip;
}

void CoreChecks::RecordCmdBeginRenderPassLayouts(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                                 const VkSubpassContents contents) {
    if (!pRenderPassBegin) {
        return;
    }
    auto cb_state = GetWrite<CMD_BUFFER_STATE>(commandBuffer);
    auto render_pass_state = Get<RENDER_PASS_STATE>(pRenderPassBegin->renderPass);
    auto framebuffer = Get<FRAMEBUFFER_STATE>(pRenderPassBegin->framebuffer);
    if (render_pass_state) {
        // transition attachments to the correct layouts for beginning of renderPass and first subpass
        TransitionBeginRenderPassLayouts(cb_state.get(), render_pass_state.get(), framebuffer.get());
    }
}

void CoreChecks::PreCallRecordCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                                 VkSubpassContents contents) {
    StateTracker::PreCallRecordCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
    RecordCmdBeginRenderPassLayouts(commandBuffer, pRenderPassBegin, contents);
}

void CoreChecks::PreCallRecordCmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                                     const VkSubpassBeginInfo *pSubpassBeginInfo) {
    StateTracker::PreCallRecordCmdBeginRenderPass2KHR(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
    RecordCmdBeginRenderPassLayouts(commandBuffer, pRenderPassBegin, pSubpassBeginInfo->contents);
}

void CoreChecks::PreCallRecordCmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                                  const VkSubpassBeginInfo *pSubpassBeginInfo) {
    StateTracker::PreCallRecordCmdBeginRenderPass2(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
    RecordCmdBeginRenderPassLayouts(commandBuffer, pRenderPassBegin, pSubpassBeginInfo->contents);
}

bool CoreChecks::ValidateCmdEndRenderPass(RenderPassCreateVersion rp_version, VkCommandBuffer commandBuffer, CMD_TYPE cmd_type,
                                          const VkSubpassEndInfo *pSubpassEndInfo) const {
    auto cb_state = GetRead<CMD_BUFFER_STATE>(commandBuffer);
    assert(cb_state);
    bool skip = false;
    const bool use_rp2 = (rp_version == RENDER_PASS_VERSION_2);
    const char *vuid;
    const char *function_name = CommandTypeString(cmd_type);

    RENDER_PASS_STATE *rp_state = cb_state->activeRenderPass.get();
    if (rp_state) {
        const VkRenderPassCreateInfo2 *rpci = rp_state->createInfo.ptr();
        if (!rp_state->UsesDynamicRendering() && (cb_state->activeSubpass != rp_state->createInfo.subpassCount - 1)) {
            vuid = use_rp2 ? "VUID-vkCmdEndRenderPass2-None-03103" : "VUID-vkCmdEndRenderPass-None-00910";
            skip |= LogError(commandBuffer, vuid, "%s: Called before reaching final subpass.", function_name);
        }

        if (rp_state->UsesDynamicRendering()) {
            vuid = use_rp2 ? "VUID-vkCmdEndRenderPass2-None-06171" : "VUID-vkCmdEndRenderPass-None-06170";
            skip |= LogError(commandBuffer, vuid, "%s: Called when the render pass instance was begun with %s().", function_name,
                             cb_state->begin_rendering_func_name.c_str());
        }

        if (pSubpassEndInfo && pSubpassEndInfo->pNext) {
            const auto *fdm_offset_info = LvlFindInChain<VkSubpassFragmentDensityMapOffsetEndInfoQCOM>(pSubpassEndInfo->pNext);
            if (fdm_offset_info != nullptr) {
                if (fdm_offset_info->fragmentDensityOffsetCount != 0) {
                    if ((!enabled_features.fragment_density_map_offset_features.fragmentDensityMapOffset) ||
                        (!enabled_features.fragment_density_map_features.fragmentDensityMap)) {
                        skip |=
                            LogError(device, "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-fragmentDensityMapOffsets-06503",
                                     "%s(): fragmentDensityOffsetCount is %" PRIu32 " but must be 0 when feature is not enabled.",
                                     function_name, fdm_offset_info->fragmentDensityOffsetCount);
                    }

                    bool fdm_non_zero_offsets = false;
                    for (uint32_t k = 0; k < fdm_offset_info->fragmentDensityOffsetCount; k++) {
                        if ((fdm_offset_info->pFragmentDensityOffsets[k].x != 0) ||
                            (fdm_offset_info->pFragmentDensityOffsets[k].y != 0)) {
                            fdm_non_zero_offsets = true;
                            uint32_t width =
                                phys_dev_ext_props.fragment_density_map_offset_props.fragmentDensityOffsetGranularity.width;
                            uint32_t height =
                                phys_dev_ext_props.fragment_density_map_offset_props.fragmentDensityOffsetGranularity.height;

                            if (SafeModulo(fdm_offset_info->pFragmentDensityOffsets[k].x, width) != 0) {
                                skip |=
                                    LogError(device, "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-x-06512",
                                             "%s(): X component in fragmentDensityOffsets[%u] (%" PRIu32
                                             ") is"
                                             " not an integer multiple of fragmentDensityOffsetGranularity.width (%" PRIu32 ").",
                                             function_name, k, fdm_offset_info->pFragmentDensityOffsets[k].x, width);
                            }

                            if (SafeModulo(fdm_offset_info->pFragmentDensityOffsets[k].y, height) != 0) {
                                skip |=
                                    LogError(device, "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-y-06513",
                                             "%s(): Y component in fragmentDensityOffsets[%u] (%" PRIu32
                                             ") is"
                                             " not an integer multiple of fragmentDensityOffsetGranularity.height (%" PRIu32 ").",
                                             function_name, k, fdm_offset_info->pFragmentDensityOffsets[k].y, height);
                            }
                        }
                    }

                    const VkImageView *image_views = cb_state->activeFramebuffer.get()->createInfo.pAttachments;
                    for (uint32_t i = 0; i < rpci->attachmentCount; ++i) {
                        auto view_state = Get<IMAGE_VIEW_STATE>(image_views[i]);
                        const auto &ici = view_state->image_state->createInfo;

                        if ((fdm_non_zero_offsets == true) &&
                            ((ici.flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) == 0)) {
                            skip |= LogError(device, "VUID-VkFramebufferCreateInfo-renderPass-06502",
                                             "%s(): Attachment #%" PRIu32
                                             " is not created with flag value"
                                             " VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM and renderPass"
                                             " uses non-zero fdm offsets.",
                                             function_name, i);
                        }

                        // fdm attachment
                        const auto *fdm_attachment = LvlFindInChain<VkRenderPassFragmentDensityMapCreateInfoEXT>(rpci->pNext);
                        const VkSubpassDescription2 &subpass = rpci->pSubpasses[cb_state->activeSubpass];
                        if (fdm_attachment && fdm_attachment->fragmentDensityMapAttachment.attachment != VK_ATTACHMENT_UNUSED) {
                            if (fdm_attachment->fragmentDensityMapAttachment.attachment == i) {
                                if ((ici.flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) == 0) {
                                    skip |= LogError(
                                        device,
                                        "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-fragmentDensityMapAttachment-06504",
                                        "%s(): Fragment density map attachment #%" PRIu32
                                        " is not created with"
                                        " VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM and fragmentDensityOffsetCount"
                                        " is %" PRIu32 " but must be 0 due to missing fragmentDensityOffset feature bit.",
                                        function_name, i, fdm_offset_info->fragmentDensityOffsetCount);
                                }

                                if ((subpass.viewMask != 0) && (view_state->create_info.subresourceRange.layerCount !=
                                                                fdm_offset_info->fragmentDensityOffsetCount)) {
                                    skip |= LogError(
                                        device,
                                        "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-fragmentDensityOffsetCount-06510",
                                        "%s(): fragmentDensityOffsetCount %" PRIu32
                                        " does not match the fragment density map attachment (%" PRIu32
                                        ") view layer count (%" PRIu32 ").",
                                        function_name, fdm_offset_info->fragmentDensityOffsetCount, i,
                                        view_state->create_info.subresourceRange.layerCount);
                                }

                                if ((subpass.viewMask == 0) && (fdm_offset_info->fragmentDensityOffsetCount != 1)) {
                                    skip |= LogError(
                                        device,
                                        "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-fragmentDensityOffsetCount-06511",
                                        "%s(): fragmentDensityOffsetCount %" PRIu32 " should be 1 when multiview is not enabled.",
                                        function_name, fdm_offset_info->fragmentDensityOffsetCount);
                                }
                            }
                        }

                        // depth stencil attachment
                        if ((subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) &&
                            (subpass.pDepthStencilAttachment->attachment == i) &&
                            ((ici.flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) == 0)) {
                            skip |=
                                LogError(device, "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-pDepthStencilAttachment-06505",
                                         "%s(): Depth/Stencil attachment #%" PRIu32
                                         " is not created with"
                                         " VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM and fragmentDensityOffsetCount"
                                         " is %" PRIu32 " but must be 0 due to missing fragmentDensityOffset feature bit.",
                                         function_name, i, fdm_offset_info->fragmentDensityOffsetCount);
                        }

                        // input attachments
                        for (uint32_t k = 0; k < subpass.inputAttachmentCount; k++) {
                            const auto attachment = subpass.pInputAttachments[k].attachment;
                            if ((attachment != VK_ATTACHMENT_UNUSED) && (attachment == i) &&
                                ((ici.flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) == 0)) {
                                skip |=
                                    LogError(device, "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-pInputAttachments-06506",
                                             "%s(): Input attachment #%" PRIu32
                                             " is not created with"
                                             " VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM and fragmentDensityOffsetCount"
                                             " is %" PRIu32 " but must be 0 due to missing fragmentDensityOffset feature bit.",
                                             function_name, i, fdm_offset_info->fragmentDensityOffsetCount);
                            }
                        }

                        // color attachments
                        for (uint32_t k = 0; k < subpass.colorAttachmentCount; k++) {
                            const auto attachment = subpass.pColorAttachments[k].attachment;
                            if ((attachment != VK_ATTACHMENT_UNUSED) && (attachment == i) &&
                                ((ici.flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) == 0)) {
                                skip |=
                                    LogError(device, "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-pColorAttachments-06507",
                                             "%s(): Color attachment #%" PRIu32
                                             " is not created with"
                                             " VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM and fragmentDensityOffsetCount"
                                             " is %" PRIu32 " but must be 0 due to missing fragmentDensityOffset feature bit.",
                                             function_name, i, fdm_offset_info->fragmentDensityOffsetCount);
                            }
                        }

                        // Resolve attachments
                        for (uint32_t k = 0; k < subpass.colorAttachmentCount; k++) {
                            const auto attachment = subpass.pResolveAttachments[k].attachment;
                            if ((attachment != VK_ATTACHMENT_UNUSED) && (attachment == i) &&
                                ((ici.flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) == 0)) {
                                skip |=
                                    LogError(device, "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-pResolveAttachments-06508",
                                             "%s(): Resolve attachment #%" PRIu32
                                             " is not created with"
                                             " VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM and fragmentDensityOffsetCount"
                                             " is %" PRIu32 " but must be 0 due to missing fragmentDensityOffset feature bit.",
                                             function_name, i, fdm_offset_info->fragmentDensityOffsetCount);
                            }
                        }

                        // Preserve attachments
                        for (uint32_t k = 0; k < subpass.preserveAttachmentCount; k++) {
                            const auto attachment = subpass.pPreserveAttachments[k];
                            if ((attachment != VK_ATTACHMENT_UNUSED) && (attachment == i) &&
                                ((ici.flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) == 0)) {
                                skip |=
                                    LogError(device, "VUID-VkSubpassFragmentDensityMapOffsetEndInfoQCOM-pPreserveAttachments-06509",
                                             "%s(): Preserve attachment #%" PRIu32
                                             " is not created with"
                                             " VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM and fragmentDensityOffsetCount"
                                             " is %" PRIu32 " but must be 0 due to missing fragmentDensityOffset feature bit.",
                                             function_name, i, fdm_offset_info->fragmentDensityOffsetCount);
                            }
                        }
                    }
                }
            }
        }
    }

    if (cb_state->transform_feedback_active) {
        vuid = use_rp2 ? "VUID-vkCmdEndRenderPass2-None-02352" : "VUID-vkCmdEndRenderPass-None-02351";
        skip |= LogError(device, vuid, "%s(): transform feedback is active.", function_name);
    }

    skip |= ValidateCmd(*cb_state, cmd_type);
    return skip;
}

bool CoreChecks::PreCallValidateCmdEndRenderPass(VkCommandBuffer commandBuffer) const {
    bool skip = ValidateCmdEndRenderPass(RENDER_PASS_VERSION_1, commandBuffer, CMD_ENDRENDERPASS, VK_NULL_HANDLE);
    return skip;
}

bool CoreChecks::PreCallValidateCmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo) const {
    bool skip = ValidateCmdEndRenderPass(RENDER_PASS_VERSION_2, commandBuffer, CMD_ENDRENDERPASS2KHR, pSubpassEndInfo);
    return skip;
}

bool CoreChecks::PreCallValidateCmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo) const {
    bool skip = ValidateCmdEndRenderPass(RENDER_PASS_VERSION_2, commandBuffer, CMD_ENDRENDERPASS2, pSubpassEndInfo);
    return skip;
}

void CoreChecks::RecordCmdEndRenderPassLayouts(VkCommandBuffer commandBuffer) {
    auto cb_state = GetWrite<CMD_BUFFER_STATE>(commandBuffer);
    TransitionFinalSubpassLayouts(cb_state.get(), cb_state->activeRenderPassBeginInfo.ptr(), cb_state->activeFramebuffer.get());
}

void CoreChecks::PostCallRecordCmdEndRenderPass(VkCommandBuffer commandBuffer) {
    // Record the end at the CoreLevel to ensure StateTracker cleanup doesn't step on anything we need.
    RecordCmdEndRenderPassLayouts(commandBuffer);
    StateTracker::PostCallRecordCmdEndRenderPass(commandBuffer);
}

void CoreChecks::PostCallRecordCmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo) {
    // Record the end at the CoreLevel to ensure StateTracker cleanup doesn't step on anything we need.
    RecordCmdEndRenderPassLayouts(commandBuffer);
    StateTracker::PostCallRecordCmdEndRenderPass2KHR(commandBuffer, pSubpassEndInfo);
}

void CoreChecks::PostCallRecordCmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo) {
    RecordCmdEndRenderPassLayouts(commandBuffer);
    StateTracker::PostCallRecordCmdEndRenderPass2(commandBuffer, pSubpassEndInfo);
}

bool CoreChecks::VerifyRenderAreaBounds(const VkRenderPassBeginInfo *pRenderPassBegin, const char *func_name) const {
    bool skip = false;

    bool device_group = false;
    uint32_t device_group_area_count = 0;
    const VkDeviceGroupRenderPassBeginInfo *device_group_render_pass_begin_info =
        LvlFindInChain<VkDeviceGroupRenderPassBeginInfo>(pRenderPassBegin->pNext);
    if (IsExtEnabled(device_extensions.vk_khr_device_group)) {
        device_group = true;
        if (device_group_render_pass_begin_info) {
            device_group_area_count = device_group_render_pass_begin_info->deviceRenderAreaCount;
        }
    }
    auto framebuffer_state = Get<FRAMEBUFFER_STATE>(pRenderPassBegin->framebuffer);
    const auto *framebuffer_info = &framebuffer_state->createInfo;
    if (device_group && device_group_area_count > 0) {
        for (uint32_t i = 0; i < device_group_render_pass_begin_info->deviceRenderAreaCount; ++i) {
            const auto &deviceRenderArea = device_group_render_pass_begin_info->pDeviceRenderAreas[i];
            if (deviceRenderArea.offset.x < 0) {
                skip |= LogError(pRenderPassBegin->renderPass, "VUID-VkDeviceGroupRenderPassBeginInfo-offset-06166",
                                 "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer, "
                                 "VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32 "].offset.x is negative (%" PRIi32
                                 ").",
                                 func_name, i, deviceRenderArea.offset.x);
            }
            if (deviceRenderArea.offset.y < 0) {
                skip |= LogError(pRenderPassBegin->renderPass, "VUID-VkDeviceGroupRenderPassBeginInfo-offset-06167",
                                 "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer, "
                                 "VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32 "].offset.y is negative (%" PRIi32
                                 ").",
                                 func_name, i, deviceRenderArea.offset.y);
            }
            if ((deviceRenderArea.offset.x + deviceRenderArea.extent.width) > framebuffer_info->width) {
                skip |= LogError(pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-pNext-02856",
                                 "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer, "
                                 "VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32 "] offset.x (%" PRIi32
                                 ") + extent.width (%" PRIi32 ") is greater than framebuffer width (%" PRIi32 ").",
                                 func_name, i, deviceRenderArea.offset.x, deviceRenderArea.extent.width, framebuffer_info->width);
            }
            if ((deviceRenderArea.offset.y + deviceRenderArea.extent.height) > framebuffer_info->height) {
                skip |= LogError(pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-pNext-02857",
                                 "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer, "
                                 "VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32 "] offset.y (%" PRIi32
                                 ") + extent.height (%" PRIi32 ") is greater than framebuffer height (%" PRIi32 ").",
                                 func_name, i, deviceRenderArea.offset.y, deviceRenderArea.extent.height, framebuffer_info->height);
            }
        }
    } else {
        if (pRenderPassBegin->renderArea.offset.x < 0) {
            if (device_group) {
                skip |=
                    LogError(pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-pNext-02850",
                             "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer and pNext "
                             "of VkRenderPassBeginInfo does not contain VkDeviceGroupRenderPassBeginInfo or its "
                             "deviceRenderAreaCount is 0, renderArea.offset.x is negative (%" PRIi32 ") .",
                             func_name, pRenderPassBegin->renderArea.offset.x);
            } else {
                skip |= LogError(pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-renderArea-02846",
                                 "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer, "
                                 "renderArea.offset.x is negative (%" PRIi32 ") .",
                                 func_name, pRenderPassBegin->renderArea.offset.x);
            }
        }
        if (pRenderPassBegin->renderArea.offset.y < 0) {
            if (device_group) {
                skip |=
                    LogError(pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-pNext-02851",
                             "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer and pNext "
                             "of VkRenderPassBeginInfo does not contain VkDeviceGroupRenderPassBeginInfo or its "
                             "deviceRenderAreaCount is 0, renderArea.offset.y is negative (%" PRIi32 ") .",
                             func_name, pRenderPassBegin->renderArea.offset.y);
            } else {
                skip |= LogError(pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-renderArea-02847",
                                 "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer, "
                                 "renderArea.offset.y is negative (%" PRIi32 ") .",
                                 func_name, pRenderPassBegin->renderArea.offset.y);
            }
        }

        const auto x_adjusted_extent = static_cast<int64_t>(pRenderPassBegin->renderArea.offset.x) +
                                       static_cast<int64_t>(pRenderPassBegin->renderArea.extent.width);
        if (x_adjusted_extent > static_cast<int64_t>(framebuffer_info->width)) {
            if (device_group) {
                skip |=
                    LogError(pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-pNext-02852",
                             "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer and pNext "
                             "of VkRenderPassBeginInfo does not contain VkDeviceGroupRenderPassBeginInfo or its "
                             "deviceRenderAreaCount is 0, renderArea.offset.x (%" PRIi32 ") + renderArea.extent.width (%" PRIi32
                             ") is greater than framebuffer width (%" PRIi32 ").",
                             func_name, pRenderPassBegin->renderArea.offset.x, pRenderPassBegin->renderArea.extent.width,
                             framebuffer_info->width);
            } else {
                skip |= LogError(
                    pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-renderArea-02848",
                    "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer, renderArea.offset.x "
                    "(%" PRIi32 ") + renderArea.extent.width (%" PRIi32 ") is greater than framebuffer width (%" PRIi32 ").",
                    func_name, pRenderPassBegin->renderArea.offset.x, pRenderPassBegin->renderArea.extent.width,
                    framebuffer_info->width);
            }
        }

        const auto y_adjusted_extent = static_cast<int64_t>(pRenderPassBegin->renderArea.offset.y) +
                                       static_cast<int64_t>(pRenderPassBegin->renderArea.extent.height);
        if (y_adjusted_extent > static_cast<int64_t>(framebuffer_info->height)) {
            if (device_group) {
                skip |=
                    LogError(pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-pNext-02853",
                             "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer and pNext "
                             "of VkRenderPassBeginInfo does not contain VkDeviceGroupRenderPassBeginInfo or its "
                             "deviceRenderAreaCount is 0, renderArea.offset.y (%" PRIi32 ") + renderArea.extent.height (%" PRIi32
                             ") is greater than framebuffer height (%" PRIi32 ").",
                             func_name, pRenderPassBegin->renderArea.offset.y, pRenderPassBegin->renderArea.extent.height,
                             framebuffer_info->height);
            } else {
                skip |= LogError(
                    pRenderPassBegin->renderPass, "VUID-VkRenderPassBeginInfo-renderArea-02849",
                    "%s: Cannot execute a render pass with renderArea not within the bound of the framebuffer, renderArea.offset.y "
                    "(%" PRIi32 ") + renderArea.extent.height (%" PRIi32 ") is greater than framebuffer height (%" PRIi32 ").",
                    func_name, pRenderPassBegin->renderArea.offset.y, pRenderPassBegin->renderArea.extent.height,
                    framebuffer_info->height);
            }
        }
    }
    return skip;
}

bool CoreChecks::VerifyFramebufferAndRenderPassImageViews(const VkRenderPassBeginInfo *pRenderPassBeginInfo,
                                                          const char *func_name) const {
    bool skip = false;
    const VkRenderPassAttachmentBeginInfo *render_pass_attachment_begin_info =
        LvlFindInChain<VkRenderPassAttachmentBeginInfo>(pRenderPassBeginInfo->pNext);

    if (render_pass_attachment_begin_info && render_pass_attachment_begin_info->attachmentCount != 0) {
        auto framebuffer_state = Get<FRAMEBUFFER_STATE>(pRenderPassBeginInfo->framebuffer);
        const auto *framebuffer_create_info = &framebuffer_state->createInfo;
        const VkFramebufferAttachmentsCreateInfo *framebuffer_attachments_create_info =
            LvlFindInChain<VkFramebufferAttachmentsCreateInfo>(framebuffer_create_info->pNext);
        if ((framebuffer_create_info->flags & VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT) == 0) {
            skip |= LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03207",
                             "%s: Image views specified at render pass begin, but framebuffer not created with "
                             "VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT",
                             func_name);
        } else if (framebuffer_attachments_create_info) {
            if (framebuffer_attachments_create_info->attachmentImageInfoCount !=
                render_pass_attachment_begin_info->attachmentCount) {
                skip |= LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03208",
                                 "%s: %u image views specified at render pass begin, but framebuffer "
                                 "created expecting %u attachments",
                                 func_name, render_pass_attachment_begin_info->attachmentCount,
                                 framebuffer_attachments_create_info->attachmentImageInfoCount);
            } else {
                auto render_pass_state = Get<RENDER_PASS_STATE>(pRenderPassBeginInfo->renderPass);
                const auto *render_pass_create_info = &render_pass_state->createInfo;
                for (uint32_t i = 0; i < render_pass_attachment_begin_info->attachmentCount; ++i) {
                    auto image_view_state = Get<IMAGE_VIEW_STATE>(render_pass_attachment_begin_info->pAttachments[i]);
                    const VkImageViewCreateInfo *image_view_create_info = &image_view_state->create_info;
                    const auto &subresource_range = image_view_state->normalized_subresource_range;
                    const VkFramebufferAttachmentImageInfo *framebuffer_attachment_image_info =
                        &framebuffer_attachments_create_info->pAttachmentImageInfos[i];
                    const auto *image_create_info = &image_view_state->image_state->createInfo;

                    if (framebuffer_attachment_image_info->flags != image_create_info->flags) {
                        skip |= LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03209",
                                         "%s: Image view #%u created from an image with flags set as 0x%X, "
                                         "but image info #%u used to create the framebuffer had flags set as 0x%X",
                                         func_name, i, image_create_info->flags, i, framebuffer_attachment_image_info->flags);
                    }

                    if (framebuffer_attachment_image_info->usage != image_view_state->inherited_usage) {
                        // Give clearer message if this error is due to the "inherited" part or not
                        if (image_create_info->usage == image_view_state->inherited_usage) {
                            skip |= LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-04627",
                                             "%s: Image view #%" PRIu32
                                             " created from an image with usage set as (%s), "
                                             "but image info #%" PRIu32 " used to create the framebuffer had usage set as (%s).",
                                             func_name, i, string_VkImageUsageFlags(image_create_info->usage).c_str(), i,
                                             string_VkImageUsageFlags(framebuffer_attachment_image_info->usage).c_str());
                        } else {
                            skip |=
                                LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-04627",
                                         "%s: Image view #%" PRIu32
                                         " created from an image with usage set as (%s) but using "
                                         "VkImageViewUsageCreateInfo the inherited usage is the subset (%s) "
                                         "and the image info #%" PRIu32 " used to create the framebuffer had usage set as (%s).",
                                         func_name, i, string_VkImageUsageFlags(image_create_info->usage).c_str(),
                                         string_VkImageUsageFlags(image_view_state->inherited_usage).c_str(), i,
                                         string_VkImageUsageFlags(framebuffer_attachment_image_info->usage).c_str());
                        }
                    }

                    const auto view_width = std::max(1u, image_create_info->extent.width >> subresource_range.baseMipLevel);
                    if (framebuffer_attachment_image_info->width != view_width) {
                        skip |= LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03211",
                                         "%s: For VkRenderPassAttachmentBeginInfo::pAttachments[%" PRIu32
                                         "], VkImageView width (%" PRIu32 ") at mip level %" PRIu32 " (%" PRIu32
                                         ") != VkFramebufferAttachmentsCreateInfo::pAttachments[%" PRIu32 "]::width (%" PRIu32 ").",
                                         func_name, i, image_create_info->extent.width, subresource_range.baseMipLevel, view_width,
                                         i, framebuffer_attachment_image_info->width);
                    }

                    const bool is_1d = (image_view_create_info->viewType == VK_IMAGE_VIEW_TYPE_1D) ||
                                       (image_view_create_info->viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY);
                    const auto view_height = (!is_1d)
                                                 ? std::max(1u, image_create_info->extent.height >> subresource_range.baseMipLevel)
                                                 : image_create_info->extent.height;
                    if (framebuffer_attachment_image_info->height != view_height) {
                        skip |=
                            LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03212",
                                     "%s: For VkRenderPassAttachmentBeginInfo::pAttachments[%" PRIu32
                                     "], VkImageView height (%" PRIu32 ") at mip level %" PRIu32 " (%" PRIu32
                                     ") != VkFramebufferAttachmentsCreateInfo::pAttachments[%" PRIu32 "]::height (%" PRIu32 ").",
                                     func_name, i, image_create_info->extent.height, subresource_range.baseMipLevel, view_height, i,
                                     framebuffer_attachment_image_info->height);
                    }

                    const uint32_t layerCount =
                        image_view_state->create_info.subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS
                            ? image_view_state->create_info.subresourceRange.layerCount
                            : image_create_info->extent.depth;
                    if (framebuffer_attachment_image_info->layerCount != layerCount) {
                        skip |= LogError(
                            pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03213",
                            "%s: Image view #%" PRIu32 " created with a subresource range with a layerCount of %" PRIu32
                            ", but image info #%" PRIu32 " used to create the framebuffer had layerCount set as %" PRIu32 "",
                            func_name, i, layerCount, i, framebuffer_attachment_image_info->layerCount);
                    }

                    const VkImageFormatListCreateInfo *image_format_list_create_info =
                        LvlFindInChain<VkImageFormatListCreateInfo>(image_create_info->pNext);
                    if (image_format_list_create_info) {
                        if (image_format_list_create_info->viewFormatCount != framebuffer_attachment_image_info->viewFormatCount) {
                            skip |= LogError(
                                pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03214",
                                "VkRenderPassBeginInfo: Image view #%u created with an image with a viewFormatCount of %u, "
                                "but image info #%u used to create the framebuffer had viewFormatCount set as %u",
                                i, image_format_list_create_info->viewFormatCount, i,
                                framebuffer_attachment_image_info->viewFormatCount);
                        }

                        for (uint32_t j = 0; j < image_format_list_create_info->viewFormatCount; ++j) {
                            bool format_found = false;
                            for (uint32_t k = 0; k < framebuffer_attachment_image_info->viewFormatCount; ++k) {
                                if (image_format_list_create_info->pViewFormats[j] ==
                                    framebuffer_attachment_image_info->pViewFormats[k]) {
                                    format_found = true;
                                }
                            }
                            if (!format_found) {
                                skip |= LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03215",
                                                 "VkRenderPassBeginInfo: Image view #%u created with an image including the format "
                                                 "%s in its view format list, "
                                                 "but image info #%u used to create the framebuffer does not include this format",
                                                 i, string_VkFormat(image_format_list_create_info->pViewFormats[j]), i);
                            }
                        }
                    }

                    if (render_pass_create_info->pAttachments[i].format != image_view_create_info->format) {
                        skip |= LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03216",
                                         "%s: Image view #%u created with a format of %s, "
                                         "but render pass attachment description #%u created with a format of %s",
                                         func_name, i, string_VkFormat(image_view_create_info->format), i,
                                         string_VkFormat(render_pass_create_info->pAttachments[i].format));
                    }

                    if (render_pass_create_info->pAttachments[i].samples != image_create_info->samples) {
                        skip |= LogError(pRenderPassBeginInfo->renderPass, "VUID-VkRenderPassBeginInfo-framebuffer-03217",
                                         "%s: Image view #%u created with an image with %s samples, "
                                         "but render pass attachment description #%u created with %s samples",
                                         func_name, i, string_VkSampleCountFlagBits(image_create_info->samples), i,
                                         string_VkSampleCountFlagBits(render_pass_create_info->pAttachments[i].samples));
                    }

                    if (subresource_range.levelCount != 1) {
                        skip |= LogError(render_pass_attachment_begin_info->pAttachments[i],
                                         "VUID-VkRenderPassAttachmentBeginInfo-pAttachments-03218",
                                         "%s: Image view #%u created with multiple (%u) mip levels.", func_name, i,
                                         subresource_range.levelCount);
                    }

                    if (IsIdentitySwizzle(image_view_create_info->components) == false) {
                        skip |= LogError(
                            render_pass_attachment_begin_info->pAttachments[i],
                            "VUID-VkRenderPassAttachmentBeginInfo-pAttachments-03219",
                            "%s: Image view #%u created with non-identity swizzle. All "
                            "framebuffer attachments must have been created with the identity swizzle. Here are the actual "
                            "swizzle values:\n"
                            "r swizzle = %s\n"
                            "g swizzle = %s\n"
                            "b swizzle = %s\n"
                            "a swizzle = %s\n",
                            func_name, i, string_VkComponentSwizzle(image_view_create_info->components.r),
                            string_VkComponentSwizzle(image_view_create_info->components.g),
                            string_VkComponentSwizzle(image_view_create_info->components.b),
                            string_VkComponentSwizzle(image_view_create_info->components.a));
                    }

                    if (image_view_create_info->viewType == VK_IMAGE_VIEW_TYPE_3D) {
                        skip |= LogError(render_pass_attachment_begin_info->pAttachments[i],
                                         "VUID-VkRenderPassAttachmentBeginInfo-pAttachments-04114",
                                         "%s: Image view #%u created with type VK_IMAGE_VIEW_TYPE_3D", func_name, i);
                    }
                }
            }
        }
    }

    return skip;
}

static bool FindDependency(const uint32_t index, const uint32_t dependent, const std::vector<DAGNode> &subpass_to_node,
                           layer_data::unordered_set<uint32_t> &processed_nodes) {
    // If we have already checked this node we have not found a dependency path so return false.
    if (processed_nodes.count(index)) return false;
    processed_nodes.insert(index);
    const DAGNode &node = subpass_to_node[index];
    // Look for a dependency path. If one exists return true else recurse on the previous nodes.
    if (std::find(node.prev.begin(), node.prev.end(), dependent) == node.prev.end()) {
        for (auto elem : node.prev) {
            if (FindDependency(elem, dependent, subpass_to_node, processed_nodes)) return true;
        }
    } else {
        return true;
    }
    return false;
}

bool CoreChecks::CheckDependencyExists(const VkRenderPass renderpass, const uint32_t subpass, const VkImageLayout layout,
                                       const std::vector<SubpassLayout> &dependent_subpasses,
                                       const std::vector<DAGNode> &subpass_to_node, bool &skip) const {
    bool result = true;
    const bool b_image_layout_read_only = IsImageLayoutReadOnly(layout);
    // Loop through all subpasses that share the same attachment and make sure a dependency exists
    for (uint32_t k = 0; k < dependent_subpasses.size(); ++k) {
        const SubpassLayout &sp = dependent_subpasses[k];
        if (subpass == sp.index) continue;
        if (b_image_layout_read_only && IsImageLayoutReadOnly(sp.layout)) continue;

        const DAGNode &node = subpass_to_node[subpass];
        // Check for a specified dependency between the two nodes. If one exists we are done.
        auto prev_elem = std::find(node.prev.begin(), node.prev.end(), sp.index);
        auto next_elem = std::find(node.next.begin(), node.next.end(), sp.index);
        if (prev_elem == node.prev.end() && next_elem == node.next.end()) {
            // If no dependency exits an implicit dependency still might. If not, throw an error.
            layer_data::unordered_set<uint32_t> processed_nodes;
            if (!(FindDependency(subpass, sp.index, subpass_to_node, processed_nodes) ||
                  FindDependency(sp.index, subpass, subpass_to_node, processed_nodes))) {
                skip |=
                    LogError(renderpass, kVUID_Core_DrawState_InvalidRenderpass,
                             "A dependency between subpasses %d and %d must exist but one is not specified.", subpass, sp.index);
                result = false;
            }
        }
    }
    return result;
}

bool CoreChecks::CheckPreserved(const VkRenderPass renderpass, const VkRenderPassCreateInfo2 *pCreateInfo, const int index,
                                const uint32_t attachment, const std::vector<DAGNode> &subpass_to_node, int depth,
                                bool &skip) const {
    const DAGNode &node = subpass_to_node[index];
    // If this node writes to the attachment return true as next nodes need to preserve the attachment.
    const VkSubpassDescription2 &subpass = pCreateInfo->pSubpasses[index];
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        if (attachment == subpass.pColorAttachments[j].attachment) return true;
    }
    for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
        if (attachment == subpass.pInputAttachments[j].attachment) return true;
    }
    if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
        if (attachment == subpass.pDepthStencilAttachment->attachment) return true;
    }
    bool result = false;
    // Loop through previous nodes and see if any of them write to the attachment.
    for (auto elem : node.prev) {
        result |= CheckPreserved(renderpass, pCreateInfo, elem, attachment, subpass_to_node, depth + 1, skip);
    }
    // If the attachment was written to by a previous node than this node needs to preserve it.
    if (result && depth > 0) {
        bool has_preserved = false;
        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            if (subpass.pPreserveAttachments[j] == attachment) {
                has_preserved = true;
                break;
            }
        }
        if (!has_preserved) {
            skip |= LogError(renderpass, kVUID_Core_DrawState_InvalidRenderpass,
                             "Attachment %d is used by a later subpass and must be preserved in subpass %d.", attachment, index);
        }
    }
    return result;
}

template <class T>
bool IsRangeOverlapping(T offset1, T size1, T offset2, T size2) {
    return (((offset1 + size1) > offset2) && ((offset1 + size1) < (offset2 + size2))) ||
           ((offset1 > offset2) && (offset1 < (offset2 + size2)));
}

bool IsRegionOverlapping(VkImageSubresourceRange range1, VkImageSubresourceRange range2) {
    return (IsRangeOverlapping(range1.baseMipLevel, range1.levelCount, range2.baseMipLevel, range2.levelCount) &&
            IsRangeOverlapping(range1.baseArrayLayer, range1.layerCount, range2.baseArrayLayer, range2.layerCount));
}

bool CoreChecks::ValidateAttachmentIndex(RenderPassCreateVersion rp_version, uint32_t attachment, uint32_t attachment_count,
                                         const char *error_type, const char *function_name) const {
    bool skip = false;
    const bool use_rp2 = (rp_version == RENDER_PASS_VERSION_2);
    assert(attachment != VK_ATTACHMENT_UNUSED);
    if (attachment >= attachment_count) {
        const char *vuid =
            use_rp2 ? "VUID-VkRenderPassCreateInfo2-attachment-03051" : "VUID-VkRenderPassCreateInfo-attachment-00834";
        skip |= LogError(device, vuid, "%s: %s attachment %d must be less than the total number of attachments %d.", function_name,
                         error_type, attachment, attachment_count);
    }
    return skip;
}

enum AttachmentType {
    ATTACHMENT_COLOR = 1,
    ATTACHMENT_DEPTH = 2,
    ATTACHMENT_INPUT = 4,
    ATTACHMENT_PRESERVE = 8,
    ATTACHMENT_RESOLVE = 16,
};

char const *StringAttachmentType(uint8_t type) {
    switch (type) {
        case ATTACHMENT_COLOR:
            return "color";
        case ATTACHMENT_DEPTH:
            return "depth";
        case ATTACHMENT_INPUT:
            return "input";
        case ATTACHMENT_PRESERVE:
            return "preserve";
        case ATTACHMENT_RESOLVE:
            return "resolve";
        default:
            return "(multiple)";
    }
}

bool CoreChecks::AddAttachmentUse(RenderPassCreateVersion rp_version, uint32_t subpass, std::vector<uint8_t> &attachment_uses,
                                  std::vector<VkImageLayout> &attachment_layouts, uint32_t attachment, uint8_t new_use,
                                  VkImageLayout new_layout) const {
    if (attachment >= attachment_uses.size()) return false; /* out of range, but already reported */

    bool skip = false;
    auto &uses = attachment_uses[attachment];
    const bool use_rp2 = (rp_version == RENDER_PASS_VERSION_2);
    const char *vuid;
    const char *const function_name = use_rp2 ? "vkCreateRenderPass2()" : "vkCreateRenderPass()";

    if (uses & new_use) {
        if (attachment_layouts[attachment] != new_layout) {
            vuid = use_rp2 ? "VUID-VkSubpassDescription2-layout-02528" : "VUID-VkSubpassDescription-layout-02519";
            skip |= LogError(device, vuid, "%s: subpass %u already uses attachment %u with a different image layout (%s vs %s).",
                             function_name, subpass, attachment, string_VkImageLayout(attachment_layouts[attachment]),
                             string_VkImageLayout(new_layout));
        }
    } else if (((new_use & ATTACHMENT_COLOR) && (uses & ATTACHMENT_DEPTH)) ||
               ((uses & ATTACHMENT_COLOR) && (new_use & ATTACHMENT_DEPTH))) {
        vuid = use_rp2 ? "VUID-VkSubpassDescription2-pDepthStencilAttachment-04440"
                       : "VUID-VkSubpassDescription-pDepthStencilAttachment-04438";
        skip |= LogError(device, vuid, "%s: subpass %u uses attachment %u as both %s and %s attachment.", function_name, subpass,
                         attachment, StringAttachmentType(uses), StringAttachmentType(new_use));
    } else if ((uses && (new_use & ATTACHMENT_PRESERVE)) || (new_use && (uses & ATTACHMENT_PRESERVE))) {
        vuid = use_rp2 ? "VUID-VkSubpassDescription2-pPreserveAttachments-03074"
                       : "VUID-VkSubpassDescription-pPreserveAttachments-00854";
        skip |= LogError(device, vuid, "%s: subpass %u uses attachment %u as both %s and %s attachment.", function_name, subpass,
                         attachment, StringAttachmentType(uses), StringAttachmentType(new_use));
    } else {
        attachment_layouts[attachment] = new_layout;
        uses |= new_use;
    }

    return skip;
}

// Handles attachment references regardless of type (input, color, depth, etc)
// Input attachments have extra VUs associated with them
bool CoreChecks::ValidateAttachmentReference(RenderPassCreateVersion rp_version, VkAttachmentReference2 reference,
                                             const VkFormat attachment_format, bool input, const char *error_type,
                                             const char *function_name) const {
    bool skip = false;
    const bool use_rp2 = (rp_version == RENDER_PASS_VERSION_2);
    const char *vuid;

    // Currently all VUs require attachment to not be UNUSED
    assert(reference.attachment != VK_ATTACHMENT_UNUSED);

    // currently VkAttachmentReference and VkAttachmentReference2 have no overlapping VUs
    const auto *attachment_reference_stencil_layout = LvlFindInChain<VkAttachmentReferenceStencilLayout>(reference.pNext);
    switch (reference.layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            vuid = (use_rp2) ? "VUID-VkAttachmentReference2-layout-03077" : "VUID-VkAttachmentReference-layout-03077";
            skip |= LogError(device, vuid,
                             "%s: Layout for %s is %s but must not be VK_IMAGE_LAYOUT_[UNDEFINED|PREINITIALIZED|PRESENT_SRC_KHR].",
                             function_name, error_type, string_VkImageLayout(reference.layout));
            break;

        // Only other layouts in VUs to be checked
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
        case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
            // First need to make sure feature bit is enabled and the format is actually a depth and/or stencil
            if (!enabled_features.core12.separateDepthStencilLayouts) {
                skip |= LogError(device, "VUID-VkAttachmentReference2-separateDepthStencilLayouts-03313",
                                 "%s: Layout for %s is %s but without separateDepthStencilLayouts enabled the layout must not "
                                 "be VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, "
                                 "VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL, or VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL.",
                                 function_name, error_type, string_VkImageLayout(reference.layout));
            } else if ((reference.layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ||
                       (reference.layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)) {
                if (attachment_reference_stencil_layout) {
                    // This check doesn't rely on the aspect mask value
                    const VkImageLayout stencil_layout = attachment_reference_stencil_layout->stencilLayout;
                    if (stencil_layout == VK_IMAGE_LAYOUT_UNDEFINED || stencil_layout == VK_IMAGE_LAYOUT_PREINITIALIZED ||
                        stencil_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ||
                        stencil_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
                        stencil_layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL ||
                        stencil_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                        stencil_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL ||
                        stencil_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL ||
                        stencil_layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
                        stencil_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                        skip |= LogError(device, "VUID-VkAttachmentReferenceStencilLayout-stencilLayout-03318",
                                         "%s: In %s with pNext chain instance VkAttachmentReferenceStencilLayout, "
                                         "the stencilLayout (%s) must not be "
                                         "VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PREINITIALIZED, "
                                         "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, "
                                         "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, "
                                         "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, "
                                         "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, "
                                         "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, "
                                         "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, "
                                         "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, or "
                                         "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.",
                                         function_name, error_type, string_VkImageLayout(stencil_layout));
                    }
                }
            }
            break;
        case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR:
        case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR:
            if (!enabled_features.core13.synchronization2) {
                vuid = (use_rp2) ? "VUID-VkAttachmentReference2-synchronization2-06910"
                                 : "VUID-VkAttachmentReference-synchronization2-06910";
                skip |= LogError(device, vuid,
                                 "%s: Layout for %s is %s but without synchronization2 enabled the layout must not "
                                 "be VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR or VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR.",
                                 function_name, error_type, string_VkImageLayout(reference.layout));
            }
            break;
        case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
            if (!enabled_features.attachment_feedback_loop_layout_features.attachmentFeedbackLoopLayout) {
                vuid = (use_rp2) ? "VUID-VkAttachmentReference2-attachmentFeedbackLoopLayout-07311"
                                 : "VUID-VkAttachmentReference-attachmentFeedbackLoopLayout-07311";
                skip |= LogError(device, vuid,
                                 "%s: Layout for %s is VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT but the "
                                 "attachmentFeedbackLoopLayout feature is not enabled",
                                 function_name, error_type);
            }
            break;

        default:
            break;
    }

    return skip;
}

bool CoreChecks::ValidateRenderpassAttachmentUsage(RenderPassCreateVersion rp_version, const VkRenderPassCreateInfo2 *pCreateInfo,
                                                   const char *function_name) const {
    bool skip = false;
    const bool use_rp2 = (rp_version == RENDER_PASS_VERSION_2);
    const char *vuid;

    // Track when we're observing the first use of an attachment
    std::vector<bool> attach_first_use(pCreateInfo->attachmentCount, true);

    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription2 &subpass = pCreateInfo->pSubpasses[i];
        const auto ms_render_to_single_sample = LvlFindInChain<VkMultisampledRenderToSingleSampledInfoEXT>(subpass.pNext);
        const auto subpass_depth_stencil_resolve = LvlFindInChain<VkSubpassDescriptionDepthStencilResolve>(subpass.pNext);
        std::vector<uint8_t> attachment_uses(pCreateInfo->attachmentCount);
        std::vector<VkImageLayout> attachment_layouts(pCreateInfo->attachmentCount);

        // Track if attachments are used as input as well as another type
        layer_data::unordered_set<uint32_t> input_attachments;

        if (!IsExtEnabled(device_extensions.vk_huawei_subpass_shading)) {
            if (subpass.pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS) {
                vuid = use_rp2 ? "VUID-VkSubpassDescription2-pipelineBindPoint-03062"
                               : "VUID-VkSubpassDescription-pipelineBindPoint-00844";
                skip |= LogError(device, vuid,
                                 "%s: Pipeline bind point for pSubpasses[%" PRIu32 "] must be VK_PIPELINE_BIND_POINT_GRAPHICS.",
                                 function_name, i);
            }
        } else {
            if (subpass.pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS &&
                subpass.pipelineBindPoint != VK_PIPELINE_BIND_POINT_SUBPASS_SHADING_HUAWEI) {
                vuid = use_rp2 ? "VUID-VkSubpassDescription2-pipelineBindPoint-04953"
                               : "VUID-VkSubpassDescription-pipelineBindPoint-04952";
                skip |= LogError(device, vuid,
                                 "%s: Pipeline bind point for pSubpasses[%" PRIu32
                                 "] must be VK_PIPELINE_BIND_POINT_GRAPHICS or "
                                 "VK_PIPELINE_BIND_POINT_SUBPASS_SHADING_HUAWEI.",
                                 function_name, i);
            }
        }

        // Check input attachments first
        // - so we can detect first-use-as-input for VU #00349
        // - if other color or depth/stencil is also input, it limits valid layouts
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            auto const &attachment_ref = subpass.pInputAttachments[j];
            const uint32_t attachment_index = attachment_ref.attachment;
            const VkImageAspectFlags aspect_mask = attachment_ref.aspectMask;
            if (attachment_index != VK_ATTACHMENT_UNUSED) {
                input_attachments.insert(attachment_index);
                std::string error_type = "pSubpasses[" + std::to_string(i) + "].pInputAttachments[" + std::to_string(j) + "]";
                skip |= ValidateAttachmentIndex(rp_version, attachment_index, pCreateInfo->attachmentCount, error_type.c_str(),
                                                function_name);

                if (aspect_mask & VK_IMAGE_ASPECT_METADATA_BIT) {
                    vuid = use_rp2 ? "VUID-VkSubpassDescription2-attachment-02801"
                                   : "VUID-VkInputAttachmentAspectReference-aspectMask-01964";
                    skip |= LogError(
                        device, vuid,
                        "%s: Aspect mask for input attachment reference %d in subpass %d includes VK_IMAGE_ASPECT_METADATA_BIT.",
                        function_name, j, i);
                } else if (aspect_mask & (VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT | VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT |
                                          VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT | VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT)) {
                    vuid = use_rp2 ? "VUID-VkSubpassDescription2-attachment-04563"
                                   : "VUID-VkInputAttachmentAspectReference-aspectMask-02250";
                    skip |= LogError(device, vuid,
                                     "%s: Aspect mask for input attachment reference %d in subpass %d includes "
                                     "VK_IMAGE_ASPECT_MEMORY_PLANE_*_BIT_EXT bit.",
                                     function_name, j, i);
                }

                // safe to dereference pCreateInfo->pAttachments[]
                if (attachment_index < pCreateInfo->attachmentCount) {
                    const VkFormat attachment_format = pCreateInfo->pAttachments[attachment_index].format;
                    skip |= ValidateAttachmentReference(rp_version, attachment_ref, attachment_format, true, error_type.c_str(),
                                                        function_name);

                    skip |= AddAttachmentUse(rp_version, i, attachment_uses, attachment_layouts, attachment_index, ATTACHMENT_INPUT,
                                             attachment_ref.layout);

                    vuid = use_rp2 ? "VUID-VkRenderPassCreateInfo2-attachment-02525" : "VUID-VkRenderPassCreateInfo-pNext-01963";
                    // Assuming no disjoint image since there's no handle
                    skip |= ValidateImageAspectMask(VK_NULL_HANDLE, attachment_format, aspect_mask, false, function_name, vuid);

                    if (attach_first_use[attachment_index]) {
                        skip |=
                            ValidateLayoutVsAttachmentDescription(report_data, rp_version, subpass.pInputAttachments[j].layout,
                                                                  attachment_index, pCreateInfo->pAttachments[attachment_index]);

                        const bool used_as_depth = (subpass.pDepthStencilAttachment != NULL &&
                                                    subpass.pDepthStencilAttachment->attachment == attachment_index);
                        bool used_as_color = false;
                        for (uint32_t k = 0; !used_as_depth && !used_as_color && k < subpass.colorAttachmentCount; ++k) {
                            used_as_color = (subpass.pColorAttachments[k].attachment == attachment_index);
                        }
                        if (!used_as_depth && !used_as_color &&
                            pCreateInfo->pAttachments[attachment_index].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                            vuid = use_rp2 ? "VUID-VkSubpassDescription2-loadOp-03064" : "VUID-VkSubpassDescription-loadOp-00846";
                            skip |= LogError(device, vuid,
                                             "%s: attachment %u is first used as an input attachment in %s with loadOp set to "
                                             "VK_ATTACHMENT_LOAD_OP_CLEAR.",
                                             function_name, attachment_index, error_type.c_str());
                        }
                    }
                    attach_first_use[attachment_index] = false;

                    const VkFormatFeatureFlags2KHR valid_flags =
                        VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT_KHR | VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT_KHR;
                    const VkFormatFeatureFlags2KHR format_features = GetPotentialFormatFeatures(attachment_format);
                    if ((format_features & valid_flags) == 0) {
                        if (!enabled_features.linear_color_attachment_features.linearColorAttachment) {
                            vuid = use_rp2 ? "VUID-VkSubpassDescription2-pInputAttachments-02897"
                                           : "VUID-VkSubpassDescription-pInputAttachments-02647";
                            skip |= LogError(
                                device, vuid,
                                "%s: Input attachment %s format (%s) does not contain VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT "
                                "| VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT.",
                                function_name, error_type.c_str(), string_VkFormat(attachment_format));
                        } else if ((format_features & VK_FORMAT_FEATURE_2_LINEAR_COLOR_ATTACHMENT_BIT_NV) == 0) {
                            vuid = use_rp2 ? "VUID-VkSubpassDescription2-linearColorAttachment-06499"
                                           : "VUID-VkSubpassDescription-linearColorAttachment-06496";
                            skip |= LogError(
                                device, vuid,
                                "%s: Input attachment %s format (%s) does not contain VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT "
                                "| VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | "
                                "VK_FORMAT_FEATURE_2_LINEAR_COLOR_ATTACHMENT_BIT_NV.",
                                function_name, error_type.c_str(), string_VkFormat(attachment_format));
                        }
                    }
                }

                if (rp_version == RENDER_PASS_VERSION_2) {
                    // These are validated automatically as part of parameter validation for create renderpass 1
                    // as they are in a struct that only applies to input attachments - not so for v2.

                    // Check for 0
                    if (aspect_mask == 0) {
                        skip |= LogError(device, "VUID-VkSubpassDescription2-attachment-02800",
                                         "%s: Input attachment %s aspect mask must not be 0.", function_name, error_type.c_str());
                    } else {
                        const VkImageAspectFlags valid_bits =
                            (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT |
                             VK_IMAGE_ASPECT_METADATA_BIT | VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT |
                             VK_IMAGE_ASPECT_PLANE_2_BIT | VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT |
                             VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT | VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT |
                             VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT);

                        // Check for valid aspect mask bits
                        if (aspect_mask & ~valid_bits) {
                            skip |= LogError(device, "VUID-VkSubpassDescription2-attachment-02799",
                                             "%s: Input attachment %s aspect mask (0x%" PRIx32 ")is invalid.", function_name,
                                             error_type.c_str(), aspect_mask);
                        }
                    }
                }
            }
        }

        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            std::string error_type = "pSubpasses[" + std::to_string(i) + "].pPreserveAttachments[" + std::to_string(j) + "]";
            uint32_t attachment = subpass.pPreserveAttachments[j];
            if (attachment == VK_ATTACHMENT_UNUSED) {
                vuid = use_rp2 ? "VUID-VkSubpassDescription2-attachment-03073" : "VUID-VkSubpassDescription-attachment-00853";
                skip |= LogError(device, vuid, "%s:  Preserve attachment (%d) must not be VK_ATTACHMENT_UNUSED.", function_name, j);
            } else {
                skip |= ValidateAttachmentIndex(rp_version, attachment, pCreateInfo->attachmentCount, error_type.c_str(),
                                                function_name);
                if (attachment < pCreateInfo->attachmentCount) {
                    skip |= AddAttachmentUse(rp_version, i, attachment_uses, attachment_layouts, attachment, ATTACHMENT_PRESERVE,
                                             VkImageLayout(0) /* preserve doesn't have any layout */);
                }
            }
        }

        bool subpass_performs_resolve = false;

        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            if (subpass.pResolveAttachments) {
                std::string error_type = "pSubpasses[" + std::to_string(i) + "].pResolveAttachments[" + std::to_string(j) + "]";
                auto const &attachment_ref = subpass.pResolveAttachments[j];
                if (attachment_ref.attachment != VK_ATTACHMENT_UNUSED) {
                    skip |= ValidateAttachmentIndex(rp_version, attachment_ref.attachment, pCreateInfo->attachmentCount,
                                                    error_type.c_str(), function_name);

                    // safe to dereference pCreateInfo->pAttachments[]
                    if (attachment_ref.attachment < pCreateInfo->attachmentCount) {
                        const VkFormat attachment_format = pCreateInfo->pAttachments[attachment_ref.attachment].format;
                        skip |= ValidateAttachmentReference(rp_version, attachment_ref, attachment_format, false,
                                                            error_type.c_str(), function_name);
                        skip |= AddAttachmentUse(rp_version, i, attachment_uses, attachment_layouts, attachment_ref.attachment,
                                                 ATTACHMENT_RESOLVE, attachment_ref.layout);

                        subpass_performs_resolve = true;

                        if (pCreateInfo->pAttachments[attachment_ref.attachment].samples != VK_SAMPLE_COUNT_1_BIT) {
                            vuid = use_rp2 ? "VUID-VkSubpassDescription2-pResolveAttachments-03067"
                                           : "VUID-VkSubpassDescription-pResolveAttachments-00849";
                            skip |= LogError(
                                device, vuid,
                                "%s: Subpass %u requests multisample resolve into attachment %u, which must "
                                "have VK_SAMPLE_COUNT_1_BIT but has %s.",
                                function_name, i, attachment_ref.attachment,
                                string_VkSampleCountFlagBits(pCreateInfo->pAttachments[attachment_ref.attachment].samples));
                        }

                        const VkFormatFeatureFlags2KHR format_features = GetPotentialFormatFeatures(attachment_format);
                        if ((format_features & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT_KHR) == 0) {
                            if (!enabled_features.linear_color_attachment_features.linearColorAttachment) {
                                vuid = use_rp2 ? "VUID-VkSubpassDescription2-pResolveAttachments-02899"
                                               : "VUID-VkSubpassDescription-pResolveAttachments-02649";
                                skip |= LogError(device, vuid,
                                                 "%s: Resolve attachment %s format (%s) does not contain "
                                                 "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT.",
                                                 function_name, error_type.c_str(), string_VkFormat(attachment_format));
                            } else if ((format_features & VK_FORMAT_FEATURE_2_LINEAR_COLOR_ATTACHMENT_BIT_NV) == 0) {
                                vuid = use_rp2 ? "VUID-VkSubpassDescription2-linearColorAttachment-06501"
                                               : "VUID-VkSubpassDescription-linearColorAttachment-06498";
                                skip |= LogError(
                                    device, vuid,
                                    "%s: Resolve attachment %s format (%s) does not contain "
                                    "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT or VK_FORMAT_FEATURE_2_LINEAR_COLOR_ATTACHMENT_BIT_NV.",
                                    function_name, error_type.c_str(), string_VkFormat(attachment_format));
                            }
                        }

                        //  VK_QCOM_render_pass_shader_resolve check of resolve attachmnents
                        if ((subpass.flags & VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM) != 0) {
                            vuid = use_rp2 ? "VUID-VkRenderPassCreateInfo2-flags-04907" : "VUID-VkSubpassDescription-flags-03341";
                            skip |= LogError(
                                device, vuid,
                                "%s: Subpass %u enables shader resolve, which requires every element of pResolve attachments"
                                " must be VK_ATTACHMENT_UNUSED, but element %u contains a reference to attachment %u instead.",
                                function_name, i, j, attachment_ref.attachment);
                        }
                    }
                }
            }
        }

        if (subpass.pDepthStencilAttachment) {
            std::string error_type = "pSubpasses[" + std::to_string(i) + "].pDepthStencilAttachment";
            const uint32_t attachment = subpass.pDepthStencilAttachment->attachment;
            const VkImageLayout image_layout = subpass.pDepthStencilAttachment->layout;
            if (attachment != VK_ATTACHMENT_UNUSED) {
                skip |= ValidateAttachmentIndex(rp_version, attachment, pCreateInfo->attachmentCount, error_type.c_str(),
                                                function_name);

                // safe to dereference pCreateInfo->pAttachments[]
                if (attachment < pCreateInfo->attachmentCount) {
                    const VkFormat attachment_format = pCreateInfo->pAttachments[attachment].format;
                    skip |= ValidateAttachmentReference(rp_version, *subpass.pDepthStencilAttachment, attachment_format, false,
                                                        error_type.c_str(), function_name);
                    skip |= AddAttachmentUse(rp_version, i, attachment_uses, attachment_layouts, attachment, ATTACHMENT_DEPTH,
                                             image_layout);

                    if (attach_first_use[attachment]) {
                        skip |= ValidateLayoutVsAttachmentDescription(report_data, rp_version, image_layout, attachment,
                                                                      pCreateInfo->pAttachments[attachment]);
                    }
                    attach_first_use[attachment] = false;

                    const VkFormatFeatureFlags2KHR format_features = GetPotentialFormatFeatures(attachment_format);
                    if ((format_features & VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT_KHR) == 0) {
                        vuid = use_rp2 ? "VUID-VkSubpassDescription2-pDepthStencilAttachment-02900"
                                       : "VUID-VkSubpassDescription-pDepthStencilAttachment-02650";
                        skip |= LogError(device, vuid,
                                         "%s: Depth Stencil %s format (%s) does not contain "
                                         "VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT.",
                                         function_name, error_type.c_str(), string_VkFormat(attachment_format));
                    }

                    if (use_rp2 &&
                        enabled_features.multisampled_render_to_single_sampled_features.multisampledRenderToSingleSampled &&
                        ms_render_to_single_sample && ms_render_to_single_sample->multisampledRenderToSingleSampledEnable) {
                        const auto depth_stencil_attachment =
                            pCreateInfo->pAttachments[subpass.pDepthStencilAttachment->attachment];
                        const auto depth_stencil_sample_count = depth_stencil_attachment.samples;
                        const auto depth_stencil_format = depth_stencil_attachment.format;
                        if ((depth_stencil_sample_count == VK_SAMPLE_COUNT_1_BIT) &&
                            (!subpass_depth_stencil_resolve ||
                             (subpass_depth_stencil_resolve->pDepthStencilResolveAttachment != VK_NULL_HANDLE &&
                              subpass_depth_stencil_resolve->pDepthStencilResolveAttachment->attachment != VK_ATTACHMENT_UNUSED))) {
                            std::stringstream message;
                            message << function_name << ": Subpass " << i
                                    << " has a VkMultisampledRenderToSingleSampledInfoEXT struct in its "
                                       "VkSubpassDescription2 pNext chain with multisampledRenderToSingleSampled set to "
                                       "VK_TRUE and pDepthStencilAttachment has a sample count of VK_SAMPLE_COUNT_1_BIT ";
                            if (!subpass_depth_stencil_resolve) {
                                message << "but there is no VkSubpassDescriptionDepthStencilResolve in the pNext chain of "
                                           "the VkSubpassDescription2 struct for this subpass";
                            } else {
                                message << "but the pSubpassResolveAttachment member of the "
                                           "VkSubpassDescriptionDepthStencilResolve in the pNext chain of "
                                           "the VkSubpassDescription2 struct for this subpass is not NULL, and its attachment "
                                           "is not VK_ATTACHMENT_UNUSED";
                            }
                            skip |= LogError(device, "VUID-VkSubpassDescription2-pNext-06871", "%s", message.str().c_str());
                        }
                        if (subpass_depth_stencil_resolve) {
                            if (subpass_depth_stencil_resolve->depthResolveMode == VK_RESOLVE_MODE_NONE &&
                                subpass_depth_stencil_resolve->stencilResolveMode == VK_RESOLVE_MODE_NONE) {
                                std::stringstream message;
                                message << function_name << ": Subpass " << i
                                        << " has a VkMultisampledRenderToSingleSampledInfoEXT struct in its "
                                           "VkSubpassDescription2 pNext chain with multisampledRenderToSingleSampled set to "
                                           "VK_TRUE and there is also a VkSubpassDescriptionDepthStencilResolve struct in the "
                                           "pNext "
                                           "chain whose depthResolveMode and stencilResolveMode members are both "
                                           "VK_RESOLVE_MODE_NONE";
                                skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pNext-06873", "%s",
                                                 message.str().c_str());
                            }
                            if (FormatHasDepth(depth_stencil_format)) {
                                if (subpass_depth_stencil_resolve->depthResolveMode != VK_RESOLVE_MODE_NONE &&
                                    !(subpass_depth_stencil_resolve->depthResolveMode &
                                      phys_dev_props_core12.supportedDepthResolveModes)) {
                                    skip |= LogError(
                                        device, "VUID-VkSubpassDescriptionDepthStencilResolve-pNext-06874",
                                        "%s:  Subpass %" PRIu32
                                        " has a pDepthStencilAttachment with format %s and has a "
                                        "VkMultisampledRenderToSingleSampledInfoEXT struct in its "
                                        "VkSubpassDescription2 pNext chain with multisampledRenderToSingleSampled set to "
                                        "VK_TRUE and a VkSubpassDescriptionDepthStencilResolve in the VkSubpassDescription2 pNext "
                                        "chain with a depthResolveMode (%s) that is not in "
                                        "VkPhysicalDeviceDepthStencilResolveProperties::supportedDepthResolveModes (%s) or "
                                        "VK_RESOLVE_MODE_NONE",
                                        function_name, i, string_VkFormat(depth_stencil_format),
                                        string_VkResolveModeFlagBits(subpass_depth_stencil_resolve->depthResolveMode),
                                        string_VkResolveModeFlags(phys_dev_props_core12.supportedDepthResolveModes).c_str());
                                }
                            }
                            if (FormatHasStencil(depth_stencil_format)) {
                                if (subpass_depth_stencil_resolve->stencilResolveMode != VK_RESOLVE_MODE_NONE &&
                                    !(subpass_depth_stencil_resolve->stencilResolveMode &
                                      phys_dev_props_core12.supportedStencilResolveModes)) {
                                    skip |= LogError(
                                        device, "VUID-VkSubpassDescriptionDepthStencilResolve-pNext-06875",
                                        "%s:  Subpass %" PRIu32
                                        " has a pDepthStencilAttachment with format %s and has a "
                                        "VkMultisampledRenderToSingleSampledInfoEXT struct in its "
                                        "VkSubpassDescription2 pNext chain with multisampledRenderToSingleSampled set to "
                                        "VK_TRUE and a VkSubpassDescriptionDepthStencilResolve in the VkSubpassDescription2 pNext "
                                        "chain with a stencilResolveMode (%s) that is not in "
                                        "VkPhysicalDeviceDepthStencilResolveProperties::supportedStencilResolveModes (%s) or "
                                        "VK_RESOLVE_MODE_NONE",
                                        function_name, i, string_VkFormat(depth_stencil_format),
                                        string_VkResolveModeFlagBits(subpass_depth_stencil_resolve->stencilResolveMode),
                                        string_VkResolveModeFlags(phys_dev_props_core12.supportedStencilResolveModes).c_str());
                                }
                            }
                            if (FormatIsDepthAndStencil(depth_stencil_format)) {
                                if (phys_dev_props_core12.independentResolve == VK_FALSE &&
                                    phys_dev_props_core12.independentResolveNone == VK_FALSE &&
                                    (subpass_depth_stencil_resolve->stencilResolveMode !=
                                     subpass_depth_stencil_resolve->depthResolveMode)) {
                                    skip |= LogError(
                                        device, "VUID-VkSubpassDescriptionDepthStencilResolve-pNext-06876",
                                        "%s:  Subpass %" PRIu32
                                        " has a pDepthStencilAttachment with format %s and has a "
                                        "VkMultisampledRenderToSingleSampledInfoEXT struct in its "
                                        "VkSubpassDescription2 pNext chain with multisampledRenderToSingleSampled set to "
                                        "VK_TRUE and a VkSubpassDescriptionDepthStencilResolve in the VkSubpassDescription2 pNext "
                                        "chain with a stencilResolveMode (%s) that is not identical to depthResolveMode (%s) "
                                        "even though VkPhysicalDeviceDepthStencilResolveProperties::independentResolve and "
                                        "VkPhysicalDeviceDepthStencilResolveProperties::independentResolveNone are both VK_FALSE,",
                                        function_name, i, string_VkFormat(depth_stencil_format),
                                        string_VkResolveModeFlagBits(subpass_depth_stencil_resolve->stencilResolveMode),
                                        string_VkResolveModeFlagBits(subpass_depth_stencil_resolve->depthResolveMode));
                                }
                                if (phys_dev_props_core12.independentResolve == VK_FALSE &&
                                    phys_dev_props_core12.independentResolveNone == VK_TRUE &&
                                    ((subpass_depth_stencil_resolve->stencilResolveMode !=
                                      subpass_depth_stencil_resolve->depthResolveMode) &&
                                     ((subpass_depth_stencil_resolve->depthResolveMode != VK_RESOLVE_MODE_NONE) &&
                                      (subpass_depth_stencil_resolve->stencilResolveMode != VK_RESOLVE_MODE_NONE)))) {
                                    skip |= LogError(
                                        device, "VUID-VkSubpassDescriptionDepthStencilResolve-pNext-06877",
                                        "%s:  Subpass %" PRIu32
                                        " has a pDepthStencilAttachment with format %s and has a "
                                        "VkMultisampledRenderToSingleSampledInfoEXT struct in its "
                                        "VkSubpassDescription2 pNext chain with multisampledRenderToSingleSampled set to "
                                        "VK_TRUE and a VkSubpassDescriptionDepthStencilResolve in the VkSubpassDescription2 pNext "
                                        "chain with a stencilResolveMode (%s) that is not identical to depthResolveMode (%s) and "
                                        "neither of them is VK_RESOLVE_MODE_NONE, "
                                        "even though VkPhysicalDeviceDepthStencilResolveProperties::independentResolve == VK_FALSE "
                                        "and "
                                        "VkPhysicalDeviceDepthStencilResolveProperties::independentResolveNone == VK_TRUE",
                                        function_name, i, string_VkFormat(depth_stencil_format),
                                        string_VkResolveModeFlagBits(subpass_depth_stencil_resolve->stencilResolveMode),
                                        string_VkResolveModeFlagBits(subpass_depth_stencil_resolve->depthResolveMode));
                                }
                            }
                        }
                    }
                }
            }
        }

        uint32_t last_sample_count_attachment = VK_ATTACHMENT_UNUSED;
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            std::string error_type = "pSubpasses[" + std::to_string(i) + "].pColorAttachments[" + std::to_string(j) + "]";
            auto const &attachment_ref = subpass.pColorAttachments[j];
            const uint32_t attachment_index = attachment_ref.attachment;
            if (attachment_index != VK_ATTACHMENT_UNUSED) {
                skip |= ValidateAttachmentIndex(rp_version, attachment_index, pCreateInfo->attachmentCount, error_type.c_str(),
                                                function_name);

                // safe to dereference pCreateInfo->pAttachments[]
                if (attachment_index < pCreateInfo->attachmentCount) {
                    const VkFormat attachment_format = pCreateInfo->pAttachments[attachment_index].format;
                    skip |= ValidateAttachmentReference(rp_version, attachment_ref, attachment_format, false, error_type.c_str(),
                                                        function_name);
                    skip |= AddAttachmentUse(rp_version, i, attachment_uses, attachment_layouts, attachment_index, ATTACHMENT_COLOR,
                                             attachment_ref.layout);

                    VkSampleCountFlagBits current_sample_count = pCreateInfo->pAttachments[attachment_index].samples;
                    if (use_rp2 &&
                        enabled_features.multisampled_render_to_single_sampled_features.multisampledRenderToSingleSampled &&
                        ms_render_to_single_sample && ms_render_to_single_sample->multisampledRenderToSingleSampledEnable) {
                        if (current_sample_count != VK_SAMPLE_COUNT_1_BIT &&
                            current_sample_count != ms_render_to_single_sample->rasterizationSamples) {
                            skip |= LogError(device, "VUID-VkSubpassDescription2-pNext-06870",
                                             "%s:  Subpass %u has a VkMultisampledRenderToSingleSampledInfoEXT struct in its "
                                             "VkSubpassDescription2 pNext chain with multisampledRenderToSingleSampled set to "
                                             "VK_TRUE and rasterizationSamples set to %s "
                                             "but color attachment ref %u has a sample count of %s.",
                                             function_name, i,
                                             string_VkSampleCountFlagBits(ms_render_to_single_sample->rasterizationSamples), j,
                                             string_VkSampleCountFlagBits(current_sample_count));
                        }
                    }
                    if (last_sample_count_attachment != VK_ATTACHMENT_UNUSED) {
                        if (!(IsExtEnabled(device_extensions.vk_amd_mixed_attachment_samples) ||
                              IsExtEnabled(device_extensions.vk_nv_framebuffer_mixed_samples) ||
                              ((enabled_features.multisampled_render_to_single_sampled_features
                                    .multisampledRenderToSingleSampled) &&
                               use_rp2))) {
                            VkSampleCountFlagBits last_sample_count =
                                pCreateInfo->pAttachments[subpass.pColorAttachments[last_sample_count_attachment].attachment]
                                    .samples;
                            if (current_sample_count != last_sample_count) {
                                // Also VUID-VkSubpassDescription2-multisampledRenderToSingleSampled-06869
                                vuid = use_rp2 ? "VUID-VkSubpassDescription2-multisampledRenderToSingleSampled-06872"
                                               : "VUID-VkSubpassDescription-pColorAttachments-06868";
                                skip |= LogError(
                                    device, vuid,
                                    "%s: Subpass %u attempts to render to color attachments with inconsistent sample counts."
                                    "Color attachment ref %u has sample count %s, whereas previous color attachment ref %u has "
                                    "sample count %s.",
                                    function_name, i, j, string_VkSampleCountFlagBits(current_sample_count),
                                    last_sample_count_attachment, string_VkSampleCountFlagBits(last_sample_count));
                            }
                        }
                    }
                    last_sample_count_attachment = j;

                    if (subpass_performs_resolve && current_sample_count == VK_SAMPLE_COUNT_1_BIT) {
                        vuid = use_rp2 ? "VUID-VkSubpassDescription2-pResolveAttachments-03066"
                                       : "VUID-VkSubpassDescription-pResolveAttachments-00848";
                        skip |= LogError(device, vuid,
                                         "%s: Subpass %u requests multisample resolve from attachment %u which has "
                                         "VK_SAMPLE_COUNT_1_BIT.",
                                         function_name, i, attachment_index);
                    }

                    if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED &&
                        subpass.pDepthStencilAttachment->attachment < pCreateInfo->attachmentCount) {
                        const auto depth_stencil_sample_count =
                            pCreateInfo->pAttachments[subpass.pDepthStencilAttachment->attachment].samples;

                        if (IsExtEnabled(device_extensions.vk_amd_mixed_attachment_samples)) {
                            if (current_sample_count > depth_stencil_sample_count) {
                                vuid = use_rp2 ? "VUID-VkSubpassDescription2-pColorAttachments-03070"
                                               : "VUID-VkSubpassDescription-pColorAttachments-01506";
                                skip |=
                                    LogError(device, vuid, "%s: %s has %s which is larger than depth/stencil attachment %s.",
                                             function_name, error_type.c_str(), string_VkSampleCountFlagBits(current_sample_count),
                                             string_VkSampleCountFlagBits(depth_stencil_sample_count));
                                break;
                            }
                        }

                        if (!IsExtEnabled(device_extensions.vk_amd_mixed_attachment_samples) &&
                            !IsExtEnabled(device_extensions.vk_nv_framebuffer_mixed_samples) &&
                            !(use_rp2 &&
                              enabled_features.multisampled_render_to_single_sampled_features.multisampledRenderToSingleSampled) &&
                            current_sample_count != depth_stencil_sample_count) {
                            vuid = use_rp2 ? "VUID-VkSubpassDescription2-multisampledRenderToSingleSampled-06872"
                                           : "VUID-VkSubpassDescription-pDepthStencilAttachment-01418";
                            skip |= LogError(device, vuid,
                                             "%s:  Subpass %u attempts to render to use a depth/stencil attachment with sample "
                                             "count that differs "
                                             "from color attachment %u."
                                             "The depth attachment ref has sample count %s, whereas color attachment ref %u has "
                                             "sample count %s.",
                                             function_name, i, j, string_VkSampleCountFlagBits(depth_stencil_sample_count), j,
                                             string_VkSampleCountFlagBits(current_sample_count));
                            break;
                        }
                    }

                    const VkFormatFeatureFlags2KHR format_features = GetPotentialFormatFeatures(attachment_format);
                    if ((format_features & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT_KHR) == 0) {
                        if (!enabled_features.linear_color_attachment_features.linearColorAttachment) {
                            vuid = use_rp2 ? "VUID-VkSubpassDescription2-pColorAttachments-02898"
                                           : "VUID-VkSubpassDescription-pColorAttachments-02648";
                            skip |= LogError(device, vuid,
                                             "%s: Color attachment %s format (%s) does not contain "
                                             "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT.",
                                             function_name, error_type.c_str(), string_VkFormat(attachment_format));
                        } else if ((format_features & VK_FORMAT_FEATURE_2_LINEAR_COLOR_ATTACHMENT_BIT_NV) == 0) {
                            vuid = use_rp2 ? "VUID-VkSubpassDescription2-linearColorAttachment-06500"
                                           : "VUID-VkSubpassDescription-linearColorAttachment-06497";
                            skip |= LogError(
                                device, vuid,
                                "%s: Color attachment %s format (%s) does not contain "
                                "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT or VK_FORMAT_FEATURE_2_LINEAR_COLOR_ATTACHMENT_BIT_NV.",
                                function_name, error_type.c_str(), string_VkFormat(attachment_format));
                        }
                    }

                    if (attach_first_use[attachment_index]) {
                        skip |=
                            ValidateLayoutVsAttachmentDescription(report_data, rp_version, subpass.pColorAttachments[j].layout,
                                                                  attachment_index, pCreateInfo->pAttachments[attachment_index]);
                    }
                    attach_first_use[attachment_index] = false;
                }
            }

            if (subpass_performs_resolve && subpass.pResolveAttachments[j].attachment != VK_ATTACHMENT_UNUSED &&
                subpass.pResolveAttachments[j].attachment < pCreateInfo->attachmentCount) {
                if (attachment_index == VK_ATTACHMENT_UNUSED) {
                    vuid = use_rp2 ? "VUID-VkSubpassDescription2-pResolveAttachments-03065"
                                   : "VUID-VkSubpassDescription-pResolveAttachments-00847";
                    skip |= LogError(device, vuid,
                                     "%s: Subpass %u requests multisample resolve from attachment %u which has "
                                     "attachment=VK_ATTACHMENT_UNUSED.",
                                     function_name, i, attachment_index);
                } else {
                    const auto &color_desc = pCreateInfo->pAttachments[attachment_index];
                    const auto &resolve_desc = pCreateInfo->pAttachments[subpass.pResolveAttachments[j].attachment];
                    if (color_desc.format != resolve_desc.format) {
                        vuid = use_rp2 ? "VUID-VkSubpassDescription2-pResolveAttachments-03068"
                                       : "VUID-VkSubpassDescription-pResolveAttachments-00850";
                        skip |= LogError(device, vuid,
                                         "%s: %s resolves to an attachment with a "
                                         "different format. color format: %u, resolve format: %u.",
                                         function_name, error_type.c_str(), color_desc.format, resolve_desc.format);
                    }
                }
            }
        }

        if (use_rp2 && enabled_features.multisampled_render_to_single_sampled_features.multisampledRenderToSingleSampled &&
            ms_render_to_single_sample) {
            if (ms_render_to_single_sample->rasterizationSamples == VK_SAMPLE_COUNT_1_BIT) {
                skip |= LogError(
                    device, "VUID-VkMultisampledRenderToSingleSampledInfoEXT-rasterizationSamples-06878",
                    "%s(): A VkMultisampledRenderToSingleSampledInfoEXT struct is in the pNext chain of  VkSubpassDescription2 "
                    "with a rasterizationSamples value of VK_SAMPLE_COUNT_1_BIT which is not allowed",
                    function_name);
            }
        }
    }
    return skip;
}

bool CoreChecks::ValidateDependencies(FRAMEBUFFER_STATE const *framebuffer, RENDER_PASS_STATE const *renderPass) const {
    bool skip = false;
    auto const framebuffer_info = framebuffer->createInfo.ptr();
    auto const create_info = renderPass->createInfo.ptr();
    auto const &subpass_to_node = renderPass->subpass_to_node;

    struct Attachment {
        std::vector<SubpassLayout> outputs;
        std::vector<SubpassLayout> inputs;
        std::vector<uint32_t> overlapping;
    };

    std::vector<Attachment> attachments(create_info->attachmentCount);

    if (!(framebuffer_info->flags & VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
        // Find overlapping attachments
        for (uint32_t i = 0; i < framebuffer_info->attachmentCount; ++i) {
            for (uint32_t j = i + 1; j < framebuffer_info->attachmentCount; ++j) {
                VkImageView viewi = framebuffer_info->pAttachments[i];
                VkImageView viewj = framebuffer_info->pAttachments[j];
                if (viewi == viewj) {
                    attachments[i].overlapping.emplace_back(j);
                    attachments[j].overlapping.emplace_back(i);
                    continue;
                }
                if (i >= framebuffer->attachments_view_state.size() || j >= framebuffer->attachments_view_state.size()) {
                    continue;
                }
                auto *view_state_i = framebuffer->attachments_view_state[i].get();
                auto *view_state_j = framebuffer->attachments_view_state[j].get();
                if (!view_state_i || !view_state_j) {
                    continue;
                }
                auto view_ci_i = view_state_i->create_info;
                auto view_ci_j = view_state_j->create_info;
                if (view_ci_i.image == view_ci_j.image &&
                    IsRegionOverlapping(view_ci_i.subresourceRange, view_ci_j.subresourceRange)) {
                    attachments[i].overlapping.emplace_back(j);
                    attachments[j].overlapping.emplace_back(i);
                    continue;
                }
                auto *image_data_i = view_state_i->image_state.get();
                auto *image_data_j = view_state_j->image_state.get();
                if (!image_data_i || !image_data_j) {
                    continue;
                }

                if (!image_data_i->sparse && !image_data_j->sparse) {
                    subresource_adapter::ImageRangeGenerator generator_i{*image_data_i->fragment_encoder.get(),
                                                                         view_state_i->create_info.subresourceRange, 0u,
                                                                         view_state_i->IsDepthSliced()};

                    subresource_adapter::ImageRangeGenerator generator_j{*image_data_j->fragment_encoder.get(),
                                                                         view_state_j->create_info.subresourceRange, 0u,
                                                                         view_state_j->IsDepthSliced()};
                    for (; generator_i->non_empty(); ++generator_i) {
                        subresource_adapter::ImageRangeGenerator generator_j_copy = generator_j;
                        for (; generator_j_copy->non_empty(); ++generator_j_copy) {
                            sparse_container::range<VkDeviceSize> range_i{generator_i->begin, generator_i->end};
                            sparse_container::range<VkDeviceSize> range_j{generator_j_copy->begin, generator_j_copy->end};

                            if (image_data_i->DoesResourceMemoryOverlap(range_i, image_data_j, range_j)) {
                                attachments[i].overlapping.emplace_back(j);
                                attachments[j].overlapping.emplace_back(i);
                            }
                        }
                    }
                }
            }
        }
    }
    // Find for each attachment the subpasses that use them.
    layer_data::unordered_set<uint32_t> attachment_indices;
    for (uint32_t i = 0; i < create_info->subpassCount; ++i) {
        const VkSubpassDescription2 &subpass = create_info->pSubpasses[i];
        attachment_indices.clear();
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED) continue;
            SubpassLayout sp = {i, subpass.pInputAttachments[j].layout};
            attachments[attachment].inputs.emplace_back(sp);
            for (auto overlapping_attachment : attachments[attachment].overlapping) {
                attachments[overlapping_attachment].inputs.emplace_back(sp);
            }
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment = subpass.pColorAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED) continue;
            SubpassLayout sp = {i, subpass.pColorAttachments[j].layout};
            attachments[attachment].outputs.emplace_back(sp);
            for (auto overlapping_attachment : attachments[attachment].overlapping) {
                attachments[overlapping_attachment].outputs.emplace_back(sp);
            }
            attachment_indices.insert(attachment);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            uint32_t attachment = subpass.pDepthStencilAttachment->attachment;
            SubpassLayout sp = {i, subpass.pDepthStencilAttachment->layout};
            attachments[attachment].outputs.emplace_back(sp);
            for (auto overlapping_attachment : attachments[attachment].overlapping) {
                attachments[overlapping_attachment].outputs.emplace_back(sp);
            }

            if (attachment_indices.count(attachment)) {
                skip |=
                    LogError(renderPass->renderPass(), kVUID_Core_DrawState_InvalidRenderpass,
                             "Cannot use same attachment (%u) as both color and depth output in same subpass (%u).", attachment, i);
            }
        }
    }
    // If there is a dependency needed make sure one exists
    for (uint32_t i = 0; i < create_info->subpassCount; ++i) {
        const VkSubpassDescription2 &subpass = create_info->pSubpasses[i];
        // If the attachment is an input then all subpasses that output must have a dependency relationship
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED) continue;
            CheckDependencyExists(renderPass->renderPass(), i, subpass.pInputAttachments[j].layout, attachments[attachment].outputs,
                                  subpass_to_node, skip);
        }
        // If the attachment is an output then all subpasses that use the attachment must have a dependency relationship
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment = subpass.pColorAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED) continue;
            CheckDependencyExists(renderPass->renderPass(), i, subpass.pColorAttachments[j].layout, attachments[attachment].outputs,
                                  subpass_to_node, skip);
            CheckDependencyExists(renderPass->renderPass(), i, subpass.pColorAttachments[j].layout, attachments[attachment].inputs,
                                  subpass_to_node, skip);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            const uint32_t &attachment = subpass.pDepthStencilAttachment->attachment;
            CheckDependencyExists(renderPass->renderPass(), i, subpass.pDepthStencilAttachment->layout,
                                  attachments[attachment].outputs, subpass_to_node, skip);
            CheckDependencyExists(renderPass->renderPass(), i, subpass.pDepthStencilAttachment->layout,
                                  attachments[attachment].inputs, subpass_to_node, skip);
        }
    }
    // Loop through implicit dependencies, if this pass reads make sure the attachment is preserved for all passes after it was
    // written.
    for (uint32_t i = 0; i < create_info->subpassCount; ++i) {
        const VkSubpassDescription2 &subpass = create_info->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            CheckPreserved(renderPass->renderPass(), create_info, i, subpass.pInputAttachments[j].attachment, subpass_to_node, 0,
                           skip);
        }
    }
    return skip;
}

static bool HasNonFramebufferStagePipelineStageFlags(VkPipelineStageFlags2KHR inflags) {
    return (inflags & ~(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)) != 0;
}

static bool HasFramebufferStagePipelineStageFlags(VkPipelineStageFlags2KHR inflags) {
    return (inflags & (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)) != 0;
}

bool CoreChecks::ValidateRenderPassDAG(RenderPassCreateVersion rp_version, const VkRenderPassCreateInfo2 *pCreateInfo) const {
    bool skip = false;
    const char *vuid;
    const bool use_rp2 = (rp_version == RENDER_PASS_VERSION_2);

    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency2 &dependency = pCreateInfo->pDependencies[i];
        auto latest_src_stage = sync_utils::GetLogicallyLatestGraphicsPipelineStage(dependency.srcStageMask);
        auto earliest_dst_stage = sync_utils::GetLogicallyEarliestGraphicsPipelineStage(dependency.dstStageMask);

        // The first subpass here serves as a good proxy for "is multiview enabled" - since all view masks need to be non-zero if
        // any are, which enables multiview.
        if (use_rp2 && (dependency.dependencyFlags & VK_DEPENDENCY_VIEW_LOCAL_BIT) && (pCreateInfo->pSubpasses[0].viewMask == 0)) {
            skip |= LogError(
                device, "VUID-VkRenderPassCreateInfo2-viewMask-03059",
                "Dependency %u specifies the VK_DEPENDENCY_VIEW_LOCAL_BIT, but multiview is not enabled for this render pass.", i);
        } else if (use_rp2 && !(dependency.dependencyFlags & VK_DEPENDENCY_VIEW_LOCAL_BIT) && dependency.viewOffset != 0) {
            skip |= LogError(device, "VUID-VkSubpassDependency2-dependencyFlags-03092",
                             "Dependency %u specifies the VK_DEPENDENCY_VIEW_LOCAL_BIT, but also specifies a view offset of %u.", i,
                             dependency.viewOffset);
        } else if (dependency.srcSubpass == VK_SUBPASS_EXTERNAL || dependency.dstSubpass == VK_SUBPASS_EXTERNAL) {
            if (dependency.srcSubpass == dependency.dstSubpass) {
                vuid = use_rp2 ? "VUID-VkSubpassDependency2-srcSubpass-03085" : "VUID-VkSubpassDependency-srcSubpass-00865";
                skip |= LogError(device, vuid, "The src and dst subpasses in dependency %u are both external.", i);
            } else if (dependency.dependencyFlags & VK_DEPENDENCY_VIEW_LOCAL_BIT) {
                if (dependency.srcSubpass == VK_SUBPASS_EXTERNAL) {
                    vuid = "VUID-VkSubpassDependency-dependencyFlags-02520";
                } else {  // dependency.dstSubpass == VK_SUBPASS_EXTERNAL
                    vuid = "VUID-VkSubpassDependency-dependencyFlags-02521";
                }
                if (use_rp2) {
                    // Create render pass 2 distinguishes between source and destination external dependencies.
                    if (dependency.srcSubpass == VK_SUBPASS_EXTERNAL) {
                        vuid = "VUID-VkSubpassDependency2-dependencyFlags-03090";
                    } else {
                        vuid = "VUID-VkSubpassDependency2-dependencyFlags-03091";
                    }
                }
                skip |=
                    LogError(device, vuid,
                             "Dependency %u specifies an external dependency but also specifies VK_DEPENDENCY_VIEW_LOCAL_BIT.", i);
            }
        } else if (dependency.srcSubpass > dependency.dstSubpass) {
            vuid = use_rp2 ? "VUID-VkSubpassDependency2-srcSubpass-03084" : "VUID-VkSubpassDependency-srcSubpass-00864";
            skip |= LogError(device, vuid,
                             "Dependency %u specifies a dependency from a later subpass (%u) to an earlier subpass (%u), which is "
                             "disallowed to prevent cyclic dependencies.",
                             i, dependency.srcSubpass, dependency.dstSubpass);
        } else if (dependency.srcSubpass == dependency.dstSubpass) {
            if (dependency.viewOffset != 0) {
                vuid = use_rp2 ? "VUID-VkSubpassDependency2-viewOffset-02530" : "VUID-VkRenderPassCreateInfo-pNext-01930";
                skip |= LogError(device, vuid, "Dependency %u specifies a self-dependency but has a non-zero view offset of %u", i,
                                 dependency.viewOffset);
            } else if ((dependency.dependencyFlags | VK_DEPENDENCY_VIEW_LOCAL_BIT) != dependency.dependencyFlags &&
                       pCreateInfo->pSubpasses[dependency.srcSubpass].viewMask > 1) {
                vuid = use_rp2 ? "VUID-VkRenderPassCreateInfo2-pDependencies-03060" : "VUID-VkSubpassDependency-srcSubpass-00872";
                skip |= LogError(device, vuid,
                                 "Dependency %u specifies a self-dependency for subpass %u with a non-zero view mask, but does not "
                                 "specify VK_DEPENDENCY_VIEW_LOCAL_BIT.",
                                 i, dependency.srcSubpass);
            } else if (HasFramebufferStagePipelineStageFlags(dependency.srcStageMask) &&
                       HasNonFramebufferStagePipelineStageFlags(dependency.dstStageMask)) {
                vuid = use_rp2 ? "VUID-VkSubpassDependency2-srcSubpass-06810" : "VUID-VkSubpassDependency-srcSubpass-06809";
                skip |= LogError(device, vuid,
                                 "Dependency %" PRIu32
                                 " specifies a self-dependency from a stage (%s) that accesses framebuffer space (%s) to a stage "
                                 "(%s) that accesses non-framebuffer space (%s).",
                                 i, sync_utils::StringPipelineStageFlags(latest_src_stage).c_str(),
                                 string_VkPipelineStageFlags(dependency.srcStageMask).c_str(),
                                 sync_utils::StringPipelineStageFlags(earliest_dst_stage).c_str(),
                                 string_VkPipelineStageFlags(dependency.dstStageMask).c_str());
            } else if ((HasNonFramebufferStagePipelineStageFlags(dependency.srcStageMask) == false) &&
                       (HasNonFramebufferStagePipelineStageFlags(dependency.dstStageMask) == false) &&
                       ((dependency.dependencyFlags & VK_DEPENDENCY_BY_REGION_BIT) == 0)) {
                vuid = use_rp2 ? "VUID-VkSubpassDependency2-srcSubpass-02245" : "VUID-VkSubpassDependency-srcSubpass-02243";
                skip |= LogError(device, vuid,
                                 "Dependency %u specifies a self-dependency for subpass %u with both stages including a "
                                 "framebuffer-space stage, but does not specify VK_DEPENDENCY_BY_REGION_BIT in dependencyFlags.",
                                 i, dependency.srcSubpass);
            }
        } else if ((dependency.srcSubpass < dependency.dstSubpass) &&
                   ((pCreateInfo->pSubpasses[dependency.srcSubpass].flags & VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM) != 0)) {
            vuid = use_rp2 ? "VUID-VkRenderPassCreateInfo2-flags-04909" : "VUID-VkSubpassDescription-flags-03343";
            skip |= LogError(device, vuid,
                             "Dependency %u specifies that subpass %u has a dependency on a later subpass"
                             "and includes VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM subpass flags.",
                             i, dependency.srcSubpass);
        }
    }
    return skip;
}

bool CoreChecks::ValidateCreateRenderPass(VkDevice device, RenderPassCreateVersion rp_version,
                                          const VkRenderPassCreateInfo2 *pCreateInfo, const char *function_name) const {
    bool skip = false;
    const bool use_rp2 = (rp_version == RENDER_PASS_VERSION_2);
    const char *vuid;

    skip |= ValidateRenderpassAttachmentUsage(rp_version, pCreateInfo, function_name);

    skip |= ValidateRenderPassDAG(rp_version, pCreateInfo);

    // Validate multiview correlation and view masks
    bool view_mask_zero = false;
    bool view_mask_non_zero = false;

    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription2 &subpass = pCreateInfo->pSubpasses[i];
        if (subpass.viewMask != 0) {
            view_mask_non_zero = true;
            if (!enabled_features.core11.multiview) {
                skip |= LogError(device, "VUID-VkSubpassDescription2-multiview-06558",
                                 "%s: pCreateInfo->pSubpasses[%" PRIu32 "].viewMask is %" PRIu32
                                 ", but multiview feature is not enabled.",
                                 function_name, i, subpass.viewMask);
            }
            int highest_view_bit = MostSignificantBit(subpass.viewMask);
            if (highest_view_bit > 0 &&
                static_cast<uint32_t>(highest_view_bit) >= phys_dev_ext_props.multiview_props.maxMultiviewViewCount) {
                skip |= LogError(device, "VUID-VkSubpassDescription2-viewMask-06706",
                                 "vkCreateRenderPass(): pCreateInfo::pSubpasses[%" PRIu32 "] highest bit (%" PRIu32
                                 ") is not less than VkPhysicalDeviceMultiviewProperties::maxMultiviewViewCount (%" PRIu32 ").",
                                 i, highest_view_bit, phys_dev_ext_props.multiview_props.maxMultiviewViewCount);
            }
        } else {
            view_mask_zero = true;
        }

        if ((subpass.flags & VK_SUBPASS_DESCRIPTION_PER_VIEW_POSITION_X_ONLY_BIT_NVX) != 0 &&
            (subpass.flags & VK_SUBPASS_DESCRIPTION_PER_VIEW_ATTRIBUTES_BIT_NVX) == 0) {
            vuid = use_rp2 ? "VUID-VkSubpassDescription2-flags-03076" : "VUID-VkSubpassDescription-flags-00856";
            skip |= LogError(device, vuid,
                             "%s: The flags parameter of subpass description %u includes "
                             "VK_SUBPASS_DESCRIPTION_PER_VIEW_POSITION_X_ONLY_BIT_NVX but does not also include "
                             "VK_SUBPASS_DESCRIPTION_PER_VIEW_ATTRIBUTES_BIT_NVX.",
                             function_name, i);
        }
    }

    if (rp_version == RENDER_PASS_VERSION_2) {
        if (view_mask_non_zero && view_mask_zero) {
            skip |= LogError(device, "VUID-VkRenderPassCreateInfo2-viewMask-03058",
                             "%s: Some view masks are non-zero whilst others are zero.", function_name);
        }

        if (view_mask_zero && pCreateInfo->correlatedViewMaskCount != 0) {
            skip |= LogError(device, "VUID-VkRenderPassCreateInfo2-viewMask-03057",
                             "%s: Multiview is not enabled but correlation masks are still provided", function_name);
        }
    }
    uint32_t aggregated_cvms = 0;
    for (uint32_t i = 0; i < pCreateInfo->correlatedViewMaskCount; ++i) {
        if (aggregated_cvms & pCreateInfo->pCorrelatedViewMasks[i]) {
            vuid = use_rp2 ? "VUID-VkRenderPassCreateInfo2-pCorrelatedViewMasks-03056"
                           : "VUID-VkRenderPassMultiviewCreateInfo-pCorrelationMasks-00841";
            skip |=
                LogError(device, vuid, "%s: pCorrelatedViewMasks[%u] contains a previously appearing view bit.", function_name, i);
        }
        aggregated_cvms |= pCreateInfo->pCorrelatedViewMasks[i];
    }

    const auto *fragment_density_map_info = LvlFindInChain<VkRenderPassFragmentDensityMapCreateInfoEXT>(pCreateInfo->pNext);
    if (fragment_density_map_info) {
        if (fragment_density_map_info->fragmentDensityMapAttachment.attachment != VK_ATTACHMENT_UNUSED) {
            if (fragment_density_map_info->fragmentDensityMapAttachment.attachment >= pCreateInfo->attachmentCount) {
                vuid = use_rp2 ? "VUID-VkRenderPassCreateInfo2-fragmentDensityMapAttachment-06472"
                               : "VUID-VkRenderPassCreateInfo-fragmentDensityMapAttachment-06471";
                skip |= LogError(device, vuid,
                                 "vkCreateRenderPass(): fragmentDensityMapAttachment %" PRIu32
                                 " must be less than attachmentCount %" PRIu32 " of for this render pass.",
                                 fragment_density_map_info->fragmentDensityMapAttachment.attachment, pCreateInfo->attachmentCount);
            } else {
                if (!(fragment_density_map_info->fragmentDensityMapAttachment.layout ==
                          VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT ||
                      fragment_density_map_info->fragmentDensityMapAttachment.layout == VK_IMAGE_LAYOUT_GENERAL)) {
                    skip |= LogError(device, "VUID-VkRenderPassFragmentDensityMapCreateInfoEXT-fragmentDensityMapAttachment-02549",
                                     "vkCreateRenderPass(): Layout of fragmentDensityMapAttachment %" PRIu32
                                     " must be equal to "
                                     "VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT, or VK_IMAGE_LAYOUT_GENERAL.",
                                     fragment_density_map_info->fragmentDensityMapAttachment.attachment);
                }
                if (!(pCreateInfo->pAttachments[fragment_density_map_info->fragmentDensityMapAttachment.attachment].loadOp ==
                          VK_ATTACHMENT_LOAD_OP_LOAD ||
                      pCreateInfo->pAttachments[fragment_density_map_info->fragmentDensityMapAttachment.attachment].loadOp ==
                          VK_ATTACHMENT_LOAD_OP_DONT_CARE)) {
                    skip |= LogError(device, "VUID-VkRenderPassFragmentDensityMapCreateInfoEXT-fragmentDensityMapAttachment-02550",
                                     "vkCreateRenderPass(): FragmentDensityMapAttachment %" PRIu32
                                     " must reference an attachment with a loadOp "
                                     "equal to VK_ATTACHMENT_LOAD_OP_LOAD or VK_ATTACHMENT_LOAD_OP_DONT_CARE.",
                                     fragment_density_map_info->fragmentDensityMapAttachment.attachment);
                }
                if (pCreateInfo->pAttachments[fragment_density_map_info->fragmentDensityMapAttachment.attachment].storeOp !=
                    VK_ATTACHMENT_STORE_OP_DONT_CARE) {
                    skip |= LogError(device, "VUID-VkRenderPassFragmentDensityMapCreateInfoEXT-fragmentDensityMapAttachment-02551",
                                     "vkCreateRenderPass(): FragmentDensityMapAttachment %" PRIu32
                                     " must reference an attachment with a storeOp "
                                     "equal to VK_ATTACHMENT_STORE_OP_DONT_CARE.",
                                     fragment_density_map_info->fragmentDensityMapAttachment.attachment);
                }
            }
        }
    }

    const LogObjectList objlist(device);

    auto func_name = use_rp2 ? Func::vkCreateRenderPass2 : Func::vkCreateRenderPass;
    auto structure = use_rp2 ? Struct::VkSubpassDependency2 : Struct::VkSubpassDependency;
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        auto const &dependency = pCreateInfo->pDependencies[i];
        Location loc(func_name, structure, Field::pDependencies, i);
        skip |= ValidateSubpassDependency(objlist, loc, dependency);
    }
    return skip;
}

bool CoreChecks::PreCallValidateCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) const {
    bool skip = false;
    // Handle extension structs from KHR_multiview and KHR_maintenance2 that can only be validated for RP1 (indices out of bounds)
    const VkRenderPassMultiviewCreateInfo *multiview_info = LvlFindInChain<VkRenderPassMultiviewCreateInfo>(pCreateInfo->pNext);
    if (multiview_info) {
        if (multiview_info->subpassCount && multiview_info->subpassCount != pCreateInfo->subpassCount) {
            skip |= LogError(device, "VUID-VkRenderPassCreateInfo-pNext-01928",
                             "vkCreateRenderPass(): Subpass count is %u but multiview info has a subpass count of %u.",
                             pCreateInfo->subpassCount, multiview_info->subpassCount);
        } else if (multiview_info->dependencyCount && multiview_info->dependencyCount != pCreateInfo->dependencyCount) {
            skip |= LogError(device, "VUID-VkRenderPassCreateInfo-pNext-01929",
                             "vkCreateRenderPass(): Dependency count is %u but multiview info has a dependency count of %u.",
                             pCreateInfo->dependencyCount, multiview_info->dependencyCount);
        }
        bool all_zero = true;
        bool all_not_zero = true;
        for (uint32_t i = 0; i < multiview_info->subpassCount; ++i) {
            all_zero &= multiview_info->pViewMasks[i] == 0;
            all_not_zero &= !(multiview_info->pViewMasks[i] == 0);
            if (MostSignificantBit(multiview_info->pViewMasks[i]) >=
                static_cast<int32_t>(phys_dev_props_core11.maxMultiviewViewCount)) {
                skip |= LogError(device, "VUID-VkRenderPassMultiviewCreateInfo-pViewMasks-06697",
                                 "vkCreateRenderPass(): Most significant bit in "
                                 "VkRenderPassMultiviewCreateInfo->pViewMask[%" PRIu32 "] (%" PRIu32
                                 ") must be less than maxMultiviewViewCount(%" PRIu32 ").",
                                 i, multiview_info->pViewMasks[i], phys_dev_props_core11.maxMultiviewViewCount);
            }
        }
        if (!all_zero && !all_not_zero) {
            skip |= LogError(
                device, "VUID-VkRenderPassCreateInfo-pNext-02513",
                "vkCreateRenderPass(): elements of VkRenderPassMultiviewCreateInfo pViewMasks must all be either 0 or not 0.");
        }
        if (all_zero && multiview_info->correlationMaskCount != 0) {
            skip |= LogError(device, "VUID-VkRenderPassCreateInfo-pNext-02515",
                             "vkCreateRenderPass(): VkRenderPassCreateInfo::correlationMaskCount is %" PRIu32
                             ", but all elements of pViewMasks are 0.",
                             multiview_info->correlationMaskCount);
        }
        for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
            if ((pCreateInfo->pDependencies[i].dependencyFlags & VK_DEPENDENCY_VIEW_LOCAL_BIT) == 0) {
                if (i < multiview_info->dependencyCount && multiview_info->pViewOffsets[i] != 0) {
                    skip |= LogError(device, "VUID-VkRenderPassCreateInfo-pNext-02512",
                                     "vkCreateRenderPass(): VkRenderPassCreateInfo::pDependencies[%" PRIu32
                                     "].dependencyFlags does not have VK_DEPENDENCY_VIEW_LOCAL_BIT bit set, but the corresponding "
                                     "VkRenderPassMultiviewCreateInfo::pViewOffsets[%" PRIu32 "] is %" PRIi32 ".",
                                     i, i, multiview_info->pViewOffsets[i]);
                }
            } else if (all_zero) {
                skip |=
                    LogError(device, "VUID-VkRenderPassCreateInfo-pNext-02514",
                             "vkCreateRenderPass(): VkRenderPassCreateInfo::pDependencies[%" PRIu32
                             "].dependencyFlags contains VK_DEPENDENCY_VIEW_LOCAL_BIT bit, but all elements of pViewMasks are 0.",
                             i);
            }
        }
    }
    const VkRenderPassInputAttachmentAspectCreateInfo *input_attachment_aspect_info =
        LvlFindInChain<VkRenderPassInputAttachmentAspectCreateInfo>(pCreateInfo->pNext);
    if (input_attachment_aspect_info) {
        for (uint32_t i = 0; i < input_attachment_aspect_info->aspectReferenceCount; ++i) {
            uint32_t subpass = input_attachment_aspect_info->pAspectReferences[i].subpass;
            uint32_t attachment = input_attachment_aspect_info->pAspectReferences[i].inputAttachmentIndex;
            if (subpass >= pCreateInfo->subpassCount) {
                skip |= LogError(device, "VUID-VkRenderPassCreateInfo-pNext-01926",
                                 "vkCreateRenderPass(): Subpass index %u specified by input attachment aspect info %u is greater "
                                 "than the subpass "
                                 "count of %u for this render pass.",
                                 subpass, i, pCreateInfo->subpassCount);
            } else if (pCreateInfo->pSubpasses && attachment >= pCreateInfo->pSubpasses[subpass].inputAttachmentCount) {
                skip |= LogError(device, "VUID-VkRenderPassCreateInfo-pNext-01927",
                                 "vkCreateRenderPass(): Input attachment index %u specified by input attachment aspect info %u is "
                                 "greater than the "
                                 "input attachment count of %u for this subpass.",
                                 attachment, i, pCreateInfo->pSubpasses[subpass].inputAttachmentCount);
            }
        }
    }

    if (!skip) {
        safe_VkRenderPassCreateInfo2 create_info_2;
        ConvertVkRenderPassCreateInfoToV2KHR(*pCreateInfo, &create_info_2);
        skip |= ValidateCreateRenderPass(device, RENDER_PASS_VERSION_1, create_info_2.ptr(), "vkCreateRenderPass()");
    }

    return skip;
}

// VK_KHR_depth_stencil_resolve was added with a requirement on VK_KHR_create_renderpass2 so this will never be able to use
// VkRenderPassCreateInfo
bool CoreChecks::ValidateDepthStencilResolve(const VkRenderPassCreateInfo2 *pCreateInfo, const char *function_name) const {
    bool skip = false;

    // If the pNext list of VkSubpassDescription2 includes a VkSubpassDescriptionDepthStencilResolve structure,
    // then that structure describes depth/stencil resolve operations for the subpass.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
        const VkSubpassDescription2 &subpass = pCreateInfo->pSubpasses[i];
        const auto *resolve = LvlFindInChain<VkSubpassDescriptionDepthStencilResolve>(subpass.pNext);

        // All of the VUs are wrapped in the wording:
        // "If pDepthStencilResolveAttachment is not NULL"
        if (resolve == nullptr || resolve->pDepthStencilResolveAttachment == nullptr) {
            continue;
        }

        // The spec says
        // "If pDepthStencilAttachment is NULL, or if its attachment index is VK_ATTACHMENT_UNUSED, it indicates that no
        // depth/stencil attachment will be used in the subpass."
        if (subpass.pDepthStencilAttachment == nullptr) {
            continue;
        } else if (subpass.pDepthStencilAttachment->attachment == VK_ATTACHMENT_UNUSED) {
            // while should be ignored, this is an explicit VU and some drivers will crash if this is let through
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03177",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve "
                             "structure with resolve attachment %" PRIu32 ", but pDepthStencilAttachment=VK_ATTACHMENT_UNUSED.",
                             function_name, i, resolve->pDepthStencilResolveAttachment->attachment);
            continue;
        }

        const uint32_t ds_attachment = subpass.pDepthStencilAttachment->attachment;
        const uint32_t resolve_attachment = resolve->pDepthStencilResolveAttachment->attachment;

        // ValidateAttachmentIndex() should catch if this is invalid, but skip to avoid crashing
        if (ds_attachment >= pCreateInfo->attachmentCount) {
            continue;
        }

        // All VUs in VkSubpassDescriptionDepthStencilResolve are wrapped with language saying it is not unused
        if (resolve_attachment == VK_ATTACHMENT_UNUSED) {
            continue;
        }

        if (resolve_attachment >= pCreateInfo->attachmentCount) {
            skip |= LogError(device, "VUID-VkRenderPassCreateInfo2-pSubpasses-06473",
                             "%s: pDepthStencilResolveAttachment %" PRIu32 " must be less than attachmentCount %" PRIu32
                             " of for this render pass.",
                             function_name, resolve_attachment, pCreateInfo->attachmentCount);
            // if the index is invalid need to skip everything else to prevent out of bounds index accesses crashing
            continue;
        }

        const VkFormat ds_attachment_format = pCreateInfo->pAttachments[ds_attachment].format;
        const VkFormat resolve_attachment_format = pCreateInfo->pAttachments[resolve_attachment].format;

        // "depthResolveMode is ignored if the VkFormat of the pDepthStencilResolveAttachment does not have a depth component"
        const bool resolve_has_depth = FormatHasDepth(resolve_attachment_format);
        // "stencilResolveMode is ignored if the VkFormat of the pDepthStencilResolveAttachment does not have a stencil component"
        const bool resolve_has_stencil = FormatHasStencil(resolve_attachment_format);

        if (resolve_has_depth) {
            if (!(resolve->depthResolveMode == VK_RESOLVE_MODE_NONE ||
                  resolve->depthResolveMode & phys_dev_props_core12.supportedDepthResolveModes)) {
                skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-depthResolveMode-03183",
                                 "%s: Subpass %" PRIu32
                                 " includes a VkSubpassDescriptionDepthStencilResolve "
                                 "structure with invalid depthResolveMode (%s), must be VK_RESOLVE_MODE_NONE or a value from "
                                 "supportedDepthResolveModes (%s).",
                                 function_name, i, string_VkResolveModeFlagBits(resolve->depthResolveMode),
                                 string_VkResolveModeFlags(phys_dev_props_core12.supportedDepthResolveModes).c_str());
            }
        }

        if (resolve_has_stencil) {
            if (!(resolve->stencilResolveMode == VK_RESOLVE_MODE_NONE ||
                  resolve->stencilResolveMode & phys_dev_props_core12.supportedStencilResolveModes)) {
                skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-stencilResolveMode-03184",
                                 "%s: Subpass %" PRIu32
                                 " includes a VkSubpassDescriptionDepthStencilResolve "
                                 "structure with invalid stencilResolveMode (%s), must be VK_RESOLVE_MODE_NONE or a value from "
                                 "supportedStencilResolveModes (%s).",
                                 function_name, i, string_VkResolveModeFlagBits(resolve->stencilResolveMode),
                                 string_VkResolveModeFlags(phys_dev_props_core12.supportedStencilResolveModes).c_str());
            }
        }

        if (resolve_has_depth && resolve_has_stencil) {
            if (phys_dev_props_core12.independentResolve == VK_FALSE && phys_dev_props_core12.independentResolveNone == VK_FALSE &&
                !(resolve->depthResolveMode == resolve->stencilResolveMode)) {
                skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03185",
                                 "%s: Subpass %" PRIu32
                                 " includes a VkSubpassDescriptionDepthStencilResolve "
                                 "structure. The values of depthResolveMode (%s) and stencilResolveMode (%s) must be identical.",
                                 function_name, i, string_VkResolveModeFlagBits(resolve->depthResolveMode),
                                 string_VkResolveModeFlagBits(resolve->stencilResolveMode));
            }

            if (phys_dev_props_core12.independentResolve == VK_FALSE && phys_dev_props_core12.independentResolveNone == VK_TRUE &&
                !(resolve->depthResolveMode == resolve->stencilResolveMode || resolve->depthResolveMode == VK_RESOLVE_MODE_NONE ||
                  resolve->stencilResolveMode == VK_RESOLVE_MODE_NONE)) {
                skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03186",
                                 "%s: Subpass %" PRIu32
                                 " includes a VkSubpassDescriptionDepthStencilResolve "
                                 "structure. The values of depthResolveMode (%s) and stencilResolveMode (%s) must be identical, or "
                                 "one of them must be VK_RESOLVE_MODE_NONE.",
                                 function_name, i, string_VkResolveModeFlagBits(resolve->depthResolveMode),
                                 string_VkResolveModeFlagBits(resolve->stencilResolveMode));
            }
        }

        // Same VU, but better error message if one of the resolves are ignored
        if (resolve_has_depth && !resolve_has_stencil && resolve->depthResolveMode == VK_RESOLVE_MODE_NONE) {
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03178",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve structure with resolve "
                             "attachment %" PRIu32
                             ", but the depth resolve mode is VK_RESOLVE_MODE_NONE (stencil resolve mode is "
                             "ignored due to format not having stencil component).",
                             function_name, i, resolve_attachment);
        } else if (!resolve_has_depth && resolve_has_stencil && resolve->stencilResolveMode == VK_RESOLVE_MODE_NONE) {
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03178",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve structure with resolve "
                             "attachment %" PRIu32
                             ", but the stencil resolve mode is VK_RESOLVE_MODE_NONE (depth resolve mode is "
                             "ignored due to format not having depth component).",
                             function_name, i, resolve_attachment);
        } else if (resolve_has_depth && resolve_has_stencil && resolve->depthResolveMode == VK_RESOLVE_MODE_NONE &&
                   resolve->stencilResolveMode == VK_RESOLVE_MODE_NONE) {
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03178",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve structure with resolve "
                             "attachment %" PRIu32 ", but both depth and stencil resolve modes are VK_RESOLVE_MODE_NONE.",
                             function_name, i, resolve_attachment);
        }

        const uint32_t resolve_depth_size = FormatDepthSize(resolve_attachment_format);
        const uint32_t resolve_stencil_size = FormatStencilSize(resolve_attachment_format);

        if (resolve_depth_size > 0 &&
            ((FormatDepthSize(ds_attachment_format) != resolve_depth_size) ||
             (FormatDepthNumericalType(ds_attachment_format) != FormatDepthNumericalType(ds_attachment_format)))) {
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03181",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve "
                             "structure with resolve attachment %" PRIu32 " which has a depth component (size %" PRIu32
                             "). The depth component "
                             "of pDepthStencilAttachment must have the same number of bits (currently %" PRIu32
                             ") and the same numerical type.",
                             function_name, i, resolve_attachment, resolve_depth_size, FormatDepthSize(ds_attachment_format));
        }

        if (resolve_stencil_size > 0 &&
            ((FormatStencilSize(ds_attachment_format) != resolve_stencil_size) ||
             (FormatStencilNumericalType(ds_attachment_format) != FormatStencilNumericalType(resolve_attachment_format)))) {
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03182",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve "
                             "structure with resolve attachment %" PRIu32 " which has a stencil component (size %" PRIu32
                             "). The stencil component "
                             "of pDepthStencilAttachment must have the same number of bits (currently %" PRIu32
                             ") and the same numerical type.",
                             function_name, i, resolve_attachment, resolve_stencil_size, FormatStencilSize(ds_attachment_format));
        }

        if (pCreateInfo->pAttachments[ds_attachment].samples == VK_SAMPLE_COUNT_1_BIT) {
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03179",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve "
                             "structure with resolve attachment %" PRIu32
                             ". However pDepthStencilAttachment has sample count=VK_SAMPLE_COUNT_1_BIT.",
                             function_name, i, resolve_attachment);
        }

        if (pCreateInfo->pAttachments[resolve_attachment].samples != VK_SAMPLE_COUNT_1_BIT) {
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-03180",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve "
                             "structure with resolve attachment %" PRIu32 " which has sample count=VK_SAMPLE_COUNT_1_BIT.",
                             function_name, i, resolve_attachment);
        }

        const VkFormatFeatureFlags2KHR potential_format_features = GetPotentialFormatFeatures(resolve_attachment_format);
        if ((potential_format_features & VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT_KHR) == 0) {
            skip |= LogError(device, "VUID-VkSubpassDescriptionDepthStencilResolve-pDepthStencilResolveAttachment-02651",
                             "%s: Subpass %" PRIu32
                             " includes a VkSubpassDescriptionDepthStencilResolve "
                             "structure with resolve attachment %" PRIu32
                             " with a format (%s) whose features do not contain VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT.",
                             function_name, i, resolve_attachment, string_VkFormat(resolve_attachment_format));
        }

        //  VK_QCOM_render_pass_shader_resolve check of depth/stencil attachmnent
        if ((subpass.flags & VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM) != 0) {
            skip |= LogError(device, "VUID-VkRenderPassCreateInfo2-flags-04908",
                             "%s: Subpass %" PRIu32
                             " enables shader resolve, which requires the depth/stencil resolve attachment"
                             " must be VK_ATTACHMENT_UNUSED, but a reference to attachment %" PRIu32 " was found instead.",
                             function_name, i, resolve_attachment);
        }
    }

    return skip;
}

bool CoreChecks::ValidateCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass,
                                           const char *function_name) const {
    bool skip = false;

    if (IsExtEnabled(device_extensions.vk_khr_depth_stencil_resolve)) {
        skip |= ValidateDepthStencilResolve(pCreateInfo, function_name);
    }

    skip |= ValidateFragmentShadingRateAttachments(device, pCreateInfo);

    safe_VkRenderPassCreateInfo2 create_info_2(pCreateInfo);
    skip |= ValidateCreateRenderPass(device, RENDER_PASS_VERSION_2, create_info_2.ptr(), function_name);

    return skip;
}

bool CoreChecks::ValidateFragmentShadingRateAttachments(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo) const {
    bool skip = false;

    if (enabled_features.fragment_shading_rate_features.attachmentFragmentShadingRate) {
        for (uint32_t attachment_description = 0; attachment_description < pCreateInfo->attachmentCount; ++attachment_description) {
            std::vector<uint32_t> used_as_fragment_shading_rate_attachment;

            // Prepass to find any use as a fragment shading rate attachment structures and validate them independently
            for (uint32_t subpass = 0; subpass < pCreateInfo->subpassCount; ++subpass) {
                const VkFragmentShadingRateAttachmentInfoKHR *fragment_shading_rate_attachment =
                    LvlFindInChain<VkFragmentShadingRateAttachmentInfoKHR>(pCreateInfo->pSubpasses[subpass].pNext);

                if (fragment_shading_rate_attachment && fragment_shading_rate_attachment->pFragmentShadingRateAttachment) {
                    const VkAttachmentReference2 &attachment_reference =
                        *(fragment_shading_rate_attachment->pFragmentShadingRateAttachment);
                    if (attachment_reference.attachment == attachment_description) {
                        used_as_fragment_shading_rate_attachment.push_back(subpass);
                    }

                    if (((pCreateInfo->flags & VK_RENDER_PASS_CREATE_TRANSFORM_BIT_QCOM) != 0) &&
                        (attachment_reference.attachment != VK_ATTACHMENT_UNUSED)) {
                        skip |= LogError(device, "VUID-VkRenderPassCreateInfo2-flags-04521",
                                         "vkCreateRenderPass2: Render pass includes VK_RENDER_PASS_CREATE_TRANSFORM_BIT_QCOM but "
                                         "a fragment shading rate attachment is specified in subpass %u.",
                                         subpass);
                    }

                    if (attachment_reference.attachment != VK_ATTACHMENT_UNUSED) {
                        const VkFormatFeatureFlags2KHR potential_format_features =
                            GetPotentialFormatFeatures(pCreateInfo->pAttachments[attachment_reference.attachment].format);

                        if (!(potential_format_features & VK_FORMAT_FEATURE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)) {
                            skip |= LogError(device, "VUID-VkRenderPassCreateInfo2-pAttachments-04586",
                                             "vkCreateRenderPass2: Attachment description %u is used in subpass %u as a fragment "
                                             "shading rate attachment, but specifies format %s, which does not support "
                                             "VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR.",
                                             attachment_reference.attachment, subpass,
                                             string_VkFormat(pCreateInfo->pAttachments[attachment_reference.attachment].format));
                        }

                        if (attachment_reference.layout != VK_IMAGE_LAYOUT_GENERAL &&
                            attachment_reference.layout != VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR) {
                            skip |= LogError(
                                device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04524",
                                "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u specifies a layout of %s.",
                                subpass, string_VkImageLayout(attachment_reference.layout));
                        }

                        if (!IsPowerOfTwo(fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width)) {
                            skip |=
                                LogError(device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04525",
                                         "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u has a "
                                         "non-power-of-two texel width of %u.",
                                         subpass, fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width);
                        }
                        if (fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width <
                            phys_dev_ext_props.fragment_shading_rate_props.minFragmentShadingRateAttachmentTexelSize.width) {
                            skip |= LogError(
                                device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04526",
                                "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u has a texel width of %u which "
                                "is lower than the advertised minimum width %u.",
                                subpass, fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width,
                                phys_dev_ext_props.fragment_shading_rate_props.minFragmentShadingRateAttachmentTexelSize.width);
                        }
                        if (fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width >
                            phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSize.width) {
                            skip |= LogError(
                                device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04527",
                                "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u has a texel width of %u which "
                                "is higher than the advertised maximum width %u.",
                                subpass, fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width,
                                phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSize.width);
                        }
                        if (!IsPowerOfTwo(fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height)) {
                            skip |=
                                LogError(device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04528",
                                         "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u has a "
                                         "non-power-of-two texel height of %u.",
                                         subpass, fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height);
                        }
                        if (fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height <
                            phys_dev_ext_props.fragment_shading_rate_props.minFragmentShadingRateAttachmentTexelSize.height) {
                            skip |= LogError(
                                device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04529",
                                "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u has a texel height of %u "
                                "which is lower than the advertised minimum height %u.",
                                subpass, fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height,
                                phys_dev_ext_props.fragment_shading_rate_props.minFragmentShadingRateAttachmentTexelSize.height);
                        }
                        if (fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height >
                            phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSize.height) {
                            skip |= LogError(
                                device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04530",
                                "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u has a texel height of %u "
                                "which is higher than the advertised maximum height %u.",
                                subpass, fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height,
                                phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSize.height);
                        }
                        uint32_t aspect_ratio = fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width /
                                                fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height;
                        uint32_t inverse_aspect_ratio = fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height /
                                                        fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width;
                        if (aspect_ratio >
                            phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSizeAspectRatio) {
                            skip |= LogError(
                                device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04531",
                                "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u has a texel size of %u by %u, "
                                "which has an aspect ratio %u, which is higher than the advertised maximum aspect ratio %u.",
                                subpass, fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width,
                                fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height, aspect_ratio,
                                phys_dev_ext_props.fragment_shading_rate_props
                                    .maxFragmentShadingRateAttachmentTexelSizeAspectRatio);
                        }
                        if (inverse_aspect_ratio >
                            phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSizeAspectRatio) {
                            skip |= LogError(
                                device, "VUID-VkFragmentShadingRateAttachmentInfoKHR-pFragmentShadingRateAttachment-04532",
                                "vkCreateRenderPass2: Fragment shading rate attachment in subpass %u has a texel size of %u by %u, "
                                "which has an inverse aspect ratio of %u, which is higher than the advertised maximum aspect ratio "
                                "%u.",
                                subpass, fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.width,
                                fragment_shading_rate_attachment->shadingRateAttachmentTexelSize.height, inverse_aspect_ratio,
                                phys_dev_ext_props.fragment_shading_rate_props
                                    .maxFragmentShadingRateAttachmentTexelSizeAspectRatio);
                        }
                    }
                }
            }

            // Lambda function turning a vector of integers into a string
            auto vector_to_string = [&](std::vector<uint32_t> vector) {
                std::stringstream ss;
                size_t size = vector.size();
                for (size_t i = 0; i < used_as_fragment_shading_rate_attachment.size(); i++) {
                    if (size == 2 && i == 1) {
                        ss << " and ";
                    } else if (size > 2 && i == size - 2) {
                        ss << ", and ";
                    } else if (i != 0) {
                        ss << ", ";
                    }
                    ss << vector[i];
                }
                return ss.str();
            };

            // Search for other uses of the same attachment
            if (!used_as_fragment_shading_rate_attachment.empty()) {
                for (uint32_t subpass = 0; subpass < pCreateInfo->subpassCount; ++subpass) {
                    const VkSubpassDescription2 &subpass_info = pCreateInfo->pSubpasses[subpass];
                    const VkSubpassDescriptionDepthStencilResolve *depth_stencil_resolve_attachment =
                        LvlFindInChain<VkSubpassDescriptionDepthStencilResolve>(subpass_info.pNext);

                    std::string fsr_attachment_subpasses_string = vector_to_string(used_as_fragment_shading_rate_attachment);

                    for (uint32_t attachment = 0; attachment < subpass_info.colorAttachmentCount; ++attachment) {
                        if (subpass_info.pColorAttachments[attachment].attachment == attachment_description) {
                            skip |= LogError(
                                device, "VUID-VkRenderPassCreateInfo2-pAttachments-04585",
                                "vkCreateRenderPass2: Attachment description %u is used as a fragment shading rate attachment in "
                                "subpass(es) %s but also as color attachment %u in subpass %u",
                                attachment_description, fsr_attachment_subpasses_string.c_str(), attachment, subpass);
                        }
                    }
                    for (uint32_t attachment = 0; attachment < subpass_info.colorAttachmentCount; ++attachment) {
                        if (subpass_info.pResolveAttachments &&
                            subpass_info.pResolveAttachments[attachment].attachment == attachment_description) {
                            skip |= LogError(
                                device, "VUID-VkRenderPassCreateInfo2-pAttachments-04585",
                                "vkCreateRenderPass2: Attachment description %u is used as a fragment shading rate attachment in "
                                "subpass(es) %s but also as color resolve attachment %u in subpass %u",
                                attachment_description, fsr_attachment_subpasses_string.c_str(), attachment, subpass);
                        }
                    }
                    for (uint32_t attachment = 0; attachment < subpass_info.inputAttachmentCount; ++attachment) {
                        if (subpass_info.pInputAttachments[attachment].attachment == attachment_description) {
                            skip |= LogError(
                                device, "VUID-VkRenderPassCreateInfo2-pAttachments-04585",
                                "vkCreateRenderPass2: Attachment description %u is used as a fragment shading rate attachment in "
                                "subpass(es) %s but also as input attachment %u in subpass %u",
                                attachment_description, fsr_attachment_subpasses_string.c_str(), attachment, subpass);
                        }
                    }
                    if (subpass_info.pDepthStencilAttachment) {
                        if (subpass_info.pDepthStencilAttachment->attachment == attachment_description) {
                            skip |= LogError(
                                device, "VUID-VkRenderPassCreateInfo2-pAttachments-04585",
                                "vkCreateRenderPass2: Attachment description %u is used as a fragment shading rate attachment in "
                                "subpass(es) %s but also as the depth/stencil attachment in subpass %u",
                                attachment_description, fsr_attachment_subpasses_string.c_str(), subpass);
                        }
                    }
                    if (depth_stencil_resolve_attachment && depth_stencil_resolve_attachment->pDepthStencilResolveAttachment) {
                        if (depth_stencil_resolve_attachment->pDepthStencilResolveAttachment->attachment ==
                            attachment_description) {
                            skip |= LogError(
                                device, "VUID-VkRenderPassCreateInfo2-pAttachments-04585",
                                "vkCreateRenderPass2: Attachment description %u is used as a fragment shading rate attachment in "
                                "subpass(es) %s but also as the depth/stencil resolve attachment in subpass %u",
                                attachment_description, fsr_attachment_subpasses_string.c_str(), subpass);
                        }
                    }
                }
            }
        }
    }

    return skip;
}

bool CoreChecks::PreCallValidateCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo,
                                                     const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) const {
    return ValidateCreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass, "vkCreateRenderPass2KHR()");
}

bool CoreChecks::PreCallValidateCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) const {
    return ValidateCreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass, "vkCreateRenderPass2()");
}

bool CoreChecks::ValidateRenderingInfoAttachment(const std::shared_ptr<const IMAGE_VIEW_STATE> &image_view, const char *attachment,
                                                 const VkRenderingInfo *pRenderingInfo, const char *func_name) const {
    bool skip = false;

    // Upcasting to handle overflow
    const bool x_extent_valid =
        static_cast<int64_t>(image_view->image_state->createInfo.extent.width) >=
        static_cast<int64_t>(pRenderingInfo->renderArea.offset.x) + static_cast<int64_t>(pRenderingInfo->renderArea.extent.width);
    const bool y_extent_valid =
        static_cast<int64_t>(image_view->image_state->createInfo.extent.height) >=
        static_cast<int64_t>(pRenderingInfo->renderArea.offset.y) + static_cast<int64_t>(pRenderingInfo->renderArea.extent.height);
    if (IsExtEnabled(device_extensions.vk_khr_device_group)) {
        auto device_group_render_pass_begin_info = LvlFindInChain<VkDeviceGroupRenderPassBeginInfo>(pRenderingInfo->pNext);
        if (!device_group_render_pass_begin_info || device_group_render_pass_begin_info->deviceRenderAreaCount == 0) {
            if (!x_extent_valid) {
                skip |= LogError(image_view->Handle(), "VUID-VkRenderingInfo-pNext-06079",
                                 "%s(): %s width (%" PRIu32 ") is less than pRenderingInfo->renderArea.offset.x (%" PRIu32
                                 ") + pRenderingInfo->renderArea.extent.width (%" PRIu32 ").",
                                 func_name, attachment, image_view->image_state->createInfo.extent.width,
                                 pRenderingInfo->renderArea.offset.x, pRenderingInfo->renderArea.extent.width);
            }

            if (!y_extent_valid) {
                skip |= LogError(image_view->Handle(), "VUID-VkRenderingInfo-pNext-06080",
                                 "%s(): %s height (%" PRIu32 ") is less than pRenderingInfo->renderArea.offset.y (%" PRIu32
                                 ") + pRenderingInfo->renderArea.extent.width (%" PRIu32 ").",
                                 func_name, attachment, image_view->image_state->createInfo.extent.height,
                                 pRenderingInfo->renderArea.offset.y, pRenderingInfo->renderArea.extent.height);
            }
        }
    } else {
        if (!x_extent_valid) {
            skip |= LogError(image_view->Handle(), "VUID-VkRenderingInfo-imageView-06075",
                             "%s(): %s width (%" PRIu32 ") is less than pRenderingInfo->renderArea.offset.x (%" PRIu32
                             ") + pRenderingInfo->renderArea.extent.width (%" PRIu32 ").",
                             func_name, attachment, image_view->image_state->createInfo.extent.width,
                             pRenderingInfo->renderArea.offset.x, pRenderingInfo->renderArea.extent.width);
        }
        if (!y_extent_valid) {
            skip |= LogError(image_view->Handle(), "VUID-VkRenderingInfo-imageView-06076",
                             "%s(): %s height (%" PRIu32 ") is less than pRenderingInfo->renderArea.offset.y (%" PRIu32
                             ") + pRenderingInfo->renderArea.extent.width (%" PRIu32 ").",
                             func_name, attachment, image_view->image_state->createInfo.extent.height,
                             pRenderingInfo->renderArea.offset.y, pRenderingInfo->renderArea.extent.height);
        }
    }

    return skip;
}

static bool UniqueImageViews(const VkRenderingInfo *pRenderingInfo, VkImageView imageView) {
    bool unique_views = true;
    for (uint32_t i = 0; i < pRenderingInfo->colorAttachmentCount; ++i) {
        if (pRenderingInfo->pColorAttachments[i].imageView == imageView) {
            unique_views = false;
        }

        if (pRenderingInfo->pColorAttachments[i].resolveImageView == imageView) {
            unique_views = false;
        }
    }

    if (pRenderingInfo->pDepthAttachment) {
        if (pRenderingInfo->pDepthAttachment->imageView == imageView) {
            unique_views = false;
        }

        if (pRenderingInfo->pDepthAttachment->resolveImageView == imageView) {
            unique_views = false;
        }
    }

    if (pRenderingInfo->pStencilAttachment) {
        if (pRenderingInfo->pStencilAttachment->imageView == imageView) {
            unique_views = false;
        }

        if (pRenderingInfo->pStencilAttachment->resolveImageView == imageView) {
            unique_views = false;
        }
    }
    return unique_views;
}

bool CoreChecks::ValidateCmdBeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo *pRenderingInfo,
                                           CMD_TYPE cmd_type) const {
    auto cb_state = GetRead<CMD_BUFFER_STATE>(commandBuffer);
    if (!cb_state) return false;
    bool skip = false;
    const char *func_name = CommandTypeString(cmd_type);

    const auto chained_device_group_struct = LvlFindInChain<VkDeviceGroupRenderPassBeginInfo>(pRenderingInfo->pNext);
    const bool non_zero_device_render_area = chained_device_group_struct && chained_device_group_struct->deviceRenderAreaCount != 0;

    if (!enabled_features.core13.dynamicRendering) {
        skip |= LogError(commandBuffer, "VUID-vkCmdBeginRendering-dynamicRendering-06446", "%s(): dynamicRendering is not enabled.",
                         func_name);
    }

    if ((cb_state->createInfo.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) &&
        ((pRenderingInfo->flags & VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR) != 0)) {
        skip |= LogError(commandBuffer, "VUID-vkCmdBeginRendering-commandBuffer-06068",
                         "%s(): pRenderingInfo->flags must not include "
                         "VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT_KHR in a secondary command buffer.",
                         func_name);
    }

    if (pRenderingInfo->viewMask == 0 && pRenderingInfo->layerCount == 0) {
        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-viewMask-06069",
                         "%s(): If viewMask is 0 (%" PRIu32 "), layerCount must not be 0 (%" PRIu32 ").", func_name,
                         pRenderingInfo->viewMask, pRenderingInfo->layerCount);
    }

    const auto rendering_fragment_shading_rate_attachment_info =
        LvlFindInChain<VkRenderingFragmentShadingRateAttachmentInfoKHR>(pRenderingInfo->pNext);
    // Upcasting to handle overflow
    const auto x_adjusted_extent =
        static_cast<int64_t>(pRenderingInfo->renderArea.offset.x) + static_cast<int64_t>(pRenderingInfo->renderArea.extent.width);
    const auto y_adjusted_extent =
        static_cast<int64_t>(pRenderingInfo->renderArea.offset.y) + static_cast<int64_t>(pRenderingInfo->renderArea.extent.height);
    if (rendering_fragment_shading_rate_attachment_info &&
        (rendering_fragment_shading_rate_attachment_info->imageView != VK_NULL_HANDLE)) {
        auto view_state = Get<IMAGE_VIEW_STATE>(rendering_fragment_shading_rate_attachment_info->imageView);
        if (pRenderingInfo->viewMask == 0) {
            if (view_state->create_info.subresourceRange.layerCount != 1 &&
                view_state->create_info.subresourceRange.layerCount < pRenderingInfo->layerCount) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06123",
                                 "%s(): imageView must have a layerCount (%" PRIu32
                                 ") that is either equal to 1 or greater than or equal to "
                                 "VkRenderingInfo::layerCount (%" PRIu32 ").",
                                 func_name, view_state->create_info.subresourceRange.layerCount, pRenderingInfo->layerCount);
            }
        } else {
            int highest_view_bit = MostSignificantBit(pRenderingInfo->viewMask);
            int32_t layer_count = view_state->normalized_subresource_range.layerCount;
            if (layer_count != 1 && layer_count < highest_view_bit) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06124",
                                 "%s(): imageView must have a layerCount (%" PRIi32
                                 ") that either is equal to 1 or greater than "
                                 " or equal to the index of the most significant bit in viewMask (%d)",
                                 func_name, layer_count, highest_view_bit);
            }
        }

        if ((view_state->inherited_usage & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR) == 0) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06148",
                             "%s(): VkRenderingFragmentShadingRateAttachmentInfoKHR::imageView was not created with "
                             "VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR.",
                             func_name);
        }

        if (UniqueImageViews(pRenderingInfo, rendering_fragment_shading_rate_attachment_info->imageView) == false) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06125",
                             "%s(): imageView or resolveImageView member of pDepthAttachment, pStencilAttachment, or any element "
                             "of pColorAttachments must not equal VkRenderingFragmentShadingRateAttachmentInfoKHR->imageView.",
                             func_name);
        }

        if (rendering_fragment_shading_rate_attachment_info->imageLayout != VK_IMAGE_LAYOUT_GENERAL &&
            rendering_fragment_shading_rate_attachment_info->imageLayout !=
                VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06147",
                             "%s(): VkRenderingFragmentShadingRateAttachmentInfoKHR->layout (%s) must be VK_IMAGE_LAYOUT_GENERAL "
                             "or VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR.",
                             func_name, string_VkImageLayout(rendering_fragment_shading_rate_attachment_info->imageLayout));
        }

        if (!IsPowerOfTwo(rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width)) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06149",
                             "%s(): shadingRateAttachmentTexelSize.width (%u) must be a power of two.", func_name,
                             rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width);
        }

        auto max_frs_attach_texel_width =
            phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSize.width;
        if (rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width > max_frs_attach_texel_width) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06150",
                             "%s(): shadingRateAttachmentTexelSize.width (%u) must be less than or equal to "
                             "maxFragmentShadingRateAttachmentTexelSize.width (%u).",
                             func_name, rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width,
                             max_frs_attach_texel_width);
        }

        auto min_frs_attach_texel_width =
            phys_dev_ext_props.fragment_shading_rate_props.minFragmentShadingRateAttachmentTexelSize.width;
        if (rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width < min_frs_attach_texel_width) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06151",
                             "%s(): shadingRateAttachmentTexelSize.width (%u) must be greater than or equal to "
                             "minFragmentShadingRateAttachmentTexelSize.width (%u).",
                             func_name, rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width,
                             min_frs_attach_texel_width);
        }

        if (!IsPowerOfTwo(rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height)) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06152",
                             "%s(): shadingRateAttachmentTexelSize.height (%u) must be a power of two.", func_name,
                             rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height);
        }

        auto max_frs_attach_texel_height =
            phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSize.height;
        if (rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height > max_frs_attach_texel_height) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06153",
                             "%s(): shadingRateAttachmentTexelSize.height (%u) must be less than or equal to "
                             "maxFragmentShadingRateAttachmentTexelSize.height (%u).",
                             func_name, rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height,
                             max_frs_attach_texel_height);
        }

        auto min_frs_attach_texel_height =
            phys_dev_ext_props.fragment_shading_rate_props.minFragmentShadingRateAttachmentTexelSize.height;
        if (rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height < min_frs_attach_texel_height) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06154",
                             "%s(): shadingRateAttachmentTexelSize.height (%u) must be greater than or equal to "
                             "minFragmentShadingRateAttachmentTexelSize.height (%u).",
                             func_name, rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height,
                             min_frs_attach_texel_height);
        }

        auto max_frs_attach_texel_aspect_ratio =
            phys_dev_ext_props.fragment_shading_rate_props.maxFragmentShadingRateAttachmentTexelSizeAspectRatio;
        if ((rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width /
             rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height) >
            max_frs_attach_texel_aspect_ratio) {
            skip |= LogError(
                commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06155",
                "%s(): the quotient of shadingRateAttachmentTexelSize.width (%u) and shadingRateAttachmentTexelSize.height (%u) "
                "must be less than or equal to maxFragmentShadingRateAttachmentTexelSizeAspectRatio (%u).",
                func_name, rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width,
                rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height,
                max_frs_attach_texel_aspect_ratio);
        }

        if ((rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height /
             rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width) >
            max_frs_attach_texel_aspect_ratio) {
            skip |= LogError(
                commandBuffer, "VUID-VkRenderingFragmentShadingRateAttachmentInfoKHR-imageView-06156",
                "%s(): the quotient of shadingRateAttachmentTexelSize.height (%u) and shadingRateAttachmentTexelSize.width (%u) "
                "must be less than or equal to maxFragmentShadingRateAttachmentTexelSizeAspectRatio (%u).",
                func_name, rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height,
                rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width,
                max_frs_attach_texel_aspect_ratio);
        }

        if (!non_zero_device_render_area) {
            if (static_cast<int64_t>(view_state->image_state->createInfo.extent.width) <
                layer_data::GetQuotientCeil(
                    x_adjusted_extent,
                    static_cast<int64_t>(rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width))) {
                const char *vuid = IsExtEnabled(device_extensions.vk_khr_device_group) ? "VUID-VkRenderingInfo-pNext-06119"
                                                                                       : "VUID-VkRenderingInfo-imageView-06117";
                skip |= LogError(commandBuffer, vuid,
                                 "%s(): width of VkRenderingFragmentShadingRateAttachmentInfoKHR imageView (%" PRIu32
                                 ") must not be less than (pRenderingInfo->renderArea.offset.x (%" PRIu32
                                 ") + pRenderingInfo->renderArea.extent.width (%" PRIu32
                                 ") ) / shadingRateAttachmentTexelSize.width (%" PRIu32 ").",
                                 func_name, view_state->image_state->createInfo.extent.width, pRenderingInfo->renderArea.offset.x,
                                 pRenderingInfo->renderArea.extent.width,
                                 rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width);
            }

            if (static_cast<int64_t>(view_state->image_state->createInfo.extent.height) <
                layer_data::GetQuotientCeil(
                    y_adjusted_extent,
                    static_cast<int64_t>(rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height))) {
                const char *vuid = IsExtEnabled(device_extensions.vk_khr_device_group) ? "VUID-VkRenderingInfo-pNext-06121"
                                                                                       : "VUID-VkRenderingInfo-imageView-06118";
                skip |= LogError(commandBuffer, vuid,
                                 "%s(): height of VkRenderingFragmentShadingRateAttachmentInfoKHR imageView (%" PRIu32
                                 ") must not be less than (pRenderingInfo->renderArea.offset.y (%" PRIu32
                                 ") + pRenderingInfo->renderArea.extent.height (%" PRIu32
                                 ") ) / shadingRateAttachmentTexelSize.height (%" PRIu32 ").",
                                 func_name, view_state->image_state->createInfo.extent.height, pRenderingInfo->renderArea.offset.y,
                                 pRenderingInfo->renderArea.extent.height,
                                 rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height);
            }
        } else {
            if (chained_device_group_struct) {
                for (uint32_t deviceRenderAreaIndex = 0; deviceRenderAreaIndex < chained_device_group_struct->deviceRenderAreaCount;
                     ++deviceRenderAreaIndex) {
                    auto offset_x = chained_device_group_struct->pDeviceRenderAreas[deviceRenderAreaIndex].offset.x;
                    auto width = chained_device_group_struct->pDeviceRenderAreas[deviceRenderAreaIndex].extent.width;
                    auto offset_y = chained_device_group_struct->pDeviceRenderAreas[deviceRenderAreaIndex].offset.y;
                    auto height = chained_device_group_struct->pDeviceRenderAreas[deviceRenderAreaIndex].extent.height;

                    IMAGE_STATE *image_state = view_state->image_state.get();
                    if (image_state->createInfo.extent.width <
                        layer_data::GetQuotientCeil(
                            offset_x + width,
                            rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width)) {
                        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06120",
                                         "%s(): width of VkRenderingFragmentShadingRateAttachmentInfoKHR imageView (%" PRIu32
                                         ") must not be less than (VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32
                                         "].offset.x (%" PRIu32 ") + VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32
                                         "].extent.width (%" PRIu32 ") ) / shadingRateAttachmentTexelSize.width (%" PRIu32 ").",
                                         func_name, image_state->createInfo.extent.width, deviceRenderAreaIndex, offset_x,
                                         deviceRenderAreaIndex, width,
                                         rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.width);
                    }
                    if (image_state->createInfo.extent.height <
                        layer_data::GetQuotientCeil(
                            offset_y + height,
                            rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height)) {
                        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06122",
                                         "%s(): height of VkRenderingFragmentShadingRateAttachmentInfoKHR imageView (%" PRIu32
                                         ") must not be less than (VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32
                                         "].offset.y (%" PRIu32 ") + VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32
                                         "].extent.height (%" PRIu32
                                         ") ) / shadingRateAttachmentTexelSize.height "
                                         "(%" PRIu32 ").",
                                         func_name, image_state->createInfo.extent.height, deviceRenderAreaIndex, offset_y,
                                         deviceRenderAreaIndex, height,
                                         rendering_fragment_shading_rate_attachment_info->shadingRateAttachmentTexelSize.height);
                    }
                }
            }
        }
    }

    if (!(IsExtEnabled(device_extensions.vk_amd_mixed_attachment_samples) ||
          IsExtEnabled(device_extensions.vk_nv_framebuffer_mixed_samples) ||
          (enabled_features.multisampled_render_to_single_sampled_features.multisampledRenderToSingleSampled))) {
        uint32_t first_sample_count_attachment = VK_ATTACHMENT_UNUSED;
        for (uint32_t j = 0; j < pRenderingInfo->colorAttachmentCount; ++j) {
            if (pRenderingInfo->pColorAttachments[j].imageView != VK_NULL_HANDLE) {
                const auto image_view = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pColorAttachments[j].imageView);
                first_sample_count_attachment = (first_sample_count_attachment == VK_ATTACHMENT_UNUSED)
                                                    ? static_cast<uint32_t>(image_view->samples)
                                                    : first_sample_count_attachment;
                if (first_sample_count_attachment != image_view->samples) {
                    skip |=
                        LogError(commandBuffer, "VUID-VkRenderingInfo-multisampledRenderToSingleSampled-06857",
                                 "%s(): Color attachment ref %" PRIu32
                                 " has sample count %s, whereas first used color "
                                 "attachment ref has sample count %" PRIu32 ".",
                                 func_name, j, string_VkSampleCountFlagBits(image_view->samples), first_sample_count_attachment);
                }
                std::stringstream msg;
                msg << "Color attachment ref % " << j;
                skip |= ValidateRenderingInfoAttachment(image_view, msg.str().c_str(), pRenderingInfo, func_name);
            }
        }
        if (pRenderingInfo->pDepthAttachment && pRenderingInfo->pDepthAttachment->imageView != VK_NULL_HANDLE) {
            const auto image_view = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pDepthAttachment->imageView);
            first_sample_count_attachment = (first_sample_count_attachment == VK_ATTACHMENT_UNUSED)
                                                ? static_cast<uint32_t>(image_view->samples)
                                                : first_sample_count_attachment;
            if (first_sample_count_attachment != image_view->samples) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-multisampledRenderToSingleSampled-06857",
                                 "%s(): Depth attachment ref has sample count %s, whereas first used color "
                                 "attachment ref has sample count %" PRIu32 ".",
                                 func_name, string_VkSampleCountFlagBits(image_view->samples), (first_sample_count_attachment));
            }
            skip |= ValidateRenderingInfoAttachment(image_view, "Depth attachment ref", pRenderingInfo, func_name);
        }
        if (pRenderingInfo->pStencilAttachment && pRenderingInfo->pStencilAttachment->imageView != VK_NULL_HANDLE) {
            const auto image_view = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pStencilAttachment->imageView);
            first_sample_count_attachment = (first_sample_count_attachment == VK_ATTACHMENT_UNUSED)
                                                ? static_cast<uint32_t>(image_view->samples)
                                                : first_sample_count_attachment;
            if (first_sample_count_attachment != image_view->samples) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-multisampledRenderToSingleSampled-06857",
                                 "%s(): Stencil attachment ref has sample count %s, whereas another "
                                 "attachment ref has sample count %" PRIu32 ".",
                                 func_name, string_VkSampleCountFlagBits(image_view->samples), (first_sample_count_attachment));
            }
            skip |= ValidateRenderingInfoAttachment(image_view, "Stencil attachment ref", pRenderingInfo, func_name);
        }
    }

    if (pRenderingInfo->colorAttachmentCount > phys_dev_props.limits.maxColorAttachments) {
        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-colorAttachmentCount-06106",
                         "%s(): colorAttachmentCount (%u) must be less than or equal to "
                         "VkPhysicalDeviceLimits::maxColorAttachments (%u).",
                         func_name, pRenderingInfo->colorAttachmentCount, phys_dev_props.limits.maxColorAttachments);
    }

    auto fragment_density_map_attachment_info =
        LvlFindInChain<VkRenderingFragmentDensityMapAttachmentInfoEXT>(pRenderingInfo->pNext);
    if (fragment_density_map_attachment_info) {
        if (!enabled_features.fragment_density_map_features.fragmentDensityMapNonSubsampledImages) {
            for (uint32_t j = 0; j < pRenderingInfo->colorAttachmentCount; ++j) {
                if (pRenderingInfo->pColorAttachments[j].imageView != VK_NULL_HANDLE) {
                    auto image_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pColorAttachments[j].imageView);
                    if (!(image_view_state->image_state->createInfo.flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT)) {
                        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06107",
                                         "%s(): color image must be created with VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT.", func_name);
                    }
                }
            }

            if (pRenderingInfo->pDepthAttachment && (pRenderingInfo->pDepthAttachment->imageView != VK_NULL_HANDLE)) {
                auto depth_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pDepthAttachment->imageView);
                if (!(depth_view_state->image_state->createInfo.flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT)) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06107",
                                     "%s(): depth image must be created with VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT.", func_name);
                }
            }

            if (pRenderingInfo->pStencilAttachment && (pRenderingInfo->pStencilAttachment->imageView != VK_NULL_HANDLE)) {
                auto stencil_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pStencilAttachment->imageView);
                if (!(stencil_view_state->image_state->createInfo.flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT)) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06107",
                                     "%s(): stencil image must be created with VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT.", func_name);
                }
            }
        }

        if (UniqueImageViews(pRenderingInfo, fragment_density_map_attachment_info->imageView) == false) {
            skip |=
                LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06116",
                         "%s(): imageView or resolveImageView member of pDepthAttachment, pStencilAttachment, or any"
                         "element of pColorAttachments must not equal VkRenderingFragmentDensityMapAttachmentInfoEXT->imageView.",
                         func_name);
        }

        if (fragment_density_map_attachment_info->imageView != VK_NULL_HANDLE) {
            if (rendering_fragment_shading_rate_attachment_info &&
                rendering_fragment_shading_rate_attachment_info->imageView != fragment_density_map_attachment_info->imageView) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06126",
                                 "%s(): imageView of VkRenderingFragmentShadingRateAttachmentInfoKHR is not equal to imageView of "
                                 "VkRenderingFragmentDensityMapAttachmentInfoEXT.",
                                 func_name);
            }
            auto fragment_density_map_view_state = Get<IMAGE_VIEW_STATE>(fragment_density_map_attachment_info->imageView);
            if ((fragment_density_map_view_state->inherited_usage & VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT) == 0) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentDensityMapAttachmentInfoEXT-imageView-06158",
                                 "%s(): imageView of VkRenderingFragmentDensityMapAttachmentInfoEXT usage (%s) "
                                 "does not include VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT.",
                                 func_name, string_VkImageUsageFlags(fragment_density_map_view_state->inherited_usage).c_str());
            }
            if ((fragment_density_map_view_state->image_state->createInfo.flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT) > 0) {
                skip |= LogError(
                    commandBuffer, "VUID-VkRenderingFragmentDensityMapAttachmentInfoEXT-imageView-06159",
                    "%s(): image used in imageView of VkRenderingFragmentDensityMapAttachmentInfoEXT was created with flags %s.",
                    func_name, string_VkImageCreateFlags(fragment_density_map_view_state->image_state->createInfo.flags).c_str());
            }
            int32_t layer_count = static_cast<int32_t>(fragment_density_map_view_state->normalized_subresource_range.layerCount);
            if (layer_count != 1) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentDensityMapAttachmentInfoEXT-imageView-06160",
                                 "%s(): imageView of VkRenderingFragmentDensityMapAttachmentInfoEXT must "
                                 "have a layer count ("
                                 "%" PRIi32 ") equal to 1.",
                                 func_name, layer_count);
            }
            if ((pRenderingInfo->viewMask == 0) && (layer_count != 1)) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06109",
                                 "%s(): imageView of VkRenderingFragmentDensityMapAttachmentInfoEXT must "
                                 "have a layer count ("
                                 "%" PRIi32 ") equal to 1 when viewMask is equal to 0",
                                 func_name, layer_count);
            }

            if ((pRenderingInfo->viewMask != 0) && (layer_count < MostSignificantBit(pRenderingInfo->viewMask))) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-imageView-06108",
                                 "%s(): imageView of VkRenderingFragmentDensityMapAttachmentInfoEXT must "
                                 "have a layer count ("
                                 "%" PRIi32 ") greater than or equal to the most significant bit in viewMask (%" PRIu32 ")",
                                 func_name, layer_count, pRenderingInfo->viewMask);
            }
            if (fragment_density_map_attachment_info->imageLayout != VK_IMAGE_LAYOUT_GENERAL &&
                fragment_density_map_attachment_info->imageLayout != VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingFragmentDensityMapAttachmentInfoEXT-imageView-06157",
                                 "%s(): VkRenderingFragmentDensityMapAttachmentInfoEXT::imageView is not VK_NULL_HANDLE, but "
                                 "VkRenderingFragmentDensityMapAttachmentInfoEXT::imageLayout is %s.",
                                 func_name, string_VkImageLayout(fragment_density_map_attachment_info->imageLayout));
            }
        }
    }

    if ((enabled_features.core11.multiview == VK_FALSE) && (pRenderingInfo->viewMask != 0)) {
        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-multiview-06127",
                         "%s(): If the multiview feature is not enabled, viewMask must be 0 (%u).", func_name,
                         pRenderingInfo->viewMask);
    }

    if (!non_zero_device_render_area) {
        if (IsExtEnabled(device_extensions.vk_khr_device_group)) {
            if (pRenderingInfo->renderArea.offset.x < 0) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06077",
                                 "%s(): renderArea.offset.x is %d and must be greater than 0.", func_name,
                                 pRenderingInfo->renderArea.offset.x);
            }

            if (pRenderingInfo->renderArea.offset.y < 0) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06078",
                                 "%s(): renderArea.offset.y is %d and must be greater than 0.", func_name,
                                 pRenderingInfo->renderArea.offset.y);
            }
        }

        if (fragment_density_map_attachment_info && fragment_density_map_attachment_info->imageView != VK_NULL_HANDLE) {
            auto view_state = Get<IMAGE_VIEW_STATE>(fragment_density_map_attachment_info->imageView);
            IMAGE_STATE *image_state = view_state->image_state.get();
            if (image_state->createInfo.extent.width <
                layer_data::GetQuotientCeil(
                    x_adjusted_extent,
                    static_cast<int64_t>(phys_dev_ext_props.fragment_density_map_props.maxFragmentDensityTexelSize.width))) {
                const char *vuid = IsExtEnabled(device_extensions.vk_khr_device_group) ? "VUID-VkRenderingInfo-pNext-06112"
                                                                                       : "VUID-VkRenderingInfo-imageView-06110";
                skip |= LogError(
                    commandBuffer, vuid,
                    "%s(): width of VkRenderingFragmentDensityMapAttachmentInfoEXT imageView (%" PRIu32
                    ") must not be less than (pRenderingInfo->renderArea.offset.x (%" PRIu32
                    ") + pRenderingInfo->renderArea.extent.width (%" PRIu32
                    ") ) / VkPhysicalDeviceFragmentDensityMapPropertiesEXT::maxFragmentDensityTexelSize.width (%" PRIu32 ").",
                    func_name, image_state->createInfo.extent.width, pRenderingInfo->renderArea.offset.x,
                    pRenderingInfo->renderArea.extent.width,
                    phys_dev_ext_props.fragment_density_map_props.maxFragmentDensityTexelSize.width);
            }
            if (image_state->createInfo.extent.height <
                layer_data::GetQuotientCeil(
                    y_adjusted_extent,
                    static_cast<int64_t>(phys_dev_ext_props.fragment_density_map_props.maxFragmentDensityTexelSize.height))) {
                const char *vuid = IsExtEnabled(device_extensions.vk_khr_device_group) ? "VUID-VkRenderingInfo-pNext-06114"
                                                                                       : "VUID-VkRenderingInfo-imageView-06111";
                skip |= LogError(
                    commandBuffer, vuid,
                    "%s(): height of VkRenderingFragmentDensityMapAttachmentInfoEXT imageView (%" PRIu32
                    ") must not be less than (pRenderingInfo->renderArea.offset.y (%" PRIu32
                    ") + pRenderingInfo->renderArea.extent.height (%" PRIu32
                    ") ) / VkPhysicalDeviceFragmentDensityMapPropertiesEXT::maxFragmentDensityTexelSize.height (%" PRIu32 ").",
                    func_name, image_state->createInfo.extent.height, pRenderingInfo->renderArea.offset.y,
                    pRenderingInfo->renderArea.extent.height,
                    phys_dev_ext_props.fragment_density_map_props.maxFragmentDensityTexelSize.height);
            }
        }
    }

    if (chained_device_group_struct) {
        for (uint32_t deviceRenderAreaIndex = 0; deviceRenderAreaIndex < chained_device_group_struct->deviceRenderAreaCount;
             ++deviceRenderAreaIndex) {
            auto offset_x = chained_device_group_struct->pDeviceRenderAreas[deviceRenderAreaIndex].offset.x;
            auto width = chained_device_group_struct->pDeviceRenderAreas[deviceRenderAreaIndex].extent.width;
            if (!(offset_x >= 0)) {
                skip |= LogError(commandBuffer, "VUID-VkDeviceGroupRenderPassBeginInfo-offset-06166",
                                 "%s(): pDeviceRenderAreas[%u].offset.x: %d must be greater than or equal to 0.", func_name,
                                 deviceRenderAreaIndex, offset_x);
            }
            if ((offset_x + width) > phys_dev_props.limits.maxFramebufferWidth) {
                skip |= LogError(commandBuffer, "VUID-VkDeviceGroupRenderPassBeginInfo-offset-06168",
                                 "vkCmdBeginRenderingKHR(): pDeviceRenderAreas[%" PRIu32 "] sum of offset.x (%" PRId32
                                 ") and extent.width (%" PRIu32 ") is greater than maxFramebufferWidth (%" PRIu32 ").",
                                 deviceRenderAreaIndex, offset_x, width, phys_dev_props.limits.maxFramebufferWidth);
            }
            auto offset_y = chained_device_group_struct->pDeviceRenderAreas[deviceRenderAreaIndex].offset.y;
            auto height = chained_device_group_struct->pDeviceRenderAreas[deviceRenderAreaIndex].extent.height;
            if (!(offset_y >= 0)) {
                skip |= LogError(commandBuffer, "VUID-VkDeviceGroupRenderPassBeginInfo-offset-06167",
                                 "%s(): pDeviceRenderAreas[%u].offset.y: %d must be greater than or equal to 0.", func_name,
                                 deviceRenderAreaIndex, offset_y);
            }
            if ((offset_y + height) > phys_dev_props.limits.maxFramebufferHeight) {
                skip |= LogError(commandBuffer, "VUID-VkDeviceGroupRenderPassBeginInfo-offset-06169",
                                 "vkCmdBeginRenderingKHR(): pDeviceRenderAreas[%" PRIu32 "] sum of offset.y (%" PRId32
                                 ") and extent.height (%" PRIu32 ") is greater than maxFramebufferHeight (%" PRIu32 ").",
                                 deviceRenderAreaIndex, offset_y, height, phys_dev_props.limits.maxFramebufferHeight);
            }

            for (uint32_t j = 0; j < pRenderingInfo->colorAttachmentCount; ++j) {
                if (pRenderingInfo->pColorAttachments[j].imageView != VK_NULL_HANDLE) {
                    auto image_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pColorAttachments[j].imageView);
                    IMAGE_STATE *image_state = image_view_state->image_state.get();
                    if (!(image_state->createInfo.extent.width >= offset_x + width)) {
                        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06083",
                                         "%s(): width of the pColorAttachments[%" PRIu32 "].imageView: %" PRIu32
                                         " must be greater than or equal to"
                                         "renderArea.offset.x (%" PRIu32 ") + renderArea.extent.width (%" PRIu32 ").",
                                         func_name, j, image_state->createInfo.extent.width, offset_x, width);
                    }
                    if (!(image_state->createInfo.extent.height >= offset_y + height)) {
                        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06084",
                                         "%s(): height of the pColorAttachments[%" PRIu32 "].imageView: %" PRIu32
                                         " must be greater than or equal to"
                                         "renderArea.offset.y (%" PRIu32 ") + renderArea.extent.height (%" PRIu32 ").",
                                         func_name, j, image_state->createInfo.extent.height, offset_y, height);
                    }
                }
            }

            if (pRenderingInfo->pDepthAttachment != VK_NULL_HANDLE) {
                auto depth_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pDepthAttachment->imageView);
                IMAGE_STATE *image_state = depth_view_state->image_state.get();
                if (!(image_state->createInfo.extent.width >= offset_x + width)) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06083",
                                     "%s(): width of the pDepthAttachment->imageView: %" PRIu32
                                     " must be greater than or equal to"
                                     "renderArea.offset.x (%" PRIu32 ") + renderArea.extent.width (%" PRIu32 ").",
                                     func_name, image_state->createInfo.extent.width, offset_x, width);
                }
                if (!(image_state->createInfo.extent.height >= offset_y + height)) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06084",
                                     "%s(): height of the pDepthAttachment->imageView: %" PRIu32
                                     " must be greater than or equal to"
                                     "renderArea.offset.y (%" PRIu32 ") + renderArea.extent.height (%" PRIu32 ").",
                                     func_name, image_state->createInfo.extent.height, offset_y, height);
                }
            }

            if (pRenderingInfo->pStencilAttachment != VK_NULL_HANDLE) {
                auto stencil_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pStencilAttachment->imageView);
                IMAGE_STATE *image_state = stencil_view_state->image_state.get();
                if (!(image_state->createInfo.extent.width >= offset_x + width)) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06083",
                                     "%s(): width of the pStencilAttachment->imageView: %" PRIu32
                                     " must be greater than or equal to"
                                     "renderArea.offset.x (%" PRIu32 ") +  renderArea.extent.width (%" PRIu32 ").",
                                     func_name, image_state->createInfo.extent.width, offset_x, width);
                }
                if (!(image_state->createInfo.extent.height >= offset_y + height)) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pNext-06084",
                                     "%s(): height of the pStencilAttachment->imageView: %" PRIu32
                                     " must be greater than or equal to"
                                     "renderArea.offset.y (%" PRIu32 ") +  renderArea.extent.height(%" PRIu32 ").",
                                     func_name, image_state->createInfo.extent.height, offset_y, height);
                }
            }

            if (fragment_density_map_attachment_info && fragment_density_map_attachment_info->imageView != VK_NULL_HANDLE) {
                auto view_state = Get<IMAGE_VIEW_STATE>(fragment_density_map_attachment_info->imageView);
                IMAGE_STATE *image_state = view_state->image_state.get();
                if (image_state->createInfo.extent.width <
                    layer_data::GetQuotientCeil(offset_x + width,
                                                phys_dev_ext_props.fragment_density_map_props.maxFragmentDensityTexelSize.width)) {
                    skip |= LogError(
                        commandBuffer, "VUID-VkRenderingInfo-pNext-06113",
                        "%s(): width of VkRenderingFragmentDensityMapAttachmentInfoEXT imageView (%" PRIu32
                        ") must not be less than (VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32
                        "].offset.x (%" PRIu32 ") + VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32
                        "].extent.width (%" PRIu32
                        ") ) / VkPhysicalDeviceFragmentDensityMapPropertiesEXT::maxFragmentDensityTexelSize.width (%" PRIu32 ").",
                        func_name, image_state->createInfo.extent.width, deviceRenderAreaIndex, offset_x, deviceRenderAreaIndex,
                        width, phys_dev_ext_props.fragment_density_map_props.maxFragmentDensityTexelSize.width);
                }
                if (image_state->createInfo.extent.height <
                    layer_data::GetQuotientCeil(offset_y + height,
                                                phys_dev_ext_props.fragment_density_map_props.maxFragmentDensityTexelSize.height)) {
                    skip |= LogError(
                        commandBuffer, "VUID-VkRenderingInfo-pNext-06115",
                        "%s(): height of VkRenderingFragmentDensityMapAttachmentInfoEXT imageView (%" PRIu32
                        ") must not be less than (VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32
                        "].offset.y (%" PRIu32 ") + VkDeviceGroupRenderPassBeginInfo::pDeviceRenderAreas[%" PRIu32
                        "].extent.height (%" PRIu32
                        ") ) / VkPhysicalDeviceFragmentDensityMapPropertiesEXT::maxFragmentDensityTexelSize.height (%" PRIu32 ").",
                        func_name, image_state->createInfo.extent.height, deviceRenderAreaIndex, offset_y, deviceRenderAreaIndex,
                        height, phys_dev_ext_props.fragment_density_map_props.maxFragmentDensityTexelSize.height);
                }
            }
        }
    }

    if (pRenderingInfo->pDepthAttachment != nullptr && pRenderingInfo->pStencilAttachment != nullptr) {
        if (pRenderingInfo->pDepthAttachment->imageView != VK_NULL_HANDLE &&
            pRenderingInfo->pStencilAttachment->imageView != VK_NULL_HANDLE) {
            if (!(pRenderingInfo->pDepthAttachment->imageView == pRenderingInfo->pStencilAttachment->imageView)) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pDepthAttachment-06085",
                                 "%s(): imageView of pDepthAttachment and pStencilAttachment must be the same.", func_name);
            }

            if ((phys_dev_props_core12.independentResolveNone == VK_FALSE) &&
                (pRenderingInfo->pDepthAttachment->resolveMode != pRenderingInfo->pStencilAttachment->resolveMode)) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pDepthAttachment-06104",
                                 "%s(): The values of depthResolveMode (%s) and stencilResolveMode (%s) must be identical.",
                                 func_name, string_VkResolveModeFlagBits(pRenderingInfo->pDepthAttachment->resolveMode),
                                 string_VkResolveModeFlagBits(pRenderingInfo->pStencilAttachment->resolveMode));
            }

            if ((phys_dev_props_core12.independentResolve == VK_FALSE) &&
                (pRenderingInfo->pDepthAttachment->resolveMode != VK_RESOLVE_MODE_NONE) &&
                (pRenderingInfo->pStencilAttachment->resolveMode != VK_RESOLVE_MODE_NONE) &&
                (pRenderingInfo->pStencilAttachment->resolveMode != pRenderingInfo->pDepthAttachment->resolveMode)) {
                skip |= LogError(device, "VUID-VkRenderingInfo-pDepthAttachment-06105",
                                 "%s(): The values of depthResolveMode (%s) and stencilResolveMode (%s) must "
                                 "be identical, or one of them must be VK_RESOLVE_MODE_NONE.",
                                 func_name, string_VkResolveModeFlagBits(pRenderingInfo->pDepthAttachment->resolveMode),
                                 string_VkResolveModeFlagBits(pRenderingInfo->pStencilAttachment->resolveMode));
            }
        }

        if (pRenderingInfo->pDepthAttachment->resolveMode != VK_RESOLVE_MODE_NONE &&
            pRenderingInfo->pStencilAttachment->resolveMode != VK_RESOLVE_MODE_NONE) {
            if (!(pRenderingInfo->pDepthAttachment->resolveImageView == pRenderingInfo->pStencilAttachment->resolveImageView)) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pDepthAttachment-06086",
                                 "%s(): resolveImageView of pDepthAttachment and pStencilAttachment must be the same.", func_name);
            }
        }
    }

    for (uint32_t j = 0; j < pRenderingInfo->colorAttachmentCount; ++j) {
        skip |= ValidateRenderingAttachmentInfo(commandBuffer, pRenderingInfo, &pRenderingInfo->pColorAttachments[j], func_name);

        if (pRenderingInfo->pColorAttachments[j].imageView != VK_NULL_HANDLE) {
            auto image_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pColorAttachments[j].imageView);
            IMAGE_STATE *image_state = image_view_state->image_state.get();
            if (!(image_state->createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
                skip |= LogError(
                    commandBuffer, "VUID-VkRenderingInfo-colorAttachmentCount-06087",
                    "%s(): VkRenderingInfo->colorAttachment[%u] must have been created with VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT.",
                    func_name, j);
            }

            if (pRenderingInfo->pColorAttachments[j].imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                pRenderingInfo->pColorAttachments[j].imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-colorAttachmentCount-06090",
                                 "%s(): imageLayout must not be "
                                 "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL or "
                                 "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL.",
                                 func_name);
            }

            if (pRenderingInfo->pColorAttachments[j].resolveMode != VK_RESOLVE_MODE_NONE) {
                if (pRenderingInfo->pColorAttachments[j].resolveImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                    pRenderingInfo->pColorAttachments[j].resolveImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-colorAttachmentCount-06091",
                                     "%s(): resolveImageLayout must not be "
                                     "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL or "
                                     "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL.",
                                     func_name);
                }
            }

            if (pRenderingInfo->pColorAttachments[j].imageLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
                pRenderingInfo->pColorAttachments[j].imageLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-colorAttachmentCount-06096",
                                 "%s(): imageLayout must not be "
                                 "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL "
                                 "or VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL.",
                                 func_name);
            }

            if (pRenderingInfo->pColorAttachments[j].resolveMode != VK_RESOLVE_MODE_NONE) {
                if (pRenderingInfo->pColorAttachments[j].resolveImageLayout ==
                        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
                    pRenderingInfo->pColorAttachments[j].resolveImageLayout ==
                        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-colorAttachmentCount-06097",
                                     "%s(): resolveImageLayout must not be "
                                     "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL or "
                                     "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL.",
                                     func_name);
                }
            }

            if (pRenderingInfo->pColorAttachments[j].imageLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
                pRenderingInfo->pColorAttachments[j].imageLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL ||
                pRenderingInfo->pColorAttachments[j].imageLayout == VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL ||
                pRenderingInfo->pColorAttachments[j].imageLayout == VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-colorAttachmentCount-06100",
                                 "%s(): imageLayout must not be VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL"
                                 " or VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL"
                                 " or VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL"
                                 " or VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL.",
                                 func_name);
            }

            if (pRenderingInfo->pColorAttachments[j].resolveMode != VK_RESOLVE_MODE_NONE) {
                if (pRenderingInfo->pColorAttachments[j].resolveImageLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
                    pRenderingInfo->pColorAttachments[j].resolveImageLayout == VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-colorAttachmentCount-06101",
                                     "%s(): resolveImageLayout must not be VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL or "
                                     "VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL.",
                                     func_name);
                }
            }
        }
    }

    if (pRenderingInfo->pDepthAttachment) {
        skip |= ValidateRenderingAttachmentInfo(commandBuffer, pRenderingInfo, pRenderingInfo->pDepthAttachment, func_name);

        if (pRenderingInfo->pDepthAttachment->imageView != VK_NULL_HANDLE) {
            auto depth_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pDepthAttachment->imageView);
            IMAGE_STATE *image_state = depth_view_state->image_state.get();
            if (!(image_state->createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pDepthAttachment-06088",
                                 "%s(): depth image must have been created with VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT.",
                                 func_name);
            }

            if (pRenderingInfo->pDepthAttachment->imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                skip |=
                    LogError(commandBuffer, "VUID-VkRenderingInfo-pDepthAttachment-06092",
                             "%s(): image must not have been created with VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.", func_name);
            }

            if (pRenderingInfo->pDepthAttachment->resolveMode != VK_RESOLVE_MODE_NONE) {
                if (pRenderingInfo->pDepthAttachment->resolveImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pDepthAttachment-06093",
                                     "%s(): resolveImageLayout must not be VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.", func_name);
                }

                if (IsExtEnabled(device_extensions.vk_khr_maintenance2) &&
                    pRenderingInfo->pDepthAttachment->resolveImageLayout ==
                        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pDepthAttachment-06098",
                                     "%s(): resolveImageLayout must not be "
                                     "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL.",
                                     func_name);
                }

                if (IsExtEnabled(device_extensions.vk_khr_depth_stencil_resolve) &&
                    !(pRenderingInfo->pDepthAttachment->resolveMode & phys_dev_props_core12.supportedDepthResolveModes)) {
                    skip |= LogError(device, "VUID-VkRenderingInfo-pDepthAttachment-06102",
                                     "%s(): Includes a resolveMode structure with invalid mode=%u.", func_name,
                                     pRenderingInfo->pDepthAttachment->resolveMode);
                }
            }

            if (!FormatHasDepth(depth_view_state->create_info.format)) {
                skip |=
                    LogError(commandBuffer, "VUID-VkRenderingInfo-pDepthAttachment-06547",
                             "%s(): pRenderingInfo->pDepthAttachment->imageView was created with a format (%s) that does not have "
                             "a depth aspect.",
                             func_name, string_VkFormat(depth_view_state->create_info.format));
            }
        }
    }

    if (pRenderingInfo->pStencilAttachment != nullptr) {
        skip |= ValidateRenderingAttachmentInfo(commandBuffer, pRenderingInfo, pRenderingInfo->pStencilAttachment, func_name);

        if (pRenderingInfo->pStencilAttachment->imageView != VK_NULL_HANDLE) {
            auto stencil_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pStencilAttachment->imageView);
            IMAGE_STATE *image_state = stencil_view_state->image_state.get();
            if (!(image_state->createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pStencilAttachment-06089",
                                 "%s(): stencil image must have been created with VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT.",
                                 func_name);
            }

            if (pRenderingInfo->pStencilAttachment->imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pStencilAttachment-06094",
                                 "%s(): imageLayout must not be VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.", func_name);
            }

            if (pRenderingInfo->pStencilAttachment->resolveMode != VK_RESOLVE_MODE_NONE) {
                if (pRenderingInfo->pStencilAttachment->resolveImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pStencilAttachment-06095",
                                     "%s(): resolveImageLayout must not be VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.", func_name);
                }
                if (pRenderingInfo->pStencilAttachment->resolveImageLayout ==
                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL) {
                    skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-pStencilAttachment-06099",
                                     "%s(): resolveImageLayout must not be "
                                     "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL.",
                                     func_name);
                }
                if (!(pRenderingInfo->pStencilAttachment->resolveMode & phys_dev_props_core12.supportedStencilResolveModes)) {
                    skip |= LogError(device, "VUID-VkRenderingInfo-pStencilAttachment-06103",
                                     "%s(): Includes a resolveMode structure with invalid mode (%s).", func_name,
                                     string_VkResolveModeFlagBits(pRenderingInfo->pStencilAttachment->resolveMode));
                }
            }

            if (!FormatHasStencil(stencil_view_state->create_info.format)) {
                skip |= LogError(
                    commandBuffer, "VUID-VkRenderingInfo-pStencilAttachment-06548",
                    "%s(): pRenderingInfo->pStencilAttachment->imageView was created with a format (%s) that does not have "
                    "a stencil aspect.",
                    func_name, string_VkFormat(stencil_view_state->create_info.format));
            }
        }
    }

    if (!IsExtEnabled(device_extensions.vk_khr_device_group)) {
        if (pRenderingInfo->renderArea.offset.x < 0) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-renderArea-06071",
                             "%s(): pRenderingInfo->renderArea.offset.x (%" PRIu32 ") must not be negative.", func_name,
                             pRenderingInfo->renderArea.offset.x);
        }
        if (pRenderingInfo->renderArea.offset.y < 0) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-renderArea-06072",
                             "%s(): pRenderingInfo->renderArea.offset.y (%" PRIu32 ") must not be negative.", func_name,
                             pRenderingInfo->renderArea.offset.y);
        }
        if (x_adjusted_extent > phys_dev_props.limits.maxFramebufferWidth) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-renderArea-06073",
                             "%s(): pRenderingInfo->renderArea.offset.x (%" PRIu32
                             ") + pRenderingInfo->renderArea.extent.width (%" PRIu32
                             ") is not less than maxFramebufferWidth (%" PRIu32 ").",
                             func_name, pRenderingInfo->renderArea.offset.x, pRenderingInfo->renderArea.extent.width,
                             phys_dev_props.limits.maxFramebufferWidth);
        }
        if (y_adjusted_extent > phys_dev_props.limits.maxFramebufferHeight) {
            skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-renderArea-06074",
                             "%s(): pRenderingInfo->renderArea.offset.y (%" PRIu32
                             ") + pRenderingInfo->renderArea.extent.height (%" PRIu32
                             ") is not less than maxFramebufferHeight (%" PRIu32 ").",
                             func_name, pRenderingInfo->renderArea.offset.y, pRenderingInfo->renderArea.extent.height,
                             phys_dev_props.limits.maxFramebufferHeight);
        }
    }

    if (MostSignificantBit(pRenderingInfo->viewMask) >= static_cast<int32_t>(phys_dev_props_core11.maxMultiviewViewCount)) {
        skip |= LogError(commandBuffer, "VUID-VkRenderingInfo-viewMask-06128",
                         "vkBeginCommandBuffer(): Most significant bit pRenderingInfo->viewMask(%" PRIu32
                         ") "
                         "must be less maxMultiviewViewCount (%" PRIu32 ")",
                         pRenderingInfo->viewMask, phys_dev_props_core11.maxMultiviewViewCount);
    }

    if (IsExtEnabled(device_extensions.vk_ext_multisampled_render_to_single_sampled)) {
        const auto msrtss_info = LvlFindInChain<VkMultisampledRenderToSingleSampledInfoEXT>(pRenderingInfo->pNext);
        if (msrtss_info) {
            for (uint32_t j = 0; j < pRenderingInfo->colorAttachmentCount; ++j) {
                if (pRenderingInfo->pColorAttachments[j].imageView != VK_NULL_HANDLE) {
                    const auto image_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pColorAttachments[j].imageView);
                    skip |= ValidateMultisampledRenderToSingleSampleView(commandBuffer, image_view_state, msrtss_info, "color",
                                                                         func_name);
                }
            }
            if (pRenderingInfo->pDepthAttachment && pRenderingInfo->pDepthAttachment->imageView != VK_NULL_HANDLE) {
                const auto depth_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pDepthAttachment->imageView);
                skip |=
                    ValidateMultisampledRenderToSingleSampleView(commandBuffer, depth_view_state, msrtss_info, "depth", func_name);
            }
            if (pRenderingInfo->pStencilAttachment && pRenderingInfo->pStencilAttachment->imageView != VK_NULL_HANDLE) {
                const auto stencil_view_state = Get<IMAGE_VIEW_STATE>(pRenderingInfo->pStencilAttachment->imageView);
                skip |= ValidateMultisampledRenderToSingleSampleView(commandBuffer, stencil_view_state, msrtss_info, "stencil",
                                                                     func_name);
            }
            if (msrtss_info->rasterizationSamples == VK_SAMPLE_COUNT_1_BIT) {
                skip |= LogError(commandBuffer, "VUID-VkMultisampledRenderToSingleSampledInfoEXT-rasterizationSamples-06878",
                                 "%s(): A VkMultisampledRenderToSingleSampledInfoEXT struct is in the pNext chain of "
                                 "VkRenderingInfo with a rasterizationSamples value of VK_SAMPLE_COUNT_1_BIT which is not allowed",
                                 func_name);
            }
        }
    }
    return skip;
}

// Flags validation error if the associated call is made inside a render pass. The apiName routine should ONLY be called outside a
// render pass.
bool CoreChecks::InsideRenderPass(const CMD_BUFFER_STATE &cb_state, const char *apiName, const char *msgCode) const {
    bool inside = false;
    if (cb_state.activeRenderPass) {
        inside = LogError(cb_state.commandBuffer(), msgCode, "%s: It is invalid to issue this call inside an active %s.", apiName,
                          report_data->FormatHandle(cb_state.activeRenderPass->renderPass()).c_str());
    }
    return inside;
}

// Flags validation error if the associated call is made outside a render pass. The apiName
// routine should ONLY be called inside a render pass.
bool CoreChecks::OutsideRenderPass(const CMD_BUFFER_STATE &cb_state, const char *apiName, const char *msgCode) const {
    bool outside = false;
    if (((cb_state.createInfo.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) && (!cb_state.activeRenderPass)) ||
        ((cb_state.createInfo.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) && (!cb_state.activeRenderPass) &&
         !(cb_state.beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT))) {
        outside =
            LogError(cb_state.commandBuffer(), msgCode, "%s: This call must be issued inside an active render pass.", apiName);
    }
    return outside;
}

bool CoreChecks::PreCallValidateCmdEndRenderingKHR(VkCommandBuffer commandBuffer) const {
    auto cb_state = GetRead<CMD_BUFFER_STATE>(commandBuffer);
    if (!cb_state) return false;
    bool skip = false;

    if (cb_state->activeRenderPass) {
        if (!cb_state->activeRenderPass->UsesDynamicRendering()) {
            skip |= LogError(
                commandBuffer, "VUID-vkCmdEndRendering-None-06161",
                "Calling vkCmdEndRenderingKHR() in a render pass instance that was not begun with vkCmdBeginRenderingKHR().");
        }
        if (cb_state->activeRenderPass->use_dynamic_rendering_inherited == true) {
            skip |= LogError(commandBuffer, "VUID-vkCmdEndRendering-commandBuffer-06162",
                             "Calling vkCmdEndRenderingKHR() in a render pass instance that was not begun in this command buffer.");
        }
    }
    return skip;
}

bool CoreChecks::PreCallValidateCmdEndRendering(VkCommandBuffer commandBuffer) const {
    auto cb_state = GetRead<CMD_BUFFER_STATE>(commandBuffer);
    if (!cb_state) return false;
    bool skip = false;

    if (cb_state->activeRenderPass) {
        if (!cb_state->activeRenderPass->UsesDynamicRendering()) {
            skip |=
                LogError(commandBuffer, "VUID-vkCmdEndRendering-None-06161",
                         "Calling vkCmdEndRendering() in a render pass instance that was not begun with vkCmdBeginRendering().");
        }
        if (cb_state->activeRenderPass->use_dynamic_rendering_inherited == true) {
            skip |= LogError(commandBuffer, "VUID-vkCmdEndRendering-commandBuffer-06162",
                             "Calling vkCmdEndRendering() in a render pass instance that was not begun in this command buffer.");
        }
    }
    return skip;
}