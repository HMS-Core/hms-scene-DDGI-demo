/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2022. All rights reserved.
 * Description: DDGI example main
 */

#include "DDGIExample.h"

DDGIExample *g_vulkanExample;

// OS specific macros for the example main entry points
#if defined(_WIN32)
// Windows entry point
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (g_vulkanExample != NULL) {
        g_vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    for (int32_t i = 0; i < __argc; i++) { DDGIExample::args.push_back(__argv[i]); };
    g_vulkanExample = new DDGIExample();
    g_vulkanExample->initVulkan();
    g_vulkanExample->setupWindow(hInstance, WndProc);
    g_vulkanExample->Prepare();
    g_vulkanExample->renderLoop();
    delete(g_vulkanExample);
    return 0;
}
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
// Android entry point
void android_main(android_app *state)
{
    g_vulkanExample = new DDGIExample();
    state->userData = g_vulkanExample;
    state->onAppCmd = DDGIExample::handleAppCommand;
    state->onInputEvent = DDGIExample::handleAppInput;
    androidApp = state;
    vks::android::getDeviceConfig();
    g_vulkanExample->renderLoop();
    delete(g_vulkanExample);
}
#endif