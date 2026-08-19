#ifndef __kGZtuWAeEpb83IAukkgB6E_h__
#define __kGZtuWAeEpb83IAukkgB6E_h__

/* Include files */
#include "simstruc.h"
#include "rtwtypes.h"
#include "multiword_types.h"
#include "slexec_vm_zc_functions.h"
#include "slexec_vm_simstruct_bridge.h"
#include "sl_sfcn_cov/sl_sfcn_cov_bridge.h"

/* Headers From Include Tags */

/* Type Definitions */
#ifndef typedef_InstanceStruct_kGZtuWAeEpb83IAukkgB6E
#define typedef_InstanceStruct_kGZtuWAeEpb83IAukkgB6E

typedef struct {
  SimStruct *S;
  PyObject *namespaceDict;
  PyGILState_STATE GIL;
  void *emlrtRootTLSGlobal;
  real_T *u0;
  real_T *u1;
  real_T *u2;
  real_T *u3;
  real_T *u4;
  void **sock;
} InstanceStruct_kGZtuWAeEpb83IAukkgB6E;

#endif                                 /* typedef_InstanceStruct_kGZtuWAeEpb83IAukkgB6E */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

/* Function Definitions */
extern void method_dispatcher_kGZtuWAeEpb83IAukkgB6E(SimStruct *S, int_T method,
  void* data);

#endif
