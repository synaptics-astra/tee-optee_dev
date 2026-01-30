/******************************************************************
*
* File name: tsp_filter_allinone_figo_pc.h - GENERATED from figo assembly file:
*            tsp_filter_allinone.fasm
*
* Description:
*      DO NOT MODIFY THIS FILE!!
*
*
******************************************************************/


#ifndef GEN__TSP_FILTER_ALLINONE_FIGO_H__
#define GEN__TSP_FILTER_ALLINONE_FIGO_H__


    /* all modules entry point */
    #define     tsp_filter_allinone_figo_main                                                0
    #define     tsp_filter_allinone_figo_DMX_CheckInFrame                                    1
    #define     tsp_filter_allinone_figo_DMX_CheckVideoSmall                                 1
    #define     tsp_filter_allinone_figo_DMX_Check_Item_Type                                 1
    #define     tsp_filter_allinone_figo_MV_DECFilter_ByPass                                 1
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DTCMStream_Write_New                     1
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_HBOStreamOut_Write                       1
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_MEMCPY                                   1
    #define     tsp_filter_allinone_figo_MV_FIGO_Tsp_DMX_VideoSmallPadding                   1
    #define     tsp_filter_allinone_figo_MV_Log_Debug_Ctx                                    1
    #define     tsp_filter_allinone_figo_MV_SWTimer_Init                                     1
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DSData                                   174
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DSCopy                                   224
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_PushDsFifo32                             281
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DSFifo32PopBytes                         389
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DSFifo32Peak                             421
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_PushDSRetQ                               532
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_UpdateDSRetQ                             563
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_MEMCPY_HW                                591
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DtcmFifo_Peak                            650
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DSStreamIn_Preload                       681
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DSStreamIn_Read                          836
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DSOutBlk                                 1023
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DSStreamOut_Write                        1123
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DTCMStream_Write                         1483
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_DTCMStream_Read                          1494
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_HBOStreamIn_Read                         1505
    #define     tsp_filter_allinone_figo_MV_Secure_IsDsOutMemPoolInvalid                     1619
    #define     tsp_filter_allinone_figo_MV_Secure_IsStrmOutInvalid                          1669
    #define     tsp_filter_allinone_figo_MV_Secure_IsStrmInSecurity                          1754
    #define     tsp_filter_allinone_figo_MV_Secure_GetDmxFilterByPidFilter                   1839
    #define     tsp_filter_allinone_figo_MV_Secure_IsLocalDataInvalid                        1882
    #define     tsp_filter_allinone_figo_MV_SecureRule_Check                                 1910
    #define     tsp_filter_allinone_figo_MV_Secure_SecureRuleCheckKeyAddrProtect             2245
    #define     tsp_filter_allinone_figo_MV_Secure_SecureRuleCheckKeyForbidPVR               2280
    #define     tsp_filter_allinone_figo_MV_Secure_SecureRuleCheckResultFromTable            2313
    #define     tsp_filter_allinone_figo_MV_Secure_SecureRuleCheckResultForPVR               2366
    #define     tsp_filter_allinone_figo_MV_Secure_SecureRuleCheckKeyAttribute               2418
    #define     tsp_filter_allinone_figo_MV_Secure_SecureRuleCheckTable                      2452
    #define     tsp_filter_allinone_figo_MV_Secure_CheckDecPhaseOutSecRlt                    2851
    #define     tsp_filter_allinone_figo_MV_Secure_CheckNSKESOutSecRlt                       3143
    #define     tsp_filter_allinone_figo_MV_Secure_UpdateNSKRecordTSOutSecRlt                3244
    #define     tsp_filter_allinone_figo_MV_Secure_UpdateGeneraRecordTSOutSecRlt             3328
    #define     tsp_filter_allinone_figo_MV_IFCP_keyTable_Partition_Check                    3644
    #define     tsp_filter_allinone_figo_MV_Send_SecureRuleCheckEvent                        3820
    #define     tsp_filter_allinone_figo_NSK_LockKTE                                         3879
    #define     tsp_filter_allinone_figo_NSK_CopySwitch                                      3947
    #define     tsp_filter_allinone_figo_NSK_CheckAlgAny64_128                               4012
    #define     tsp_filter_allinone_figo_NSK_GetAlgbySchemeCode                              4050
    #define     tsp_filter_allinone_figo_NSK_CheckDecAlgValidbyCode                          4080
    #define     tsp_filter_allinone_figo_NSK_CheckEncAlgValidbyCode                          4147
    #define     tsp_filter_allinone_figo_NSK_IsLocalDESAAlgValid                             4217
    #define     tsp_filter_allinone_figo_NSK_IsLocalSAlgValid                                4269
    #define     tsp_filter_allinone_figo_NSK_IsKeyValid                                      4323
    #define     tsp_filter_allinone_figo_NSK_Handle2Decrypt                                  4349
    #define     tsp_filter_allinone_figo_DMXFilter_Crypto_WriteCmdCnt                        4704
    #define     tsp_filter_allinone_figo_NSK_DoDecrypt                                       4720
    #define     tsp_filter_allinone_figo_MV_FIGOC_TSP_Dmx_Filter_handleCts                   6012
    #define     tsp_filter_allinone_figo_MV_FIGOC_TSP_Dmx_Filter_CalIVE                      6299
    #define     tsp_filter_allinone_figo_NSK_HandleEncrypt                                   6418
    #define     tsp_filter_allinone_figo_MV_SWTimer_Is_Timeout                               6731
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_DECRYPT_BLOCK                6740
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_MEMCPY                       6808
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_MEMSET                       6833
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_MEMREVERSE                   6888
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_GET_ErrCryptoCmdFlag         6932
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_CLEAR_ErrCryptoCmdFlag       6946
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_GET_CNT                      6958
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_CMD_SYNC                     6971
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_CMD_ASYNC                    7020
    #define     tsp_filter_allinone_figo_MV_FIGO_Tsp_Swd_Sync_Match                          7066
    #define     tsp_filter_allinone_figo_MV_FIGO_TspDiag_TSP_Dmx_FilterRun                   7165
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_PcrProc                                12740
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_Filter_Generate_TSP_Crypto_Cmd         13183
    #define     tsp_filter_allinone_figo_MV_Does_DecryptKeyCtx_Exist                         13295
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_Is_Wanted_PID                          13321
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_ReSync                                 13424
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_Is_Match_SyncPoint                     13469
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_IS_ALL_OUTPUT_READY                    13511
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_StrmStateChecking                      13739
    #define     tsp_filter_allinone_figo_MV_FIGO_DMX_StopFilter                              14063
    #define     tsp_filter_allinone_figo_MV_FIGOLIB_STREAM_GETCOUNT                          14110
    #define     tsp_filter_allinone_figo_MV_FIGO_DMX_BroadcastCtrl_Fake                      14140
    #define     tsp_filter_allinone_figo_MV_FIGO_DMX_BroadcastCtrl                           14192
    #define     tsp_filter_allinone_figo_MV_FIGO_RecOutputUnitStart                          14519
    #define     tsp_filter_allinone_figo_MV_FIGO_RecOutputVESHdrInfo                         14763
    #define     tsp_filter_allinone_figo_MV_Send_ReencEvent                                  15484
    #define     tsp_filter_allinone_figo_MV_FIGO_Tsp_DmxUpdateMinBufSpace                    15574
    #define     tsp_filter_allinone_figo_MV_FIGO_Tsp_DmxPesPkt                               15619
    #define     tsp_filter_allinone_figo_MV_FIGO_TspDiag_SefInputHBOWrite                    17226
    #define     tsp_filter_allinone_figo_MV_FIGO_TspDiag_SefOutputHBORead                    17278
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_OuputPsiSectionEvent                   17332
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_OutputPsiData                          17419
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_GetFreePartSecHdrCtxAddr               17554
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_ReleasePartSecHdrCtxAddr               17586
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_ResumeSefRulesPerPid                   17605
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_SectionFilter                          17666
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_CheckNskEmmFltPattern                  17858
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_PSI_InitSecFiltCmd                     18001
    #define     tsp_filter_allinone_figo_TSP_DMX_FIGO_PsiPkt                                 18025
    #define     tsp_filter_allinone_figo_M2M_Check_Key_Valid                                 21416
    #define     tsp_filter_allinone_figo_M2M_Is_ResidueCmd                                   21606
    #define     tsp_filter_allinone_figo_M2M_Generate_CryptoCmd                              21647
    #define     tsp_filter_allinone_figo_MV_FIGO_TSP_M2M_Task                                21853
    #define     tsp_filter_allinone_figo_MV_DECFilter_HandleFilterCmd                        22785
    #define     tsp_filter_allinone_figo_MV_FIGO_TSP_DECFilter                               22892
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_PLAYREADYIVINIT              23023
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_PLAYREADYCMDCFG              23078
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_PLAYREADY                    23150
    #define     tsp_filter_allinone_figo_MV_PrDECFilter_LSHIFT64                             23359
    #define     tsp_filter_allinone_figo_MV_PrDECFilter_RSHIFT64                             23390
    #define     tsp_filter_allinone_figo_MV_PrDECFilter_INC64                                23421
    #define     tsp_filter_allinone_figo_MV_PrDECFilter_SampleCTXUpdate                      23441
    #define     tsp_filter_allinone_figo_MV_PrDECFilter_HandleStreamCtrl                     23468
    #define     tsp_filter_allinone_figo_MV_PrDECFilter_ProcPacket                           23771
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_WIDEVINE                     23973
    #define     tsp_filter_allinone_figo_MV_WvDECFilter_HandleStreamCtrl                     24427
    #define     tsp_filter_allinone_figo_MV_WvDECFilter_ProcPacket                           24657
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_MARLIN                       25062
    #define     tsp_filter_allinone_figo_MV_MARLINDECFilter_HandleStreamCtrl                 25149
    #define     tsp_filter_allinone_figo_MV_MARLINDECFilter_ProcPacket                       25412
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_CENCAM1                      25809
    #define     tsp_filter_allinone_figo_MV_CENCAM1DECFilter_HandleStreamCtrl                25892
    #define     tsp_filter_allinone_figo_MV_CENCAM1DECFilter_ProcPacket                      26163
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_XOR                          26440
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_HDCP20                       26458
    #define     tsp_filter_allinone_figo_MV_HDCP20DECFilter_HandleStreamCtrl                 26630
    #define     tsp_filter_allinone_figo_MV_WMDEC_RSHIFT64                                   26895
    #define     tsp_filter_allinone_figo_MV_WMDEC_INC64                                      26926
    #define     tsp_filter_allinone_figo_MV_HDCP20DECFilter_ProcPacket                       26946
    #define     tsp_filter_allinone_figo_MV_FIGOC_CRYPTO_ENGINE_REENCRYPT                    27459
    #define     tsp_filter_allinone_figo_MV_ReEncFilter_HandleStreamCtrl                     27919
    #define     tsp_filter_allinone_figo_MV_ReEncFilter_ProcPacket                           28523

    #define TSP_FILTER_ALLINONE_FIGO_SIZE 28754
#endif /* GEN__TSP_FILTER_ALLINONE_FIGO_H__ */
