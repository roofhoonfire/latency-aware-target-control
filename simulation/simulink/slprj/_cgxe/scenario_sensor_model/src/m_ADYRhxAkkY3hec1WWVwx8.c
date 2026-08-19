/* Include files */

#include "modelInterface.h"
#include "m_ADYRhxAkkY3hec1WWVwx8.h"
#include "mwstringutil.h"

/* Headers From Include Tags */

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */
static void cgxe_mdl_start(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance);
static void cgxe_mdl_initialize(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance);
static void cgxe_mdl_outputs(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance);
static void cgxe_mdl_update(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance);
static void cgxe_mdl_derivative(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance);
static void cgxe_mdl_enable(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance);
static void cgxe_mdl_disable(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance);
static void cgxe_mdl_terminate(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance);
static PyObject *getPyNamespaceDict(void);
static void assignToPyDict(PyObject *dict, char_T *key, PyObject *val);
static void execPyScript(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance,
  char_T *script, PyObject *ns);
static void CheckPythonError(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *pyObjsToRelease[], int32_T numObjToRelease);
static void b_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val);
static void c_assignToPyDict(PyObject *dict, char_T *key, boolean_T val);
static void d_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val);
static void e_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val);
static void f_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val);
static void g_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val);
static void b_execPyScript(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance,
  char_T *script, PyObject *ns);
static int32_T deleteDictItem(PyObject *dict, char_T *key);
static int32_T b_deleteDictItem(PyObject *dict, char_T *key);
static int32_T c_deleteDictItem(PyObject *dict, char_T *key);
static int32_T d_deleteDictItem(PyObject *dict, char_T *key);
static int32_T e_deleteDictItem(PyObject *dict, char_T *key);
static int32_T f_deleteDictItem(PyObject *dict, char_T *key);
static void c_execPyScript(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance,
  char_T *script, PyObject *ns);
static void init_simulink_io_address(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance);

/* Function Definitions */
static void cgxe_mdl_start(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance)
{
  init_simulink_io_address(moduleInstance);
  cgxertSetSimStateCompliance(moduleInstance->S, 4);
}

static void cgxe_mdl_initialize(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance)
{
  cgxertInitMLPythonIFace();
  moduleInstance->GIL = PyGILState_Ensure();
  moduleInstance->namespaceDict = getPyNamespaceDict();
  assignToPyDict(moduleInstance->namespaceDict, "sock", Py_BuildValue(""));
  execPyScript(moduleInstance,
               "import socket\n\nsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)",
               moduleInstance->namespaceDict);
  PyGILState_Release(moduleInstance->GIL);
}

static void cgxe_mdl_outputs(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance)
{
  moduleInstance->GIL = PyGILState_Ensure();
  b_assignToPyDict(moduleInstance, moduleInstance->namespaceDict, "t",
                   *moduleInstance->u1);
  c_assignToPyDict(moduleInstance->namespaceDict, "valid", *moduleInstance->u0);
  d_assignToPyDict(moduleInstance, moduleInstance->namespaceDict, "vx",
                   *moduleInstance->u4);
  e_assignToPyDict(moduleInstance, moduleInstance->namespaceDict, "vy",
                   *moduleInstance->u5);
  f_assignToPyDict(moduleInstance, moduleInstance->namespaceDict, "x",
                   *moduleInstance->u2);
  g_assignToPyDict(moduleInstance, moduleInstance->namespaceDict, "y",
                   *moduleInstance->u3);
  b_execPyScript(moduleInstance,
                 "import struct\n\nif bool(valid):\n    payload = struct.pack(\n        \"<5d\",\n        float(t),\n        float(x),\n        fl"
                 "oat(y),\n        float(vx),\n        float(vy)\n    )\n\n    sock.sendto(payload, (\"127.0.0.1\", 51001))",
                 moduleInstance->namespaceDict);
  PyGILState_Release(moduleInstance->GIL);
}

static void cgxe_mdl_update(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance)
{
  (void)moduleInstance;
}

static void cgxe_mdl_derivative(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance)
{
  (void)moduleInstance;
}

static void cgxe_mdl_enable(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance)
{
  (void)moduleInstance;
}

static void cgxe_mdl_disable(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance)
{
  (void)moduleInstance;
}

static void cgxe_mdl_terminate(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance)
{
  moduleInstance->GIL = PyGILState_Ensure();
  deleteDictItem(moduleInstance->namespaceDict, "t");
  b_deleteDictItem(moduleInstance->namespaceDict, "valid");
  c_deleteDictItem(moduleInstance->namespaceDict, "vx");
  d_deleteDictItem(moduleInstance->namespaceDict, "vy");
  e_deleteDictItem(moduleInstance->namespaceDict, "x");
  f_deleteDictItem(moduleInstance->namespaceDict, "y");
  c_execPyScript(moduleInstance, "sock.close()", moduleInstance->namespaceDict);
  Py_DecRef(moduleInstance->namespaceDict);
  PyGILState_Release(moduleInstance->GIL);
}

static PyObject *getPyNamespaceDict(void)
{
  return PyDict_Copy(PyModule_GetDict(PyImport_AddModule("__main__")));
}

static void assignToPyDict(PyObject *dict, char_T *key, PyObject *val)
{
  if (dict != NULL) {
    PyDict_SetItemString(dict, key, val);
    Py_DecRef(val);
  }
}

static void execPyScript(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance,
  char_T *script, PyObject *ns)
{
  PyObject *pyObjArray[2];
  PyObject *codeObject;
  PyObject *modtype = NULL;
  PyObject *originalNamespace;
  PyObject *p;
  PyObject *types_mod;
  PyObject *unusedEvalResult;
  Py_ssize_t i;
  Py_ssize_t numKeysInModifiedNs;
  int32_T item;
  if (ns != NULL) {
    codeObject = Py_CompileString(script, "Python Code Block", 257);
    CheckPythonError(moduleInstance, NULL, 0);
    originalNamespace = PyDict_Copy(ns);
    unusedEvalResult = PyEval_EvalCode(codeObject, ns, ns);
    pyObjArray[0U] = codeObject;
    pyObjArray[1U] = unusedEvalResult;
    CheckPythonError(moduleInstance, pyObjArray, 2);
    Py_DecRef(codeObject);
    if (unusedEvalResult != NULL) {
      Py_DecRef(unusedEvalResult);
    }

    codeObject = PyDict_Keys(ns);
    numKeysInModifiedNs = PyList_Size(codeObject);
    for (i = 0; i < numKeysInModifiedNs; i++) {
      unusedEvalResult = PySequence_GetItem(codeObject, i);
      CheckPythonError(moduleInstance, NULL, 0);
      if (PyDict_Contains(originalNamespace, unusedEvalResult) == 0) {
        p = PyDict_GetItem(ns, unusedEvalResult);
        item = -1;
        types_mod = PyImport_ImportModule("types");
        if (types_mod != NULL) {
          modtype = PyObject_GetAttrString(types_mod, "ModuleType");
          if (modtype != NULL) {
            item = PyObject_IsInstance(p, modtype);
          }
        }

        if (modtype != NULL) {
          Py_DecRef(modtype);
        }

        if (types_mod != NULL) {
          Py_DecRef(types_mod);
        }

        PyErr_Clear();
        if (!item) {
          PyDict_DelItem(ns, unusedEvalResult);
        }
      }

      Py_DecRef(unusedEvalResult);
    }

    Py_DecRef(codeObject);
    Py_DecRef(originalNamespace);
  }
}

static void CheckPythonError(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *pyObjsToRelease[], int32_T numObjToRelease)
{
  PyObject *pMsg;
  PyObject *pTraceback = NULL;
  PyObject *pType = NULL;
  PyObject *pValue = NULL;
  PyObject *sep = NULL;
  PyObject *tracebackList = NULL;
  PyObject *tracebackModule = NULL;
  int32_T i;
  int32_T idx;
  char_T *cMsg;
  i = suStringStackSize();
  PyErr_Fetch(&pType, &pValue, &pTraceback);
  PyErr_NormalizeException(&pType, &pValue, &pTraceback);
  if (pType != NULL) {
    if (pTraceback != NULL) {
      tracebackModule = PyImport_ImportModule("traceback");
      tracebackList = PyObject_CallMethod(tracebackModule, "format_exception",
        "OOO", pType, pValue, pTraceback);
      sep = PyUnicode_FromString("");
      pMsg = PyUnicode_Join(sep, tracebackList);
    } else if (pValue != NULL) {
      pMsg = PyObject_Str(pValue);
    } else {
      pMsg = PyObject_Str(pType);
    }

    cMsg = (char_T *)PyUnicode_AsUTF8(pMsg);
    if (cMsg == NULL) {
      cMsg =
        "Simulink encountered an error when converting a python error message to UTF-8";
      PyErr_Clear();
    } else {
      cMsg = suToCStr(suAddStackString(cMsg));
    }

    if (sep != NULL) {
      Py_DecRef(sep);
    }

    if (tracebackList != NULL) {
      Py_DecRef(tracebackList);
    }

    if (tracebackModule != NULL) {
      Py_DecRef(tracebackModule);
    }

    if (pMsg != NULL) {
      Py_DecRef(pMsg);
    }

    pMsg = pType;
    if (pMsg != NULL) {
      Py_DecRef(pMsg);
    }

    pMsg = pValue;
    if (pMsg != NULL) {
      Py_DecRef(pMsg);
    }

    pMsg = pTraceback;
    if (pMsg != NULL) {
      Py_DecRef(pMsg);
    }

    for (idx = 0; idx < numObjToRelease; idx++) {
      pMsg = pyObjsToRelease[idx];
      if (pMsg != NULL) {
        Py_DecRef(pMsg);
      }
    }

    PyGILState_Release(moduleInstance->GIL);
    cgxertReportError(moduleInstance->S, -1, -1,
                      "Simulink:CustomCode:PythonRuntimeError", 3, 1, strlen
                      (cMsg), cMsg);
  }

  suMoveReturnedStringsToTopOfCallerStack(i, 0);
}

static void b_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val)
{
  PyObject *pyObj;
  if (dict != NULL) {
    pyObj = PyFloat_FromDouble(val);
    CheckPythonError(moduleInstance, NULL, 0);
    PyDict_SetItemString(dict, key, pyObj);
    Py_DecRef(pyObj);
  }
}

static void c_assignToPyDict(PyObject *dict, char_T *key, boolean_T val)
{
  PyObject *pyObj;
  if (dict != NULL) {
    pyObj = PyBool_FromLong((int32_T)val);
    PyDict_SetItemString(dict, key, pyObj);
    Py_DecRef(pyObj);
  }
}

static void d_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val)
{
  PyObject *pyObj;
  if (dict != NULL) {
    pyObj = PyFloat_FromDouble(val);
    CheckPythonError(moduleInstance, NULL, 0);
    PyDict_SetItemString(dict, key, pyObj);
    Py_DecRef(pyObj);
  }
}

static void e_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val)
{
  PyObject *pyObj;
  if (dict != NULL) {
    pyObj = PyFloat_FromDouble(val);
    CheckPythonError(moduleInstance, NULL, 0);
    PyDict_SetItemString(dict, key, pyObj);
    Py_DecRef(pyObj);
  }
}

static void f_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val)
{
  PyObject *pyObj;
  if (dict != NULL) {
    pyObj = PyFloat_FromDouble(val);
    CheckPythonError(moduleInstance, NULL, 0);
    PyDict_SetItemString(dict, key, pyObj);
    Py_DecRef(pyObj);
  }
}

static void g_assignToPyDict(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance, PyObject *dict, char_T *key, real_T val)
{
  PyObject *pyObj;
  if (dict != NULL) {
    pyObj = PyFloat_FromDouble(val);
    CheckPythonError(moduleInstance, NULL, 0);
    PyDict_SetItemString(dict, key, pyObj);
    Py_DecRef(pyObj);
  }
}

static void b_execPyScript(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance,
  char_T *script, PyObject *ns)
{
  PyObject *pyObjArray[2];
  PyObject *codeObject;
  PyObject *modtype = NULL;
  PyObject *originalNamespace;
  PyObject *p;
  PyObject *types_mod;
  PyObject *unusedEvalResult;
  Py_ssize_t i;
  Py_ssize_t numKeysInModifiedNs;
  int32_T item;
  if (ns != NULL) {
    codeObject = Py_CompileString(script, "Python Code Block", 257);
    CheckPythonError(moduleInstance, NULL, 0);
    originalNamespace = PyDict_Copy(ns);
    unusedEvalResult = PyEval_EvalCode(codeObject, ns, ns);
    pyObjArray[0U] = codeObject;
    pyObjArray[1U] = unusedEvalResult;
    CheckPythonError(moduleInstance, pyObjArray, 2);
    Py_DecRef(codeObject);
    if (unusedEvalResult != NULL) {
      Py_DecRef(unusedEvalResult);
    }

    codeObject = PyDict_Keys(ns);
    numKeysInModifiedNs = PyList_Size(codeObject);
    for (i = 0; i < numKeysInModifiedNs; i++) {
      unusedEvalResult = PySequence_GetItem(codeObject, i);
      CheckPythonError(moduleInstance, NULL, 0);
      if (PyDict_Contains(originalNamespace, unusedEvalResult) == 0) {
        p = PyDict_GetItem(ns, unusedEvalResult);
        item = -1;
        types_mod = PyImport_ImportModule("types");
        if (types_mod != NULL) {
          modtype = PyObject_GetAttrString(types_mod, "ModuleType");
          if (modtype != NULL) {
            item = PyObject_IsInstance(p, modtype);
          }
        }

        if (modtype != NULL) {
          Py_DecRef(modtype);
        }

        if (types_mod != NULL) {
          Py_DecRef(types_mod);
        }

        PyErr_Clear();
        if (!item) {
          PyDict_DelItem(ns, unusedEvalResult);
        }
      }

      Py_DecRef(unusedEvalResult);
    }

    Py_DecRef(codeObject);
    Py_DecRef(originalNamespace);
  }
}

static int32_T deleteDictItem(PyObject *dict, char_T *key)
{
  if (dict != NULL) {
    PyDict_DelItemString(dict, key);
    PyErr_Clear();
  }

  return 0;
}

static int32_T b_deleteDictItem(PyObject *dict, char_T *key)
{
  if (dict != NULL) {
    PyDict_DelItemString(dict, key);
    PyErr_Clear();
  }

  return 0;
}

static int32_T c_deleteDictItem(PyObject *dict, char_T *key)
{
  if (dict != NULL) {
    PyDict_DelItemString(dict, key);
    PyErr_Clear();
  }

  return 0;
}

static int32_T d_deleteDictItem(PyObject *dict, char_T *key)
{
  if (dict != NULL) {
    PyDict_DelItemString(dict, key);
    PyErr_Clear();
  }

  return 0;
}

static int32_T e_deleteDictItem(PyObject *dict, char_T *key)
{
  if (dict != NULL) {
    PyDict_DelItemString(dict, key);
    PyErr_Clear();
  }

  return 0;
}

static int32_T f_deleteDictItem(PyObject *dict, char_T *key)
{
  if (dict != NULL) {
    PyDict_DelItemString(dict, key);
    PyErr_Clear();
  }

  return 0;
}

static void c_execPyScript(InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance,
  char_T *script, PyObject *ns)
{
  PyObject *pyObjArray[2];
  PyObject *codeObject;
  PyObject *originalNamespace;
  PyObject *unusedEvalResult;
  if (ns != NULL) {
    codeObject = Py_CompileString(script, "Python Code Block", 257);
    CheckPythonError(moduleInstance, NULL, 0);
    originalNamespace = PyDict_Copy(ns);
    unusedEvalResult = PyEval_EvalCode(codeObject, ns, ns);
    pyObjArray[0U] = codeObject;
    pyObjArray[1U] = unusedEvalResult;
    CheckPythonError(moduleInstance, pyObjArray, 2);
    Py_DecRef(codeObject);
    if (unusedEvalResult != NULL) {
      Py_DecRef(unusedEvalResult);
    }

    Py_DecRef(originalNamespace);
  }
}

static void init_simulink_io_address(InstanceStruct_ADYRhxAkkY3hec1WWVwx8
  *moduleInstance)
{
  moduleInstance->emlrtRootTLSGlobal = (void *)cgxertGetEMLRTCtx
    (moduleInstance->S);
  moduleInstance->u0 = (boolean_T *)cgxertGetInputPortSignal(moduleInstance->S,
    0);
  moduleInstance->u1 = (real_T *)cgxertGetInputPortSignal(moduleInstance->S, 1);
  moduleInstance->u2 = (real_T *)cgxertGetInputPortSignal(moduleInstance->S, 2);
  moduleInstance->u3 = (real_T *)cgxertGetInputPortSignal(moduleInstance->S, 3);
  moduleInstance->u4 = (real_T *)cgxertGetInputPortSignal(moduleInstance->S, 4);
  moduleInstance->u5 = (real_T *)cgxertGetInputPortSignal(moduleInstance->S, 5);
  moduleInstance->sock = (void **)cgxertGetDWork(moduleInstance->S, 0);
}

/* CGXE Glue Code */
static void mdlOutputs_ADYRhxAkkY3hec1WWVwx8(SimStruct *S, int_T tid)
{
  InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance =
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *)cgxertGetRuntimeInstance(S);
  cgxe_mdl_outputs(moduleInstance);
}

static void mdlInitialize_ADYRhxAkkY3hec1WWVwx8(SimStruct *S)
{
  InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance =
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *)cgxertGetRuntimeInstance(S);
  cgxe_mdl_initialize(moduleInstance);
}

static void mdlUpdate_ADYRhxAkkY3hec1WWVwx8(SimStruct *S, int_T tid)
{
  InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance =
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *)cgxertGetRuntimeInstance(S);
  cgxe_mdl_update(moduleInstance);
}

static void mdlDerivatives_ADYRhxAkkY3hec1WWVwx8(SimStruct *S)
{
  InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance =
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *)cgxertGetRuntimeInstance(S);
  cgxe_mdl_derivative(moduleInstance);
}

static void mdlTerminate_ADYRhxAkkY3hec1WWVwx8(SimStruct *S)
{
  InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance =
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *)cgxertGetRuntimeInstance(S);
  cgxe_mdl_terminate(moduleInstance);
  free((void *)moduleInstance);
}

static void mdlEnable_ADYRhxAkkY3hec1WWVwx8(SimStruct *S)
{
  InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance =
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *)cgxertGetRuntimeInstance(S);
  cgxe_mdl_enable(moduleInstance);
}

static void mdlDisable_ADYRhxAkkY3hec1WWVwx8(SimStruct *S)
{
  InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance =
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *)cgxertGetRuntimeInstance(S);
  cgxe_mdl_disable(moduleInstance);
}

static void mdlStart_ADYRhxAkkY3hec1WWVwx8(SimStruct *S)
{
  InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *moduleInstance =
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8 *)calloc(1, sizeof
    (InstanceStruct_ADYRhxAkkY3hec1WWVwx8));
  moduleInstance->S = S;
  cgxertSetRuntimeInstance(S, (void *)moduleInstance);
  ssSetmdlOutputs(S, mdlOutputs_ADYRhxAkkY3hec1WWVwx8);
  ssSetmdlInitializeConditions(S, mdlInitialize_ADYRhxAkkY3hec1WWVwx8);
  ssSetmdlUpdate(S, mdlUpdate_ADYRhxAkkY3hec1WWVwx8);
  ssSetmdlDerivatives(S, mdlDerivatives_ADYRhxAkkY3hec1WWVwx8);
  ssSetmdlTerminate(S, mdlTerminate_ADYRhxAkkY3hec1WWVwx8);
  ssSetmdlEnable(S, mdlEnable_ADYRhxAkkY3hec1WWVwx8);
  ssSetmdlDisable(S, mdlDisable_ADYRhxAkkY3hec1WWVwx8);
  cgxe_mdl_start(moduleInstance);

  {
    uint_T options = ssGetOptions(S);
    options |= SS_OPTION_RUNTIME_EXCEPTION_FREE_CODE;
    ssSetOptions(S, options);
  }
}

static void mdlProcessParameters_ADYRhxAkkY3hec1WWVwx8(SimStruct *S)
{
}

void method_dispatcher_ADYRhxAkkY3hec1WWVwx8(SimStruct *S, int_T method, void
  *data)
{
  switch (method) {
   case SS_CALL_MDL_START:
    mdlStart_ADYRhxAkkY3hec1WWVwx8(S);
    break;

   case SS_CALL_MDL_PROCESS_PARAMETERS:
    mdlProcessParameters_ADYRhxAkkY3hec1WWVwx8(S);
    break;

   default:
    /* Unhandled method */
    /*
       sf_mex_error_message("Stateflow Internal Error:\n"
       "Error calling method dispatcher for module: ADYRhxAkkY3hec1WWVwx8.\n"
       "Can't handle method %d.\n", method);
     */
    break;
  }
}

mxArray *cgxe_ADYRhxAkkY3hec1WWVwx8_BuildInfoUpdate(void)
{
  mxArray * mxBIArgs;
  mxArray * elem_1;
  mxArray * elem_2;
  mxArray * elem_3;
  double * pointer;
  mxBIArgs = mxCreateCellMatrix(1,3);
  elem_1 = mxCreateDoubleMatrix(0,0, mxREAL);
  pointer = mxGetPr(elem_1);
  mxSetCell(mxBIArgs,0,elem_1);
  elem_2 = mxCreateDoubleMatrix(0,0, mxREAL);
  pointer = mxGetPr(elem_2);
  mxSetCell(mxBIArgs,1,elem_2);
  elem_3 = mxCreateCellMatrix(1,0);
  mxSetCell(mxBIArgs,2,elem_3);
  return mxBIArgs;
}

mxArray *cgxe_ADYRhxAkkY3hec1WWVwx8_fallback_info(void)
{
  const char* fallbackInfoFields[] = { "fallbackType", "incompatiableSymbol" };

  mxArray* fallbackInfoStruct = mxCreateStructMatrix(1, 1, 2, fallbackInfoFields);
  mxArray* fallbackType = mxCreateString("incompatibleFunction");
  mxArray* incompatibleSymbol = mxCreateString("PyEval_EvalCode");
  mxSetFieldByNumber(fallbackInfoStruct, 0, 0, fallbackType);
  mxSetFieldByNumber(fallbackInfoStruct, 0, 1, incompatibleSymbol);
  return fallbackInfoStruct;
}

void cgxe_ADYRhxAkkY3hec1WWVwx8_get_all_custom_code_global_var_addr(void
  ** gVarAddrArray)
{
  /* This function is no-op if OOP or no custom code */
  (void)gVarAddrArray;
}
