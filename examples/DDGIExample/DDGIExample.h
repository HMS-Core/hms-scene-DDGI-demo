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
#define PASS_SHADOWMAP_INTO_DDGISDK true
const int SHADER_STAGE_NUM = 2; // vertex shader, fragment shader.

enum class ModelName {
    WOODENROOM,
    SAMPLEBUILDING,
    GREYWHITEROOM,
};

enum ExampleAttach {
    COLOR_ATTACHMENT_IDX = 0,
    DEPTH_ATTACHMENT_IDX = 1,
    MAX_ATTACHMENT = 2,
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

// This sample uses SaschaWillemsVulkan open source code
// for detailed model data information, please view VulkanglTFModel.
class DDGIExample : public VulkanExampleBase {
public:
    DDGIExample();
    ~DDGIExample();

    virtual void Prepare();
    virtual void Render();
    virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
    virtual void BuildCommandBuffers();

private:
    // basecolor, normal, emissive, metallic roughness, occlusion.
    const int TEXTURE_PER_MATERIAL = 5;
    const int UNIFORM_BUFFER_NUM = 5;
    const int SHADOWMAP_WIDTH = 512;
    const int SHADOWMAP_HEIGHT = 512;

    // Render DDGI every a specific number of frames.
    // Theoretically, the bigger the number, the better the rendering performance,
    // but the worse the rendering result may be.
    const int RENDER_EVERY_NUM_FRAME = 2;

    // ddgi
    std::unique_ptr<DDGIAPI> m_ddgiRender;
    bool m_ddgiON = true;
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

    // common
    uint32_t m_frameCnt = 0;
    VkSampler m_defaultSampler;
    vkglTF::Model m_models;
    ModelName m_modelName;
    vks::Texture m_irradianceTex; // to save DDGI's irradiance texture.
    vks::Texture m_normalDepthTex; // to save DDGI's normal depth texture.
    VPMatrix m_sceneVP;
    DirectionalLight m_dirLight;
    glm::vec3 m_dirLightOriDirection;
    int m_dirLightRotation = 0;
    struct {
        float gamma = 2.2;
        // The larger the value, the smaller the texture resolution and
        // the better the performance, but the aliasing may be more obvious.
        int ddgiDownSizeScale = 4; 
        int ddgiTexWidth;
        int ddgiTexHeight;
        int ddgiEffectOn = 1;
    } m_shadingPara;

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

    // ddgi
    /**
     * Preparation stage.
     */
    void PrepareDDGI();

    /**
     * Prepare the meshes' data specified by DDGI.
     */
    void PrepareDDGIMeshes();

    /**
     * Setup the vulkan texture handler to save the DDGI's output.
     * @param[in]   texture             The texture created by user.
     * @param[in]   DDGITexture         The vulkan texture handler data specified by DDGI.
     */
    void PrepareDDGIOutputTex(const vks::Texture& texture,
                              DDGIVulkanImage* DDGITexture) const;

    /**
     * Setup the vulkan device infomation specified by DDGI.
     */
    void SetupDDGIDeviceInfo();

    /**
     * Setup the parameters of the DDGI.
     */
    void SetupDDGIParameters();

    /**
     * Setup the light's infomation specified by DDGI.
     */
    void SetupDDGILights();

    /**
     * Setup the camera's infomation specified by DDGI.
     */
    void SetupDDGICamera();

    /**
     * Setup the material's texture infomation specified by DDGI.
     * @param[in]   texture             The pointer to the texture of material.
     * @return      DDGIVulkanImage     The vulkan texture handler specified by DDGI.
     */
    DDGIVulkanImage SetupMaterialTexture(vkglTF::Texture* texture) const;

    // shadow map
    void SetupShadowmapPipeline();
    void SetupShadowmapFrameBuffer();
    void SetupShadowmapImgInfoDescriptor();
    void BuildShadowmapCmdBuffer();
    void DrawShadowmapNode(const vkglTF::Node* node, VkCommandBuffer cmdBuffer) const;
    glm::mat4 CalculateDirLightMVP() const;

    // common
    void SetupCamera(glm::vec3 position, glm::vec3 rotation, float fov);
    void SetupDirLight(glm::vec3 eye, glm::vec3 target, glm::vec3 color, float power);
    void LoadAssets(uint32_t glTFLoadingFlags, ModelName model);
    void CreateSampler();

    /**
     * Create 2 textures to save DDGI result, one for irradiance, another for normal depth.
     */
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
};
#endif
