#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
#include <assert.h>
#include "HAL/PlatformMisc.h"

#if PLATFORM_WINDOWS
	#include <vulkan/vulkan.h>
#elif PLATFORM_ANDROID
	#include "vulkan_wrapper.h"
	#include <android_native_app_glue.h>
	extern struct android_app* GNativeAndroidApp;
#endif

#undef max
#undef min

using namespace std;

const static char* APP_SHORT_NAME = "LearnVulkan";
const static char* ENGINE_SHORT_NAME = "TinyEngine";
const static char* VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";
const wchar_t* AppClassName = L"TinyEngine";
bool GIsRequestingExit = false;


struct FVulkanLayerInfo
{
	VkLayerProperties LayerInfo;
	std::vector<VkExtensionProperties> SupportedExtensions;
};

struct FVulkanContext
{
	VkInstance Instance;
	std::vector<const char*> LayerNames;
	std::vector<const char*> ExtensionNames;
	VkPhysicalDevice PhysicalDevice;
	VkDevice LogicalDevice;
	uint32_t Width, Height;
	int32_t GraphicsFamilyIndex;
	int32_t PresentFamilyIndex;
#if PLATFORM_WINDOWS
	HWND Window;
	HINSTANCE WinInstance;
#elif PLATFORM_ANDROID
	// todo
#endif
	VkSurfaceKHR Surface;
	VkFormat SwapChainFormat;
	VkQueue PresentQueue;
	VkSwapchainKHR SwapChain;
	uint32_t SwapChainImageCount;
	VkExtent2D SwapChainExtent;
	std::vector<VkImage> SwapChainImages;
	std::vector<VkImageView> SwapChainImageViews;
	std::vector<VkFramebuffer> SwapChainFramebuffers;
	VkRenderPass RenderPass;
	VkShaderModule VertShaderModule, FragShaderModule;
	VkCommandPool CommandPool;
	std::vector<VkCommandBuffer> CommandBuffers;
	VkSemaphore PresentFinishedSemaphore;
	VkSemaphore RenderFinishedSemaphore;
	VkFence Fence;
	VkPipelineLayout PipelineLayout;
	VkPipeline GraphicsPipeline;

};

bool InitLayersAndExtensions(FVulkanContext& VulkanContext, bool EnableValidationLayer)
{
	VulkanContext.ExtensionNames.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
	//VulkanContext.ExtensionNames.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#if PLATFORM_WINDOWS
	VulkanContext.ExtensionNames.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif PLATFORM_ANDROID
	VulkanContext.ExtensionNames.emplace_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif

	if (EnableValidationLayer)
	{
		uint32_t LayerCount = 0;
		std::vector<VkLayerProperties> AllLayers;

		vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
		AllLayers.resize(LayerCount);

		vkEnumerateInstanceLayerProperties(&LayerCount, AllLayers.data());
		for (uint32_t LayerIdx = 0; LayerIdx < LayerCount; ++LayerIdx)
		{
			VkLayerProperties& Layer = AllLayers[LayerIdx];
			if (strcmp(Layer.layerName, VALIDATION_LAYER_NAME) == 0)
			{
				VulkanContext.LayerNames.emplace_back(VALIDATION_LAYER_NAME);
				FPlatformMisc::LocalPrintf("!!!!! Use Validation layer: %s\n", VALIDATION_LAYER_NAME);
				break;
			}
		}
		if (VulkanContext.LayerNames.empty())
		{
			FPlatformMisc::LocalPrint("!!!!! No Validation layer Found");
		}
	}
	return true;
}

bool CreateInstance(FVulkanContext& VulkanContext)
{
	FPlatformMisc::LocalPrint("Try to Create Vulkan Instance...");

	// initialize the VkApplicationInfo structure
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = APP_SHORT_NAME;
	app_info.applicationVersion = 1;
	app_info.pEngineName = ENGINE_SHORT_NAME;
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_0;

	// initialize the VkInstanceCreateInfo structure
	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = NULL;
	inst_info.flags = 0;
	inst_info.pApplicationInfo = &app_info;
	inst_info.enabledExtensionCount = (uint32_t)VulkanContext.ExtensionNames.size();
	inst_info.ppEnabledExtensionNames = VulkanContext.ExtensionNames.data();
	inst_info.enabledLayerCount = (uint32_t)VulkanContext.LayerNames.size();
	inst_info.ppEnabledLayerNames = VulkanContext.LayerNames.size() > 0 ? VulkanContext.LayerNames.data() : nullptr;

	VkResult Res = vkCreateInstance(&inst_info, NULL, &VulkanContext.Instance);
	if (Res == VK_ERROR_INCOMPATIBLE_DRIVER) {
		FPlatformMisc::LocalPrint("Cannot find a compatible Vulkan!");
		return false;
	}
    else if (Res != VK_SUCCESS) {
		FPlatformMisc::LocalPrintf("Create Instance Failed: %d\n", (int)Res);
		return false;
	}
	FPlatformMisc::LocalPrint("Vulkan Instance Created Successfully!");
	return true;
}

bool CreateSurface(FVulkanContext& VulkanContext)
{
#if PLATFORM_WINDOWS
	VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {};
	SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	SurfaceCreateInfo.hinstance = VulkanContext.WinInstance;
	SurfaceCreateInfo.hwnd = VulkanContext.Window;
	VkResult Res = vkCreateWin32SurfaceKHR(VulkanContext.Instance, &SurfaceCreateInfo, nullptr, &VulkanContext.Surface);
#elif PLATFORM_ANDROID
	VkAndroidSurfaceCreateInfoKHR SurfaceCreateInfo = {};
	SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	SurfaceCreateInfo.window = GNativeAndroidApp->window;
	FPlatformMisc::LocalPrintf("try to create surface: %x", vkCreateAndroidSurfaceKHR);
	VkResult Res = vkCreateAndroidSurfaceKHR(VulkanContext.Instance, &SurfaceCreateInfo, nullptr, &VulkanContext.Surface);
#endif
	if (Res != VK_SUCCESS)
	{
		FPlatformMisc::LocalPrintf("Create Surface Failed: %d\n", (int32_t)Res);
		return false;
	}
	FPlatformMisc::LocalPrint("Create Surface Successfully!");
	return true;
}

bool SelectPhysicalDevice(FVulkanContext& VulkanContext)
{
	VulkanContext.PhysicalDevice = nullptr;

	uint32_t DeviceCount = 0;
	vkEnumeratePhysicalDevices(VulkanContext.Instance, &DeviceCount, nullptr);
	FPlatformMisc::LocalPrintf("Device Count: %d\n", DeviceCount);
	if (DeviceCount == 0)
	{
		FPlatformMisc::LocalPrint("Failed to find GPU with vulkan support!");
		return false;
	}
	std::vector<VkPhysicalDevice> Devices;
	Devices.resize(DeviceCount);
	assert(vkEnumeratePhysicalDevices(VulkanContext.Instance, &DeviceCount, Devices.data()) == VK_SUCCESS);
	for(uint32_t i = 0; i < Devices.size(); ++i)
	{
		VkPhysicalDevice& Device = Devices[i];
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(Device, &Properties);
		
		FPlatformMisc::LocalPrintf("Device %d: %s driver: %d api: %d type: %d\n", i, 
			Properties.deviceName, Properties.driverVersion, Properties.apiVersion, Properties.deviceType);

		//VulkanContext.PhysicalDevice = Device;
		//break;
		if (Devices.size() == 1 || (VulkanContext.PhysicalDevice == nullptr && Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
		{
			VulkanContext.PhysicalDevice = Device;
			FPlatformMisc::LocalPrintf("Choose Device: [%s]\n", Properties.deviceName);
		}
	}
	bool Success = VulkanContext.PhysicalDevice != nullptr;
	if (!Success)
	{
		FPlatformMisc::LocalPrint("Select Physical Device Failed!");
	}
	return Success;
}

bool CreateLogicalDevice(FVulkanContext& VulkanContext)
{
	uint32_t QueueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(VulkanContext.PhysicalDevice, &QueueCount, nullptr);
	assert(QueueCount > 0);

	std::vector<VkQueueFamilyProperties> QueueProperties;
	QueueProperties.resize(QueueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(VulkanContext.PhysicalDevice, &QueueCount, QueueProperties.data());

	VulkanContext.GraphicsFamilyIndex = -1;
	for (uint32_t i = 0; i < QueueCount; ++i)
	{
		if (QueueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			VulkanContext.GraphicsFamilyIndex = i;
			break;
		}
	}
	if (VulkanContext.GraphicsFamilyIndex < 0)
	{
		FPlatformMisc::LocalPrintf("Can't find Graphics Queue");
		return false;
	}

	// choose present queue
	VkBool32 PresentSupport = false;
	assert(vkGetPhysicalDeviceSurfaceSupportKHR(VulkanContext.PhysicalDevice, VulkanContext.GraphicsFamilyIndex, VulkanContext.Surface, &PresentSupport) == VK_SUCCESS);
	if (PresentSupport)
	{
		VulkanContext.PresentFamilyIndex = VulkanContext.GraphicsFamilyIndex;
	}
	else
	{
		uint32_t QueueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(VulkanContext.PhysicalDevice, &QueueCount, nullptr);
		assert(QueueCount > 0);

		for (uint32_t i = 0; i < QueueCount; ++i)
		{
			assert(vkGetPhysicalDeviceSurfaceSupportKHR(VulkanContext.PhysicalDevice, i, VulkanContext.Surface, &PresentSupport) == VK_SUCCESS);
			if (PresentSupport)
			{
				VulkanContext.PresentFamilyIndex = i;
				break;
			}
		}
	}

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	std::set<uint32_t> UniqueQueueFamilies = { (uint32_t)VulkanContext.GraphicsFamilyIndex, (uint32_t)VulkanContext.PresentFamilyIndex };
	float QueuePriority = 1.f;
	for (uint32_t QueueFamily : UniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo QueueInfo{};
		QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueInfo.queueFamilyIndex = QueueFamily;
		QueueInfo.queueCount = 1;
		QueueInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueInfo);
	}

	std::vector<const char*> deviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo DeviceInfo;
	DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceInfo.pNext = nullptr;
	DeviceInfo.flags = 0;
	DeviceInfo.queueCreateInfoCount = (uint32_t)QueueCreateInfos.size();
	DeviceInfo.pQueueCreateInfos = QueueCreateInfos.data();
	DeviceInfo.enabledLayerCount = 0;
	DeviceInfo.ppEnabledLayerNames = nullptr;
	DeviceInfo.enabledExtensionCount = (uint32_t)deviceExtensionNames.size();
	DeviceInfo.ppEnabledExtensionNames = deviceExtensionNames.data();
	DeviceInfo.pEnabledFeatures = nullptr;
	
	VkResult Result = vkCreateDevice(VulkanContext.PhysicalDevice, &DeviceInfo, nullptr, &VulkanContext.LogicalDevice);
	if (Result != VK_SUCCESS)
	{
		FPlatformMisc::LocalPrintf("Create Logical Device Failed with Error: %d\n", Result);
		return false;
	}
	FPlatformMisc::LocalPrint("Create Logical Device Successfully!");

	vkGetDeviceQueue(VulkanContext.LogicalDevice, VulkanContext.PresentFamilyIndex, 0, &VulkanContext.PresentQueue);

	return true;
}

VkSurfaceFormatKHR ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR>& SurfaceFormats)
{
	for (const VkSurfaceFormatKHR& Format : SurfaceFormats)
	{
		if (Format.format == VK_FORMAT_B8G8R8A8_SRGB && Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return Format;
	}
	assert(SurfaceFormats.size() > 0);
	return SurfaceFormats[0];
}

VkPresentModeKHR ChoosePresentMode(std::vector<VkPresentModeKHR>& PresentModes)
{
	for (const VkPresentModeKHR& Mode : PresentModes)
	{
		if (Mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return Mode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCap, uint32_t DefaultWidth, uint32_t DefaultHeight)
{
	if (SurfaceCap.currentExtent.width != UINT32_MAX)
	{
		return SurfaceCap.currentExtent;
	}

	VkExtent2D ActualExtent;
	ActualExtent.width = std::max(SurfaceCap.minImageExtent.width, std::min(SurfaceCap.maxImageExtent.width, DefaultWidth));
	ActualExtent.height = std::max(SurfaceCap.minImageExtent.height, std::min(SurfaceCap.maxImageExtent.height, DefaultHeight));
	return ActualExtent;
}

#if PLATFORM_WINDOWS
LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ACTIVATE:
		break;
	case WM_DESTROY:
	{
		GIsRequestingExit = true;
		::PostQuitMessage(0);
		break;
	}

	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool CreateWindowWin32(FVulkanContext& VulkanContext, uint32_t DefaultWidth, uint32_t DefaultHeight)
{
	// 1. init windows size
	VulkanContext.Width = DefaultWidth;
	VulkanContext.Height = DefaultHeight;
	// 2. init instance
	VulkanContext.WinInstance = GetModuleHandle(nullptr);

	WNDCLASSEX WinClass = {};
	WinClass.cbSize = sizeof(WNDCLASSEX);
	WinClass.style = CS_HREDRAW | CS_VREDRAW;
	WinClass.lpfnWndProc = WindowProc;
	WinClass.hInstance = VulkanContext.WinInstance;
	//WinClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	//WinClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WinClass.lpszClassName = AppClassName;
	RegisterClassEx(&WinClass);

	RECT Rect{ 0, 0, (LONG)VulkanContext.Width, (LONG)VulkanContext.Height };
	AdjustWindowRect(&Rect, WS_OVERLAPPEDWINDOW, FALSE);

	VulkanContext.Window = CreateWindowEx(
		0, AppClassName, AppClassName, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		100, 100, Rect.right - Rect.left, Rect.bottom - Rect.top,
		nullptr, nullptr, VulkanContext.WinInstance, &VulkanContext
	);
	//::ShowWindow(VulkanContext.Window, SW_SHOWNORMAL); // when WS_VISIBLE is on, this is not needed.
	SetWindowLongPtr(VulkanContext.Window, GWLP_USERDATA, (LONG_PTR)&VulkanContext);
	return true;
}
#endif

bool CreateSwapChain(FVulkanContext& VulkanContext)
{
	uint32_t FormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanContext.PhysicalDevice, VulkanContext.Surface, &FormatCount, nullptr);
	if (FormatCount == 0)
	{
		FPlatformMisc::LocalPrint("No surface format is supported");
		return false;
	}
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	SurfaceFormats.resize(FormatCount);
	assert(vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanContext.PhysicalDevice, VulkanContext.Surface, &FormatCount, SurfaceFormats.data()) == VK_SUCCESS);
	VkSurfaceFormatKHR SurfaceFormat = ChooseSurfaceFormat(SurfaceFormats);
	VulkanContext.SwapChainFormat = SurfaceFormat.format;

	uint32_t PresentModeCount = 0;
	assert(vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanContext.PhysicalDevice, VulkanContext.Surface, &PresentModeCount, nullptr) == VK_SUCCESS);
	if (PresentModeCount == 0)
	{
		FPlatformMisc::LocalPrint("No present mode is supported");
		return false;
	}
	std::vector< VkPresentModeKHR> PresentModes;
	PresentModes.resize(PresentModeCount);
	assert(vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanContext.PhysicalDevice, VulkanContext.Surface, &PresentModeCount, PresentModes.data()) == VK_SUCCESS);
	VkPresentModeKHR PresentMode = ChoosePresentMode(PresentModes);

	VkSurfaceCapabilitiesKHR SurfaceCap;
	VkResult Res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanContext.PhysicalDevice, VulkanContext.Surface, &SurfaceCap);
	assert(Res == VK_SUCCESS);
	VkExtent2D SwapExtend = ChooseSwapExtent(SurfaceCap, VulkanContext.Width, VulkanContext.Height);
	FPlatformMisc::LocalPrintf("Window size %d x %d", SwapExtend.width, SwapExtend.height);

	uint32_t ImageCount = 2;
	ImageCount = std::min(SurfaceCap.maxImageCount, std::max(SurfaceCap.minImageCount, ImageCount));

	VkSwapchainCreateInfoKHR SwapChainCreateInfo = {};
	SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SwapChainCreateInfo.surface = VulkanContext.Surface;
	SwapChainCreateInfo.minImageCount = ImageCount;
	SwapChainCreateInfo.imageFormat = SurfaceFormat.format;
	SwapChainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
	SwapChainCreateInfo.imageExtent = SwapExtend;
	SwapChainCreateInfo.imageArrayLayers = 1;
	SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t QueueFamilyIndices[] = {(uint32_t)VulkanContext.GraphicsFamilyIndex, (uint32_t)VulkanContext.PresentFamilyIndex};
	if (VulkanContext.GraphicsFamilyIndex != VulkanContext.PresentFamilyIndex)
	{
		SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		SwapChainCreateInfo.queueFamilyIndexCount = 2;
		SwapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndices;
	}
	else
	{
		SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		SwapChainCreateInfo.queueFamilyIndexCount = 0;
		SwapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	SwapChainCreateInfo.preTransform = SurfaceCap.currentTransform;
	SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapChainCreateInfo.presentMode = PresentMode;
	SwapChainCreateInfo.clipped = VK_TRUE;
	SwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	Res = vkCreateSwapchainKHR(VulkanContext.LogicalDevice, &SwapChainCreateInfo, nullptr, &VulkanContext.SwapChain);
	if (Res != VK_SUCCESS)
	{
		FPlatformMisc::LocalPrintf("Create Swapchain failed: %d", uint32_t(Res));
		return false;
	}

	assert(vkGetSwapchainImagesKHR(VulkanContext.LogicalDevice, VulkanContext.SwapChain, &VulkanContext.SwapChainImageCount, nullptr) == VK_SUCCESS);
	VulkanContext.SwapChainImages.resize(VulkanContext.SwapChainImageCount);
	assert(vkGetSwapchainImagesKHR(VulkanContext.LogicalDevice, VulkanContext.SwapChain, &VulkanContext.SwapChainImageCount, 
			VulkanContext.SwapChainImages.data()) == VK_SUCCESS);

	VulkanContext.SwapChainExtent = SwapExtend;

	return true;
}

bool CreateImageViews(FVulkanContext& VulkanContext)
{
	VulkanContext.SwapChainImageViews.resize(VulkanContext.SwapChainImageCount);
	for (uint32_t i = 0; i < VulkanContext.SwapChainImageCount; ++i)
	{
		VkImageViewCreateInfo CreateInfo{};
		CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		CreateInfo.image = VulkanContext.SwapChainImages[i];
		CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		CreateInfo.format = VulkanContext.SwapChainFormat;
		CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		CreateInfo.subresourceRange.baseArrayLayer = 0;
		CreateInfo.subresourceRange.baseMipLevel = 0;
		CreateInfo.subresourceRange.layerCount = 1;
		CreateInfo.subresourceRange.levelCount = 1;
		if (vkCreateImageView(VulkanContext.LogicalDevice, &CreateInfo, nullptr, &VulkanContext.SwapChainImageViews[i]) != VK_SUCCESS)
		{
			return false;
		}
	}
	FPlatformMisc::LocalPrint("Swapchain Image View created Successfuly!");
	return true;
}

bool CreateShaderModule(FVulkanContext& VulkanContext, const std::vector<char>& Code, VkShaderModule& ShaderModule)
{
	VkShaderModuleCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.codeSize = Code.size();
	CreateInfo.pCode = reinterpret_cast<const uint32_t*>(Code.data());
	
	if (vkCreateShaderModule(VulkanContext.LogicalDevice, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

bool CreateRenderPass(FVulkanContext& VulkanContext)
{
	VkAttachmentDescription ColorAttachment{};
	ColorAttachment.format = VulkanContext.SwapChainFormat;
	ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference ColorAttachmentRef{};
	ColorAttachmentRef.attachment = 0;
	ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription Subpass{};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &ColorAttachmentRef;

	VkRenderPassCreateInfo RenderPassInfo{};
	RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	RenderPassInfo.attachmentCount = 1;
	RenderPassInfo.pAttachments = &ColorAttachment;
	RenderPassInfo.subpassCount = 1;
	RenderPassInfo.pSubpasses = &Subpass;
	if (vkCreateRenderPass(VulkanContext.LogicalDevice, &RenderPassInfo, nullptr, &VulkanContext.RenderPass) != VK_SUCCESS)
	{
		FPlatformMisc::LocalPrint("Create Render Pass Failed!");
		return false;
	}
	FPlatformMisc::LocalPrint("Create Render Pass Success!");
	return true;
}

bool CreateGraphicsPipeline(FVulkanContext& VulkanContext, bool EnableDepthTest, bool EnableBlend)
{
	std::vector<char> VertShaderCode = FPlatformMisc::ReadFile("Shaders/vert.spv");
	std::vector<char> FragShaderCode = FPlatformMisc::ReadFile("Shaders/frag.spv");

	if (!CreateShaderModule(VulkanContext, VertShaderCode, VulkanContext.VertShaderModule) ||
		!CreateShaderModule(VulkanContext, FragShaderCode, VulkanContext.FragShaderModule))
	{
		FPlatformMisc::LocalPrint("Create Shader Module Failed");
		return false;
	}

	VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
	VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertShaderStageInfo.module = VulkanContext.VertShaderModule;
	VertShaderStageInfo.pName = "main";
	
	VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
	FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragShaderStageInfo.module = VulkanContext.FragShaderModule;
	FragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo};

	VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
	VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputInfo.vertexBindingDescriptionCount = 0;
	VertexInputInfo.pVertexAttributeDescriptions = nullptr;
	VertexInputInfo.vertexAttributeDescriptionCount = 0;
	VertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo InputAssembly{};
	InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	InputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport Viewport{};
	Viewport.x = Viewport.y = 0.f;
	Viewport.width = (float)VulkanContext.SwapChainExtent.width;
	Viewport.height = (float)VulkanContext.SwapChainExtent.height;
	Viewport.minDepth = 0.f;
	Viewport.maxDepth = 1.f;
	VkRect2D Scissor = {{0, 0}, VulkanContext.SwapChainExtent};
	VkPipelineViewportStateCreateInfo ViewportState{};
	ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportState.viewportCount = 1;
	ViewportState.pViewports = &Viewport;
	ViewportState.scissorCount = 1;
	ViewportState.pScissors = &Scissor;

	VkPipelineRasterizationStateCreateInfo RasterState{};
	RasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	RasterState.depthClampEnable = VK_FALSE;
	RasterState.rasterizerDiscardEnable = VK_FALSE;
	RasterState.polygonMode = VK_POLYGON_MODE_FILL;
	RasterState.lineWidth = 1.f;
	RasterState.cullMode = VK_CULL_MODE_BACK_BIT;
	RasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	RasterState.depthBiasEnable = VK_FALSE;
	RasterState.depthBiasConstantFactor = 0.f;
	RasterState.depthBiasClamp = 0.f;
	RasterState.depthBiasSlopeFactor = 0.f;

	VkPipelineMultisampleStateCreateInfo MultiSampleState{};
	MultiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	MultiSampleState.sampleShadingEnable = VK_FALSE;
	MultiSampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	MultiSampleState.minSampleShading = 1.f;
	MultiSampleState.pSampleMask = nullptr;
	MultiSampleState.alphaToOneEnable = VK_FALSE;
	MultiSampleState.alphaToCoverageEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo DepthStencilState{};
	DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	DepthStencilState.depthTestEnable = EnableDepthTest ? VK_TRUE : VK_FALSE;
	DepthStencilState.depthWriteEnable = EnableDepthTest ? VK_TRUE : VK_FALSE;
	DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	DepthStencilState.depthBoundsTestEnable = VK_FALSE;
	DepthStencilState.stencilTestEnable = VK_FALSE;
	DepthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	DepthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	DepthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	DepthStencilState.back.compareMask = 0;
	DepthStencilState.back.reference = 0;
	DepthStencilState.back.depthFailOp = VK_STENCIL_OP_KEEP;
	DepthStencilState.back.writeMask = 0;
	DepthStencilState.minDepthBounds = 0;
	DepthStencilState.maxDepthBounds = 0;
	DepthStencilState.stencilTestEnable = VK_FALSE;
	DepthStencilState.front = DepthStencilState.back;
	
	VkPipelineColorBlendAttachmentState ColorBlendAttachState[1];
	VkColorComponentFlags ColorMask = EnableBlend ? (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT 
												   | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT) : 0xf;
	ColorBlendAttachState[0].colorWriteMask = ColorMask;
	ColorBlendAttachState[0].blendEnable = EnableBlend ? VK_TRUE : VK_FALSE;
	ColorBlendAttachState[0].alphaBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachState[0].colorBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachState[0].srcColorBlendFactor = EnableBlend ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
	ColorBlendAttachState[0].dstColorBlendFactor = EnableBlend ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	VkPipelineColorBlendStateCreateInfo BlendState{};
	BlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	BlendState.attachmentCount = 1;
	BlendState.pAttachments = ColorBlendAttachState;
	BlendState.logicOpEnable = VK_FALSE;
	BlendState.logicOp = VK_LOGIC_OP_NO_OP;
	BlendState.blendConstants[0] = 1.0f;
	BlendState.blendConstants[1] = 1.0f;
	BlendState.blendConstants[2] = 1.0f;
	BlendState.blendConstants[3] = 1.0f;

	VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};
	VkPipelineDynamicStateCreateInfo DynamicState{};
	DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicState.dynamicStateCount = 2;
	DynamicState.pDynamicStates = DynamicStates;

	VkPipelineLayoutCreateInfo PipelineCreateInfo{};
	PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineCreateInfo.setLayoutCount = 0;
	PipelineCreateInfo.pSetLayouts = nullptr;
	PipelineCreateInfo.pushConstantRangeCount = 0;
	PipelineCreateInfo.pPushConstantRanges = nullptr;
	assert(vkCreatePipelineLayout(VulkanContext.LogicalDevice, &PipelineCreateInfo, nullptr, &VulkanContext.PipelineLayout) == VK_SUCCESS);

	VkGraphicsPipelineCreateInfo PipelineInfo{};
	PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	PipelineInfo.stageCount = 2;
	PipelineInfo.pStages = ShaderStages;
	PipelineInfo.pVertexInputState = &VertexInputInfo;
	PipelineInfo.pInputAssemblyState = &InputAssembly;
	PipelineInfo.pViewportState = &ViewportState;
	PipelineInfo.pRasterizationState = &RasterState;
	PipelineInfo.pMultisampleState = &MultiSampleState;
	PipelineInfo.pDepthStencilState = &DepthStencilState;
	PipelineInfo.pColorBlendState = &BlendState;
	PipelineInfo.pDynamicState = &DynamicState;
	PipelineInfo.layout = VulkanContext.PipelineLayout;
	PipelineInfo.renderPass = VulkanContext.RenderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	PipelineInfo.basePipelineIndex = -1;
	VkResult Res = vkCreateGraphicsPipelines(VulkanContext.LogicalDevice, VK_NULL_HANDLE, 1, &PipelineInfo, 
									nullptr, &VulkanContext.GraphicsPipeline);
	assert(Res == VK_SUCCESS);

	return true;
}

bool CreateFrameBuffers(FVulkanContext& VulkanContext)
{
	VulkanContext.SwapChainFramebuffers.resize(VulkanContext.SwapChainImageCount);
	for (uint32_t i = 0; i < VulkanContext.SwapChainImageCount; ++i)
	{
		VkImageView attachments[] = {VulkanContext.SwapChainImageViews[i]};
		VkFramebufferCreateInfo CreateInfo{};
		CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		CreateInfo.renderPass = VulkanContext.RenderPass;
		CreateInfo.attachmentCount = 1;
		CreateInfo.pAttachments = attachments;
		CreateInfo.width = VulkanContext.SwapChainExtent.width;
		CreateInfo.height = VulkanContext.SwapChainExtent.height;
		CreateInfo.layers = 1;
		if (vkCreateFramebuffer(VulkanContext.LogicalDevice, &CreateInfo, nullptr, &VulkanContext.SwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			return false;
		}
	}
	FPlatformMisc::LocalPrint("Create Frame Buffers Successfully!");
	return true;
}

bool CreateCommandPool(FVulkanContext& VulkanContext)
{
	VkCommandPoolCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CreateInfo.queueFamilyIndex = VulkanContext.GraphicsFamilyIndex;
	CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(VulkanContext.LogicalDevice, &CreateInfo, nullptr, &VulkanContext.CommandPool) != VK_SUCCESS)
	{
		FPlatformMisc::LocalPrint("Create Command Pool Failed!");
		return false;
	}
	else
	{
		FPlatformMisc::LocalPrint("Create Command Pool Successfully!");
		return true;
	}
}

bool CreateCommandBuffers(FVulkanContext& VulkanContext)
{
	VulkanContext.CommandBuffers.resize(VulkanContext.SwapChainImageCount);
	VkCommandBufferAllocateInfo AllocInfo{};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.commandPool = VulkanContext.CommandPool;
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandBufferCount = (uint32_t)VulkanContext.CommandBuffers.size();
	if (vkAllocateCommandBuffers(VulkanContext.LogicalDevice, &AllocInfo, VulkanContext.CommandBuffers.data()) != VK_SUCCESS)
	{
		FPlatformMisc::LocalPrint("Create Command Buffers Failed!");
		return false;
	}
	else
	{
		FPlatformMisc::LocalPrint("Create Command Buffers Successfully!");
		return true;
	}
}

bool CreateSemaphoreAndFence(FVulkanContext& VulkanContext)
{
	VkSemaphoreCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	assert(vkCreateSemaphore(VulkanContext.LogicalDevice, &CreateInfo, nullptr, &VulkanContext.PresentFinishedSemaphore) == VK_SUCCESS);
	assert(vkCreateSemaphore(VulkanContext.LogicalDevice, &CreateInfo, nullptr, &VulkanContext.RenderFinishedSemaphore) == VK_SUCCESS);

	VkFenceCreateInfo FenceInfo{};
	FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	assert(vkCreateFence(VulkanContext.LogicalDevice, &FenceInfo, nullptr, &VulkanContext.Fence) == VK_SUCCESS);
	FPlatformMisc::LocalPrint("Create Semaphore and Fence Successfully!");
	return true;
}

void DrawFrame(FVulkanContext& VulkanContext)
{
	if (GIsRequestingExit)
		return;
	vkWaitForFences(VulkanContext.LogicalDevice, 1, &VulkanContext.Fence, VK_TRUE, UINT64_MAX);
	vkResetFences(VulkanContext.LogicalDevice, 1, &VulkanContext.Fence);

	uint32_t ImageIndex;
	vkAcquireNextImageKHR(VulkanContext.LogicalDevice, VulkanContext.SwapChain, 1000000000,
		VulkanContext.PresentFinishedSemaphore, VK_NULL_HANDLE, &ImageIndex);

	VkCommandBufferBeginInfo BeginInfo{};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	assert (vkBeginCommandBuffer(VulkanContext.CommandBuffers[ImageIndex], &BeginInfo) == VK_SUCCESS);

	VkRenderPassBeginInfo RenderPassInfo{};
	RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	RenderPassInfo.renderPass = VulkanContext.RenderPass;
	RenderPassInfo.framebuffer = VulkanContext.SwapChainFramebuffers[ImageIndex];
	RenderPassInfo.renderArea = {{0, 0}, VulkanContext.SwapChainExtent};
	VkClearValue ClearColor = {0.f, 0.f, 0.f, 1.f};
	RenderPassInfo.clearValueCount = 1;
	RenderPassInfo.pClearValues = &ClearColor;
	vkCmdBeginRenderPass(VulkanContext.CommandBuffers[ImageIndex], &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(VulkanContext.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanContext.GraphicsPipeline);

	VkViewport Viewport{};
	Viewport.x = Viewport.y = 0.f;
	Viewport.width = (float)VulkanContext.SwapChainExtent.width;
	Viewport.height = (float)VulkanContext.SwapChainExtent.height;
	Viewport.minDepth = 0.f;
	Viewport.maxDepth = 1.f;
	vkCmdSetViewport(VulkanContext.CommandBuffers[ImageIndex], 0, 1, &Viewport);
	//VkRect2D Scissor = { {0, 0}, VulkanContext.SwapChainExtent };
	//VkPipelineViewportStateCreateInfo ViewportState{};
	//ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	//ViewportState.viewportCount = 1;
	//ViewportState.pViewports = &Viewport;
	//ViewportState.scissorCount = 1;
	//ViewportState.pScissors = &Scissor;
	vkCmdSetLineWidth(VulkanContext.CommandBuffers[ImageIndex], 1.f);

	vkCmdDraw(VulkanContext.CommandBuffers[ImageIndex], 3, 1, 0, 0);

	vkCmdEndRenderPass(VulkanContext.CommandBuffers[ImageIndex]);
	assert (vkEndCommandBuffer(VulkanContext.CommandBuffers[ImageIndex]) == VK_SUCCESS);

	VkSubmitInfo SubmitInfo{};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	VkSemaphore WaitSemaphores[] = { VulkanContext.PresentFinishedSemaphore };
	VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores = WaitSemaphores;
	SubmitInfo.pWaitDstStageMask = WaitStages;

	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &VulkanContext.CommandBuffers[ImageIndex];

	VkSemaphore SignalSemaphores[] = { VulkanContext.RenderFinishedSemaphore };
	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores = SignalSemaphores;

	assert (vkQueueSubmit(VulkanContext.PresentQueue, 1, &SubmitInfo, VulkanContext.Fence) == VK_SUCCESS);

	VkPresentInfoKHR PresentInfo{};
	PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = &VulkanContext.RenderFinishedSemaphore;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &VulkanContext.SwapChain;
	PresentInfo.pImageIndices = &ImageIndex;
	PresentInfo.pResults = nullptr;
	VkResult Res = vkQueuePresentKHR(VulkanContext.PresentQueue, &PresentInfo);
	if (Res != VK_SUCCESS)
	{
		FPlatformMisc::LocalPrint("vkQueuePresentKHR Failed!");
	}
	assert (Res == VK_SUCCESS);
}

int GuardedMain()
{
	FPlatformMisc::PlatformInit();
	
	FVulkanContext VulkanContext;
	bool EnableValidationLayer = true;
	assert (InitLayersAndExtensions(VulkanContext, EnableValidationLayer));
	assert (CreateInstance(VulkanContext));
#if PLATFORM_WINDOWS
	assert (CreateWindowWin32(VulkanContext, 1024, 768)); // before surface
#endif
	assert (CreateSurface(VulkanContext));
	assert (SelectPhysicalDevice(VulkanContext));
	assert (CreateLogicalDevice(VulkanContext));
	assert (CreateSwapChain(VulkanContext));
	assert (CreateImageViews(VulkanContext));
	assert (CreateRenderPass(VulkanContext));
	assert (CreateGraphicsPipeline(VulkanContext, false, false));
	assert (CreateFrameBuffers(VulkanContext));
	assert (CreateCommandPool(VulkanContext));
	assert (CreateCommandBuffers(VulkanContext));
	assert (CreateSemaphoreAndFence(VulkanContext));

	while (!GIsRequestingExit)
	{
		FPlatformMisc::PumpMessages();
		DrawFrame(VulkanContext);
	}

	vkDeviceWaitIdle(VulkanContext.LogicalDevice);
	vkDestroyPipeline(VulkanContext.LogicalDevice, VulkanContext.GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(VulkanContext.LogicalDevice, VulkanContext.PipelineLayout, nullptr);
	vkDestroyFence(VulkanContext.LogicalDevice, VulkanContext.Fence, nullptr);
	vkDestroySemaphore(VulkanContext.LogicalDevice, VulkanContext.PresentFinishedSemaphore, nullptr);
	vkDestroySemaphore(VulkanContext.LogicalDevice, VulkanContext.RenderFinishedSemaphore, nullptr);
	vkDestroyCommandPool(VulkanContext.LogicalDevice, VulkanContext.CommandPool, nullptr);
	vkDestroyShaderModule(VulkanContext.LogicalDevice, VulkanContext.VertShaderModule, nullptr);
	vkDestroyShaderModule(VulkanContext.LogicalDevice, VulkanContext.FragShaderModule, nullptr);
	for (uint32_t i = 0; i < VulkanContext.SwapChainImageCount; ++i)
	{
		vkDestroyFramebuffer(VulkanContext.LogicalDevice, VulkanContext.SwapChainFramebuffers[i], nullptr);
		vkDestroyImageView(VulkanContext.LogicalDevice, VulkanContext.SwapChainImageViews[i], nullptr);
	}
	vkDestroyRenderPass(VulkanContext.LogicalDevice, VulkanContext.RenderPass, nullptr);
	vkDestroySwapchainKHR(VulkanContext.LogicalDevice, VulkanContext.SwapChain, nullptr);
	vkDestroySurfaceKHR(VulkanContext.Instance, VulkanContext.Surface, nullptr);
	vkDestroyDevice(VulkanContext.LogicalDevice, nullptr);
	vkDestroyInstance(VulkanContext.Instance, nullptr);
	FPlatformMisc::LocalPrint("Vulkan Destroyed");
	FPlatformMisc::LocalPrint("GoodBye!");

	return 0;
}