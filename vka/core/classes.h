#ifndef VKA_CLASSES_H
#define VKA_CLASSES_H

#define X_LIST \
X_MACRO( command_pool          )\
X_MACRO( semaphore             )\
X_MACRO( descriptor_set_layout )\
X_MACRO( descriptor_set        )\



namespace vka
{
#define X_MACRO(A) class A;

X_LIST

#undef X_MACRO

class context;
}
#endif

