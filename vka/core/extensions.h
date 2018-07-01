#ifndef VKA_EXTENSIONS_H
#define VKA_EXTENSIONS_H

#pragma once

#include <vulkan/vulkan.hpp>

namespace vka
{

#define EXT_LIST \
XX(vkCreateDebugReportCallbackEXT, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) \
XX(vkDestroyDebugReportCallbackEXT, VK_EXT_DEBUG_REPORT_EXTENSION_NAME)\
XX(vkCmdPushDescriptorSetKHR, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)\

//XX(VK_KHR_push_descriptor)

struct ExtensionDispatcher
{
#define XX(A,B) PFN_ ## A A;

    EXT_LIST
    #undef XX


    void load_extensions( vk::Instance instance, std::vector<std::string> & extensions)
    {
        for(auto & e : extensions)
        {
#define XX(A,B) \
            if( e == B)\
            {\
                 A = (PFN_ ## A) instance.getProcAddr( #A );\
                 if( A == nullptr)\
                    throw std::runtime_error("Could not load extension: " + std::string( #A) );\
            }\

            EXT_LIST

#undef XX

        }
    }

};

extern ExtensionDispatcher ExtDispatcher;

}

#endif
