#ifndef VKA_EXTENSIONS_H
#define VKA_EXTENSIONS_H

#pragma once

#include <vulkan/vulkan.hpp>

namespace vka
{

#define EXT_LIST \
XX(vkCreateDebugReportCallbackEXT) \
XX(vkDestroyDebugReportCallbackEXT) \


struct ExtensionDispatcher
{
#define XX(A) PFN_ ## A A;

    EXT_LIST
    #undef XX

    //PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
};

extern ExtensionDispatcher ExtDispatcher;

}

#endif
