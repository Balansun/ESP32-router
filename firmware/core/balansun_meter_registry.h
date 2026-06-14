#pragma once

#include "balansun_source_types.h"

#include <cstddef>

class IMeterDriver;

size_t balansun_meter_instance_count();
IMeterDriver *balansun_meter_instance_at(size_t i);
IMeterDriver *balansun_meter_driver_for_id(SourceId id);
