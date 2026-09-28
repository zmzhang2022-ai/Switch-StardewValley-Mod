#pragma once

#include "common.hpp"

#define EXL_MODULE_NAME "automate_lite"

#define EXL_DEBUG
#ifndef EXL_USE_FAKEHEAP
#define EXL_USE_FAKEHEAP
#endif

/*
#define EXL_SUPPORTS_REBOOTPAYLOAD
*/

namespace exl::setting {
    /* How large the fake .bss heap will be. */
    constexpr size_t HeapSize = 0x5000;

    /* 200 bytes/trampoline: 28 normal hooks + 1 trace hook, with headroom.
       The old 4 KiB pool only held 20; v11 aborted at startup hook #22.
       tools/check_hook_budget.py validates this before every build. */
    constexpr size_t JitSize = 0x2000;

    /* How large the area will be inline hook pool. */
    constexpr size_t InlinePoolSize = 0x1000;

    /* How large the formatting buffer should be for logging. The buffer will be on the stack. */
    constexpr size_t LogBufferSize = 512;

    /* Sanity checks. */
    static_assert(ALIGN_UP(JitSize, PAGE_SIZE) == JitSize, "");
    static_assert(ALIGN_UP(InlinePoolSize, PAGE_SIZE) == InlinePoolSize, "");
}
