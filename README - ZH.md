# DDGI(动态漫反射全局光照) 使用示例代码
[English](README.md) | 中文

## 目录

 * [简介](#简介)
 * [编译](#编译)
 * [例子](#例子)
 * [参考项目](#参考项目)
 * [许可证](#许可证)

## 简介

本项目主要是用于展示如使用华为提供的移动端动态漫反射全局光照（DDGI），在前向管线上实现动态光照漫反射全局照明的效果，提升场景画质。项目中关于Vulkan的管线借鉴了SaschaWillems的VulkanExample[[1\]](https://github.com/SaschaWillems/Vulkan)项目，例子中关于PBR部分知识参考了LearnOpenGL[[2\]](https://learnopengl-cn.github.io/07%20PBR/02%20Lighting/#pbr), DDGI算法的实现参考了英伟达RTXGI[[3\]](https://https://github.com/NVIDIAGameWorks/RTXGI)。

开源场景Woodenroom上使能DDGI的效果如下，运行平台：Mate40Pro

（Woodenroom，from:https://sketchfab.com/3d-models/room-266d02119c494b4cbaf759d774df8494，License: CC Attribution）

<video src="assets/woodenroom-android.mp4"></video>

## 编译

**Android / HarmonyOS平台**：

1. 开发环境
   - Android studio 4.0及以上版本
   - ndk 20.1.5948944及以上版本
   - Android SDK 29.0.0及以上版本

设置环境变量ANDROID_HOME ANDROID_NDK_HOME分别指向 Android SDK目录和NDK目录

2. 编译运行
   1.  使用IDE：用Android Studio打开`android`目录，同步后，连接手机，点击运行按钮或快捷键Shift+F10即可执行代码。生成的apk文件归档在android\examples\bin目录下。
   2. 使用命令行：USB连接手机，开启ADB调试模式，执行以下命令

```
cd android
call .\gradlew clean
call .\gradlew installDebug 	# or `call .\gradlew assembleDebug` for just build apk
adb shell am start -n "com.huawei.ddgi.vkExample/.VulkanActivity"
```

**Windows平台**：

1. 开发环境
   - Visual Studio 2019及以上版本
   - Vulkan SDK  1.2.176.1及以上版本
   - Cmake3.16及以上版本
2. 在工程的CmakeLists.txt所在目录执行Cmake编译，打开编译后生成的"ddgi-sample.sln"文件，设置“DDGIExample”为启动项目运行即可。

注意：如果对该工程的shader代码有修改，请执行以下指令进行编译更新：

`python3 .\data\shaders\glsl\compileshaders.py`

## 例子

本例子在前向渲染管线的基础上，叠加了DDGI的间接光效果，具体流程如下图：

![DDGI](assets/DDGI-1655281049240.png)

DDGI的总体分为以下三个阶段：

- 初始化阶段：设置Vulkan环境信息，初始化DDGI；
- 准备阶段：
  - 创建保存结果的图片：分别创建用于保存全局漫反射光照和法线深度的图片，设置其分辨率，并输入到DDGI；
  - 准备并解析渲染场景数据：将用户渲染数据转换成DDGI要求的格式，并调用相应函数输入到DDGI，然后调用DDGI的Prepare函数解析输入的数据；
  - 设置DDGI算法的参数并输入到DDGI；
- 渲染阶段：
  - 更新相机和光源信息；
  - 调用DDGI的Render函数进行渲染，渲染结果保存在准备阶段创建的图片中。

最终，直接光+DDGI间接光的结果如图所示：

![1655281011338](C:\Users\s00642167\Desktop\hms-scene-DDGI-demo\1655281011338.png)

## 参考项目

[1] [SaschaWillems/Vulkan](<https://github.com/SaschaWillems/Vulkan>)

[2] [learnOpenGL/PBR](https://learnopengl-cn.github.io/07%20PBR/02%20Lighting/#pbr)

[3] [Nvidia/RTXGI](https://github.com/NVIDIAGameWorks/RTXGI)

## 许可证

DDGI示例代码采用的许可证为Apache License, version 2.0，参考 [LICENSE.md](https://github.com/HMS-Core/hms-scene-ddgi-demo/blob/master/LICENSE.md) 获取更多许可证信息；