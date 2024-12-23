#include "vulkan/vulkan_core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <algorithm>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const bool DEBUG = true;

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, create_info, allocator, debug_messenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, debug_messenger, allocator);
    }
}

typedef struct QueueFamiliyIndicies {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
} QueueFamiliyIndicies;

typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilites;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    std::vector<VkPresentModeKHR> surface_present_modes;
} SwapChainSupportDetails;

class HelloEngine {

private:

    GLFWwindow* m_window;
    VkInstance m_instance;

    VkDebugUtilsMessengerEXT m_debug_messenger;

    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_device;

    VkQueue m_graphics_queue;
    VkQueue m_present_queue;

    VkSurfaceKHR m_surface;

    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchain_images;
    VkFormat m_swapchain_format;
    VkExtent2D m_swapchain_extent;

    std::vector<const char *> get_required_extenstions() {
        uint32_t required_extension_count = 0;
        const char **required_glfw_extensions;
        required_glfw_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);

        std::vector<const char *> required_extensions(required_glfw_extensions, required_glfw_extensions + required_extension_count);
        required_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        if (DEBUG) {
            required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            //extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        return required_extensions;
    }

    void create_instance() {
        if (DEBUG && !check_validation_layer_support()) {
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "vulkan test app",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };

        std::vector<const char *> required_extensions = get_required_extenstions();

        VkInstanceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = (uint32_t)required_extensions.size(),
            .ppEnabledExtensionNames = required_extensions.data(),
        };

        if (vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vulkan instance");
        }
    }

    void init_window() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Window", nullptr, nullptr);

        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

    }

    void create_surface() {
        if(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface");
        }
    }

    void init_vulkan() {
        create_instance();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swapchain();
    }

    void create_swapchain() {
        SwapChainSupportDetails swapchain_support = query_swapchain_support(m_physical_device);

        VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.surface_formats);
        VkPresentModeKHR present_mode = choose_swapchain_present_mode(swapchain_support.surface_present_modes);
        VkExtent2D extent = choose_swapchain_extent(swapchain_support.capabilites);

        uint32_t image_count = swapchain_support.capabilites.minImageCount + 1;

        if (swapchain_support.capabilites.maxImageCount > 0 && image_count > swapchain_support.capabilites.maxImageCount) {
            image_count = swapchain_support.capabilites.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_surface,
            .minImageCount = image_count,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        };

        QueueFamiliyIndicies indicies = find_queue_families(m_physical_device);
        uint32_t queue_family_indicies[] = {indicies.graphics_family.value(), indicies.present_family.value()};

        if(indicies.graphics_family != indicies.present_family) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indicies;
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = nullptr;
        }

        create_info.preTransform = swapchain_support.capabilites.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain");
        }

        vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
        m_swapchain_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapchain_images.data());

        m_swapchain_format = surface_format.format;
        m_swapchain_extent = extent;
    }

    void create_logical_device() {
        QueueFamiliyIndicies indicies = find_queue_families(m_physical_device);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {indicies.graphics_family.value(), indicies.present_family.value()};

        float queue_priority = 1.0f;
        for (uint32_t family : unique_queue_families) {
            VkDeviceQueueCreateInfo queue_create_info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = family,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            };
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo device_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = (uint32_t)queue_create_infos.size(),
            .pQueueCreateInfos = queue_create_infos.data(),
            .enabledExtensionCount = (uint32_t)device_extensions.size(),
            .ppEnabledExtensionNames = device_extensions.data(),
            .pEnabledFeatures = &device_features,
        };

        if (DEBUG) {
            device_create_info.enabledLayerCount = (uint32_t)validation_layers.size();
            device_create_info.ppEnabledLayerNames = validation_layers.data();
        } else {
            device_create_info.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device");
        }

        vkGetDeviceQueue(m_device, indicies.graphics_family.value(), 0, &m_graphics_queue);
        vkGetDeviceQueue(m_device, indicies.present_family.value(), 0, &m_present_queue);
    }

    QueueFamiliyIndicies find_queue_families(VkPhysicalDevice device) {
        QueueFamiliyIndicies indicies;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        VkBool32 present_support = false;
        for (int i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indicies.graphics_family = i;
            }

            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support);

            if (present_support) {
                indicies.present_family = i;
            }

            if (indicies.is_complete()) {
                break;
            }
        }

        return indicies;
    }

    void pick_physical_device() {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

        if (device_count == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> physical_devices(device_count);
        vkEnumeratePhysicalDevices(m_instance, &device_count, physical_devices.data());

        for (const auto& device : physical_devices) {
            if (is_device_suitable(device)) {
                m_physical_device = device;
                break;
            }
        }

        if (m_physical_device == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable physical device");
        }
    }

    bool is_device_suitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);

        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceFeatures(device, &device_features);

        std::cout << device_properties.deviceName << "\n";
        std::cout << device_properties.deviceType << "\n";

        QueueFamiliyIndicies queue_family_indicies = find_queue_families(device);

        bool extensions_supported = check_device_extension_support(device);

        bool swapchain_adequate = false;
        if (extensions_supported) {
            SwapChainSupportDetails swapchain_support = query_swapchain_support(device);
            swapchain_adequate = !swapchain_support.surface_formats.empty() && !swapchain_support.surface_present_modes.empty();
        }

        return queue_family_indicies.is_complete() && extensions_supported && swapchain_adequate;
    }


    void populate_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
        create_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug_callback,

        };

    }

    void setup_debug_messenger() {
        if (!DEBUG) return;

        VkDebugUtilsMessengerCreateInfoEXT create_info;
        populate_debug_messenger(create_info);

        if (create_debug_utils_messenger_ext(m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to setup debug messenger");
        }

    }

    bool check_device_extension_support(VkPhysicalDevice device) {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
        for (const auto& extension : available_extensions) {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    bool check_validation_layer_support() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        std::set<std::string> required_layers(validation_layers.begin(), validation_layers.end());
        for (const auto& layer : available_layers) {
            required_layers.erase(layer.layerName);
        }

        return required_layers.empty();
    }

    SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilites);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);

        if (format_count != 0) {
            details.surface_formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, details.surface_formats.data());
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);

        if (present_mode_count != 0) {
            details.surface_present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, details.surface_present_modes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
        for (const auto& format : available_formats) {
            if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return available_formats[0];
    }

    VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {
        for (const auto& present_mode : available_present_modes) {
            if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return present_mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilites) {
        if (capabilites.currentExtent.width !=  std::numeric_limits<uint32_t>::max()) {
            return capabilites.currentExtent;
        } 

        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        VkExtent2D actual_extent = {
            .width = (uint32_t)width,
            .height = (uint32_t)height
        };

        actual_extent.width = std::clamp(actual_extent.width, capabilites.minImageExtent.width, capabilites.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilites.minImageExtent.height, capabilites.maxImageExtent.height);

        return actual_extent;
    }

    void main_loop() {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

        if (DEBUG) {
            destroy_debug_utils_messenger_ext(m_instance, m_debug_messenger, nullptr);
        }

        vkDestroyDevice(m_device, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
        std::cerr << "Validation Layer: " << callback_data->pMessage << "\n";

        return VK_FALSE;
    }


public:
    //HelloEngine();
    //~HelloEngine();

    void run() {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();

    }

};



int main() {

    /*uint32_t extension_count = 0;*/
    /*vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);*/
    /**/
    /*std::vector<VkExtensionProperties> extensions(extension_count);*/
    /*vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());*/
    /**/
    /*std::cout << "Vulkan supports " << extension_count << " extensions" << std::endl;*/
    /*for (const auto& extension : extensions) {*/
    /*    std::cout << extension.extensionName << "\n";*/
    /*}*/

    HelloEngine engine;

    try {
    engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return 0;
}

