
#pragma once

#define UNC_TAG 'TCNU' // UNCT

VOID DokanRegisterUncProvider(__in PDokanDCB Dcb);

NTSTATUS DokanRegisterUncProviderSystem(PDokanDCB dcb);

VOID DokanDeregisterUncProvider(__in PDokanDCB Dcb);
