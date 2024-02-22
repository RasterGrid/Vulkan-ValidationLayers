/*
 * Copyright (c) 2015-2023 The Khronos Group Inc.
 * Copyright (c) 2015-2023 Valve Corporation
 * Copyright (c) 2015-2023 LunarG, Inc.
 * Copyright (c) 2015-2023 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "../framework/layer_validation_tests.h"
#include "generated/vk_extension_helper.h"

TEST_F(PositiveSampler, SamplerMirrorClampToEdgeWithoutFeature) {
    TEST_DESCRIPTION("Use VK_KHR_sampler_mirror_clamp_to_edge in 1.1 before samplerMirrorClampToEdge feature was added");

    SetTargetApiVersion(VK_API_VERSION_1_1);
    AddRequiredExtensions(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
    RETURN_IF_SKIP(Init());
    InitRenderTarget();
    if (DeviceValidationVersion() != VK_API_VERSION_1_1) {
        GTEST_SKIP() << "Test requires Vulkan 1.1 exactly";
    }

    VkSamplerCreateInfo sampler_info = SafeSaneSamplerCreateInfo();
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    vkt::Sampler sampler(*m_device, sampler_info);
}

TEST_F(PositiveSampler, SamplerMirrorClampToEdgeWithoutFeature12) {
    TEST_DESCRIPTION("Use VK_KHR_sampler_mirror_clamp_to_edge in 1.2 using the extension");

    // We need to explicitly allow promoted extensions to be enabled as this test relies on this behavior
    AllowPromotedExtensions();

    SetTargetApiVersion(VK_API_VERSION_1_2);
    AddRequiredExtensions(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
    RETURN_IF_SKIP(Init());
    InitRenderTarget();

    VkSamplerCreateInfo sampler_info = SafeSaneSamplerCreateInfo();
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    vkt::Sampler sampler(*m_device, sampler_info);
}

TEST_F(PositiveSampler, SamplerMirrorClampToEdgeWithFeature) {
    TEST_DESCRIPTION("Use VK_KHR_sampler_mirror_clamp_to_edge in 1.2 with feature bit enabled");

    SetRequiredApiVersion(VK_API_VERSION_1_2);
    AddRequiredFeature(vkt::Feature::samplerMirrorClampToEdge);
    RETURN_IF_SKIP(Init());
    InitRenderTarget();

    VkSamplerCreateInfo sampler_info = SafeSaneSamplerCreateInfo();
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    vkt::Sampler sampler(*m_device, sampler_info);
}
