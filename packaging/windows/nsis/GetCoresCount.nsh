# Copied entirely from
#  http://stackoverflow.com/questions/29911549/cpu-features-getcount-return-incorrect-number-for-cpu-cores
!include LogicLib.nsh
!ifndef ERROR_INSUFFICIENT_BUFFER
!define ERROR_INSUFFICIENT_BUFFER 122
!endif
!define RelationProcessorCore 0

!if "${NSIS_PTR_SIZE}" <= 4
Function GetProcessorPhysCoreCount
System::Store S
StrCpy $9 0 ; 0 if we fail
System::Call 'kernel32::GetLogicalProcessorInformationEx(i${RelationProcessorCore},i,*i0r2)i.r0?e'
Pop $3
${If} $3 = ${ERROR_INSUFFICIENT_BUFFER}
${AndIf} $2 <> 0
    System::Alloc $2
    System::Call 'kernel32::GetLogicalProcessorInformationEx(i${RelationProcessorCore},isr1,*ir2r2)i.r0'
    Push $1
    ${If} $0 <> 0
    loop_7:
        IntOp $9 $9 + 1
        System::Call *$1(i,i.r3)
        IntOp $1 $1 + $3
        IntOp $2 $2 - $3
        IntCmp $2 0 "" loop_7 loop_7
    ${EndIf}
    Pop $1
    System::Free $1
${Else}
    System::Call 'kernel32::GetLogicalProcessorInformation(i,*i0r2)i.r0?e'
    Pop $3
    ${If} $3 = ${ERROR_INSUFFICIENT_BUFFER}
        System::Alloc $2
        System::Call 'kernel32::GetLogicalProcessorInformation(isr1,*ir2r2)i.r0'
        Push $1
        ${If} $0 <> 0
        loop_v:
            System::Call *$1(i,i.r3)
            ${If} $3 == ${RelationProcessorCore}
                IntOp $9 $9 + 1
            ${EndIf}
            IntOp $1 $1 + 24
            IntOp $2 $2 - 24
            IntCmp $2 0 "" loop_v loop_v
        ${EndIf}
        Pop $1
        System::Free $1
    ${EndIf}
${EndIf}
Push $9
System::Store L
FunctionEnd
!endif
