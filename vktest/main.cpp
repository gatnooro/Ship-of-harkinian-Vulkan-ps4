// sohvk Vulkan test for the PS4: RADV (Mesa, PS4 winsys) and the VideoOut WSI, before the full game.
//
// What it checks, in /data/sohvk/vktest.log (and boot-trace.log):
//   1. the Vulkan instance, the GPU and its memory heaps (how much video memory the app really gets)
//   2. 4000 small textures (64x64 RGBA), each with its own vkAllocateMemory, as Ship of Harkinian's Vulkan renderer
//      does: how long it takes and how much memory each one costs
//   3. big blocks of video memory, 256 MiB at a time up to 3 GiB: room for HD texture packs
//   4. drawing: the screen cycles through colors for 10 seconds, then stays GREEN (every check passed) or RED (one
//      failed; the log says which)
// Close it with the PS button (Close Application) when the screen is green or red.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <vector>

#include "ps4_vk_wsi.h"

extern "C" void ps4_boot_trace(const char* line);  // port/ps4_crashlog.cpp
extern "C" void ps4_app_startup(void);             // port/ps4_platform.cpp

namespace {

FILE* g_log = nullptr;
bool g_failed = false;

__attribute__((format(printf, 1, 2))) void say(const char* fmt, ...) {
    char line[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    ps4_boot_trace(line);
    if (g_log) {
        fprintf(g_log, "%s\n", line);
        fflush(g_log);
        fsync(fileno(g_log));
    }
}

double now_s() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return double(ts.tv_sec) + double(ts.tv_nsec) * 1e-9;
}

#define VKFN(name) PFN_##name name = nullptr
VKFN(vkGetInstanceProcAddr);
VKFN(vkCreateInstance);
VKFN(vkEnumeratePhysicalDevices);
VKFN(vkGetPhysicalDeviceProperties);
VKFN(vkGetPhysicalDeviceMemoryProperties);
VKFN(vkGetPhysicalDeviceQueueFamilyProperties);
VKFN(vkGetPhysicalDeviceFeatures);
VKFN(vkEnumerateDeviceExtensionProperties);
VKFN(vkCreateDevice);
VKFN(vkGetDeviceProcAddr);
VKFN(vkCreateHeadlessSurfaceEXT);
VKFN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
VKFN(vkGetPhysicalDeviceSurfaceFormatsKHR);
VKFN(vkGetPhysicalDeviceSurfaceSupportKHR);
VKFN(vkGetDeviceQueue);
VKFN(vkCreateSwapchainKHR);
VKFN(vkGetSwapchainImagesKHR);
VKFN(vkAcquireNextImageKHR);
VKFN(vkQueuePresentKHR);
VKFN(vkQueueSubmit);
VKFN(vkCreateCommandPool);
VKFN(vkAllocateCommandBuffers);
VKFN(vkBeginCommandBuffer);
VKFN(vkEndCommandBuffer);
VKFN(vkResetCommandBuffer);
VKFN(vkCmdPipelineBarrier);
VKFN(vkCmdClearColorImage);
VKFN(vkCreateSemaphore);
VKFN(vkCreateFence);
VKFN(vkWaitForFences);
VKFN(vkResetFences);
VKFN(vkCreateImage);
VKFN(vkDestroyImage);
VKFN(vkGetImageMemoryRequirements);
VKFN(vkAllocateMemory);
VKFN(vkFreeMemory);
VKFN(vkBindImageMemory);
VKFN(vkDeviceWaitIdle);

const char* heap_flags(VkMemoryHeapFlags f) {
    return (f & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "device-local" : "host";
}

void prop_flags(VkMemoryPropertyFlags f, char* out, size_t n) {
    snprintf(out, n, "%s%s%s%s", (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? "device-local " : "",
             (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? "host-visible " : "",
             (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? "coherent " : "",
             (f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ? "cached " : "");
}

uint32_t find_type(const VkPhysicalDeviceMemoryProperties& mp, uint32_t bits, VkMemoryPropertyFlags want) {
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want) return i;
    return UINT32_MAX;
}

// 2: many small textures, each with its own memory (Ship of Harkinian's renderer allocates one block per texture).
void small_texture_test(VkDevice dev, const VkPhysicalDeviceMemoryProperties& mp) {
    const int kCount = 4000;
    std::vector<VkImage> images;
    std::vector<VkDeviceMemory> memories;
    images.reserve(kCount);
    memories.reserve(kCount);
    VkDeviceSize total = 0, first_size = 0, first_align = 0;
    const double t0 = now_s();
    int ok = 0;
    for (int i = 0; i < kCount; i++) {
        VkImageCreateInfo ii = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ii.imageType = VK_IMAGE_TYPE_2D;
        ii.format = VK_FORMAT_R8G8B8A8_UNORM;
        ii.extent = {64, 64, 1};
        ii.mipLevels = 1;
        ii.arrayLayers = 1;
        ii.samples = VK_SAMPLE_COUNT_1_BIT;
        ii.tiling = VK_IMAGE_TILING_OPTIMAL;
        ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImage img = VK_NULL_HANDLE;
        if (vkCreateImage(dev, &ii, nullptr, &img) != VK_SUCCESS) break;
        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(dev, img, &req);
        if (i == 0) {
            first_size = req.size;
            first_align = req.alignment;
        }
        VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = find_type(mp, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDeviceMemory mem = VK_NULL_HANDLE;
        const VkResult r = vkAllocateMemory(dev, &ai, nullptr, &mem);
        if (r != VK_SUCCESS) {
            say("vktest: small texture %d: vkAllocateMemory = %d", i, int(r));
            vkDestroyImage(dev, img, nullptr);
            break;
        }
        vkBindImageMemory(dev, img, mem, 0);
        images.push_back(img);
        memories.push_back(mem);
        total += req.size;
        ok++;
    }
    const double took = now_s() - t0;
    say("vktest: small textures: %d of %d (64x64 RGBA: %llu bytes each, align %llu), %.1f MiB in all, %.2f s "
        "(%.2f ms each)",
        ok, kCount, (unsigned long long)first_size, (unsigned long long)first_align, double(total) / 1048576.0, took,
        ok ? took * 1000.0 / ok : 0.0);
    if (ok < kCount) g_failed = true;
    const double t1 = now_s();
    for (size_t i = 0; i < images.size(); i++) {
        vkDestroyImage(dev, images[i], nullptr);
        vkFreeMemory(dev, memories[i], nullptr);
    }
    say("vktest: small textures freed in %.2f s", now_s() - t1);
}

// 3: video memory in big blocks, up to 3 GiB.
void big_block_test(VkDevice dev, const VkPhysicalDeviceMemoryProperties& mp) {
    const VkDeviceSize kBlock = 256ull << 20;
    const int kMax = 12;
    std::vector<VkDeviceMemory> blocks;
    uint32_t type = UINT32_MAX;
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if ((mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
            !(mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            type = i;
            break;
        }
    if (type == UINT32_MAX) type = find_type(mp, ~0u, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    const double t0 = now_s();
    for (int i = 0; i < kMax; i++) {
        VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize = kBlock;
        ai.memoryTypeIndex = type;
        VkDeviceMemory mem = VK_NULL_HANDLE;
        const VkResult r = vkAllocateMemory(dev, &ai, nullptr, &mem);
        if (r != VK_SUCCESS) {
            say("vktest: big block %d (256 MiB, type %u): vkAllocateMemory = %d", i, type, int(r));
            break;
        }
        blocks.push_back(mem);
    }
    const unsigned mib = unsigned(blocks.size() * 256);
    say("vktest: video memory: %u MiB allocated in 256 MiB blocks (asked up to 3072), %.2f s", mib, now_s() - t0);
    if (mib < 1024) g_failed = true;
    for (VkDeviceMemory m : blocks) vkFreeMemory(dev, m, nullptr);
}

void hsv_to_rgb(float h, float* rgb) {
    const float i = floorf(h * 6.0f), f = h * 6.0f - i;
    const float q = 1.0f - f;
    switch (int(i) % 6) {
        case 0: rgb[0] = 1; rgb[1] = f; rgb[2] = 0; break;
        case 1: rgb[0] = q; rgb[1] = 1; rgb[2] = 0; break;
        case 2: rgb[0] = 0; rgb[1] = 1; rgb[2] = f; break;
        case 3: rgb[0] = 0; rgb[1] = q; rgb[2] = 1; break;
        case 4: rgb[0] = f; rgb[1] = 0; rgb[2] = 1; break;
        default: rgb[0] = 1; rgb[1] = 0; rgb[2] = q; break;
    }
}

int run() {
    g_log = fopen("/data/sohvk/vktest.log", "w");
    say("vktest: start (sohvk Vulkan test 1)");
    if (!wwhd::ps4::video_out_open()) {
        say("vktest: VideoOut did not open");
        return 1;
    }
    vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(wwhd::ps4::vk_get_instance_proc_addr());
#define GIPA(inst, name) name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(inst, #name))
    GIPA(VK_NULL_HANDLE, vkCreateInstance);

    const char* inst_ext[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME};
    VkApplicationInfo app = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "sohvk-vktest";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ici = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = 2;
    ici.ppEnabledExtensionNames = inst_ext;
    VkInstance inst = VK_NULL_HANDLE;
    VkResult r = vkCreateInstance(&ici, nullptr, &inst);
    say("vktest: vkCreateInstance = %d", int(r));
    if (r != VK_SUCCESS) return 1;
    GIPA(inst, vkEnumeratePhysicalDevices);
    GIPA(inst, vkGetPhysicalDeviceProperties);
    GIPA(inst, vkGetPhysicalDeviceMemoryProperties);
    GIPA(inst, vkGetPhysicalDeviceQueueFamilyProperties);
    GIPA(inst, vkGetPhysicalDeviceFeatures);
    GIPA(inst, vkEnumerateDeviceExtensionProperties);
    GIPA(inst, vkCreateDevice);
    GIPA(inst, vkGetDeviceProcAddr);
    GIPA(inst, vkCreateHeadlessSurfaceEXT);
    GIPA(inst, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    GIPA(inst, vkGetPhysicalDeviceSurfaceFormatsKHR);
    GIPA(inst, vkGetPhysicalDeviceSurfaceSupportKHR);

    uint32_t n = 1;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    r = vkEnumeratePhysicalDevices(inst, &n, &gpu);
    if ((r != VK_SUCCESS && r != VK_INCOMPLETE) || !gpu) {
        say("vktest: no GPU (%d)", int(r));
        return 1;
    }
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(gpu, &props);
    say("vktest: GPU '%s', Vulkan %u.%u.%u, driver %#x, vendor %#x device %#x", props.deviceName,
        VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion),
        VK_API_VERSION_PATCH(props.apiVersion), props.driverVersion, props.vendorID, props.deviceID);
    say("vktest: limits: maxImageDimension2D %u, maxMemoryAllocationCount %u, maxSamplerAnisotropy %.0f",
        props.limits.maxImageDimension2D, props.limits.maxMemoryAllocationCount, props.limits.maxSamplerAnisotropy);
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(gpu, &mp);
    for (uint32_t i = 0; i < mp.memoryHeapCount; i++)
        say("vktest: memory heap %u: %llu MiB, %s", i, (unsigned long long)(mp.memoryHeaps[i].size >> 20),
            heap_flags(mp.memoryHeaps[i].flags));
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
        char f[96];
        prop_flags(mp.memoryTypes[i].propertyFlags, f, sizeof f);
        say("vktest: memory type %u: heap %u, %s", i, mp.memoryTypes[i].heapIndex, f);
    }
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceFeatures(gpu, &feats);
    say("vktest: features: samplerAnisotropy %u, depthClamp %u, fillModeNonSolid %u, independentBlend %u",
        feats.samplerAnisotropy, feats.depthClamp, feats.fillModeNonSolid, feats.independentBlend);
    {
        uint32_t cnt = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &cnt, nullptr);
        std::vector<VkExtensionProperties> exts(cnt);
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &cnt, exts.data());
        bool mirror = false, swap = false;
        for (auto& e : exts) {
            if (!strcmp(e.extensionName, VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME)) mirror = true;
            if (!strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) swap = true;
        }
        say("vktest: %u device extensions, swapchain %s, sampler_mirror_clamp_to_edge %s", cnt, swap ? "yes" : "NO",
            mirror ? "yes" : "no");
    }

    uint32_t qn = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &qn, nullptr);
    std::vector<VkQueueFamilyProperties> qf(qn);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &qn, qf.data());
    uint32_t family = UINT32_MAX;
    for (uint32_t i = 0; i < qn; i++)
        if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            family = i;
            break;
        }
    say("vktest: %u queue families, graphics family %u", qn, family);
    if (family == UINT32_MAX) return 1;

    const float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = family;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    const char* dev_ext[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo dci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = dev_ext;
    VkDevice dev = VK_NULL_HANDLE;
    r = vkCreateDevice(gpu, &dci, nullptr, &dev);
    say("vktest: vkCreateDevice = %d", int(r));
    if (r != VK_SUCCESS) return 1;
#define GDPA(name) name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(dev, #name))
    GDPA(vkGetDeviceQueue);
    GDPA(vkCreateSwapchainKHR);
    GDPA(vkGetSwapchainImagesKHR);
    GDPA(vkAcquireNextImageKHR);
    GDPA(vkQueuePresentKHR);
    GDPA(vkQueueSubmit);
    GDPA(vkCreateCommandPool);
    GDPA(vkAllocateCommandBuffers);
    GDPA(vkBeginCommandBuffer);
    GDPA(vkEndCommandBuffer);
    GDPA(vkResetCommandBuffer);
    GDPA(vkCmdPipelineBarrier);
    GDPA(vkCmdClearColorImage);
    GDPA(vkCreateSemaphore);
    GDPA(vkCreateFence);
    GDPA(vkWaitForFences);
    GDPA(vkResetFences);
    GDPA(vkCreateImage);
    GDPA(vkDestroyImage);
    GDPA(vkGetImageMemoryRequirements);
    GDPA(vkAllocateMemory);
    GDPA(vkFreeMemory);
    GDPA(vkBindImageMemory);
    GDPA(vkDeviceWaitIdle);
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(dev, family, 0, &queue);

    small_texture_test(dev, mp);
    big_block_test(dev, mp);

    VkHeadlessSurfaceCreateInfoEXT hsci = {VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT};
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    r = vkCreateHeadlessSurfaceEXT(inst, &hsci, nullptr, &surface);
    say("vktest: vkCreateHeadlessSurfaceEXT = %d", int(r));
    if (r != VK_SUCCESS) return 1;
    VkBool32 present_ok = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family, surface, &present_ok);
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps);
    uint32_t fn = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &fn, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(fn);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &fn, formats.data());
    say("vktest: surface: present %u, %ux%u, images %u..%u, %u formats (first %d)", present_ok,
        caps.currentExtent.width, caps.currentExtent.height, caps.minImageCount, caps.maxImageCount, fn,
        fn ? int(formats[0].format) : -1);

    VkSwapchainCreateInfoKHR sci = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface = surface;
    sci.minImageCount = caps.minImageCount < 3 ? 3 : caps.minImageCount;
    if (caps.maxImageCount && sci.minImageCount > caps.maxImageCount) sci.minImageCount = caps.maxImageCount;
    sci.imageFormat = formats[0].format;
    sci.imageColorSpace = formats[0].colorSpace;
    sci.imageExtent = caps.currentExtent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    r = vkCreateSwapchainKHR(dev, &sci, nullptr, &swapchain);
    say("vktest: vkCreateSwapchainKHR = %d", int(r));
    if (r != VK_SUCCESS) return 1;
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(dev, swapchain, &image_count, nullptr);
    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(dev, swapchain, &image_count, images.data());

    VkCommandPoolCreateInfo cpci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = family;
    VkCommandPool pool = VK_NULL_HANDLE;
    vkCreateCommandPool(dev, &cpci, nullptr, &pool);
    const uint32_t kInFlight = 2;
    VkCommandBuffer cmds[kInFlight];
    VkCommandBufferAllocateInfo cbai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = kInFlight;
    vkAllocateCommandBuffers(dev, &cbai, cmds);
    VkSemaphore acquired[kInFlight], rendered[kInFlight];
    VkFence fences[kInFlight];
    for (uint32_t i = 0; i < kInFlight; i++) {
        VkSemaphoreCreateInfo sem = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(dev, &sem, nullptr, &acquired[i]);
        vkCreateSemaphore(dev, &sem, nullptr, &rendered[i]);
        VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(dev, &fci, nullptr, &fences[i]);
    }

    say("vktest: drawing: 10 s of colors, then %s", g_failed ? "RED (a check failed)" : "GREEN (every check passed)");
    const double start = now_s();
    double last_report = start;
    unsigned frames = 0, report_frames = 0;
    for (uint64_t frame = 0;; frame++) {
        const uint32_t slot = uint32_t(frame % kInFlight);
        vkWaitForFences(dev, 1, &fences[slot], VK_TRUE, UINT64_MAX);
        vkResetFences(dev, 1, &fences[slot]);
        uint32_t index = 0;
        r = vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, acquired[slot], VK_NULL_HANDLE, &index);
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
            say("vktest: vkAcquireNextImageKHR = %d", int(r));
            break;
        }
        const double t = now_s() - start;
        VkClearColorValue color = {};
        if (t < 10.0) {
            float rgb[3];
            hsv_to_rgb(float(fmod(t / 3.0, 1.0)), rgb);
            color.float32[0] = rgb[0] * 0.8f;
            color.float32[1] = rgb[1] * 0.8f;
            color.float32[2] = rgb[2] * 0.8f;
        } else if (g_failed) {
            color.float32[0] = 0.8f;
        } else {
            color.float32[1] = 0.7f;
        }
        color.float32[3] = 1.0f;
        VkCommandBuffer cmd = cmds[slot];
        vkResetCommandBuffer(cmd, 0);
        VkCommandBufferBeginInfo bi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);
        VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = images[index];
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &b);
        vkCmdClearColorImage(cmd, images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1,
                             &b.subresourceRange);
        b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask = 0;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &b);
        vkEndCommandBuffer(cmd);
        const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = &acquired[slot];
        si.pWaitDstStageMask = &wait_stage;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmd;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &rendered[slot];
        r = vkQueueSubmit(queue, 1, &si, fences[slot]);
        if (r != VK_SUCCESS) {
            say("vktest: vkQueueSubmit = %d", int(r));
            break;
        }
        VkPresentInfoKHR pi = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = &rendered[slot];
        pi.swapchainCount = 1;
        pi.pSwapchains = &swapchain;
        pi.pImageIndices = &index;
        r = vkQueuePresentKHR(queue, &pi);
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
            say("vktest: vkQueuePresentKHR = %d", int(r));
            break;
        }
        frames++;
        report_frames++;
        const double tn = now_s();
        if (tn - last_report >= 2.0 && tn - start < 12.0) {
            say("vktest: %u frames, %.1f fps", frames, report_frames / (tn - last_report));
            last_report = tn;
            report_frames = 0;
        }
        if (frames == 720) say("vktest: done - %s. Close the app with the PS button.", g_failed ? "RED" : "GREEN");
    }
    vkDeviceWaitIdle(dev);
    say("vktest: stopped after %u frames", frames);
    return 0;
}

}  // namespace

extern "C" int app_main(int, char**) {
    ps4_app_startup();
    const int r = run();
    if (g_log) fclose(g_log);
    // Nothing more to show: wait for the PS button (closing the app from the system menu)
    for (;;) sleep(1);
    return r;
}
