#include "balansun_api_ready_logic.h"

bool balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup group, bool signed_net_live,
                                      bool import_live, bool export_live, bool snapshot_live) {
  switch (group) {
    case PmqttBindingsPowerGroup::Signed:
      return signed_net_live;
    case PmqttBindingsPowerGroup::Split:
      return import_live && export_live;
    case PmqttBindingsPowerGroup::Snapshot:
      return snapshot_live;
    default:
      return false;
  }
}
