/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2022. All rights reserved.
 * Description: DDGI example head file
 */

#ifndef DDGI_SAMPLES_DDGIEXAMPLE_H
#define DDGI_SAMPLES_DDGIEXAMPLE_H

#include <unordered_map>
#include "SaschaWillems/vulkanexamplebase.h"
#include "SaschaWillems/VulkanglTFModel.h"
#include "SaschaWillems/VulkanFrameBuffer.hpp"
#include "DDGIAPI.h"

struct VPMatrix {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec3 cameraPos;
};

struct DirectionalLight {
    glm::mat4 worldToLocal;
    glm::mat4 MVP;
    glm::vec4 dirAndPower; // direction: xyz, power: w
    glm::vec4 color; // color.w: reserved
};

struct PushConstBlockMaterial {
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    int colorTextureSet;
    int physicalTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;
};

#define ENABLE_VALIDATION true
#define USE_SKYBOX_FEATURE false
#define PASS_SHADOWMAP_INTO_DDGISDK true

// basecolor, normal, emissive, metallicroughness, occlusion
const int TEXTURE_PER_MATERIAL = 5;
const int UNIFORM_BUFFER_NUM = 5;
// vertex shader, fragment shader
const int SHADER_STAGE_NUM = 2;
const int SHADOWMAP_WIDTH = 512;
const int SHADOWMAP_HEIGHT = 512;
const int RENDER_EVERY_NUM_FRAME = 2;

enum class ModelName {
    WOODENROOM,
    SAMPLEBUILDING,
    GREYWHITEROOM,
};

inline DDGI::Vec2f VecInterface(const glm::vec2& vec2)
{
    return DDGI::Vec2f(vec2.x, vec2.y);
}

inline DDGI::Vec3f VecInterface(const glm::vec3& vec3)
{
    return DDGI::Vec3f(vec3.x, vec3.y, vec3.z);
}

inline DDGI::Vec3i VecInterface(int x, int y, int z)
{
    return DDGI::Vec3i(x, y, z);
}

inline DDGI::Vec3f VecInterface(float x, float y, float z)
{
    return DDGI::Vec3f(x, y, z);
}

inline DDGI::Vec4f VecInterface(const glm::vec4& vec4)
{
    return DDGI::Vec4f(vec4.x, vec4.y, vec4.z, vec4.w);
}

inline DDGI::Vec4f VecInterface(const glm::vec3& vec3, float w)
{
    return DDGI::Vec4f(vec3.x, vec3.y, vec3.z, w);
}

inline DDGI::Vec4f VecInterface(float x, float y, float z, float w)
{
    return DDGI::Vec4f(x, y, z, w);
}

inline DDGI::Mat4f MatInterface(const glm::mat4& mat)
{
    DDGI::Vec4f v0 = DDGI::Vec4f(mat[0].x, mat[0].y, mat[0].z, mat[0].w);
    DDGI::Vec4f v1 = DDGI::Vec4f(mat[1].x, mat[1].y, mat[1].z, mat[1].w);
    DDGI::Vec4f v2 = DDGI::Vec4f(mat[2].x, mat[2].y, mat[2].z, mat[2].w);
    DDGI::Vec4f v3 = DDGI::Vec4f(mat[3].x, mat[3].y, mat[3].z, mat[3].w);
    DDGI::Mat4f matrix(v0, v1, v2, v3);
    return matrix;
}

inline std::vector<DDGI::Mat4f> MatInterface(const std::vector<glm::mat4>& mats)
{
    std::vector<DDGI::Mat4f> matrices;
    for (auto& m : mats) {
        matrices.emplace_back(MatInterface(glm::transpose(m)));
    }
    return matrices;
}

// this sample uses SaschaWillemsVulkan open source code
// for detailed model data information, please view VulkanglTFModel
class DDGIExample : public VulkanExampleBase {
public:
    enum ExampleAttach {
        COLOR_ATTACHMENT_IDX = 0,
        DEPTH_ATTACHMENT_IDX = 1,
        MAX_ATTACHMENT = 2,
    };
    DDGIExample();
    ~DDGIExample();

    virtual void Prepare();
    virtual void Render();
    virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
    virtual void BuildCommandBuffers();

private:
    // ddgi
    void PrepareDDGI();
    void PrepareDDGIMeshes();
    void PrepareDDGIOutputTex(const vks::Texture& tex,
                              DDGIVulkanImage* texture) const;
    void SetupDDGIDeviceInfo();
    void SetupDDGIParameters();
    void SetupDDGILights();
    void SetupDDGICamera();
    DDGIVulkanImage SetupMaterialTexture(vkglTF::Texture* tex) const;

    uint32_t m_frameCnt = 0;
    bool m_ddgiON = true;
    std::unique_ptr<DDGIAPI> m_ddgiRender;
    DDGIDirectionalLight m_ddgiDirLight;
    std::map<int, DDGIMesh> m_ddgiMeshes;
    DDGICamera m_ddgiCamera;
    DeviceInfo m_ddgiDeviceInfo;
    DDGISettings m_ddgiSettings;
    DDGIVulkanImage m_ddgiIrradianceTex;
    DDGIVulkanImage m_ddgiNormalDepthTex;
    DDGIVulkanImage m_ddgiShadowMapTex;

    // shadowmap
    std::unique_ptr<vks::Framebuffer> m_shadowmapFramebuffer;
    VkDescriptorImageInfo m_shadowmapDescriptor;
    VkCommandBuffer m_shadowmapCmd;
    void SetupShadowmapPipeline();
    void SetupShadowmapFrameBuffer();
    void SetupShadowmapImgInfoDescriptor();
    void BuildShadowmapCmdBuffer();
    void DrawShadowmapNode(const vkglTF::Node* node, VkCommandBuffer cmdBuffer) const;

    VkSampler m_defaultSampler;
    ModelName m_modelName;

    glm::mat4 CalculateDirLightMVP() const;
    void SetupCamera(glm::vec3 position, glm::vec3 rotation, float fov);
    void SetupDirLight(glm::vec3 eye, glm::vec3 target, glm::vec3 color, float power);
    void LoadAssets(uint32_t glTFLoadingFlags, ModelName model);
    void CreateSampler();
    void CreateDDGITexture();
    void SetupDescriptorPool();
    void SetupDescriptorSetLayouts();
    void SetupDescriptorSets();
    void PrepareUniformBuffers();
    void SetupPipelines();
    void SetupPipelineLayouts();
    void UpdateUniformBuffers();
    void DrawSceneNode(const vkglTF::Node* node, VkCommandBuffer cmdBuffer) const;
    void DrawScene(VkCommandBuffer commandBuffer) const;
    void Draw();

    vkglTF::Model m_models;
    vks::Texture m_irradianceTex;
    vks::Texture m_normalDepthTex;
    struct {
        float gamma = 2.2;
        int ddgiDownSizeScale = 4;
        int ddgiTexWidth;
        int ddgiTexHeight;
        int ddgiEffectOn = 1;
    } m_shadingPara;

    VPMatrix m_sceneVP;
    DirectionalLight m_dirLight;
    glm::vec3 m_dirLightOriDirection;
    int m_dirLightRotation = 0;

    // The skybox isn't supported now
    // The related components of skybox are reserved for extension.
    vks::TextureCubeMap m_cubemap;

    struct {
        vks::Buffer sceneVP;
        vks::Buffer dirLight;
        vks::Buffer shadingPara;
        vks::Buffer skyBoxVP;
    } m_uniformBuffers;

    struct {
        VkPipeline scene;
        VkPipeline shadowmap;
    } m_pipelines;

    struct {
        VkPipelineLayout scene;
        VkPipelineLayout shadowmap;
    } m_pipelineLayouts;

    struct {
        VkDescriptorSetLayout sceneUniformBuffers = VK_NULL_HANDLE;
        VkDescriptorSetLayout sceneTexs = VK_NULL_HANDLE;
        VkDescriptorSetLayout meshUniformBuffer = VK_NULL_HANDLE;
        VkDescriptorSetLayout materialTexs = VK_NULL_HANDLE;
    } m_descriptorSetLayouts;

    struct {
        VkDescriptorSet sceneUniformBuffers;
        VkDescriptorSet sceneTexs;
        VkDescriptorSet meshUniformBuffer;
        VkDescriptorSet materialTexs;
    } m_descriptorSets;
};
#endif
