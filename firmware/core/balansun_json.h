#pragma once

#include <ArduinoJson.h>

#include <cstddef>

/*
 * balansun_json.h — ArduinoJson 7 helpers with PSRAM-backed pools for large docs.
 *
 * Large transient documents (config PUT, backup GET, fleet export):
 *   BalansunJsonDoc doc_pool = balansun_json_doc_alloc(kPutBodyMax);
 *   JsonDocument &doc = doc_pool;
 *
 * Small responses (errors, acks): stack-scoped default JsonDocument on internal heap:
 *   JsonDocument doc;
 *   doc["ok"] = true;
 *
 * AJ7 removed StaticJsonDocument / DynamicJsonDocument; capacity grows elastically.
 * minCapacity is a hint only (documents may shrink via shrinkToFit() after build).
 */

struct BalansunJsonDoc {
  struct CacheAllocator : ArduinoJson::Allocator {
    void *allocate(size_t size) override;
    void deallocate(void *pointer) override;
    void *reallocate(void *pointer, size_t new_size) override;
  };

  CacheAllocator alloc;
  JsonDocument doc;

  explicit BalansunJsonDoc(size_t minCapacity = 0);

  operator JsonDocument &() { return doc; }
  operator const JsonDocument &() const { return doc; }
};

BalansunJsonDoc balansun_json_doc_alloc(size_t minCapacity);
