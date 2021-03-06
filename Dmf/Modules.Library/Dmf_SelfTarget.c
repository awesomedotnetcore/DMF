/*++

    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the MIT license.

Module Name:

    Dmf_SelfTarget.c

Abstract:

    Supports sending requests to Client Driver that instantiated this Module.

Environment:

    Kernel-mode Driver Framework
    User-mode Driver Framework

--*/

// DMF and this Module's Library specific definitions.
//
#include "DmfModules.Library.h"
#include "DmfModules.Library.Trace.h"

#include "Dmf_SelfTarget.tmh"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Module Private Enumerations and Structures
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Module Private Context
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

// Contains elements needed to send Requests to this driver.
//
typedef struct
{
    // Underlying target.
    //
    WDFIOTARGET IoTarget;
    //
    //
    DMFMODULE DmfModuleContinuousRequestTarget;
} DMF_CONTEXT_SelfTarget;

// This macro declares the following function:
// DMF_CONTEXT_GET()
//
DMF_MODULE_DECLARE_CONTEXT(SelfTarget)

// This Module has no Config.
//
DMF_MODULE_DECLARE_NO_CONFIG(SelfTarget)

///////////////////////////////////////////////////////////////////////////////////////////////////////
// DMF Module Support Code
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Wdf Module Callbacks
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

///////////////////////////////////////////////////////////////////////////////////////////////////////
// DMF Module Callbacks
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
static
NTSTATUS
DMF_SelfTarget_Open(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Initialize an instance of a DMF Module of type SelfTarget.

Arguments:

    DmfModule - This Module's handle.

Return Value:

    STATUS_SUCCESS

--*/
{
    NTSTATUS ntStatus;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    DMF_CONTEXT_SelfTarget* moduleContext;

    PAGED_CODE();

    FuncEntry(DMF_TRACE_SelfTarget);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    device = DMF_AttachedDeviceGet(DmfModule);

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = DmfModule;

    ntStatus = WdfIoTargetCreate(device,
                                 &objectAttributes,
                                 &moduleContext->IoTarget);
    if (! NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE_SelfTarget, "RESOURCE_HUB_CREATE_PATH_FROM_ID fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    WDF_IO_TARGET_OPEN_PARAMS_INIT_EXISTING_DEVICE(&openParams,
                                                   WdfDeviceWdmGetDeviceObject(device));
    openParams.ShareAccess = FILE_SHARE_WRITE | FILE_SHARE_READ;

    // Open the IoTarget for I/O operation.
    //
    ntStatus = WdfIoTargetOpen(moduleContext->IoTarget,
                               &openParams);
    if (! NT_SUCCESS(ntStatus))
    {
        ASSERT(NT_SUCCESS(ntStatus));
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE_SelfTarget, "WdfIoTargetOpen fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    DMF_ContinuousRequestTarget_IoTargetSet(moduleContext->DmfModuleContinuousRequestTarget,
                                            moduleContext->IoTarget);

Exit:

    FuncExit(DMF_TRACE_SelfTarget, "ntStatus=%!STATUS!", ntStatus);

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
static
VOID
DMF_SelfTarget_Close(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Uninitialize an instance of a DMF Module of type SelfTarget.

Arguments:

    DmfModule - This Module's handle.

Return Value:

    None

--*/
{
    DMF_CONTEXT_SelfTarget* moduleContext;

    PAGED_CODE();

    FuncEntry(DMF_TRACE_SelfTarget);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    if (moduleContext->IoTarget != NULL)
    {
        WdfIoTargetClose(moduleContext->IoTarget);
        WdfObjectDelete(moduleContext->IoTarget);
        moduleContext->IoTarget = NULL;
    }

    FuncExitVoid(DMF_TRACE_SelfTarget);
}
#pragma code_seg()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// DMF Module Descriptor
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

static DMF_MODULE_DESCRIPTOR DmfModuleDescriptor_SelfTarget;
static DMF_CALLBACKS_DMF DmfCallbacksDmf_SelfTarget;

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Calls by Client
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_SelfTarget_Create(
    _In_ WDFDEVICE Device,
    _In_ DMF_MODULE_ATTRIBUTES* DmfModuleAttributes,
    _In_ WDF_OBJECT_ATTRIBUTES* ObjectAttributes,
    _Out_ DMFMODULE* DmfModule
    )
/*++

Routine Description:

    Create an instance of a DMF Module of type SelfTarget.

Arguments:

    Device - Client driver's WDFDEVICE object.
    DmfModuleAttributes - Opaque structure that contains parameters DMF needs to initialize the Module.
    ObjectAttributes - WDF object attributes for DMFMODULE.
    DmfModule - Address of the location where the created DMFMODULE handle is returned.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus;
    DMFMODULE dmfModule;
    DMF_CONTEXT_SelfTarget* moduleContext;
    DMF_CONFIG_ContinuousRequestTarget continuousRequestTargetModuleConfig;
    WDF_OBJECT_ATTRIBUTES attributes;
    DMF_MODULE_ATTRIBUTES moduleAttributes;

    PAGED_CODE();

    FuncEntry(DMF_TRACE_SelfTarget);

    DMF_CALLBACKS_DMF_INIT(&DmfCallbacksDmf_SelfTarget);
    DmfCallbacksDmf_SelfTarget.DeviceOpen = DMF_SelfTarget_Open;
    DmfCallbacksDmf_SelfTarget.DeviceClose = DMF_SelfTarget_Close;

    DMF_MODULE_DESCRIPTOR_INIT_CONTEXT_TYPE(DmfModuleDescriptor_SelfTarget,
                                            SelfTarget,
                                            DMF_CONTEXT_SelfTarget,
                                            DMF_MODULE_OPTIONS_PASSIVE,
                                            DMF_MODULE_OPEN_OPTION_OPEN_PrepareHardware);

    DmfModuleDescriptor_SelfTarget.CallbacksDmf = &DmfCallbacksDmf_SelfTarget;

    ntStatus = DMF_ModuleCreate(Device,
                                DmfModuleAttributes,
                                ObjectAttributes,
                                &DmfModuleDescriptor_SelfTarget,
                                &dmfModule);
    if (! NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE_SelfTarget, "DMF_ModuleCreate fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    moduleContext = DMF_CONTEXT_GET(dmfModule);

    // dmfModule will be set as ParentObject for all child modules.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = dmfModule;

    // ContinuousRequestTarget
    // -----------------------
    //
    DMF_CONFIG_ContinuousRequestTarget_AND_ATTRIBUTES_INIT(&continuousRequestTargetModuleConfig,
                                                           &moduleAttributes);
    ntStatus = DMF_ContinuousRequestTarget_Create(Device,
                                                  &moduleAttributes,
                                                  &attributes,
                                                  &moduleContext->DmfModuleContinuousRequestTarget);
    if (! NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE_SelfTarget, "DMF_ContinuousRequestTarget_Create fails: ntStatus=%!STATUS!", ntStatus);
        DMF_Module_Destroy(dmfModule);
        dmfModule = NULL;
        goto Exit;
    }

Exit:

    *DmfModule = dmfModule;

    FuncExit(DMF_TRACE_SelfTarget, "ntStatus=%!STATUS!", ntStatus);

    return(ntStatus);
}
#pragma code_seg()

// Module Methods
//

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DMF_SelfTarget_Get(
    _In_ DMFMODULE DmfModule,
    _Out_ WDFIOTARGET* IoTarget
    )
{
    DMF_CONTEXT_SelfTarget* moduleContext;

    PAGED_CODE();

    DMF_HandleValidate_ModuleMethod(DmfModule,
                                    &DmfModuleDescriptor_SelfTarget);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    ASSERT(IoTarget != NULL);
    ASSERT(moduleContext->IoTarget != NULL);
    *IoTarget = moduleContext->IoTarget;

    return STATUS_SUCCESS;
}
#pragma code_seg()

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
DMF_SelfTarget_Send(
    _In_ DMFMODULE DmfModule,
    _In_reads_bytes_(RequestLength) VOID* RequestBuffer,
    _In_ size_t RequestLength,
    _Out_writes_bytes_(ResponseLength) VOID* ResponseBuffer,
    _In_ size_t ResponseLength,
    _In_ ContinuousRequestTarget_RequestType RequestType,
    _In_ ULONG RequestIoctl,
    _In_ ULONG RequestTimeoutMilliseconds,
    _In_opt_ EVT_DMF_ContinuousRequestTarget_SingleAsynchronousBufferOutput* EvtContinuousRequestTargetSingleAsynchronousRequest,
    _In_opt_ VOID* SingleAsynchronousRequestClientContext
    )
/*++

Routine Description:

    Creates and sends an Asynchronous request to the IoTarget given a buffer, IOCTL and other information.

Arguments:

    DmfModule - This Module's handle.
    RequestBuffer - Buffer of data to attach to request to be sent.
    RequestLength - Number of bytes to in RequestBuffer to send.
    ResponseBuffer - Buffer of data that is returned by the request.
    ResponseLength - Size of Response Buffer in bytes.
    RequestType - Read or Write or Ioctl
    RequestIoctl - The given IOCTL.
    RequestTimeoutMilliseconds - Timeout value in milliseconds of the transfer or zero for no timeout.
    EvtContinuousRequestTargetSingleAsynchronousRequest - Callback to be called in completion routine.
    SingleAsynchronousRequestClientContext - Client context sent in callback

Return Value:

    STATUS_SUCCESS if a buffer is added to the list.
    Other NTSTATUS if there is an error.

--*/
{
    NTSTATUS ntStatus;
    DMF_CONTEXT_SelfTarget* moduleContext;

    FuncEntry(DMF_TRACE_SelfTarget);

    DMF_HandleValidate_ModuleMethod(DmfModule,
                                    &DmfModuleDescriptor_SelfTarget);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    ASSERT(moduleContext->IoTarget != NULL);

    ntStatus = DMF_ContinuousRequestTarget_Send(moduleContext->DmfModuleContinuousRequestTarget,
                                                RequestBuffer,
                                                RequestLength,
                                                ResponseBuffer,
                                                ResponseLength,
                                                RequestType,
                                                RequestIoctl,
                                                RequestTimeoutMilliseconds,
                                                EvtContinuousRequestTargetSingleAsynchronousRequest,
                                                SingleAsynchronousRequestClientContext);

    FuncExit(DMF_TRACE_SelfTarget, "ntStatus=%!STATUS!", ntStatus);

    return ntStatus;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
DMF_SelfTarget_SendSynchronously(
    _In_ DMFMODULE DmfModule,
    _In_reads_bytes_(RequestLength) VOID* RequestBuffer,
    _In_ size_t RequestLength,
    _Out_writes_bytes_(ResponseLength) VOID* ResponseBuffer,
    _In_ size_t ResponseLength,
    _In_ ContinuousRequestTarget_RequestType RequestType,
    _In_ ULONG RequestIoctl,
    _In_ ULONG RequestTimeoutMilliseconds,
    _Out_opt_ size_t* BytesWritten
    )
/*++

Routine Description:

    Creates and sends a synchronous request to the IoTarget given a buffer, IOCTL and other information.

Arguments:

    DmfModule - This Module's handle.
    RequestBuffer - Buffer of data to attach to request to be sent.
    RequestLength - Number of bytes to in RequestBuffer to send.
    ResponseBuffer - Buffer of data that is returned by the request.
    ResponseLength - Size of Response Buffer in bytes.
    RequestType - Read or Write or Ioctl
    RequestIoctl - The given IOCTL.
    RequestTimeoutMilliseconds - Timeout value in milliseconds of the transfer or zero for no timeout.
    BytesWritten - Bytes returned by the transaction.

Return Value:

    STATUS_SUCCESS if a buffer is added to the list.
    Other NTSTATUS if there is an error.

--*/
{
    NTSTATUS ntStatus;
    DMF_CONTEXT_SelfTarget* moduleContext;

    FuncEntry(DMF_TRACE_SelfTarget);

    DMF_HandleValidate_ModuleMethod(DmfModule,
                                    &DmfModuleDescriptor_SelfTarget);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    ASSERT(moduleContext->IoTarget != NULL);

    ntStatus = DMF_ContinuousRequestTarget_SendSynchronously(moduleContext->DmfModuleContinuousRequestTarget,
                                                             RequestBuffer,
                                                             RequestLength,
                                                             ResponseBuffer,
                                                             ResponseLength,
                                                             RequestType,
                                                             RequestIoctl,
                                                             RequestTimeoutMilliseconds,
                                                             BytesWritten);

    FuncExit(DMF_TRACE_SelfTarget, "ntStatus=%!STATUS!", ntStatus);

    return ntStatus;
}

// eof: Dmf_SelfTarget.c
//
