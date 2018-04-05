#include "tagged.h"

int64_t get_null64()
{
    return 0x00000000ffffffff;
}

int32_t get_null32()
{
    return 0xffffffff;
}
bool is_null(int32_t index)
{
    return index == (int32_t)0xffffffff;
}

bool is_null(int64_t index)
{
    return (index & 0x00000000ffffffff) == 0x00000000ffffffff;
}

void set_null(int32_t &index)
{
    index = 0xffffffff;
}

void set_null(int64_t &index)
{
    index = 0x00000000ffffffff;
}

void set_null(std::atomic_int_fast64_t &index)
{
    index.store(0x00000000ffffffff);
}

int32_t get_tag(int64_t index)
{
    return index >> 32;
}

int32_t get_next_tag(int64_t index)
{
    return  get_tag(index) + 1;
}

int32_t get_ptr(int64_t index)
{
    return index;
}

int64_t combine_ptr_tag(int32_t index, int32_t tag)
{
    int64_t tmp = tag;
    tmp = tmp << 32;
    tmp |=  index;
    return tmp;
}


