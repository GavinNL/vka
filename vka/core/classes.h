#ifndef VKA_CLASSES_H
#define VKA_CLASSES_H

#define X_LIST  X_MACRO( semaphore             )


namespace vka
{
#define X_MACRO(A) class A;

X_LIST

#undef X_MACRO

class context;
}
#endif

