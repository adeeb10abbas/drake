#pragma once

#include <map>
#include <string>

#include "drake/lcm/drake_lcm_interface.h"
#include "drake/lcm/drake_lcm_params.h"
#include "drake/systems/framework/diagram_builder.h"
#include "drake/systems/lcm/lcm_buses.h"

namespace drake {
namespace systems {
namespace lcm {

/** Given LCM bus names and parameters, adds an LcmInterfaceSystem for each bus
within the given diagram builder, and returns an LcmBuses object that provides
access to the DrakeLcmInterface objects that were created.

Because the interfaces live within the builder (and so eventually, the diagram),
the diagram will pump the interfaces when it's used with a simulator. Refer to
the LcmInterfaceSystem documentation for details.

The interface pointers remain owned by the builder; the LcmBuses object merely
aliases into the builder (and then eventually, the diagram).

@param lcm_buses A map of {bus_name: params} for LCM transceivers, to be used
used by drivers, sensors, etc. */
LcmBuses ApplyLcmBusConfig(
    const std::map<std::string, drake::lcm::DrakeLcmParams>& lcm_buses,
    systems::DiagramBuilder<double>* builder);

/** (Advanced) Returns an LCM interface based on a convenient set of heuristics.

If the `forced_result` is non-null, then returns `forced_result` and does
nothing else.

Otherwise, if `lcm_buses` is null and `bus_name` is "default", then creates a
new DrakeLcm object owned by the `builder` and returns a pointer to it.

Otherwise, if `lcm_buses` is null, then throws an exception.

Otherwise, returns the `lcm_buses->Find(description_of_caller, bus_name)`
which might throw if there is so such `bus_name`.

The return value is an alias into memory owned elsewhere (typically by a
DiagramBuilder or a Diagram) and is never nullptr.

@param forced_result can be null
@param lcm_buses can be null
@param builder must not be null
*/
drake::lcm::DrakeLcmInterface* FindOrCreateLcmBus(
    drake::lcm::DrakeLcmInterface* forced_result, const LcmBuses* lcm_buses,
    DiagramBuilder<double>* builder, std::string_view description_of_caller,
    const std::string& bus_name);

}  // namespace lcm
}  // namespace systems
}  // namespace drake
