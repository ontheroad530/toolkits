#ifndef TAGGED_H
#define TAGGED_H

#include <cinttypes>
#include <atomic>

int32_t get_null32();
int64_t get_null64();

bool is_null(int32_t index);
bool is_null(int64_t index);

void set_null(int32_t& index);
void set_null(int64_t& index);
void set_null(std::atomic_int_fast64_t& index);

int32_t get_tag(int64_t index);
int32_t get_next_tag(int64_t index);
int32_t get_ptr(int64_t index);
int64_t combine_ptr_tag(int32_t index, int32_t tag);

#endif // TAGGED_H
