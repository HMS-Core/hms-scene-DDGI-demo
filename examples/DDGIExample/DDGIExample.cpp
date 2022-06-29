/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2022. All rights reserved.
 * Description: DDGI example
 */

#include "DDGIExample.h"
#include "SaschaWillems/VulkanTools.h"
#ifdef __ANDROID__
#include <jni.h>
#endif

using namespace std;
using namespace glm;
using namespace vks;

#ifdef __ANDROID__
static JavaVM* g_JVM = nullptr;
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    (void)reserved;
    g_JVM = vm;
    return JNI_VERSION_1_6;
}
JNIEnv* CreateJNIEnv()
{
    if (g_JVM == nullptr) {
        return nullptr;
    }
    JNIEnv* jniEnv = nullptr;
    bool detached = g_JVM->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6) == JNI_EDETACHED;
    if (detached) {
        g_JVM->AttachCurrentThread(&jniEnv, nullptr);
    }
    return jniEnv;
}
#endif

#ifdef __ANDROID__
DDGIExample::DDGIExample() : VulkanExampleBase(false)
#else
DDGIExample::DDGIExample() : VulkanExampleBase(ENABLE_VALIDATION)
#endif
{
    title = "ddgi";
}

DDGIExample::~DDGIExample()
{
    // Clean up used Vulkan resources
    // Note : Inherited destructor cleans up resources stored in base class

    vkDestroySampler(device, m_defaultSampler, nullptr);
    vkDestroyPipeline(device, m_pipelines.scene, nullptr);
    vkDestroyPipelineLayout(device, m_pipelineLayouts.scene, nullptr);

    vkDestroyDescriptorSetLayout(device, m_descriptorSetLayouts.sceneUniformBuffers, nullptr);
    vkDestroyDescriptorSetLayout(device, m_descriptorSetLayouts.sceneTexs, nullptr);
    if (m_descriptorSetLayouts.meshUniformBuffer != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayouts.meshUniformBuffer, nullptr);
    }
    if (m_descriptorSetLayouts.materialTexs != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayouts.materialTexs, nullptr);
    }

    // Uniform buffers
    m_uniformBuffers.sceneVP.destroy();
    m_uniformBuffers.dirLight.destroy();
    m_uniformBuffers.shadingPara.destroy();
}

void DDGIExample::Prepare()
{
    VulkanExampleBase::Prepare();
    CreateSampler();
    m_ddgiRender = make_unique<DDGIAPI>();
    m_shadingPara.gamma = 2.2f;
    const uint32_t glTFLoadingFlags = 0;
    m_modelName = ModelName::WOODENROOM;
    LoadAssets(glTFLoadingFlags, m_modelName);
    if (glTFLoadingFlags & vkglTF::FileLoadingFlags::FlipY) {
        SetupCamera(vec3(0.0f, 0.0f, 15.0f), vec3(0.0f, 0.0f, 0.0f), 60.f);
        SetupDirLight(vec3(0.f, 0.f, -10.f), vec3(0.f, 0.f, 0.f), vec3(1.0f), 5.f);
    } else {
        if (m_modelName == ModelName::WOODENROOM) {
            SetupCamera(vec3(-3.5f, 1.5f, -12.5f), vec3(0.0f, 0.0f, 0.0f), 60.f);
            SetupDirLight(vec3(5.f, 5.f, 10.f), vec3(0.f, 0.f, 0.f), vec3(1.0f), 5.f);
        } else if (m_modelName == ModelName::SAMPLEBUILDING) {
            SetupCamera(vec3(0.f, 2.5f, -3.6f), vec3(2.0f, 125.0f, 0.0f), 60.f);
            SetupDirLight(vec3(-10.f, 10.0f, -10.f), vec3(0.f, 0.f, 0.f), vec3(1.0f), 8.f);
        } else if (m_modelName == ModelName::GREYWHITEROOM) {
            SetupCamera(vec3(-1.75f, 0.4f, -5.25f), vec3(-18.0f, 270.0f, 0.0f), 60.f);
            SetupDirLight(vec3(-5.f, 10.f, 5.f), vec3(0.f, 0.f, 0.f), vec3(1.0f), 8.f);
        }
    }

    m_shadingPara.ddgiTexWidth = width;
    m_shadingPara.ddgiTexHeight = height;

    PrepareUniformBuffers();
    SetupDescriptorSetLayouts();
    SetupPipelineLayouts();
    SetupPipelines();
    SetupDescriptorPool();
    SetupShadowmapFrameBuffer();
    SetupShadowmapPipeline();
    SetupShadowmapImgInfoDescriptor();
    CreateDDGITexture();

    PrepareDDGI();

    SetupDescriptorSets();
    BuildShadowmapCmdBuffer();
    BuildCommandBuffers();

    prepared = true;
}

void DDGIExample::Render()
{
    if (!prepared)
        return;
    SetupDDGICamera();

    Draw();
    if (!paused || m_camera.updated) {
        UpdateUniformBuffers();
    }
    m_frameCnt++;
}

void DDGIExample::SetupCamera(vec3 position, vec3 rotation, float fov)
{
    timerSpeed *= 0.5f;
    m_camera.type = SaCamera::CameraType::lookat;
    m_camera.setRotation(rotation);
    m_camera.setPosition(position);
    m_camera.setPerspective(fov, (float)width / (float)height, 0.1f, 256.0f);
}

void DDGIExample::SetupDirLight(vec3 eye, vec3 target, vec3 color, float power)
{
    m_dirLight.color = vec4(color, 1.0);
    vec3 direction = normalize(target - eye);
    m_dirLight.dirAndPower = vec4(direction, power);

    // The transformation matrix form world to light.
    m_dirLight.worldToLocal = lookAt(eye, target, vec3(0, 1, 0));
    m_dirLightOriDirection = direction;
}

void DDGIExample::CreateSampler()
{
    // Create sampler to sample from the color attachments
    VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &m_defaultSampler));
}

void DDGIExample::BuildCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    std::array<VkClearValue, MAX_ATTACHMENT> clearValues;
    VkViewport viewport;
    VkRect2D scissor;

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

        clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = frameBuffers[i];
        renderPassBeginInfo.renderArea.extent.width = width;
        renderPassBeginInfo.renderArea.extent.height = height;
        renderPassBeginInfo.clearValueCount = clearValues.size();
        renderPassBeginInfo.pClearValues = clearValues.data();

        viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        scissor = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        DrawScene(drawCmdBuffers[i]);
        drawUI(drawCmdBuffers[i]);

        vkCmdEndRenderPass(drawCmdBuffers[i]);
        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}

void DDGIExample::DrawSceneNode(const vkglTF::Node* node, VkCommandBuffer cmdBuffer) const
{
    if (node->mesh) {
        for (vkglTF::Primitive* primitive : node->mesh->primitives) {
            std::vector<VkDescriptorSet> descriptorsets = {};
            descriptorsets.emplace_back(m_descriptorSets.sceneUniformBuffers);
            descriptorsets.emplace_back(m_descriptorSets.sceneTexs);
            descriptorsets.emplace_back(node->mesh->uniformBuffer.descriptorSet);
            descriptorsets.emplace_back(primitive->material.descriptorSet);
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayouts.scene, 0,
                descriptorsets.size(), descriptorsets.data(), 0, NULL);

            PushConstBlockMaterial pushConstBlockMaterial = {};
            pushConstBlockMaterial.baseColorFactor = primitive->material.baseColorFactor;
            pushConstBlockMaterial.roughnessFactor = primitive->material.roughnessFactor;
            pushConstBlockMaterial.metallicFactor = primitive->material.metallicFactor;
            pushConstBlockMaterial.colorTextureSet = primitive->material.baseColorTexture ? 1 : -1;
            pushConstBlockMaterial.physicalTextureSet = primitive->material.metallicRoughnessTexture ? 1 : -1;
            pushConstBlockMaterial.normalTextureSet = primitive->material.normalTexture ? 1 : -1;
            pushConstBlockMaterial.occlusionTextureSet = primitive->material.occlusionTexture ? 1 : -1;
            pushConstBlockMaterial.emissiveTextureSet = primitive->material.emissiveTexture ? 1 : -1;
            vkCmdPushConstants(cmdBuffer,
                               m_pipelineLayouts.scene,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(PushConstBlockMaterial), &pushConstBlockMaterial);

            vkCmdDrawIndexed(cmdBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
        }
    }
    for (const auto& child : node->children) {
        DrawSceneNode(child, cmdBuffer);
    }
}

void DDGIExample::DrawScene(VkCommandBuffer cmdBuffer) const
{
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.scene);
    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_models.vertices.buffer, offsets);
    if (m_models.indices.buffer != VK_NULL_HANDLE) {
        vkCmdBindIndexBuffer(cmdBuffer, m_models.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    }
    for (const auto& node : m_models.nodes) {
        DrawSceneNode(node, cmdBuffer);
    }
}

void DDGIExample::LoadAssets(uint32_t glTFLoadingFlags, ModelName model)
{
    if (m_modelName == ModelName::WOODENROOM) {
        m_models.loadFromFile(getAssetPath() + "models/woodenRoom.gltf",
                              vulkanDevice, queue, glTFLoadingFlags);
    } else if (m_modelName == ModelName::SAMPLEBUILDING) {
        m_models.loadFromFile(getAssetPath() + "models/samplebuilding.gltf",
                              vulkanDevice, queue, glTFLoadingFlags);
    } else if (m_modelName == ModelName::GREYWHITEROOM) {
        m_models.loadFromFile(getAssetPath() + "models/GreyWhiteRoom1.gltf",
                              vulkanDevice, queue, glTFLoadingFlags);
    } else {
        std::cout << "NO this Model file!" << std::endl;
    }
}

void DDGIExample::SetupDescriptorPool()
{
    int ubSize = UNIFORM_BUFFER_NUM;
    int texSize = m_models.materials.size() * TEXTURE_PER_MATERIAL;
    int descSetSize = m_models.materials.size() + m_models.linearNodes.size();
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ubSize),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texSize)
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, descSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void DDGIExample::SetupDescriptorSetLayouts()
{
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;

    // scene uniform buffers descriptorSetLayout
    setLayoutBindings = {
        // Binding 0: m_sceneVP uniform buffer to pass view, perspective, camera position data
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      0),
        // Binding 1: m_dirLight uniform buffer to pass directional light data
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      1),
        // Binding 2: some shading parameters uniform buffer
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      2),
    };
    descriptorSetLayoutCreateInfo =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(),
                                                         static_cast<uint32_t>(setLayoutBindings.size()));
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device,
                                                &descriptorSetLayoutCreateInfo,
                                                nullptr,
                                                &m_descriptorSetLayouts.sceneUniformBuffers));

    // some textures are needed when rendering whole scene
    setLayoutBindings = {
        // Binding 0 : DDGI irradiance result texture
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                      VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      0),
        // Binding 1 : DDGI normal and depth result texture
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                      VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      1),
        // Binding 2 : reserved for shadowmap texture
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                      VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      2),
    };
    descriptorSetLayoutCreateInfo =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), setLayoutBindings.size());
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device,
                                                &descriptorSetLayoutCreateInfo,
                                                nullptr,
                                                &m_descriptorSetLayouts.sceneTexs));
}

void DDGIExample::SetupPipelineLayouts()
{
    std::vector<VkDescriptorSetLayout> setLayouts = {};
    setLayouts.emplace_back(m_descriptorSetLayouts.sceneUniformBuffers);
    setLayouts.emplace_back(m_descriptorSetLayouts.sceneTexs);
    setLayouts.emplace_back(m_models.descriptorSetLayoutUbo);
    setLayouts.emplace_back(m_models.descriptorSetLayoutImage);

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));

    // pushConstant declare
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.size = sizeof(PushConstBlockMaterial);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK_RESULT(
        vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayouts.scene));

    setLayouts.clear();
    setLayouts.emplace_back(m_descriptorSetLayouts.sceneUniformBuffers);
    setLayouts.emplace_back(m_models.descriptorSetLayoutUbo);
    pPipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
    VK_CHECK_RESULT(
        vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayouts.shadowmap));
}

void DDGIExample::SetupDescriptorSets()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo;
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;

    // update scene uniform buffers descriptor set
    descriptorSetAllocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorPool,
                                                     &m_descriptorSetLayouts.sceneUniformBuffers,
                                                     1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &m_descriptorSets.sceneUniformBuffers));
    writeDescriptorSets = {
        // Binding 0: Fragment shader uniform buffer
        vks::initializers::writeDescriptorSet(m_descriptorSets.sceneUniformBuffers,
                                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              0,
                                              &m_uniformBuffers.sceneVP.descriptor),
        // Binding 1: m_dirLight uniform buffer
        vks::initializers::writeDescriptorSet(m_descriptorSets.sceneUniformBuffers,
                                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              1,
                                              &m_uniformBuffers.dirLight.descriptor),
        // Binding 2: some shading parameters uniform buffer
        vks::initializers::writeDescriptorSet(m_descriptorSets.sceneUniformBuffers,
                                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              2,
                                              &m_uniformBuffers.shadingPara.descriptor),
    };
    vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

    descriptorSetAllocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorPool,
                                                     &m_descriptorSetLayouts.sceneTexs,
                                                     1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &m_descriptorSets.sceneTexs));
    writeDescriptorSets = {
        // Binding 0: DDGI irradiance result texture
        vks::initializers::writeDescriptorSet(m_descriptorSets.sceneTexs,
                                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              0,
                                              &m_irradianceTex.descriptor),
        // Binding 1: DDGI normal and depthresult texture
        vks::initializers::writeDescriptorSet(m_descriptorSets.sceneTexs,
                                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              1,
                                              &m_normalDepthTex.descriptor),
        // Binding 2: reserved for shadowmap texture
        vks::initializers::writeDescriptorSet(m_descriptorSets.sceneTexs,
                                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              2,
                                              &m_shadowmapDescriptor),
    };
    vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
}

void DDGIExample::DrawShadowmapNode(const vkglTF::Node* node, VkCommandBuffer cmdBuffer) const
{
    if (node->mesh) {
        for (vkglTF::Primitive* primitive : node->mesh->primitives) {
            std::vector<VkDescriptorSet> descriptorsets = {};
            descriptorsets.emplace_back(m_descriptorSets.sceneUniformBuffers);
            descriptorsets.emplace_back(node->mesh->uniformBuffer.descriptorSet);
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayouts.shadowmap, 0,
                descriptorsets.size(), descriptorsets.data(), 0, NULL);
            vkCmdDrawIndexed(cmdBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
        }
    }
    for (const auto& child : node->children) {
        DrawShadowmapNode(child, cmdBuffer);
    }
}

void DDGIExample::BuildShadowmapCmdBuffer()
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = cmdPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &m_shadowmapCmd));
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(m_shadowmapCmd, &cmdBufInfo));
    VkClearValue clearValues[1];
    clearValues[0].depthStencil = { 1.0f, 0 };

    VkViewport viewport =
        vks::initializers::viewport((float)m_shadowmapFramebuffer->width,
                                    (float)m_shadowmapFramebuffer->height,
                                    0.0f,
                                    1.0f);
    vkCmdSetViewport(m_shadowmapCmd, 0, 1, &viewport);
    VkRect2D scissor = vks::initializers::rect2D(m_shadowmapFramebuffer->width, m_shadowmapFramebuffer->height, 0, 0);
    vkCmdSetScissor(m_shadowmapCmd, 0, 1, &scissor);

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = m_shadowmapFramebuffer->renderPass;
    renderPassBeginInfo.framebuffer = m_shadowmapFramebuffer->framebuffer;
    renderPassBeginInfo.renderArea.extent.width = m_shadowmapFramebuffer->width;
    renderPassBeginInfo.renderArea.extent.height = m_shadowmapFramebuffer->height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    vkCmdBeginRenderPass(m_shadowmapCmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetDepthBias(m_shadowmapCmd, 1.25f, 0.0f, 1.75f);
    vkCmdBindPipeline(m_shadowmapCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.shadowmap);
    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(m_shadowmapCmd, 0, 1, &m_models.vertices.buffer, offsets);
    if (m_models.indices.buffer != VK_NULL_HANDLE) {
        vkCmdBindIndexBuffer(m_shadowmapCmd, m_models.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    }
    for (const auto& node : m_models.nodes) {
        DrawShadowmapNode(node, m_shadowmapCmd);
    }
    vkCmdEndRenderPass(m_shadowmapCmd);

    VK_CHECK_RESULT(vkEndCommandBuffer(m_shadowmapCmd));
}

void DDGIExample::SetupShadowmapImgInfoDescriptor()
{
    m_shadowmapDescriptor =
        vks::initializers::descriptorImageInfo(m_shadowmapFramebuffer->sampler,
                                               m_shadowmapFramebuffer->attachments[0].view,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
}

void DDGIExample::SetupShadowmapFrameBuffer()
{
    vks::AttachmentCreateInfo attachCI;
    attachCI.format = VK_FORMAT_D16_UNORM;
    attachCI.width = SHADOWMAP_WIDTH;
    attachCI.height = SHADOWMAP_HEIGHT;
    attachCI.layerCount = 1;
    attachCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    m_shadowmapFramebuffer = make_unique<Framebuffer>(vulkanDevice);
    m_shadowmapFramebuffer->addAttachment(attachCI);
    m_shadowmapFramebuffer->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    m_shadowmapFramebuffer->createRenderPass();
}

void DDGIExample::SetupShadowmapPipeline()
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationStateCI =
        vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL,
                                                                VK_CULL_MODE_NONE,
                                                                VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                0);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendStateCI =
        vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI =
        vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleStateCI =
        vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCI =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);
    std::array<VkPipelineShaderStageCreateInfo, 1> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI =
        vks::initializers::pipelineCreateInfo(m_pipelineLayouts.shadowmap, m_shadowmapFramebuffer->renderPass, 0);
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pRasterizationState = &rasterizationStateCI;
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.pMultisampleState = &multisampleStateCI;
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pDepthStencilState = &depthStencilStateCI;
    pipelineCI.pDynamicState = &dynamicStateCI;
    pipelineCI.stageCount = shaderStages.size();
    pipelineCI.pStages = shaderStages.data();

    shaderStages[0] = loadShader(getShadersPath() + "woodenroom/shadowmap.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState(
        {
            vkglTF::VertexComponent::Position,
            vkglTF::VertexComponent::Normal,
            vkglTF::VertexComponent::UV,
            vkglTF::VertexComponent::Color,
            vkglTF::VertexComponent::Joint0,
            vkglTF::VertexComponent::Weight0,
            vkglTF::VertexComponent::Tangent
        });

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &m_pipelines.shadowmap));
}

void DDGIExample::SetupPipelines()
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationStateCI =
        vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL,
                                                                VK_CULL_MODE_NONE,
                                                                VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                0);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendStateCI =
        vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI =
        vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportStateCI =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleStateCI =
        vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCI =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);
    std::array<VkPipelineShaderStageCreateInfo, SHADER_STAGE_NUM> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI =
        vks::initializers::pipelineCreateInfo(m_pipelineLayouts.scene, renderPass, 0);
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pRasterizationState = &rasterizationStateCI;
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.pMultisampleState = &multisampleStateCI;
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pDepthStencilState = &depthStencilStateCI;
    pipelineCI.pDynamicState = &dynamicStateCI;
    pipelineCI.stageCount = shaderStages.size();
    pipelineCI.pStages = shaderStages.data();

    shaderStages[0] = loadShader(getShadersPath() + "woodenroom/forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "woodenroom/forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    // Empty vertex input state
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState(
        {
            vkglTF::VertexComponent::Position,
            vkglTF::VertexComponent::Normal,
            vkglTF::VertexComponent::UV,
            vkglTF::VertexComponent::Color,
            vkglTF::VertexComponent::Joint0,
            vkglTF::VertexComponent::Weight0,
            vkglTF::VertexComponent::Tangent
        });

    pipelineCI.layout = m_pipelineLayouts.scene;
    pipelineCI.renderPass = renderPass;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &m_pipelines.scene));
}

// Prepare and initialize uniform buffer containing shader uniforms
void DDGIExample::PrepareUniformBuffers()
{
    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_uniformBuffers.sceneVP, sizeof(VPMatrix)));
    VK_CHECK_RESULT(m_uniformBuffers.sceneVP.map());

    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_uniformBuffers.dirLight, sizeof(DirectionalLight)));
    VK_CHECK_RESULT(m_uniformBuffers.dirLight.map());

    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_uniformBuffers.shadingPara, sizeof(m_shadingPara)));
    VK_CHECK_RESULT(m_uniformBuffers.shadingPara.map());

    UpdateUniformBuffers();
}

glm::mat4 DDGIExample::CalculateDirLightMVP() const
{
    glm::mat4 localToWorld = glm::inverse(m_dirLight.worldToLocal);
    glm::vec3 sceneBboxMin = m_models.dimensions.min;
    glm::vec3 sceneBboxMax = m_models.dimensions.max;
    std::vector<glm::vec3> corners;
    corners.emplace_back(sceneBboxMin.x, sceneBboxMin.y, sceneBboxMin.z);
    corners.emplace_back(sceneBboxMax.x, sceneBboxMin.y, sceneBboxMin.z);
    corners.emplace_back(sceneBboxMin.x, sceneBboxMax.y, sceneBboxMin.z);
    corners.emplace_back(sceneBboxMin.x, sceneBboxMin.y, sceneBboxMax.z);
    corners.emplace_back(sceneBboxMax.x, sceneBboxMax.y, sceneBboxMin.z);
    corners.emplace_back(sceneBboxMax.x, sceneBboxMin.y, sceneBboxMax.z);
    corners.emplace_back(sceneBboxMin.x, sceneBboxMax.y, sceneBboxMax.z);
    corners.emplace_back(sceneBboxMax.x, sceneBboxMax.y, sceneBboxMax.z);

    glm::vec3 center = sceneBboxMin + (sceneBboxMax - sceneBboxMin) * 0.5f;
    localToWorld[3] = glm::vec4(center, 1.0f); // localToWorld[3] is position vector
    glm::mat4 worldToLocal = glm::inverse(localToWorld);

    glm::mat4 lightView = worldToLocal;
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float maxY = std::numeric_limits<float>::min();
    float maxZ = std::numeric_limits<float>::min();
    for (auto c : corners) {
        glm::vec3 corner = glm::vec3(lightView * glm::vec4(c, 1.f));
        minX = std::min(minX, corner.x);
        maxX = std::max(maxX, corner.x);
        minY = std::min(minY, corner.y);
        maxY = std::max(maxY, corner.y);
        minZ = std::min(minZ, corner.z);
        maxZ = std::max(maxZ, corner.z);
    }
    glm::mat4 directionalLightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

    glm::mat4 yFlip = glm::mat4(1.0f);
    yFlip[1][1] = -1;
    return yFlip * directionalLightProjection * lightView;
}

void DDGIExample::UpdateUniformBuffers()
{
    // scene
    m_sceneVP.projection = m_camera.matrices.perspective;
    m_sceneVP.view = m_camera.matrices.view;
    m_sceneVP.cameraPos = m_camera.position;
    memcpy(m_uniformBuffers.sceneVP.mapped, &m_sceneVP, sizeof(VPMatrix));

    // directional light
    m_dirLight.MVP = CalculateDirLightMVP();
    memcpy(m_uniformBuffers.dirLight.mapped, &m_dirLight, sizeof(DirectionalLight));

    // shading parameters
    memcpy(m_uniformBuffers.shadingPara.mapped, &m_shadingPara, sizeof(m_shadingPara));
}

void DDGIExample::Draw()
{
    VulkanExampleBase::prepareFrame();
    VkSubmitInfo shadowmapSubmitInfo = vks::initializers::submitInfo();
    shadowmapSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    shadowmapSubmitInfo.commandBufferCount = 1;
    shadowmapSubmitInfo.pCommandBuffers = &m_shadowmapCmd;
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &shadowmapSubmitInfo, VK_NULL_HANDLE));
    if (m_ddgiON && m_frameCnt % RENDER_EVERY_NUM_FRAME == 0) {
        m_ddgiRender->UpdateDirectionalLight(m_ddgiDirLight);
        m_ddgiRender->UpdateCamera(m_ddgiCamera);
        m_ddgiRender->DDGIRender();
    }

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VulkanExampleBase::submitFrame();
}

void DDGIExample::OnUpdateUIOverlay(vks::UIOverlay* overlay)
{
    int stroke = 20;
    float stride = 10.0f; // unit: degree
    if (overlay->header("Settings")) {
        if (overlay->sliderInt("light rotation", &m_dirLightRotation, -1 * stroke, stroke)) {
            float angle = static_cast<float>(m_dirLightRotation) * stride;
            glm::mat4 yRotate = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.f, 1.f, 0.f));
            glm::vec4 dir = yRotate * glm::vec4(m_dirLightOriDirection, 1.0f);
            m_dirLight.dirAndPower = glm::vec4(glm::vec3(dir), m_dirLight.dirAndPower.w);
            m_dirLight.worldToLocal = glm::lookAt(glm::vec3(-1.0f * dir), glm::vec3(0.0f), vec3(0, 1, 0));
            SetupDDGILights();
        }
        if (overlay->checkBox("DDGI Effect", &m_ddgiON)) {
            if (m_ddgiON) {
                m_shadingPara.ddgiEffectOn = 1;
            } else {
                m_shadingPara.ddgiEffectOn = 0;
            }
            UpdateUniformBuffers();
        }
    }
}

void DDGIExample::PrepareDDGIOutputTex(const vks::Texture& texture,
                                       DDGIVulkanImage* DDGItexture) const
{
    DDGItexture->image = texture.image;
    DDGItexture->format = texture.format;
    DDGItexture->type = VK_IMAGE_TYPE_2D;
    DDGItexture->extent.width = texture.width;
    DDGItexture->extent.height = texture.height;
    DDGItexture->extent.depth = 1;
    DDGItexture->usage = texture.usage;
    DDGItexture->layout = texture.imageLayout;
    DDGItexture->layers = 1;
    DDGItexture->mipCount = 1;
    DDGItexture->samples = VK_SAMPLE_COUNT_1_BIT;
    DDGItexture->tiling = VK_IMAGE_TILING_OPTIMAL;
}

void DDGIExample::CreateDDGITexture()
{
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    int ddgiTexWidth = width / m_shadingPara.ddgiDownSizeScale;
    int ddgiTexHeight = height / m_shadingPara.ddgiDownSizeScale;
    glm::ivec2 size(ddgiTexWidth, ddgiTexHeight);
    m_irradianceTex.CreateAttachment(vulkanDevice,
                                     ddgiTexWidth,
                                     ddgiTexHeight,
                                     VK_FORMAT_R16G16B16A16_SFLOAT,
                                     usage,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     m_defaultSampler);
    m_normalDepthTex.CreateAttachment(vulkanDevice,
                                      ddgiTexWidth,
                                      ddgiTexHeight,
                                      VK_FORMAT_R16G16B16A16_SFLOAT,
                                      usage,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      m_defaultSampler);
}

DDGIVulkanImage DDGIExample::SetupMaterialTexture(vkglTF::Texture* texture) const
{
    DDGIVulkanImage ddgiImg;
    if (texture == nullptr) {
        ddgiImg.image = VK_NULL_HANDLE;
    } else {
        ddgiImg.image = texture->image;
        ddgiImg.useImageDescInfo = true;
        ddgiImg.imageDescriptorInfo.imageLayout = texture->imageLayout;
        ddgiImg.imageDescriptorInfo.imageView = texture->view;
        ddgiImg.imageDescriptorInfo.sampler = texture->sampler;
    }
    return ddgiImg;
}

void DDGIExample::PrepareDDGIMeshes()
{
    // In this sample, a node in the model, which is gltf format, is equal to a mesh in DDGI,
    // and a primitive is equal to a submesh in DDGI.
    for (const auto& node : m_models.linearNodes) {
        DDGIMesh tmpMesh;
        tmpMesh.meshName = node->name;
        if (node->mesh) {
            tmpMesh.meshName = node->mesh->name;
            tmpMesh.localToWorld = MatInterface(node->getMatrix());
            if (node->skin) {
                tmpMesh.hasAnimation = true;
                for (auto& matrix : node->skin->inverseBindMatrices) {
                    tmpMesh.boneTransforms.emplace_back(MatInterface(matrix));
                }
            }

            // Double check the correspondence of the submesh data.
            for (vkglTF::Primitive* primitive : node->mesh->primitives) {
                DDGIMaterial material;
                material.alphaCutoff = primitive->material.alphaCutoff;
                material.baseColorFactor = VecInterface(primitive->material.baseColorFactor);
                material.metallicFactor = primitive->material.metallicFactor;
                material.roughnessFactor = primitive->material.roughnessFactor;
                material.baseColorTexture = SetupMaterialTexture(primitive->material.baseColorTexture);
                material.metallicRoughnessTexture = SetupMaterialTexture(primitive->material.metallicRoughnessTexture);
                tmpMesh.materials.emplace_back(material);

                // Setup the index data of mesh.
                tmpMesh.subMeshStartIndexes.push_back(tmpMesh.meshIndice.size());
                int currentVertNum = tmpMesh.meshVertex.size();
                int indexStart = primitive->firstIndex;
                for (int i = 0; i < primitive->indexCount; i++) {
                    tmpMesh.meshIndice.push_back(m_models.m_localIndexBuffer.at(indexStart + i) + currentVertNum);
                }
                tmpMesh.subMeshIndexCnts.push_back(primitive->indexCount);

                int vertexStart = primitive->firstVertex;
                // Setup the vertex data of mesh.
                for (int i = 0; i < primitive->vertexCount; i++) {
                    DDGIVertex vertex;
                    vertex.pos = VecInterface(m_models.m_vertexBuffer.at(vertexStart + i).pos);
                    vertex.normal = VecInterface(m_models.m_vertexBuffer.at(vertexStart + i).normal);
                    vertex.tangent = VecInterface(m_models.m_vertexBuffer.at(vertexStart + i).tangent);
                    vertex.uv0 = VecInterface(m_models.m_vertexBuffer.at(vertexStart + i).uv);
                    vertex.uv1 = VecInterface(glm::vec2(0.0f, 0.0f));
                    vertex.joint0 = VecInterface(m_models.m_vertexBuffer.at(vertexStart + i).joint0);
                    vertex.weight0 = VecInterface(m_models.m_vertexBuffer.at(vertexStart + i).weight0);
                    tmpMesh.meshVertex.emplace_back(vertex);
                }
            }
        }
        m_ddgiMeshes.emplace(std::make_pair(node->index, tmpMesh));
    }
}

void DDGIExample::SetupDDGIDeviceInfo()
{
    m_ddgiDeviceInfo.physicalDevice = physicalDevice;
    m_ddgiDeviceInfo.logicalDevice = device;
    m_ddgiDeviceInfo.queue = queue;
    m_ddgiDeviceInfo.queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
}

void DDGIExample::SetupDDGIParameters()
{
    if (m_modelName == ModelName::WOODENROOM) {
        m_ddgiSettings.origin = VecInterface(3.5f, 1.5f, 4.25f);
        m_ddgiSettings.probeStep = VecInterface(1.3f, 0.55f, 1.5f);
    } else if (m_modelName == ModelName::SAMPLEBUILDING) {
        m_ddgiSettings.origin = VecInterface(0.f, 4.5f, 0.f);
        m_ddgiSettings.probeStep = VecInterface( 2.5f, 1.0f, 2.5f);
    } else if (m_modelName == ModelName::GREYWHITEROOM) {
        m_ddgiSettings.origin = VecInterface(2.3f, 1.5f, -1.53f);
        m_ddgiSettings.probeStep = VecInterface(1.f, 0.75f, 0.75f);
    }
    // the number of probes in x, y, z axis
    m_ddgiSettings.probeCount = VecInterface(8, 8, 8);
    m_ddgiSettings.probeHysteresis = 0.97f;
    m_ddgiSettings.viewBias = 0.35f;
    m_ddgiSettings.normalBias = 0.5f;
}

void DDGIExample::SetupDDGILights()
{
    m_ddgiDirLight.color = VecInterface(m_dirLight.color);
    m_ddgiDirLight.dirAndIntensity = VecInterface(m_dirLight.dirAndPower);
    m_ddgiDirLight.localToWorld = MatInterface(inverse(m_dirLight.worldToLocal));
    m_ddgiDirLight.lightId = 0;
}

void DDGIExample::SetupDDGICamera()
{
    m_ddgiCamera.pos = VecInterface(m_camera.viewPos);
    m_ddgiCamera.rotation = VecInterface(m_camera.rotation, 1.0);
    m_ddgiCamera.viewMat = MatInterface(m_camera.matrices.view);
    glm::mat4 yFlip = glm::mat4(1.0f);
    yFlip[1][1] = -1;
    m_ddgiCamera.perspectiveMat = MatInterface(m_camera.matrices.perspective * yFlip);
}

void DDGIExample::PrepareDDGI()
{
    SetupDDGIParameters();
    SetupDDGIDeviceInfo();
    m_ddgiRender->SetResolution(width / m_shadingPara.ddgiDownSizeScale, height / m_shadingPara.ddgiDownSizeScale);
    m_ddgiRender->SetCalcIrradianceSignal(true);
    SetupDDGILights();
    SetupDDGICamera();
    PrepareDDGIMeshes();
    PrepareDDGIOutputTex(m_irradianceTex, &m_ddgiIrradianceTex);
    PrepareDDGIOutputTex(m_normalDepthTex, &m_ddgiNormalDepthTex);

    m_ddgiRender->SetAdditionalTexHandler(m_ddgiIrradianceTex, AttachmentTextureType::DDGI_IRRADIANCE);
    m_ddgiRender->SetAdditionalTexHandler(m_ddgiNormalDepthTex, AttachmentTextureType::DDGI_NORMAL_DEPTH);
#if PASS_SHADOWMAP_INTO_DDGISDK
    m_ddgiShadowMapTex.imageDescriptorInfo = m_shadowmapDescriptor;
    m_ddgiShadowMapTex.imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    m_ddgiShadowMapTex.useImageDescInfo = true;
    m_ddgiRender->SetShadowmapToDDGI(m_ddgiShadowMapTex);
#endif
#ifdef __ANDROID__
    m_ddgiRender->InitDDGI(m_ddgiDeviceInfo, CreateJNIEnv());
#else
    m_ddgiRender->InitDDGI(m_ddgiDeviceInfo);
#endif
    m_ddgiRender->SetMeshes(m_ddgiMeshes);
    m_ddgiRender->UpdateDDGIProbes(m_ddgiSettings);
    m_ddgiRender->UpdateDirectionalLight(m_ddgiDirLight);
    m_ddgiRender->Prepare();
    m_ddgiRender->UpdateCamera(m_ddgiCamera);
}
