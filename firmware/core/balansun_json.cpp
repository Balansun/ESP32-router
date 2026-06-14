#include "balansun_json.h"

#include "balansun_psram.h"

#include <esp_heap_caps.h>

void *BalansunJsonDoc::CacheAllocator::allocate(size_t size) {
  return balansun_cache_malloc(size);
}

void BalansunJsonDoc::CacheAllocator::deallocate(void *pointer) {
  balansun_cache_free(pointer);
}

void *BalansunJsonDoc::CacheAllocator::reallocate(void *pointer, size_t new_size) {
  if (pointer == nullptr) return allocate(new_size);
  if (new_size == 0) {
    deallocate(pointer);
    return nullptr;
  }
  if (balansun_psram_available()) {
    void *p = heap_caps_realloc(pointer, new_size, MALLOC_CAP_SPIRAM);
    if (p) return p;
  }
  return realloc(pointer, new_size);
}

BalansunJsonDoc::BalansunJsonDoc(size_t minCapacity) : doc(&alloc) {
  (void)minCapacity;
}

BalansunJsonDoc balansun_json_doc_alloc(size_t minCapacity) { return BalansunJsonDoc(minCapacity); }
