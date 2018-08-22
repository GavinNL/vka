#ifndef VKA_CONTEXT_CHILD
#define VKA_CONTEXT_CHILD

#include <vulkan/vulkan.hpp>

namespace vka
{

class context;

class context_child
{
  public:
    context_child(context * parent) : m_parent(parent){}

    vk::Device          get_device();
    vk::PhysicalDevice  get_physical_device();
    context*            get_parent_context();

  private:
    context  *m_parent = nullptr;

};

}
#endif
