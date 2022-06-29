/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2022. All rights reserved.
 * Description: DDGI Api head file
 */

#ifndef DDGI_API_H
#define DDGI_API_H

#include <map>
#include <string>
#include <vector>
#include "vulkan/vulkan.h"

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace DDGI {
    template <int N, typename T> struct Vec;
    template <typename T>
    struct Vec<2, T> {
        Vec() : x(0), y(0) {}
        explicit Vec(T scalar) : x(scalar), y(scalar) {}
        Vec(T v0, T v1) : x(v0), y(v1) {}

        T x, y;
    };

    template <typename T>
    struct Vec<3, T> {
        Vec() : x(0), y(0), z(0) {}
        explicit Vec(T scalar) : x(scalar), y(scalar), z(scalar) {}
        explicit Vec(const Vec<2, T>& v) : x(v.x), y(v.y), z(0) {}
        Vec(T v0, T v1, T v2) : x(v0), y(v1), z(v2) {}

        T x, y, z;
    };

    template <typename T>
    struct Vec<4, T> {
        Vec() : x(0), y(0), z(0), w(0) {}
        explicit Vec(T scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}
        explicit Vec(const Vec<3, T>& v) : x(v.x), y(v.y), z(v.z), w(0) {}
        Vec(T v0, T v1, T v2, T v3) : x(v0), y(v1), z(v2), w(v3) {}

        T x, y, z, w;
    };

    // column-major sequence
    template <typename T>
    struct Mat4X4 {
        Vec<4, T> m[4];
        Mat4X4() = default;
        explicit Mat4X4(const Vec<4, T>& v0, const Vec<4, T>& v1, const Vec<4, T>& v2, const Vec<4, T>& v3)
            : m{ v0, v1, v2, v3 } {}
        explicit Mat4X4(T t) { m[0].x = t; m[1].y = t; m[2].z = t; m[3].w = t;}
    };
    using Vec2f = Vec<2, float>;
    using Vec2 = Vec2f;
    using Vec3f = Vec<3, float>;
    using Vec3 = Vec3f;
    using Vec3i = Vec<3, int>;
    using Vec4f = Vec<4, float>;
    using Vec4 = Vec4f;
    using Vec4i = Vec<4, int>;
    using Mat4f = Mat4X4<float>;
    using Mat4 = Mat4f;
}

struct DeviceInfo {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex;
    VkQueue queue = VK_NULL_HANDLE;
};

enum class DDGIResult {
    SUCCESS = 0,          /* *< Success. */
    NOT_READY,            /* *< Vulkan device is not available. */
    INVALID_PARAMETER,    /* *< Invalid parameter. */
    OUT_OF_MEMORY,        /* *< Not enough memory. */
    SHADER_COMPILE_ERROR, /* *< Shader compile error. */
    UNKNOWN_ERROR,        /* *< Other unknown errors. */
};

enum class AttachmentTextureType {
    DDGI_IRRADIANCE = 0,
    DDGI_RELOCATION,
    DDGI_NORMAL_DEPTH,
    DDGI_PROBE_IRRADIANCE,
    DDGI_PROBE_DISTANCE
};

struct DDGIVulkanImage {
    bool useImageDescInfo = false;
    VkDescriptorImageInfo imageDescriptorInfo = {}; // more convenient method

    VkImage image = VK_NULL_HANDLE; // Vulkan image handle
    VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // current layout, may change resource access
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent3D extent = {};
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageType type = VK_IMAGE_TYPE_2D;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    int layers = 1;
    int mipCount = 1;
};

// PBR Material
struct DDGIMaterial {
    float alphaCutoff = 1.0f;
    float metallicFactor = 0.0f;
    float roughnessFactor = 0.5f;
    DDGI::Vec4f baseColorFactor = DDGI::Vec4f(1.0f);
    DDGI::Vec4f emissiveFactor = DDGI::Vec4f(1.0f);

    DDGIVulkanImage baseColorTexture;
    DDGIVulkanImage metallicRoughnessTexture;
};

// vertex attribute.
struct DDGIVertex {
    DDGI::Vec3f pos; // vertex's position in local space.
    DDGI::Vec3f normal;
    DDGI::Vec2f uv0;
    DDGI::Vec2f uv1;
    // DDGI sdk's skin mesh supports up to 4 joints
    DDGI::Vec4f joint0;
    DDGI::Vec4f weight0;
    DDGI::Vec4f tangent;
};

// DDGI only support meshes with indices
// and only support triangle primitive
struct DDGIMesh {
    std::string meshName;
    std::vector<DDGIVertex> meshVertex;
    std::vector<uint32_t> meshIndice;
    std::vector<DDGIMaterial> materials;
    std::vector<uint32_t> subMeshStartIndexes;
    std::vector<uint32_t> subMeshIndexCnts;
    DDGI::Mat4f localToWorld = DDGI::Mat4f();
    /**
    * assume that a vertex is affected by a joint, and the transformation is,
    * local space --> bone space --> mesh space --> world space,
    * outputPosition = MeshToWorldMat * BoneToMeshMat * BindPoseToMeshMat * inputPosition.
    * the boneTransforms matrix used in DDGI means that combined three transform,
    * so boneTransforms matrix is Bone to World,
    * outputPosition = BoneToWorldMat * inputPosition
    */
    bool isSkinMesh = false;
    std::vector<DDGI::Mat4f> boneTransforms;

    bool isDDGIMesh = true;
    bool hasIndices = true;
    bool hasAnimation = false;
};

struct DDGISettings {
    DDGI::Vec3f origin = DDGI::Vec3f();
    DDGI::Vec3f probeStep = DDGI::Vec3f(4.0f, 4.0f, 4.0f);
    DDGI::Vec3i probeCount = DDGI::Vec3i(8, 8, 8);
    float probeHysteresis = 0.99f;
    float viewBias = 4.0f;
    float normalBias = 1.0f;
    float probeMinFrontfaceDistance = 5.f; // relocation param
    float probeBackfaceThreshold = 0.25f;  // relocation param
    bool enableRelocation = false;
    // if enableRelocationDebug is true, relocation will continue to update, if not it will only update 100 frames
    bool enableRelocationDebug = false;
};

enum class CoordSystem : uint32_t {
    RIGHT_HANDED = 0,
    LEFT_HANDED = 1
};

struct DDGIDirectionalLight {
    // In order to fill the whole shadowmap as much as possible,
    // the SDK internally recalculates the orghographic projection matrix,
    // which involves the coordinate system.
    CoordSystem coordSystem = CoordSystem::RIGHT_HANDED;
    int lightId;
    DDGI::Mat4f localToWorld;
    DDGI::Vec4f color;
    DDGI::Vec4f dirAndIntensity;
};

struct DDGICamera {
    DDGI::Vec4f pos             = DDGI::Vec4f(0.0f);
    DDGI::Vec4f rotation        = DDGI::Vec4f(0.0f);
    DDGI::Mat4f viewMat         = DDGI::Mat4f(1.0f);
    DDGI::Mat4f perspectiveMat  = DDGI::Mat4f(1.0f);
};

class DDGIAPI {
public:
    DDGIAPI();
    ~DDGIAPI();

#ifdef __ANDROID__
    DDGIResult InitDDGI(const DeviceInfo& deviceInfo, JNIEnv *env = nullptr);
#else
    DDGIResult InitDDGI(const DeviceInfo& deviceInfo);
#endif
    DDGIResult Prepare() const;
    DDGIResult SetMeshes(const std::map<int, DDGIMesh>& meshes) const;
    DDGIResult SetAdditionalTexHandler(const DDGIVulkanImage ddgiTexture, const AttachmentTextureType type);
    DDGIResult SetResolution(const uint32_t w, const uint32_t h) const;
    DDGIResult SetCalcIrradianceSignal(bool flag);

    DDGIResult UpdateCamera(const DDGICamera& camera) const;
    DDGIResult UpdateDirectionalLight(const DDGIDirectionalLight& light) const;
    DDGIResult UpdateDDGIProbes(const DDGISettings& settings);
    DDGIResult UpdateLocalToWorldMatrix(int objID, const DDGI::Mat4f& matrix) const;
    DDGIResult UpdateSkinMeshMatrices(int objID, const std::vector<DDGI::Mat4f>& matrices) const;
    DDGIResult DDGIRender() const;
    void DDGIClear() const;

    DDGIResult SetShadowmapToDDGI(const DDGIVulkanImage shadowmapTex);

private:
    std::map<int, DDGIMesh> m_meshes;
    std::map<int, DDGIDirectionalLight> m_directionalLights;
    DeviceInfo m_deviceInfo;
    DDGISettings m_DDGISettings;
    DDGICamera m_camera;
    DDGIVulkanImage m_DDGIIrradianceTexture;
    DDGIVulkanImage m_DDGIProbeIradianceTexture;
    DDGIVulkanImage m_DDGIProbeDistanceTexture;
    DDGIVulkanImage m_DDGINormalDepthTexture;
    DDGIVulkanImage m_DDGIRelocationTexture;
    DDGIVulkanImage m_inShadowmap;
    bool m_useOutsideRelocationTex = false;
    bool m_enableCalcIrradiance = true; // enable calculate irradiance in ddgicore
};

#endif // DDGI_API_H
