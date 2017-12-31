#ifndef VKA_DELETER_H
#define VKA_DELETER_H


namespace vka
{

template<typename T>
class deleter
{
public:
    void operator()(T * obj)
    {
        delete obj;
    }
};

}

#endif
