#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <stdexcept>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const bool DEBUG = true;

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
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

class HelloEngine {

private:

    GLFWwindow* m_window;
    VkInstance m_instance;

    VkDebugUtilsMessengerEXT m_debug_messenger;

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

    void init_vulkan() {
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

        setup_debug_messenger();
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

    bool check_validation_layer_support() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char* layer_name : validation_layers) {
            bool layer_found = false;

            for (const auto& layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                return false;
            }
        }

        return true;
    }

    void main_loop() {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        if (DEBUG) {
            destroy_debug_utils_messenger_ext(m_instance, m_debug_messenger, nullptr);
        }

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

