#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <iostream>
#include <vector>

using namespace std;

class VulkanManager 
{
public:
    VulkanManager()
    {
        window = make_window();
        start_vulkan();
        process();
        cleanup();
    }

private:
    vector<const char*> device_extensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    vector<VkImage> swap_chain_images;
    GLFWwindow* window;
    VkInstance vulkan_instance;
    VkPhysicalDevice phys_device = VK_NULL_HANDLE;
    VkDevice logical_device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swap_chain;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;

    void process();
    void start_vulkan();
    void cleanup();
    
    void create_vulkan();
    VkApplicationInfo paste_app_info();
    VkInstanceCreateInfo paste_create_info(VkApplicationInfo* app_info);
    VkPhysicalDevice get_physical_device();
    void get_logical_device();
    uint32_t get_graphics_queue_index();
    void add_surface();
    void add_swap_chain();
    VkSurfaceFormatKHR get_swap_surface_format();
    VkPresentModeKHR get_swap_present_mode();
    VkExtent2D get_swap_extend(VkSurfaceCapabilitiesKHR capabilities);
    
    GLFWwindow* make_window();
};

void VulkanManager::start_vulkan()
{
    create_vulkan();
    add_surface();
    phys_device = get_physical_device();
    get_logical_device();
    add_swap_chain();
}

void VulkanManager::create_vulkan()
{
    VkApplicationInfo app_info = VulkanManager::paste_app_info();
    VkInstanceCreateInfo create_info = VulkanManager::paste_create_info(&app_info);

    if (vkCreateInstance(&create_info, nullptr, &vulkan_instance) != VK_SUCCESS)
        cout << "Creating vulkan application error" << endl;
    cout << "Creating vulkan application success!" << endl;
}

VkApplicationInfo VulkanManager::paste_app_info()
{
    VkApplicationInfo app_info{};

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    return app_info;
}

VkInstanceCreateInfo VulkanManager::paste_create_info(VkApplicationInfo* app_info)
{
    VkInstanceCreateInfo create_info{};
    uint32_t glfw_extensions_count = 0;
    const char** glfw_extensions;

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = app_info;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

    create_info.enabledExtensionCount = (uint32_t) glfw_extensions_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;

    create_info.enabledLayerCount = 0;

    return create_info;
}

VkPhysicalDevice VulkanManager::get_physical_device()
{
    uint32_t device_count = 0;
    VkPhysicalDevice* devices;
    VkPhysicalDevice new_device;

    vkEnumeratePhysicalDevices(vulkan_instance, &device_count, nullptr);
    devices = new VkPhysicalDevice[device_count];
    vkEnumeratePhysicalDevices(vulkan_instance, &device_count, devices);

    new_device = devices[0];
    
    delete[] devices;
    cout << "Getting physical device success!" << endl;
    return new_device;
}

void VulkanManager::get_logical_device()
{
    float queue_priority = 1.0;
    int queue_index_count = get_graphics_queue_index();

    VkDeviceQueueCreateInfo logical_device_queue_create_info{};
    VkDeviceQueueCreateInfo* queue_create_infos = new VkDeviceQueueCreateInfo[queue_index_count];

    VkPhysicalDeviceFeatures device_features{};
    VkDeviceCreateInfo logical_device_create_info{};

    for (int index = 0; index < queue_index_count; index++)
    {
        logical_device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        logical_device_queue_create_info.queueFamilyIndex = index;
        logical_device_queue_create_info.queueCount = 1;
        logical_device_queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos[index] = logical_device_queue_create_info;
    }

    logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logical_device_create_info.pQueueCreateInfos = queue_create_infos;
    logical_device_create_info.queueCreateInfoCount = 1;
    logical_device_create_info.pEnabledFeatures = &device_features;
    logical_device_create_info.enabledExtensionCount = device_extensions.size();
    logical_device_create_info.ppEnabledExtensionNames = device_extensions.data();
    logical_device_create_info.enabledLayerCount = 0;
    
    if (vkCreateDevice(phys_device, &logical_device_create_info, nullptr, &logical_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    cout << "Logical device making success!" << endl;
    delete[] queue_create_infos;
}

uint32_t VulkanManager::get_graphics_queue_index()
{
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);

    return queue_family_count;
}

VkSurfaceFormatKHR VulkanManager::get_swap_surface_format()
{
    vector<VkSurfaceFormatKHR> available_formats;
    uint32_t formats_count = 0;

    vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &formats_count, nullptr);
    available_formats.resize(formats_count);
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &formats_count, available_formats.data());

    for (const VkSurfaceFormatKHR& format : available_formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB and format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }
    return available_formats[0];
}

VkPresentModeKHR VulkanManager::get_swap_present_mode()
{
    vector<VkPresentModeKHR> available_present_modes;
    uint32_t present_modes_count;

    vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, surface, &present_modes_count, nullptr);
    available_present_modes.resize(present_modes_count);

    vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, surface, &present_modes_count, available_present_modes.data());

    for (const VkPresentModeKHR& present_mode : available_present_modes)
    {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return present_mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanManager::get_swap_extend(VkSurfaceCapabilitiesKHR capabilities)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    extent.width = min<uint32_t>(capabilities.maxImageExtent.width, max<uint32_t>(capabilities.minImageExtent.width, extent.width));
    extent.height = min<uint32_t>(capabilities.maxImageExtent.height, max<uint32_t>(capabilities.minImageExtent.height, extent.height));

    return extent;
}

void VulkanManager::add_swap_chain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, surface, &capabilities);
    uint32_t swap_chain_images_count = 0;

    VkSurfaceFormatKHR surface_format = get_swap_surface_format();
    VkPresentModeKHR present_mode = get_swap_present_mode();
    VkExtent2D extend = get_swap_extend(capabilities);

    uint32_t image_count = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 and image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;

    uint32_t queue_index_count = get_graphics_queue_index();
    uint32_t* queue_indexes = new uint32_t[queue_index_count];

    for (int i = 0; i < queue_index_count; i++)
        queue_indexes[i] = i;

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extend;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = queue_index_count;
    create_info.pQueueFamilyIndices = queue_indexes;
    create_info.preTransform = capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(logical_device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
        cout << "Error to create swapchain!" << endl;

    vkGetSwapchainImagesKHR(logical_device, swap_chain, &swap_chain_images_count, nullptr);
    swap_chain_images.resize(swap_chain_images_count);
    vkGetSwapchainImagesKHR(logical_device, swap_chain, &swap_chain_images_count, swap_chain_images.data());

    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extend;

    delete[] queue_indexes;
}

void VulkanManager::add_surface()
{
    if (glfwCreateWindowSurface(vulkan_instance, window, nullptr, &surface) != VK_SUCCESS)
        cout << "Making Window surface error!";
    cout << "Making surface success!" << endl;
}

void VulkanManager::process()
{
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();
}

void VulkanManager::cleanup()
{
    vkDestroySwapchainKHR(logical_device, swap_chain, nullptr);
    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

GLFWwindow* VulkanManager::make_window()
{
    int window_h = 640, window_w = 800;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, false);

    cout << "Creating application window success!" << endl;

    return glfwCreateWindow(window_w, window_h, "VulkanTestApplication", nullptr, nullptr);
}

int main()
{
    VulkanManager vulkan;
    return EXIT_SUCCESS;
}