## Primatives

Create methods for generating primatives:
* cone
* cylinder
* torus
* axis







## New Buffer Idea

### vka::buffer_pool
* A child class of a vka::buffer, it is initialized to hold
  vertex|uniform|index data.
* Always Device Local
* Allocates vka::sub_buffers
* Should be large enough to allocate all the buffer's the application would need


### vka::sub_buffer
* Contains a unique vk::Buffer
* bound to the same memory as vka::memory_pool but with an offset
* destoying sub_buffer deallocates it from vka::memory_pool
* Normally would want a
* allocate's vka::buffer_object

### vka::buffer_object
* buffer_objects represent a specific block of memeory within a sub_buffer
* can be used for vertices or indices of a single mesh
* destroying the buffer_object does not "free" the actual memeory, it goes back
  into the memory bool of teh vka::sub_buffer


+------------------------------------------------------------------------------+
| buffer pool (allocated once)                                                 |
+------------------------------------------------------------------------------+
           | sub buffers allocated                   |
           V                                         V
+------------------------------------------------------------------------------+
| vertices /indices                                | uniform data              |
+------------------------------------------------------------------------------+
 |  buffer_objects allocated                         |
 V                                                   V
+------------------------------------------------------------------------------+
| obj1 |  obj2 | obj3 |                            | uniform data              |
+------------------------------------------------------------------------------+



## vka::command_buffer

Same as vk::CommandBuffer, but adds the following:
- bindSubBuffer( sub_buffer )
