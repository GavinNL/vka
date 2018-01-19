#ifndef VKA_CLASSES_H
#define VKA_CLASSES_H

#define X_LIST \
X_MACRO( renderpass            )\
X_MACRO( command_pool          )\
X_MACRO( buffer                )\
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


namespace vka
{
#define X_MACRO(A) class A;

X_LIST

#undef X_MACRO
}
#endif

