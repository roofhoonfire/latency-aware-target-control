/* Include files */

#include "scenario_sensor_model_cgxe.h"
#include "m_ADYRhxAkkY3hec1WWVwx8.h"

unsigned int cgxe_scenario_sensor_model_method_dispatcher(SimStruct* S, int_T
  method, void* data)
{
  if (ssGetChecksum0(S) == 1054739644 &&
      ssGetChecksum1(S) == 1374286826 &&
      ssGetChecksum2(S) == 1230871422 &&
      ssGetChecksum3(S) == 1494390627) {
    method_dispatcher_ADYRhxAkkY3hec1WWVwx8(S, method, data);
    return 1;
  }

  return 0;
}
