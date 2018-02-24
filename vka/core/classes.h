#ifndef VKA_CLASSES_H
#define VKA_CLASSES_H

#define X_LIST \
X_MACRO( screen                )\
X_MACRO( renderpass            )\
X_MACRO( command_pool          )\
X_MACRO( buffer                )\
X_MACRO( managed_buffer        )\
X_MACRO( buffer_pool           )\
X_MACRO( framebuffer           )\
X_MACRO( shader                )\
X_MACRO( pipeline              )\
X_MACRO( semaphore             )\
X_MACRO( texture               )\
X_MACRO( texture2d             )\
X_MACRO( texture2darray        )\
X_MACRO( descriptor_pool       )\
X_MACRO( descriptor_set_layout )\
X_MACRO( descriptor_set        )\
X_MACRO( render_target         )\
X_MACRO( offscreen_target      )\




namespace vka
{
#define X_MACRO(A) class A;

X_LIST

#undef X_MACRO
}
#endif

