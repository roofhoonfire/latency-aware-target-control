#ifndef __ADYRhxAkkY3hec1WWVwx8_h__
#define __ADYRhxAkkY3hec1WWVwx8_h__

/* Include files */
#include "simstruc.h"
#include "rtwtypes.h"
#include "multiword_types.h"
#include "slexec_vm_zc_functions.h"
#include "slexec_vm_simstruct_bridge.h"
#include "sl_sfcn_cov/sl_sfcn_cov_bridge.h"

/* Headers From Include Tags */

/* Type Definitions */
#ifndef typedef_InstanceStruct_ADYRhxAkkY3hec1WWVwx8
#define typedef_InstanceStruct_ADYRhxAkkY3hec1WWVwx8

typedef struct {
  SimStruct *S;
  PyObject *namespaceDict;
  PyGILState_STATE GIL;
  void *emlrtRootTLSGlobal;
  boolean_T *u0;
  real_T *u1;
  real_T *u2;
  real_T *u3;
  real_T *u4;
  real_T *u5;
  void **sock;
} InstanceStruct_ADYRhxAkkY3hec1WWVwx8;

#endif                                 /* typedef_InstanceStruct_ADYRhxAkkY3hec1WWVwx8 */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

/* Function Definitions */
extern void method_dispatcher_ADYRhxAkkY3hec1WWVwx8(SimStruct *S, int_T method,
  void* data);

#endif
