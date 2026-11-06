option casemap:none

EXTERN gMapSeedOverrideActive:DWORD
EXTERN gSelectedMapSeed:DWORD
EXTERN gDrlgSeedOverrideCalls:DWORD
EXTERN gDrlgSeedInput:DWORD
EXTERN gDrlgSeedInputState:DWORD
EXTERN gDrlgSeedWritten:DWORD
EXTERN gDrlgSeedObservedCalls:DWORD
EXTERN gDrlgSeedObservedStored:DWORD
EXTERN gDrlgSeedObservedLcg:DWORD
EXTERN gGameSeedIngressCalls:DWORD
EXTERN gGameSeedIngressInput:DWORD
EXTERN gGameSeedIngressWritten:DWORD
EXTERN gSavedSeedCopyACalls:DWORD
EXTERN gSavedSeedCopyAInput:DWORD
EXTERN gSavedSeedCopyAWritten:DWORD
EXTERN gSavedSeedCopyBCalls:DWORD
EXTERN gSavedSeedCopyBInput:DWORD
EXTERN gSavedSeedCopyBWritten:DWORD
EXTERN gSavedSeedCopyCCalls:DWORD
EXTERN gSavedSeedCopyCInput:DWORD
EXTERN gSavedSeedCopyCWritten:DWORD
EXTERN gOriginalDrlgSeedInitialization:QWORD
EXTERN gOriginalDrlgSeedPostWrite:QWORD
EXTERN gOriginalGameSeedIngress:QWORD
EXTERN gOriginalSavedSeedCopyA:QWORD
EXTERN gOriginalSavedSeedCopyB:QWORD
EXTERN gOriginalSavedSeedCopyC:QWORD

.code
PUBLIC MapDrlgSeedInitializationHook

MapDrlgSeedInitializationHook PROC
    pushfq
    push r11
    mov r11d, DWORD PTR [gMapSeedOverrideActive]
    test r11d, r11d
    jz SeedInitializationUseOriginal
    mov eax, DWORD PTR [rdi]
    mov DWORD PTR [gDrlgSeedInput], eax
    mov eax, DWORD PTR [rdi+4]
    mov DWORD PTR [gDrlgSeedInputState], eax
    mov ebx, DWORD PTR [gSelectedMapSeed]
    mov DWORD PTR [gDrlgSeedWritten], ebx
    mov DWORD PTR [rdi], ebx
    mov DWORD PTR [rdi+4], 29Ah
    lock inc DWORD PTR [gDrlgSeedOverrideCalls]
SeedInitializationUseOriginal:
    pop r11
    popfq
    jmp QWORD PTR [gOriginalDrlgSeedInitialization]
MapDrlgSeedInitializationHook ENDP

PUBLIC MapDrlgSeedPostWriteHook

MapDrlgSeedPostWriteHook PROC
    pushfq
    mov eax, DWORD PTR [gMapSeedOverrideActive]
    test eax, eax
    jz SeedPostWriteUseOriginal
    mov eax, DWORD PTR [rdi+840h]
    mov DWORD PTR [gDrlgSeedObservedStored], eax
    mov DWORD PTR [gDrlgSeedObservedLcg], r9d
    lock inc DWORD PTR [gDrlgSeedObservedCalls]
SeedPostWriteUseOriginal:
    popfq
    jmp QWORD PTR [gOriginalDrlgSeedPostWrite]
MapDrlgSeedPostWriteHook ENDP

PUBLIC MapGameSeedIngressHook

MapGameSeedIngressHook PROC
    pushfq
    push r10
    push r11
    mov r10d, DWORD PTR [rdi+12Ch]
    mov DWORD PTR [gGameSeedIngressInput], r10d
    lock inc DWORD PTR [gGameSeedIngressCalls]
    mov r11d, DWORD PTR [gMapSeedOverrideActive]
    test r11d, r11d
    jz GameSeedIngressRecordWritten
    mov r11d, DWORD PTR [gSelectedMapSeed]
    mov DWORD PTR [rdi+12Ch], r11d
    mov r10d, r11d
GameSeedIngressRecordWritten:
    mov DWORD PTR [gGameSeedIngressWritten], r10d
    pop r11
    pop r10
    popfq
    jmp QWORD PTR [gOriginalGameSeedIngress]
MapGameSeedIngressHook ENDP

PUBLIC MapSavedSeedCopyAHook

MapSavedSeedCopyAHook PROC
    pushfq
    push r10
    push r11
    mov r10d, eax
    mov DWORD PTR [gSavedSeedCopyAInput], r10d
    lock inc DWORD PTR [gSavedSeedCopyACalls]
    mov r11d, DWORD PTR [gMapSeedOverrideActive]
    test r11d, r11d
    jz SavedSeedCopyARecordWritten
    mov eax, DWORD PTR [gSelectedMapSeed]
SavedSeedCopyARecordWritten:
    mov r10d, eax
    mov DWORD PTR [gSavedSeedCopyAWritten], r10d
    pop r11
    pop r10
    popfq
    jmp QWORD PTR [gOriginalSavedSeedCopyA]
MapSavedSeedCopyAHook ENDP

PUBLIC MapSavedSeedCopyBHook

MapSavedSeedCopyBHook PROC
    pushfq
    push r10
    push r11
    mov r10d, eax
    mov DWORD PTR [gSavedSeedCopyBInput], r10d
    lock inc DWORD PTR [gSavedSeedCopyBCalls]
    mov r11d, DWORD PTR [gMapSeedOverrideActive]
    test r11d, r11d
    jz SavedSeedCopyBRecordWritten
    mov eax, DWORD PTR [gSelectedMapSeed]
SavedSeedCopyBRecordWritten:
    mov r10d, eax
    mov DWORD PTR [gSavedSeedCopyBWritten], r10d
    pop r11
    pop r10
    popfq
    jmp QWORD PTR [gOriginalSavedSeedCopyB]
MapSavedSeedCopyBHook ENDP

PUBLIC MapSavedSeedCopyCHook

MapSavedSeedCopyCHook PROC
    pushfq
    push r10
    push r11
    mov r10d, eax
    mov DWORD PTR [gSavedSeedCopyCInput], r10d
    lock inc DWORD PTR [gSavedSeedCopyCCalls]
    mov r11d, DWORD PTR [gMapSeedOverrideActive]
    test r11d, r11d
    jz SavedSeedCopyCRecordWritten
    mov eax, DWORD PTR [gSelectedMapSeed]
SavedSeedCopyCRecordWritten:
    mov r10d, eax
    mov DWORD PTR [gSavedSeedCopyCWritten], r10d
    pop r11
    pop r10
    popfq
    jmp QWORD PTR [gOriginalSavedSeedCopyC]
MapSavedSeedCopyCHook ENDP

END
