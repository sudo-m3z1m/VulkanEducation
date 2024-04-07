#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJLOADER_IMPLEMENTATION

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>
#include <array>
#include <chrono>

using namespace std;

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 tex_coord;

    static VkVertexInputBindingDescription get_binding_description()
    {
        VkVertexInputBindingDescription binding_description{};
        
        binding_description.binding = 0;
        binding_description.stride = sizeof(Vertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions()
    {
        array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};
        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(Vertex, position);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(Vertex, color);

        attribute_descriptions[2].binding = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

        return attribute_descriptions;
    }
};

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class VulkanManager
{
public:
    VulkanManager()
    {
        make_window();
        start_vulkan();
        process();
        cleanup();
    }

private:
    const int MAX_FRAMES_IN_FLIGHT = 2;
    const string model_path = "Models/donut.obj";

    vector<Vertex> verticles;
    vector<uint32_t> indices;
    vector<const char*> device_extensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    vector<VkImage> swap_chain_images;
    vector<VkImageView> swap_chain_image_views;
    vector<VkFramebuffer> swap_chain_framebuffers;
    GLFWwindow* window;
    VkInstance vulkan_instance;
    VkPhysicalDevice phys_device = VK_NULL_HANDLE;
    VkDevice logical_device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swap_chain;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    VkPipeline pipeline;
    VkCommandPool command_pool;
    VkDescriptorPool descriptor_pool;
    vector<VkDescriptorSet> descriptor_sets;
    vector<VkCommandBuffer> command_buffers;
    uint32_t current_frame = 0;
    bool frame_buffer_resized = false;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    vector<VkBuffer> uniform_buffers;
    vector<VkDeviceMemory> uniform_buffers_memory;
    vector<void*> uniform_buffers_mapped;

    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    VkSampler texture_sampler;

    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    vector<VkSemaphore> image_semaphores;
    vector<VkSemaphore> render_semaphores;
    vector<VkFence> in_flight_fences;

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
    void recreate_swap_chain();
    void remove_swap_chain();
    VkImageView add_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);
    void add_image_views();
    VkSurfaceFormatKHR get_swap_surface_format();
    VkPresentModeKHR get_swap_present_mode();
    VkExtent2D get_swap_extend(VkSurfaceCapabilitiesKHR capabilities);
    void add_descriptor_set_layout();
    void add_graphics_pipeline();
    void add_render_pass();
    void add_framebuffers();
    void add_command_pool();
    void add_descriptor_pool();
    void add_descriptor_sets();
    void add_command_buffers();
    void add_sync_objects();
    void add_buffer(VkBuffer& buff, VkDeviceMemory& buff_memory, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void copy_buffer(VkBuffer from_buff, VkBuffer to_buff, VkDeviceSize size);
    void load_model();
    void add_vertex_buffer();
    void add_indices_buffer();
    void add_uniform_buffers();
    void update_uniform_buffer(uint32_t current_frame);
    vector<char> get_shader_code(string filename);
    VkShaderModule get_shader_module(vector<char> shader_code);
    uint32_t get_graphics_family_index();
    uint32_t get_present_family_index();
    uint32_t get_memory_type(uint32_t filter, VkMemoryPropertyFlags properties);

    void add_depth_resources();
    VkFormat get_supported_format(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat find_depth_format();
    bool has_stencil_component(VkFormat format);

    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buff);
    void add_texture_image();
    void add_texture_image_view();
    void add_image(uint32_t texture_width, uint32_t texture_height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);
    void add_texture_sampler();
    void change_image_layout(VkImage image, VkFormat format, VkImageLayout layout, VkImageLayout new_layout);
    void copy_buffer_to_image(VkBuffer buff, VkImage image, uint32_t width, uint32_t height);
    
    void record_command_buffer(VkCommandBuffer buff, uint32_t image_index);
    void draw_frame();
    
    static void frame_buffer_resize_callback(GLFWwindow* window, int width, int height);

    void make_window();
};

void VulkanManager::start_vulkan()
{
    create_vulkan();
    add_surface();
    phys_device = get_physical_device();
    get_logical_device();
    add_swap_chain();
    add_image_views();
    add_render_pass();
    add_descriptor_set_layout();
    add_graphics_pipeline();
    add_command_pool();
    add_depth_resources();
    add_framebuffers();
    add_texture_image();
    add_texture_image_view();
    add_texture_sampler();
    load_model();
    add_vertex_buffer();
    add_indices_buffer();
    add_uniform_buffers();
    add_descriptor_pool();
    add_descriptor_sets();
    add_command_buffers();
    add_sync_objects();
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

uint32_t VulkanManager::get_graphics_family_index()
{
    uint32_t queue_family_count = 0;
    vector<VkQueueFamilyProperties> families_property;
    
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);
    families_property.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, families_property.data());
    
    for (int family_index = 0; family_index < queue_family_count; family_index++)
    {
        if (families_property[family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            return family_index;
    }
}

uint32_t VulkanManager::get_present_family_index()
{
    VkBool32 present_support;
    uint32_t queue_family_count = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);

    for (int family_index = 0; family_index < queue_family_count; family_index++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(phys_device, family_index, surface, &present_support);
        if (present_support)
            return family_index;
    }
    return 0;
}

uint32_t VulkanManager::get_memory_type(uint32_t filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;

    vkGetPhysicalDeviceMemoryProperties(phys_device, &memory_properties);

    for (uint32_t type = 0; type < memory_properties.memoryTypeCount; type++)
    {
        if ((filter & (1 << type)) and (memory_properties.memoryTypes[type].propertyFlags & properties) == properties)
            return type;
    }
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

    device_features.samplerAnisotropy = VK_TRUE;

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

VkImageView VulkanManager::add_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo img_view_create_info{};
    VkImageView image_view;

    img_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_create_info.image = image;
    img_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    img_view_create_info.format = format;
    img_view_create_info.subresourceRange.aspectMask = aspect_flags;
    img_view_create_info.subresourceRange.baseMipLevel = 0;
    img_view_create_info.subresourceRange.baseArrayLayer = 0;
    img_view_create_info.subresourceRange.levelCount = 1;
    img_view_create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(logical_device, &img_view_create_info, nullptr, &image_view) != VK_SUCCESS)
    {
        cout << "Adding texture image view error!" << endl;
        return image_view;
    }

    return image_view;
}

void VulkanManager::add_image_views()
{
    swap_chain_image_views.resize(swap_chain_images.size());

    for (int image_index = 0; image_index < swap_chain_image_views.size(); image_index++)
    {
        swap_chain_image_views[image_index] = add_image_view(swap_chain_images[image_index], swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    cout << "Create image views success!" << endl;
}

void VulkanManager::recreate_swap_chain()
{
    vkDeviceWaitIdle(logical_device);

    remove_swap_chain();

    add_swap_chain();
    add_image_views();
    add_depth_resources();
    add_framebuffers();
}

void VulkanManager::remove_swap_chain()
{
    for (VkFramebuffer framebuffer : swap_chain_framebuffers)
        vkDestroyFramebuffer(logical_device, framebuffer, nullptr);

    for (VkImageView image_view : swap_chain_image_views)
        vkDestroyImageView(logical_device, image_view, nullptr);

    vkDestroySwapchainKHR(logical_device, swap_chain, nullptr);
}

void VulkanManager::add_buffer(VkBuffer& buff, VkDeviceMemory& buff_memory, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo buffer_create_info{};
    VkMemoryRequirements memory_requirements{};
    VkMemoryAllocateInfo allocate_info{};
    void* data;

    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(logical_device, &buffer_create_info, nullptr, &buff) != VK_SUCCESS)
    {
        cout << "Creating vertex buffer error!" << endl;
        return;
    }

    vkGetBufferMemoryRequirements(logical_device, buff, &memory_requirements);

    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = get_memory_type(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(logical_device, &allocate_info, nullptr, &buff_memory))
    {
        cout << "Allocating vertex buffer memory error!" << endl;
        return;
    }
    vkBindBufferMemory(logical_device, buff, buff_memory, 0);
}

void VulkanManager::copy_buffer(VkBuffer from_buff, VkBuffer to_buff, VkDeviceSize size)
{
    VkBufferCopy copy_region{};
    VkCommandBuffer command_buff = begin_single_time_commands();

    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;

    vkCmdCopyBuffer(command_buff, from_buff, to_buff, 1, &copy_region);
    end_single_time_commands(command_buff);
}

void VulkanManager::load_model()
{
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str()))
        cout << warn + err;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = 
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.tex_coord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            verticles.push_back(vertex);
            indices.push_back(indices.size());
        }
    }
}

void VulkanManager::add_vertex_buffer()
{
    VkDeviceSize size = sizeof(verticles[0]) * verticles.size();
    VkBuffer staging_buffer{};
    VkDeviceMemory staging_buffer_memory{};
    void* data;
    
    add_buffer(staging_buffer, staging_buffer_memory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkMapMemory(logical_device, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, verticles.data(), (size_t)size);
    vkUnmapMemory(logical_device, staging_buffer_memory);

    add_buffer(vertex_buffer, vertex_buffer_memory, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(staging_buffer, vertex_buffer, size);

    vkDestroyBuffer(logical_device, staging_buffer, nullptr);
    vkFreeMemory(logical_device, staging_buffer_memory, nullptr);
}

void VulkanManager::add_indices_buffer()
{
    VkDeviceSize size = sizeof(indices[0]) * indices.size();
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    void* data;

    add_buffer(staging_buffer, staging_buffer_memory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkMapMemory(logical_device, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, indices.data(), (size_t)size);
    vkUnmapMemory(logical_device, staging_buffer_memory);

    add_buffer(index_buffer, index_buffer_memory, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(staging_buffer, index_buffer, size);

    vkDestroyBuffer(logical_device, staging_buffer, nullptr);
    vkFreeMemory(logical_device, staging_buffer_memory, nullptr);
}

void VulkanManager::add_uniform_buffers()
{
    VkDeviceSize size = sizeof(UniformBufferObject);

    uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t buffer_index = 0; buffer_index < MAX_FRAMES_IN_FLIGHT; buffer_index++)
    {
        add_buffer(uniform_buffers[buffer_index], uniform_buffers_memory[buffer_index],
            size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkMapMemory(logical_device, uniform_buffers_memory[buffer_index], 0, size, 0, &uniform_buffers_mapped[buffer_index]);
    }
}

void VulkanManager::update_uniform_buffer(uint32_t current_frame)
{
    static auto start_time = chrono::high_resolution_clock::now();

    UniformBufferObject ubo{};
    auto current_time = chrono::high_resolution_clock::now();
    float time = chrono::duration<float, chrono::seconds::period>(current_time - start_time).count();

    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (float) swap_chain_extent.width / swap_chain_extent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    memcpy(uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));
}

void VulkanManager::add_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    VkDescriptorSetLayoutCreateInfo layout_create_info{};

    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.pImmutableSamplers = nullptr;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };

    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_create_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(logical_device, &layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
    {
        cout << "Creating descriptor set layout error!" << endl;
        return;
    }
}

void VulkanManager::add_descriptor_pool()
{
    array<VkDescriptorPoolSize, 2> sizes{};
    VkDescriptorPoolCreateInfo pool_create_info{};

    sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool_create_info.pPoolSizes = sizes.data();
    pool_create_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(logical_device, &pool_create_info, nullptr, &descriptor_pool) != VK_SUCCESS)
        cout << "Creating descriptors pool error!" << endl;
}

void VulkanManager::add_descriptor_sets()
{
    vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info{};

    descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);

    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool = descriptor_pool;
    descriptor_set_alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    descriptor_set_alloc_info.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(logical_device, &descriptor_set_alloc_info, descriptor_sets.data()) != VK_SUCCESS)
        cout << "Allocating descriptor sets error!" << endl;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo buffer_info{};
        VkDescriptorImageInfo image_info{};
        array<VkWriteDescriptorSet, 2> descriptor_writes{};

        buffer_info.buffer = uniform_buffers[i];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(UniformBufferObject);

        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = texture_image_view;
        image_info.sampler = texture_sampler;

        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = descriptor_sets[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &buffer_info;
        descriptor_writes[0].pImageInfo = nullptr;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = descriptor_sets[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pImageInfo = &image_info;
        
        vkUpdateDescriptorSets(logical_device, static_cast<uint32_t>(descriptor_writes.size()),
            descriptor_writes.data(), 0, nullptr);
    }
}

void VulkanManager::add_graphics_pipeline()
{
    vector<VkDynamicState> dynamic_states = 
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkPipelineDynamicStateCreateInfo dynamic_states_create_info{};
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
    VkVertexInputBindingDescription vertex_binding = Vertex::get_binding_description();
    array<VkVertexInputAttributeDescription, 3> vertex_attributes = Vertex::get_attribute_descriptions();
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
    VkPipelineViewportStateCreateInfo viewport_state{};
    VkPipelineRasterizationStateCreateInfo rasterizer_create_info{};
    VkPipelineMultisampleStateCreateInfo multisampling_create_info{};
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    VkPipelineColorBlendStateCreateInfo color_blending_create_info{};
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    VkPipelineShaderStageCreateInfo vert_shader_stage_create_info{};
    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info{};
    VkPipelineShaderStageCreateInfo shader_stages_create_infos[2];
    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info{};

    dynamic_states_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_states_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_states_create_info.pDynamicStates = dynamic_states.data();

    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &vertex_binding;
    vertex_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attributes.size());
    vertex_input_create_info.pVertexAttributeDescriptions = vertex_attributes.data();

    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_create_info.depthClampEnable = VK_FALSE;
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_create_info.lineWidth = 1.0;
    rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_create_info.depthBiasEnable = VK_FALSE;
    rasterizer_create_info.depthBiasConstantFactor = 0.0;
    rasterizer_create_info.depthBiasClamp = 0.0;
    rasterizer_create_info.depthBiasSlopeFactor = 0.0;

    multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_create_info.sampleShadingEnable = VK_FALSE;
    multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_create_info.minSampleShading = 1.0;
    multisampling_create_info.pSampleMask = nullptr;
    multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_create_info.alphaToOneEnable = VK_FALSE;

    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_create_info.logicOpEnable = VK_FALSE;
    color_blending_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blending_create_info.attachmentCount = 1;
    color_blending_create_info.pAttachments = &color_blend_attachment;
    color_blending_create_info.blendConstants[0] = 0.0;
    color_blending_create_info.blendConstants[1] = 0.0;
    color_blending_create_info.blendConstants[2] = 0.0;
    color_blending_create_info.blendConstants[3] = 0.0;

    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    auto vert_shader_code = get_shader_code("Shaders/vert.spv");
    auto frag_shader_code = get_shader_code("Shaders/frag.spv");

    vert_shader_module = get_shader_module(vert_shader_code);
    frag_shader_module = get_shader_module(frag_shader_code);

    vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_create_info.module = vert_shader_module;
    vert_shader_stage_create_info.pName = "main";

    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = frag_shader_module;
    frag_shader_stage_create_info.pName = "main";
    
    shader_stages_create_infos[0] = vert_shader_stage_create_info;
    shader_stages_create_infos[1] = frag_shader_stage_create_info;

    if (vkCreatePipelineLayout(logical_device, &pipeline_layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
        cout << "Creating pipeline layout error!" << endl;

    depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_create_info.minDepthBounds = 0.0;
    depth_stencil_create_info.maxDepthBounds = 1.0;
    depth_stencil_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_create_info.front = {};
    depth_stencil_create_info.back = {};

    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages_create_infos;
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState = &multisampling_create_info;
    pipeline_create_info.pDepthStencilState = nullptr;
    pipeline_create_info.pColorBlendState = &color_blending_create_info;
    pipeline_create_info.pDynamicState = &dynamic_states_create_info;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;
    pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;

    if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline) != VK_SUCCESS)
        cout << "Creating pipeline error!" << endl;
    else
        cout << "Creating pipeline success!" << endl;

    vkDestroyShaderModule(logical_device, vert_shader_module, nullptr);
    vkDestroyShaderModule(logical_device, frag_shader_module, nullptr);
}

vector<char> VulkanManager::get_shader_code(string filename)
{
    ifstream file(filename, ios::ate | ios::binary);

    size_t file_size = (size_t)file.tellg();
    vector<char> code_array(file_size);
    file.seekg(0);
    file.read(code_array.data(), file_size);

    file.close();

    return code_array;
}

VkShaderModule VulkanManager::get_shader_module(vector<char> shader_code)
{
    VkShaderModule shader_module;
    VkShaderModuleCreateInfo module_create_info{};
    module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_create_info.codeSize = shader_code.size();
    module_create_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

    if (vkCreateShaderModule(logical_device, &module_create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        cout << "Getting shader module error!" << endl;
        return shader_module;
    }

    return shader_module;
}

void VulkanManager::add_render_pass()
{
    VkAttachmentDescription color_attachment{};
    VkAttachmentReference attachment_reference{};
    VkAttachmentDescription depth_attachment{};
    VkAttachmentReference depth_attachment_reference{};
    VkSubpassDescription subpass_description{};
    VkRenderPassCreateInfo render_pass_create_info{};
    VkSubpassDependency subpass_dependency{};

    color_attachment.format = swap_chain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    depth_attachment.format = find_depth_format();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &attachment_reference;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;

    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments = attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    if (vkCreateRenderPass(logical_device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
    {
        cout << "Creating render pass error!" << endl;
        return;
    }
    cout << "Creating render pass success!" << endl;
}

void VulkanManager::add_framebuffers()
{
    swap_chain_framebuffers.resize(swap_chain_image_views.size());

    for (size_t i = 0; i < swap_chain_image_views.size(); i++)
    {
        VkFramebufferCreateInfo frame_buffer_create_info{};
        array<VkImageView, 2> attachments = { swap_chain_image_views[i], depth_image_view };

        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = render_pass;
        frame_buffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        frame_buffer_create_info.pAttachments = attachments.data();
        frame_buffer_create_info.width = swap_chain_extent.width;
        frame_buffer_create_info.height = swap_chain_extent.height;
        frame_buffer_create_info.layers = 1;

        if (vkCreateFramebuffer(logical_device, &frame_buffer_create_info, nullptr, &swap_chain_framebuffers[i]) != VK_SUCCESS)
        {
            cout << "Creating framebuffer error!" << endl;
            return;
        }
    }
}

VkFormat VulkanManager::get_supported_format(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(phys_device, format, &properties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
            return format;
    }
}

bool VulkanManager::has_stencil_component(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT or format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat VulkanManager::find_depth_format()
{
    return get_supported_format({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanManager::add_depth_resources()
{
    VkFormat depth_format = find_depth_format();

    add_image(swap_chain_extent.width, swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);
    depth_image_view = add_image_view(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
    change_image_layout(depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void VulkanManager::add_texture_sampler()
{
    VkSamplerCreateInfo sampler_create_info{};
    VkPhysicalDeviceProperties properties{};

    vkGetPhysicalDeviceProperties(phys_device, &properties);

    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.mipLodBias = 0.0;
    sampler_create_info.minLod = 0.0;
    sampler_create_info.maxLod = 0.0;

    if (vkCreateSampler(logical_device, &sampler_create_info, nullptr, &texture_sampler) != VK_SUCCESS)
    {
        cout << "Adding texture sampler error!" << endl;
        return;
    }

    cout << "Adding texture sampler success!" << endl;
}

void VulkanManager::add_image(uint32_t texture_width, uint32_t texture_height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory)
{
    VkImageCreateInfo image_create_info{};
    VkMemoryRequirements memory_requirements{};
    VkMemoryAllocateInfo texture_image_memory_alloc_info{};

    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = texture_width;
    image_create_info.extent.height = texture_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(logical_device, &image_create_info, nullptr, &image) != VK_SUCCESS)
        cout << "Adding image error!" << endl;

    vkGetImageMemoryRequirements(logical_device, image, &memory_requirements);
    texture_image_memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texture_image_memory_alloc_info.allocationSize = memory_requirements.size;
    texture_image_memory_alloc_info.memoryTypeIndex = get_memory_type(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(logical_device, &texture_image_memory_alloc_info, nullptr, &image_memory) != VK_SUCCESS)
        cout << "Allocating image memory error!" << endl;

    vkBindImageMemory(logical_device, image, image_memory, 0);
}

void VulkanManager::add_texture_image()
{
    int image_width, image_height, image_channels;
    VkDeviceSize image_size;
    stbi_uc* image_pixels = stbi_load("Textures/Gabe.jpg", 
        &image_width, &image_height, &image_channels, STBI_rgb_alpha);
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    void* data;

    image_size = image_width * image_height * 4;

    add_buffer(staging_buffer, staging_buffer_memory, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkMapMemory(logical_device, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, image_pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(logical_device, staging_buffer_memory);

    stbi_image_free(image_pixels);

    add_image(image_width, image_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture_image, texture_image_memory);
    change_image_layout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(staging_buffer, texture_image, static_cast<uint32_t>(image_width), static_cast<uint32_t>(image_height));
    change_image_layout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(logical_device, staging_buffer, nullptr);
    vkFreeMemory(logical_device, staging_buffer_memory, nullptr);
}

void VulkanManager::add_texture_image_view()
{
    texture_image_view = add_image_view(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanManager::change_image_layout(VkImage image, VkFormat format, VkImageLayout layout, VkImageLayout new_layout)
{
    VkCommandBuffer command_buff = begin_single_time_commands();
    VkImageMemoryBarrier barrier{};
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (layout == VK_IMAGE_LAYOUT_UNDEFINED and new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    else if(layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL and new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_UNDEFINED and new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    vkCmdPipelineBarrier(command_buff, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    end_single_time_commands(command_buff);
}

void VulkanManager::copy_buffer_to_image(VkBuffer buff, VkImage image, uint32_t width, uint32_t height)
{
    VkBufferImageCopy region{};
    VkCommandBuffer command_buff = begin_single_time_commands();

    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(command_buff, buff, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    end_single_time_commands(command_buff);
}

void VulkanManager::add_command_pool()
{
    VkCommandPoolCreateInfo command_pool_create_info{};
    uint32_t graphics_family_index = get_graphics_family_index();

    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = graphics_family_index;

    if (vkCreateCommandPool(logical_device, &command_pool_create_info, nullptr, &command_pool) != VK_SUCCESS)
    {
        cout << "Creating command pool error!" << endl;
        return;
    }
    cout << "Creating command pool success!" << endl;
}

VkCommandBuffer VulkanManager::begin_single_time_commands()
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    VkCommandBufferBeginInfo begin_info{};
    VkCommandBuffer command_buff;

    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkAllocateCommandBuffers(logical_device, &command_buffer_allocate_info, &command_buff);
    vkBeginCommandBuffer(command_buff, &begin_info);

    return command_buff;
}

void VulkanManager::end_single_time_commands(VkCommandBuffer command_buff)
{
    VkQueue graphics_queue;
    VkSubmitInfo submit_info{};

    vkGetDeviceQueue(logical_device, get_graphics_family_index(), 0, &graphics_queue);

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buff;

    vkEndCommandBuffer(command_buff);
    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buff);
}

void VulkanManager::add_command_buffers()
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};

    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = (uint32_t)command_buffers.size();

    if (vkAllocateCommandBuffers(logical_device, &command_buffer_allocate_info, command_buffers.data()) != VK_SUCCESS)
    {
        cout << "Creating comand buffer error!" << endl;
        return;
    }
    cout << "Creating comand buffer success!" << endl;
}

void VulkanManager::record_command_buffer(VkCommandBuffer buff, uint32_t image_index)
{
    VkViewport viewport{};
    VkRect2D scissors{};
    VkCommandBufferBeginInfo begin_info{};
    VkRenderPassBeginInfo render_pass_info{};
    VkBuffer vertex_buffers[] = { vertex_buffer };
    VkDeviceSize offsets[] = { 0 };
    array<VkClearValue, 2> clear_values{};
    
    clear_values[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clear_values[1].depthStencil = { 1.0f, 0 };

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swap_chain_framebuffers[image_index];
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = swap_chain_extent;
    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = static_cast<float>(swap_chain_extent.width);
    viewport.height = static_cast<float>(swap_chain_extent.height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    scissors.offset = { 0, 0 };
    scissors.extent = swap_chain_extent;

    if (vkBeginCommandBuffer(buff, &begin_info) != VK_SUCCESS)
        cout << "Begin recording error!" << endl;
    vkCmdBeginRenderPass(buff, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetViewport(buff, 0, 1, &viewport);
    vkCmdSetScissor(buff, 0, 1, &scissors);
    vkCmdBindVertexBuffers(buff, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(buff, index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
        0, 1, &descriptor_sets[current_frame], 0, nullptr);
    vkCmdDrawIndexed(buff, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(buff);

    if (vkEndCommandBuffer(buff) != VK_SUCCESS)
        cout << "Recording command buffer error!" << endl;
}

void VulkanManager::add_sync_objects()
{
    VkSemaphoreCreateInfo semaphore_create_info{};
    VkFenceCreateInfo fence_create_info{};

    image_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t sync_obj_index = 0; sync_obj_index < MAX_FRAMES_IN_FLIGHT; sync_obj_index++)
    {
        if (vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr, &image_semaphores[sync_obj_index]) != VK_SUCCESS or
            vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr, &render_semaphores[sync_obj_index]) != VK_SUCCESS or
            vkCreateFence(logical_device, &fence_create_info, nullptr, &in_flight_fences[sync_obj_index]) != VK_SUCCESS)
            cout << "Creating sync objects error!" << endl;
    }
}

void VulkanManager::add_surface()
{
    if (glfwCreateWindowSurface(vulkan_instance, window, nullptr, &surface) != VK_SUCCESS)
        cout << "Making Window surface error!";
    cout << "Making surface success!" << endl;
}

void VulkanManager::draw_frame()
{
    uint32_t image_index;
    uint32_t graphics_family_index = get_graphics_family_index();
    uint32_t present_family_index = get_present_family_index();
    VkSubmitInfo submit_info{};
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkPresentInfoKHR present_info{};
    VkSwapchainKHR swap_chains[] = { swap_chain };
    VkResult acquire_next_image_result;
    VkResult present_result;

    update_uniform_buffer(current_frame);

    vkGetDeviceQueue(logical_device, graphics_family_index, 0, &graphics_queue);
    vkGetDeviceQueue(logical_device, present_family_index, 0, &present_queue);

    vkWaitForFences(logical_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

    acquire_next_image_result = vkAcquireNextImageKHR(logical_device, swap_chain, UINT64_MAX, image_semaphores[current_frame], VK_NULL_HANDLE, &image_index);
    
    if (acquire_next_image_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swap_chain();
        return;
    }

    vkResetFences(logical_device, 1, &in_flight_fences[current_frame]);

    vkResetCommandBuffer(command_buffers[current_frame], 0);
    record_command_buffer(command_buffers[current_frame], image_index);

    VkSemaphore wait_semaphores[] = { image_semaphores[current_frame] };
    VkSemaphore signal_semaphores[] = { render_semaphores[current_frame] };

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[current_frame];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
        cout << "Submitting draw comand buffer error!" << endl;
    
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    present_result = vkQueuePresentKHR(present_queue, &present_info);

    if (present_result == VK_ERROR_OUT_OF_DATE_KHR or present_result == VK_SUBOPTIMAL_KHR or frame_buffer_resized)
    {
        recreate_swap_chain();
        frame_buffer_resized = false;
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanManager::process()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        draw_frame();
    }
    
    vkDeviceWaitIdle(logical_device);
}

void VulkanManager::cleanup()
{
    vkDestroyImageView(logical_device, depth_image_view, nullptr);
    vkDestroyImage(logical_device, depth_image, nullptr);
    vkFreeMemory(logical_device, depth_image_memory, nullptr);
    remove_swap_chain();

    vkDestroySampler(logical_device, texture_sampler, nullptr);
    vkDestroyImageView(logical_device, texture_image_view, nullptr);

    vkDestroyImage(logical_device, texture_image, nullptr);
    vkFreeMemory(logical_device, texture_image_memory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(logical_device, uniform_buffers[i], nullptr);
        vkFreeMemory(logical_device, uniform_buffers_memory[i], nullptr);
    }

    vkDestroyDescriptorPool(logical_device, descriptor_pool, nullptr);

    vkDestroyDescriptorSetLayout(logical_device, descriptor_set_layout, nullptr);

    vkDestroyBuffer(logical_device, index_buffer, nullptr);
    vkFreeMemory(logical_device, index_buffer_memory, nullptr);
    vkDestroyBuffer(logical_device, vertex_buffer, nullptr);
    vkFreeMemory(logical_device, vertex_buffer_memory, nullptr);
    
    for (size_t sync_obj_index = 0; sync_obj_index < MAX_FRAMES_IN_FLIGHT; sync_obj_index++)
    {
        vkDestroySemaphore(logical_device, image_semaphores[sync_obj_index], nullptr);
        vkDestroySemaphore(logical_device, render_semaphores[sync_obj_index], nullptr);
        vkDestroyFence(logical_device, in_flight_fences[sync_obj_index], nullptr);
    }
    vkDestroyCommandPool(logical_device, command_pool, nullptr);
    vkDestroyPipeline(logical_device, pipeline, nullptr);
    vkDestroyPipelineLayout(logical_device, pipeline_layout, nullptr);
    vkDestroyRenderPass(logical_device, render_pass, nullptr);
    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanManager::frame_buffer_resize_callback(GLFWwindow* window, int width, int height)
{
    VulkanManager* vulkan = reinterpret_cast<VulkanManager*>(glfwGetWindowUserPointer(window));
    vulkan->frame_buffer_resized = true;
}

void VulkanManager::make_window()
{
    int window_h = 640, window_w = 800;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    cout << "Creating application window success!" << endl;

    window = glfwCreateWindow(window_w, window_h, "VulkanTestApplication", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frame_buffer_resize_callback);
}

int main()
{
    VulkanManager vulkan;
    return EXIT_SUCCESS;
}