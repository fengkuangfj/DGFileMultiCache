
#pragma once

#define  EXCEPTION_TAG 'ECXE' // EXCE

NTSTATUS
DokanExceptionFilter(__in PIRP Irp, __in PEXCEPTION_POINTERS ExceptionPointer);

NTSTATUS
DokanExceptionHandler(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in NTSTATUS ExceptionCode);
