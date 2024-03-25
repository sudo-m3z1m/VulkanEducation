#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

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
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    VkPipeline pipeline;
    VkCommandPool command_pool;
    vector<VkCommandBuffer> command_buffers;
    uint32_t current_frame = 0;
    bool frame_buffer_resized = false;

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
    void add_image_views();
    VkSurfaceFormatKHR get_swap_surface_format();
    VkPresentModeKHR get_swap_present_mode();
    VkExtent2D get_swap_extend(VkSurfaceCapabilitiesKHR capabilities);
    void add_graphics_pipeline();
    void add_render_pass();
    void add_framebuffers();
    void add_command_pool();
    void add_command_buffers();
    void add_sync_objects();
    vector<char> get_shader_code(string filename);
    VkShaderModule get_shader_module(vector<char> shader_code);
    uint32_t get_graphics_family_index();
    uint32_t get_present_family_index();
    
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
    add_graphics_pipeline();
    add_framebuffers();
    add_command_pool();
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

void VulkanManager::add_image_views()
{
    swap_chain_image_views.resize(swap_chain_images.size());

    for (int image_index = 0; image_index < swap_chain_image_views.size(); image_index++)
    {
        VkImageViewCreateInfo image_view_create_info{};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = swap_chain_images[image_index];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = swap_chain_image_format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(logical_device, &image_view_create_info, nullptr, &swap_chain_image_views[image_index]) != VK_SUCCESS)
        {
            cout << "Create image view with " << image_index << " index error.";
            continue;
        }
    }
    cout << "Create image views success!" << endl;
}

void VulkanManager::recreate_swap_chain()
{
    vkDeviceWaitIdle(logical_device);

    remove_swap_chain();

    add_swap_chain();
    add_image_views();
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

    dynamic_states_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_states_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_states_create_info.pDynamicStates = dynamic_states.data();

    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 0;
    vertex_input_create_info.pVertexBindingDescriptions = nullptr;
    vertex_input_create_info.vertexAttributeDescriptionCount = 0;
    vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

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
    rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipeline_layout_create_info.setLayoutCount = 0;
    pipeline_layout_create_info.pSetLayouts = nullptr;
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

    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &attachment_reference;

    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment;
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
        VkImageView attachments[] = {
            swap_chain_image_views[i]
        };

        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = render_pass;
        frame_buffer_create_info.attachmentCount = 1;
        frame_buffer_create_info.pAttachments = attachments;
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
    VkClearValue clear_color = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swap_chain_framebuffers[image_index];
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = swap_chain_extent;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

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
    vkCmdDraw(buff, 3, 1, 0, 0);
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
    remove_swap_chain();
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