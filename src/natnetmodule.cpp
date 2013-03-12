#include <Python.h>
#include <cstdio>
#include <NatNetTypes.h>
#include <NatNetClient.h>

PyObject *FrameOfMocapData;
PyObject *RigidBodyData;
PyObject *MarkerSetData;
PyObject *SkeletonData;
PyObject *MarkerSetDescription;
PyObject *RigidBodyDescription;
PyObject *SkeletonDescription;

static PyObject *parseRigidBodies(sRigidBodyData *rigidBodyData, int nRigidBodies) {
    PyObject *rigidBodies = PyList_New(0);
    for (int i = 0; i < nRigidBodies; i++) {
        sRigidBodyData body = rigidBodyData[i];
        PyObject *rigidBodyArgs = Py_BuildValue("(ifffffff)", body.ID, body.x, body.y, body.z, body.qx, body.qy, body.qz, body.qw);
        PyObject *dict = PyDict_New();
        PyObject *rigidBody = PyInstance_New(RigidBodyData, rigidBodyArgs, dict);
        Py_DECREF(dict);
        Py_DECREF(rigidBodyArgs);
        PyList_Append(rigidBodies, rigidBody);
        Py_DECREF(rigidBody);
    }
    return rigidBodies;
}

static PyObject *parseMarkerSets(sMarkerSetData * markerSetData, int nMarkerSets) {
    PyObject *markerSets = PyList_New(0);
    for (int i = 0; i < nMarkerSets; i++) {
      sMarkerSetData markerSet = markerSetData[i];
      PyObject * markersData = PyTuple_New(markerSet.nMarkers);
      for (int ind = 0; ind < markerSet.nMarkers; ind++) {
        PyTuple_SetItem(markersData, ind, Py_BuildValue("(fff)",
              markerSet.Markers[ind][0],
              markerSet.Markers[ind][1],
              markerSet.Markers[ind][2]));
      }
      PyObject *markerSetDataArgs = Py_BuildValue("(sO)", markerSet.szName, markersData);
      PyObject *dict = PyDict_New();
      PyObject *pyMarkerSet = PyInstance_New(MarkerSetData, markerSetDataArgs, dict);
      PyList_Append(markerSets, pyMarkerSet);
      Py_DECREF(dict);
      Py_DECREF(markerSetDataArgs);
      Py_DECREF(markersData);
      Py_DECREF(pyMarkerSet);
    }
    return markerSets;
}

static PyObject *parseSkeletons(sSkeletonData *skeletonData, int nSkeletons) {
    PyObject *skeletons = PyList_New(0);
    for (int i = 0; i < nSkeletons; i++) {
        sSkeletonData skeleton = skeletonData[i];
        PyObject *rigidBodies = parseRigidBodies(skeleton.RigidBodyData, skeleton.nRigidBodies);
        PyObject *skeletonArgs = Py_BuildValue("(iO)", skeleton.skeletonID, rigidBodies);
        PyObject *dict = PyDict_New();
        PyObject *pySkeleton = PyInstance_New(SkeletonData, skeletonArgs, dict);
        Py_DECREF(dict);
        Py_DECREF(skeletonArgs);
        PyList_Append(skeletons, pySkeleton);
        Py_DECREF(rigidBodies);
        Py_DECREF(pySkeleton);
    }
    return skeletons;
}

void DataHandler(sFrameOfMocapData *data, void *pUserData) {
    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject *callback = (PyObject *)pUserData;
    if (!callback) return;

    PyObject *rigidBodies = parseRigidBodies(data->RigidBodies, data->nRigidBodies);
    PyObject *markerSets = parseMarkerSets(data->MocapData, data->nMarkerSets);
    PyObject *skeletons = parseSkeletons(data->Skeletons, data->nSkeletons);
    PyObject *mocapDataArgs = Py_BuildValue("(iOOOf)", data->iFrame, rigidBodies, markerSets, skeletons, data->fLatency);
    PyObject *dict = PyDict_New();
    PyObject *mocapInst = PyInstance_New(FrameOfMocapData, mocapDataArgs, dict);
    Py_DECREF(dict);
    PyObject *arglist = Py_BuildValue("(O)", mocapInst);
    PyEval_CallObject(callback, arglist);
    Py_DECREF(arglist);
    Py_DECREF(mocapInst);
    Py_DECREF(mocapDataArgs);
    Py_DECREF(rigidBodies);
    Py_DECREF(skeletons);

    PyGILState_Release(gstate);
}

PyObject *cnatnet_constructor(PyObject *self, PyObject *args) {
    int iType;
    if (!PyArg_ParseTuple(args, "i", &iType))
        return NULL;
    NatNetClient *inst = new NatNetClient(iType);
    PyObject *ret = PyCObject_FromVoidPtr(inst, NULL);
    return ret;
}

PyObject *cnatnet_initialize(PyObject *self, PyObject *args) {
    PyObject *pyInst;
    char *myIpAddress, *serverIpAddress;
    int hostCommandPort, hostDataPort;
    PyArg_ParseTuple(args, "Oss|ii", &pyInst, &myIpAddress, &serverIpAddress, &hostCommandPort, &hostDataPort);
    NatNetClient *inst = (NatNetClient *)PyCObject_AsVoidPtr(pyInst);
    char szMyIPAddress[128] = {0};
    char szServerIPAddress[128] = {0};
    strncpy(szMyIPAddress, myIpAddress, 127);
    strncpy(szServerIPAddress, serverIpAddress, 127);
    int ret;
    Py_BEGIN_ALLOW_THREADS
    switch (PyTuple_Size(args)) {
        case 3:
            ret = inst->Initialize(szMyIPAddress, szServerIPAddress);
            break;
        case 4:
            ret = inst->Initialize(szMyIPAddress, szServerIPAddress, hostCommandPort);
            break;
        case 5:
            ret = inst->Initialize(szMyIPAddress, szServerIPAddress, hostCommandPort, hostDataPort);
            break;
        default:
            printf("fail, %d args\n", PyTuple_Size(args));
            ret = 1;
    }
    Py_END_ALLOW_THREADS
    return PyInt_FromLong(ret);
}

PyObject *cnatnet_uninitialize(PyObject *self, PyObject *args) {
    PyObject *pyInst;
    PyArg_ParseTuple(args, "O", &pyInst);
    NatNetClient *inst = (NatNetClient *)PyCObject_AsVoidPtr(pyInst);
    int ret = inst->Uninitialize();
    return PyInt_FromLong(ret);
}

PyObject *cnatnet_natNetVersion(PyObject *self, PyObject *args) {
    PyObject *pyInst;
    PyArg_ParseTuple(args, "O", &pyInst);
    NatNetClient *inst = (NatNetClient *)PyCObject_AsVoidPtr(pyInst);
    unsigned char ver[4];
    inst->NatNetVersion(ver);
    return PyTuple_Pack(4, PyInt_FromSize_t(ver[0]), PyInt_FromSize_t(ver[1]), PyInt_FromSize_t(ver[2]), PyInt_FromSize_t(ver[3]));
}

PyObject *cnatnet_getDataDescriptions(PyObject *self, PyObject *args) {
    PyObject *pyInst;
    PyArg_ParseTuple(args, "O", &pyInst);
    NatNetClient *inst = (NatNetClient *)PyCObject_AsVoidPtr(pyInst);
    sDataDescriptions *dataDescriptions;
    inst->GetDataDescriptions(&dataDescriptions);
    PyObject *retList = PyList_New(0);
    for (int i = 0; i < dataDescriptions->nDataDescriptions; i++) {
        sDataDescription *dataDescription = &dataDescriptions->arrDataDescriptions[i];
        PyObject *descObj;
        PyObject *args;
        PyObject *dict = PyDict_New();
        switch (dataDescription->type) {
            case Descriptor_RigidBody: {
                sRigidBodyDescription *rigidBodyDesc = dataDescription->Data.RigidBodyDescription;
                args = Py_BuildValue("(siifff)", rigidBodyDesc->szName, rigidBodyDesc->ID, rigidBodyDesc->parentID, rigidBodyDesc->offsetx, rigidBodyDesc->offsety, rigidBodyDesc->offsetz);
                descObj = PyInstance_New(RigidBodyDescription, args, dict);
                }
                break;
            case Descriptor_MarkerSet: {
                sMarkerSetDescription *markerDesc = dataDescription->Data.MarkerSetDescription;
                markerDesc->nMarkers;
                markerDesc->szName;
                markerDesc->szMarkerNames; //Char  ** names
                PyObject *names = PyTuple_New(markerDesc->nMarkers);
                for (int ind = 0; ind < markerDesc->nMarkers; ++ind) {
                  PyTuple_SetItem(names, ind, Py_BuildValue("s", markerDesc->szMarkerNames[ind]));
                }
                args = Py_BuildValue("(sO)", markerDesc->szName, names);
                descObj = PyInstance_New(MarkerSetDescription, args, dict);
                Py_DECREF(names); //needed?
                }
                break;
            case Descriptor_Skeleton:
                // not yet supported
                continue;
        }
        Py_DECREF(dict);
        Py_DECREF(args);
        PyList_Append(retList, descObj);
    }
    return retList;
}

PyObject *cnatnet_getServerDescription(PyObject *self, PyObject *args) {
    Py_RETURN_NONE;
}

PyObject *cnatnet_setMessageCallback(PyObject *self, PyObject *args) {
    Py_RETURN_NONE;
}

PyObject *cnatnet_setVerbosityLevel(PyObject *self, PyObject *args) {
    PyObject *pyInst;
    int level;
    PyArg_ParseTuple(args, "Oi", &pyInst, &level);
    NatNetClient *inst = (NatNetClient *)PyCObject_AsVoidPtr(pyInst);
    inst->SetVerbosityLevel(level);
    Py_RETURN_NONE;
}

PyObject *cnatnet_setDataCallback(PyObject *self, PyObject *args) {
    PyObject *pyInst;
    PyObject *callback;
    PyArg_ParseTuple(args, "OO", &pyInst, &callback);
    Py_INCREF(callback);
    NatNetClient *inst = (NatNetClient *)PyCObject_AsVoidPtr(pyInst);
    inst->SetDataCallback(DataHandler, callback);
    Py_RETURN_NONE;
}

PyMethodDef cnatnet_funcs[] = {
    {"constructor", cnatnet_constructor, METH_VARARGS, "NatNetClient::NatNetClient"},
    {"initialize", cnatnet_initialize, METH_VARARGS, "NatNetClient::Initialize"},
    {"uninitialize", cnatnet_uninitialize, METH_VARARGS, "NatNetClient::Uninitialize"},
    {"natNetVersion", cnatnet_natNetVersion, METH_VARARGS, "NatNetClient::NatNetVersion"},
    {"getServerDescription", cnatnet_getServerDescription, METH_VARARGS, "NatNetClient::GetServerDescription"},
    {"getDataDescriptions", cnatnet_getDataDescriptions, METH_VARARGS, "NatNetClient::GetDataDescriptions"},
    {"setMessageCallback", cnatnet_setMessageCallback, METH_VARARGS, "NatNetClient::SetMessageCallback"},
    {"setVerbosityLevel", cnatnet_setVerbosityLevel, METH_VARARGS, "NatNetClient::SetVerbosityLevel"},
    {"setDataCallback", cnatnet_setDataCallback, METH_VARARGS, "NatNetClient::SetDataCallback"},
    {NULL, NULL, 0, NULL}    /* Sentinel */
};

PyMODINIT_FUNC initcnatnet() {
    Py_InitModule("cnatnet", cnatnet_funcs);
    PyEval_InitThreads();
    FrameOfMocapData = PyObject_GetAttrString(PyImport_ImportModule("NatNet.FrameOfMocapData"), "FrameOfMocapData");
    Py_INCREF(FrameOfMocapData);
    RigidBodyData = PyObject_GetAttrString(PyImport_ImportModule("NatNet.RigidBodyData"), "RigidBodyData");
    Py_INCREF(RigidBodyData);
    MarkerSetData = PyObject_GetAttrString(PyImport_ImportModule("NatNet.MarkerSetData"), "MarkerSetData");
    Py_INCREF(MarkerSetData);
    SkeletonData = PyObject_GetAttrString(PyImport_ImportModule("NatNet.SkeletonData"), "SkeletonData");
    Py_INCREF(SkeletonData);

    PyObject *descriptionModule = PyImport_ImportModule("NatNet.DataDescription");
    MarkerSetDescription = PyObject_GetAttrString(descriptionModule, "MarkerSetDescription");
    Py_INCREF(MarkerSetDescription);
    RigidBodyDescription = PyObject_GetAttrString(descriptionModule, "RigidBodyDescription");
    Py_INCREF(RigidBodyDescription);
    SkeletonDescription = PyObject_GetAttrString(descriptionModule, "SkeletonDescription");
    Py_INCREF(SkeletonDescription);
}
