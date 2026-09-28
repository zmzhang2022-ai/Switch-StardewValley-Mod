#include "lib/hook/nx64/pool_layout.hpp"
using namespace exl::hook::nx64;
static_assert(pool::Capacity(0x1000)==20);
static_assert(pool::Contains(0x1000,19));
static_assert(!pool::Contains(0x1000,20), "first out-of-bounds slot must be rejected");
static_assert(!pool::Contains(0x1000,21), "v11 crash allocation must be rejected");
static_assert(pool::Capacity(0x2000)==40);
static_assert(pool::Contains(0x2000,28), "all normal and trace hooks fit");
static_assert(pool::Contains(0x2000,39));
static_assert(!pool::Contains(0x2000,40));
static_assert(!pool::Contains(0,0));
static_assert(!pool::Contains(199,0));
static_assert(pool::Contains(200,0) && !pool::Contains(200,1));
