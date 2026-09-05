#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WIN_WIDTH  1024
#define WIN_HEIGHT 1024

typedef struct
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    uint32_t graphicsFamilyIndex;
    VkSwapchainKHR swapchainKHR;
    uint32_t swapchainImageCount;
    VkFormat imageFormat;
    VkImageView* imageViews;
    VkRenderPass renderPass;
    VkFramebuffer* framebuffers;
} VulkanContext;
    uint32_t attachmentCount;
void InitGLFW();

void ClearVulkanContext( VulkanContext* _vkContext );

VkResult CreateVkInstance( VkInstance* _instance );

void GetPhysicalDevice( VkInstance _instance, VkPhysicalDevice* _physDevice );

void GetFamilyQueues( VkPhysicalDevice _physDevice , uint32_t* _graphicsFamilyIndex );

void GetDeviceExtensions( VkPhysicalDevice _physDevice, uint32_t* extentionsCount, const char** extentionsNames );

VkResult CreateDevice( VulkanContext* _vkContext );

VkResult CreateSwapChain( VulkanContext* _vkContext );

VkResult CreateImageViews( VulkanContext* _vkContext );

VkResult CreateRenderPass( VulkanContext* _vkContext );

VkResult CreateFrameBuffers( VulkanContext* _vkContext );

int main()
{
    InitGLFW();
    GLFWwindow* window = glfwCreateWindow( WIN_WIDTH, WIN_HEIGHT, "VulkanApp", NULL, NULL );

    VulkanContext vkContext = {};

    if ( CreateVkInstance( &vkContext.instance ) != VK_SUCCESS )
    {
        printf("Failed to create instance\n");
        return -1;
    }

    glfwCreateWindowSurface( vkContext.instance, window, NULL, &vkContext.surface );

    if ( CreateDevice( &vkContext ) != VK_SUCCESS ) {
        printf("Failed to create logical device\n");
        return -1;
    }

    if ( CreateSwapChain( &vkContext ) != VK_SUCCESS )
    {
        printf("Failed to create swapchain\n");
        return -1;
    }

    if (CreateImageViews( &vkContext ) != VK_SUCCESS )
    {
        printf("Failed to create ImageViews\n");
        return -1;
    }

    if( CreateRenderPass( &vkContext ) )
    {
        printf("Failed to create RenderPass\n");
        return -1;
    }

    if( CreateFrameBuffers( &vkContext ) )
    {
        printf("Failed to create FrameBuffers\n");
        return -1;
    }

    while ( !glfwWindowShouldClose( window ) )
    {
        glfwPollEvents();

    }

    glfwDestroyWindow(window);
    glfwTerminate();

    ClearVulkanContext( &vkContext );

    return 0;
}


//========== GLFW ==========//
void InitGLFW()
{
    glfwInit();

    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
}


//========== Vulkan ==========//
VkResult CreateVkInstance( VkInstance* _instance )
{
    VkResult result = VK_SUCCESS;

    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "VulkanApp",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = NULL,
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_4
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL
    };

    extensions = NULL;

    result = vkCreateInstance( &createInfo, NULL, _instance);

    return result;
}


void GetPhysicalDevice( VkInstance _instance, VkPhysicalDevice* _physDevice )
{
    uint32_t physicalDeviceCount = 0;

    vkEnumeratePhysicalDevices( _instance, &physicalDeviceCount, NULL);
    VkPhysicalDevice* physicalDevices = malloc( physicalDeviceCount * sizeof( VkPhysicalDevice ) );
    vkEnumeratePhysicalDevices( _instance, &physicalDeviceCount, physicalDevices );

    *_physDevice = physicalDevices[0];

    free( physicalDevices );
    physicalDevices = NULL;
}


void GetFamilyQueues( VkPhysicalDevice _physDevice , uint32_t* _graphicsFamilyIndex )
{
    *_graphicsFamilyIndex = UINT32_MAX;

    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties ( _physDevice, &queueFamilyPropertyCount, NULL );
    VkQueueFamilyProperties* queueFamilyProperties = malloc(queueFamilyPropertyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties ( _physDevice, &queueFamilyPropertyCount, queueFamilyProperties );

    for (uint32_t i = 0; i < queueFamilyPropertyCount; i++)
    {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            *_graphicsFamilyIndex = i;
            break;
        }
    }

    free( queueFamilyProperties );
    queueFamilyProperties = NULL;
}


void GetDeviceExtensions( VkPhysicalDevice _physDevice, uint32_t* extentionsCount, const char** extentionsNames )
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties( _physDevice, NULL, &count, NULL );
    VkExtensionProperties* graphicsExtantionProperties = malloc( count * sizeof( VkExtensionProperties ) );
    vkEnumerateDeviceExtensionProperties( _physDevice, NULL, &count, graphicsExtantionProperties );

    for( uint32_t i = 0; i < count; i++ )
    {
        if (strcmp(graphicsExtantionProperties[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
        {
            extentionsNames[0] = graphicsExtantionProperties[i].extensionName;
            (*extentionsCount)++;
            break;
        }
    }

    free( graphicsExtantionProperties );
    graphicsExtantionProperties = NULL;
}


VkResult CreateDevice( VulkanContext* _vkContext )
{
    VulkanContext context = *(_vkContext);

    GetPhysicalDevice( context.instance, &context.physicalDevice );

    GetFamilyQueues( context.physicalDevice, &context.graphicsFamilyIndex );

    if (context.graphicsFamilyIndex == UINT32_MAX ) {
        printf("No get queues family!\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo graphicsQueue = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = context.graphicsFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    uint32_t extentionsCount = 0;
    const char* extentions[1];

    GetDeviceExtensions( context.physicalDevice, &extentionsCount, extentions);
    if ( extentionsCount == 0 )
    {
        printf("No extentions found!\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &graphicsQueue,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = extentionsCount,
        .ppEnabledExtensionNames = extentions,
        .pEnabledFeatures = NULL
    };

    VkResult result = vkCreateDevice(context.physicalDevice, &deviceCreateInfo, NULL, &context.device);
    if( result == VK_SUCCESS )
    {
        vkGetDeviceQueue(context.device, context.graphicsFamilyIndex, 0, &context.graphicsQueue);
    }

    *(_vkContext) = context;

    return result;
}


VkResult CreateSwapChain( VulkanContext* _vkContext )
{
    VulkanContext context = *(_vkContext);

    VkBool32 presentSupport = VK_SUCCESS;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        context.physicalDevice,
        context.graphicsFamilyIndex,
        context.surface,
        &presentSupport
    );

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( context.physicalDevice, context.surface, &caps );

    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( context.physicalDevice, context.surface, &surfaceFormatCount, NULL );
    VkSurfaceFormatKHR* surfaceFormatKHR = malloc( surfaceFormatCount * sizeof( VkSurfaceFormatKHR ) );
    vkGetPhysicalDeviceSurfaceFormatsKHR( context.physicalDevice, context.surface, &surfaceFormatCount, surfaceFormatKHR );

    context.imageFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    for ( uint32_t i = 0; i < surfaceFormatCount; i++ )
    {
        if ( surfaceFormatKHR[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
             surfaceFormatKHR[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            context.imageFormat = surfaceFormatKHR[i].format;
            imageColorSpace = surfaceFormatKHR[i].colorSpace;
            break;
        }
    }

    VkExtent2D imageExtent = { WIN_WIDTH, WIN_HEIGHT };

    VkSwapchainCreateInfoKHR swapchainCreateInfoKHR = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = context.surface,
        .minImageCount = caps.minImageCount + 1,
        .imageFormat = context.imageFormat,
        .imageColorSpace = imageColorSpace,
        .imageExtent = imageExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    VkResult result = vkCreateSwapchainKHR( context.device, &swapchainCreateInfoKHR, NULL, &_vkContext->swapchainKHR );

    *(_vkContext) = context;

    free( surfaceFormatKHR );
    surfaceFormatKHR = NULL;

    return result;
}


VkResult CreateImageViews( VulkanContext* _vkContext )
{
    VulkanContext context = *(_vkContext);
    VkResult result = VK_SUCCESS;

    vkGetSwapchainImagesKHR( context.device, context.swapchainKHR, &context.swapchainImageCount, NULL);
    VkImage* images = malloc( context.swapchainImageCount * sizeof( VkImage ) );
    context.imageViews = malloc( context.swapchainImageCount * sizeof( VkImageView ) );
    vkGetSwapchainImagesKHR( context.device, context.swapchainKHR, &context.swapchainImageCount, images );

    for ( uint32_t i = 0; i < context.swapchainImageCount; i++ )
    {
        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = context.imageFormat,
            .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        result = vkCreateImageView( context.device, &viewInfo, NULL, &context.imageViews[i] );

        if (result != VK_SUCCESS)
        {
            printf("Failed to create ImageViews %d\n", i);
            break;
        }
    }

    free( images ); images = NULL;

    return result;
}


VkResult CreateRenderPass( VulkanContext* _vkContext )
{
    VkResult result = VK_SUCCESS;
    VulkanContext context = *( _vkContext );

    VkAttachmentDescription colorAttachment = {
        .flags = 0,
        .format = context.imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = NULL,
        .preserveAttachmentCount = 0,
        .pDepthStencilAttachment = NULL
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    VkRenderPassCreateInfo renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    result = vkCreateRenderPass( context.device, &renderPassCreateInfo, NULL, &context.renderPass );

    *(_vkContext) = context;

    return result;
}


VkResult CreateFrameBuffers( VulkanContext* _vkContext )
{
    VkResult result = VK_SUCCESS;

    VulkanContext context = *(_vkContext);

    context.framebuffers = malloc( context.swapchainImageCount * sizeof( VkFramebuffer ) );

    for (uint32_t i = 0; i < context.swapchainImageCount; i++)
    {
        VkFramebufferCreateInfo frameBufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = context.renderPass,
            .attachmentCount = 1,
            .pAttachments = &context.imageViews[i],
            .width = WIN_WIDTH,
            .height = WIN_HEIGHT,
            .layers = 1
        };

        result = vkCreateFramebuffer( context.device, &frameBufferCreateInfo, NULL, &context.framebuffers[i]);
        if( result != VK_SUCCESS )
        {
            printf( "Failed to create FrameBuffer[%d]\n", i );
            break;
        }
    }

    *(_vkContext) = context;

    return result;
}


void ClearVulkanContext( VulkanContext* _vkContext )
{
    for ( uint32_t i = 0; i < _vkContext->swapchainImageCount; i++ )
    {
        vkDestroyFramebuffer( _vkContext->device, _vkContext->framebuffers[i], NULL);
        _vkContext->framebuffers[i] = VK_NULL_HANDLE;
    }

    free ( _vkContext->framebuffers );
    _vkContext->framebuffers = NULL;

    if ( _vkContext->renderPass )
    {
        vkDestroyRenderPass( _vkContext->device, _vkContext->renderPass, NULL );
        _vkContext->renderPass = VK_NULL_HANDLE;
    }

    for ( uint32_t i = 0; i < _vkContext->swapchainImageCount; i++ )
    {
        vkDestroyImageView( _vkContext->device, _vkContext->imageViews[i], NULL );
        _vkContext->imageViews[i] = VK_NULL_HANDLE;
    }

    free ( _vkContext->imageViews ); _vkContext->imageViews = NULL;
    _vkContext->imageViews = NULL;

    if ( _vkContext->swapchainKHR )
    {
        vkDestroySwapchainKHR(_vkContext->device, _vkContext->swapchainKHR, NULL);
        _vkContext->swapchainKHR = VK_NULL_HANDLE;
    }

    if ( _vkContext->device )
    {
        vkDestroyDevice(_vkContext->device, NULL);
        _vkContext->device = VK_NULL_HANDLE;
    }

    if ( _vkContext->instance )
    {
        vkDestroyInstance(_vkContext->instance, NULL);
        _vkContext->instance = VK_NULL_HANDLE;
    }
}
