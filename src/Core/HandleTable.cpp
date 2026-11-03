#include "HandleTable.h"

#include <atomic>
#include <stdexcept>

namespace CE::Core::Detail {
// Code Modification - Identity: Keep handles from destroyed tables invalid when a replacement table starts.
// Scope: process-local table identities. Performance: one atomic reservation per table. Toggle: none.
std::uint64_t allocateHandleDomain()
{
    static std::atomic<std::uint64_t> next{1};
    auto domain = next.load(std::memory_order_relaxed);
    do {
        if (domain == std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error("Handle table domain exhausted");
        }
    } while (!next.compare_exchange_weak(domain, domain + 1, std::memory_order_relaxed));
    return domain;
}
// End of Code Modification - Identity
}
