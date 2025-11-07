/* ---- CodeMeter.h ----------------------------------------------------------
 * CodeMeter Development-Kit ANSI/Microsoft C header file.
 * (C) Copyright 2002-2024 by WIBU-SYSTEMS AG
 * D-76137 Karlsruhe, Germany
 * Version 8.10 of 2024Feb19
 
 * ---------------------------------------------------------------------------
 */

#if !defined(_CODEMETER_INCLUDED)
#define _CODEMETER_INCLUDED

/* Define Version for this header file as MAJOR * 100 + MINOR */
/* since 4.00 */
#define CODEMETER_VERSION 810

#if defined(__cplusplus)
/* CodeMeter.h contains only C declarations */
extern "C" {
#endif

#if !defined(CMAPIENTRY)
  #if defined(_WIN32)
    /* Win32 API */
    #define CMAPIENTRY __cdecl
    #define CMSTDCALL __stdcall
  #else
    /* Standard */
    #define CMAPIENTRY
    #define CMSTDCALL
  #endif
#endif /* #if !defined(CMAPIENTRY) */

#if !defined(CODEMETER_API)
  #if defined(_WIN32) && !defined(_CODEMETER_API_STATIC_LIB)
    /* Win32 API */
    /* for source code including WibuCm32/64.dll */
    #define CODEMETER_API __declspec(dllimport)
  #else
/* Sandard */
    #define CODEMETER_API
  #endif
#endif /* #if !defined(CODEMETER_API) */


#if !defined(_CODEMETER_NODEFLIB)
  /* library string concatenation macros */
  #define CM_PPCX(s1, s2) s1##s2
  #define CM_PPCAT(s1, s2) CM_PPCX(s1, s2)
  #if defined(_WIN32) || defined(_WIN64)
    /* platform suffix */
    #if defined(_WIN64)
      #define CODEMETER_ARCH_SUFFIX        64
    #else
      /* standard Win32 has no suffix */
      #define CODEMETER_ARCH_SUFFIX        32
    #endif
    #if defined(_CODEMETER_API_STATIC_LIB) && !defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER>=1200
      /* link static lib, only possible when release is wanted */
      /* compiler version variant */
      #if _MSC_VER >= 1900   /* Visual Studio 2015, 2017, 2019, 2022 */
        #define CODEMETER_LIB_VARIANT_SUB StaticV14
      #elif _MSC_VER >= 1800 /* Visual Studio 2013 */
        #error "Visual C++ Version 12: Static library not longer supported"
      #elif _MSC_VER >= 1700 /* Visual Studio 2012 */
        #error "Visual C++ Version 11: Static library not longer supported"
      #elif _MSC_VER >= 1600 /* Visual Studio 2010 */
        #error "Visual C++ Version 10: Static library not longer supported"
      #elif _MSC_VER >= 1500
        #error "Visual C++ Version 9: Static library not longer supported"
      #elif _MSC_VER >= 1400
        #error "Visual C++ Version 8: Static library not longer supported"
      #elif _MSC_VER >= 1300
        #error "Visual C++ Version 7: Static library not longer supported"
      #elif _MSC_VER >= 1200
        #error "Visual C++ Version 6: Static library not longer supported"
      #endif
      /* C/C++ runtime in DLL or static */
      #if defined(_DLL)
        /* link MFC based variant */ 
        #if defined(_UNICODE)
          /* with Unicode support */
          #define CODEMETER_LIB_VARIANT CM_PPCAT(CODEMETER_LIB_VARIANT_SUB, MDU)
        #else
          #define CODEMETER_LIB_VARIANT CM_PPCAT(CODEMETER_LIB_VARIANT_SUB, MD)
        #endif
      #else
        #if defined(_UNICODE)
          /* with Unicode support */
          #define CODEMETER_LIB_VARIANT CM_PPCAT(CODEMETER_LIB_VARIANT_SUB, MTU)
        #else
          #define CODEMETER_LIB_VARIANT CM_PPCAT(CODEMETER_LIB_VARIANT_SUB, MT)
        #endif
      #endif
    #else
      /* link dynamic lib/dll */
      #define CODEMETER_LIB_VARIANT
    #endif
  #endif
  #if !defined(__GNUC__)
    /* build name string */
    #define CODEMETER_LIB_NAME_STR(s) #s
    #define CODEMETER_LIB_NAME_XSTR(s) CODEMETER_LIB_NAME_STR(s)
    #define CODEMETER_LIB_NAME  CODEMETER_LIB_NAME_XSTR(CM_PPCAT(WibuCm, CM_PPCAT(CODEMETER_ARCH_SUFFIX, CODEMETER_LIB_VARIANT)))
    /* pass library name to linker */
    #if defined(__BORLANDC__)
      /* Borland C++ Builder 3 doesn't accept macros in pragma */
      #if defined(_WIN64)
        #pragma comment(lib,"WibuCm64BC.lib")
      #else
        #pragma comment(lib,"WibuCm32BC.lib")
      #endif
    #else
      #pragma comment(lib,CODEMETER_LIB_NAME)
    #endif
  #endif
#endif /* #if !defined(_CODEMETER_NODEFLIB) */

#if defined(_WIN32)
  #define CMUINT     unsigned int
  #define CMULONG    unsigned long
  #define CMUSHORT   unsigned short
  #define CMBYTE     unsigned char
#elif defined(_WIN64)
  #define CMUINT     unsigned int
  #define CMULONG    unsigned long
  #define CMUSHORT   unsigned short
  #define CMBYTE     unsigned char
#elif defined(__APPLE__)
  #define CMUINT     unsigned int
  #define CMULONG    unsigned int
  #define CMUSHORT   unsigned short
  #define CMBYTE     unsigned char
#elif defined(__linux__)
  #define CMUINT     unsigned int
  #define CMULONG    unsigned int
  #define CMUSHORT   unsigned short
  #define CMBYTE     unsigned char
#else
  #define CMUINT     unsigned int
  #define CMULONG    unsigned long
  #define CMUSHORT   unsigned short
  #define CMBYTE     unsigned char
#endif

/*****************************************************************************
 ************************* CodeMeter error codes *****************************
******************************************************************************/

/* Some of the CodeMeter error codes use the full range of 32 bit. The result
   type of CmGetLastErrorCode requires signed values, but we want to use hex
   values. */
#define MAKE_CMERROR(e)    ((int)(e))

/* common errors of the CodeMeter driver components */
/* common no error code is 0 */
#define CMERROR_NO_ERROR                                        0
/* box specific error codes */
#define CMERROR_COMMANDUNDEFINED                                1
#define CMERROR_COMMANDINVALID                                  2
#define CMERROR_COMMANDTOOLARGE                                 3
#define CMERROR_COMMANDWRONG                                    4
#define CMERROR_COMMANDTVBWRONG                                 5
#define CMERROR_COMMANDINCOMPLETE                               6
#define CMERROR_AWAITENCRYPTIONRESTART                          7
#define CMERROR_ANSWERBUFFERFULL                                8
#define CMERROR_VALUETOOLARGE                                   9
#define CMERROR_OPTIONUNDEFINED                                 10
#define CMERROR_OPTIONINVALID                                   11
#define CMERROR_TYPEUNDEFINED                                   12
#define CMERROR_TYPEWRONG                                       13
#define CMERROR_KEYSOURCEUNDEFINED                              14
#define CMERROR_KEYSOURCEINVALID                                15
#define CMERROR_KEYSOURCEMISSED                                 16
#define CMERROR_KEYSOURCEWRONG                                  17
#define CMERROR_DATATOOSHORT                                    18
#define CMERROR_DATATOOLONG                                     19
#define CMERROR_DATAWRONG                                       20
#define CMERROR_FIRMITEMINVALID                                 21
#define CMERROR_FIRMCODEEXISTS                                  22
#define CMERROR_PRODUCTITEMINVALID                              23
#define CMERROR_PRODUCTITEMOPTIONUNDEFINED                      24
#define CMERROR_PRODUCTITEMOPTIONINVALID                        25
#define CMERROR_PRODUCTITEMOPTIONMISSED                         26
#define CMERROR_FIRMCODEWRONG                                   27
#define CMERROR_PRODUCTCODEWRONG                                28
#define CMERROR_FEATUREMAPMISSED                                29
#define CMERROR_FEATUREMAPINCOMPATIBLE                          30
#define CMERROR_UNITCOUNTERMISSED                               31
#define CMERROR_UNITCOUNTERZERO                                 32
#define CMERROR_UNITCOUNTERUNDERRUN                             33
#define CMERROR_EXPIRATIONTIMEMISSED                            34
#define CMERROR_EXPIRATIONTIMEOVERRUN                           35
#define CMERROR_ACTIVATIONTIMEMISSED                            36
#define CMERROR_ACTIVATIONTIMEUNDERRUN                          37
#define CMERROR_FIRMACCESSCOUNTERZERO                           38
#define CMERROR_CERTIFIEDTIMEOBSOLETE                           39
#define CMERROR_ENCRYPTIONINVALID                               40
#define CMERROR_DIFFIEHELLMANFAILED                             41
#define CMERROR_ECIESFAILED                                     42
#define CMERROR_SUBCOMMANDUNDEFINED                             43
#define CMERROR_SUBCOMMANDINVALID                               44
#define CMERROR_SUBCOMMANDINCOMPLETE                            45
#define CMERROR_SUBCOMMANDSTRUCTUREWRONG                        46
#define CMERROR_TVBWRONG                                        47
#define CMERROR_FIRMUPDATECOUNTERWRONG                          48
#define CMERROR_PITVBFLAGINVALID                                49
#define CMERROR_PITVBFLAGINCOMPATIBLE                           50
#define CMERROR_ENABLEBLOCKUSED                                 51
#define CMERROR_BOXDISABLED                                     52
#define CMERROR_FIRMITEMDISABLED                                53
#define CMERROR_PRODUCTITEMDISABLED                             54
#define CMERROR_INDEXINVALID                                    55
#define CMERROR_LEVELINVALID                                    56
#define CMERROR_TABLEFULL                                       57
#define CMERROR_MEMORYFULL                                      58
#define CMERROR_MEMORYDAMAGED                                   59
#define CMERROR_QUEENMEMORYDAMAGED                              60
#define CMERROR_FIRMITEMDAMAGED                                 61
#define CMERROR_CRTOKENOBSOLETE                                 62
#define CMERROR_MEMORYWRITEFAILED                               63
#define CMERROR_BOXBUSY                                         64
#define CMERROR_FEATURECODEINVALID                              65
#define CMERROR_COMMANDUNSUPPORTED                              66
#define CMERROR_FIELDUPDATELIMITEXCEEDED                        67
#define CMERROR_FLASHWRITEFAILED                                68
#define CMERROR_ACCESSMODEWRONG                                 69
#define CMERROR_BOXLOST                                         70
#define CMERROR_USAGEPERIODOVERRUN                              71
#define CMERROR_USAGEPERIODUNDERRUN                             72
#define CMERROR_BOXLOCKED                                       73
#define CMERROR_BOX_DAMAGED                                     74
#define CMERROR_BOX_LOCKED_TO                                   75
#define CMERROR_MAINTENANCEPERIODMISSING                        76
#define CMERROR_MAINTENANCEPERIODUNDERRUN                       77
#define CMERROR_MAINTENANCEPERIODOVERRUN                        78
#define CMERROR_MAINTENANCEPERIODINVALID                        79

#define CMERROR_BOXMEMORYDAMAGED                                80
#define CMERROR_BOSSMEMORY_DAMAGED                              81
#define CMERROR_FIRMWAREMEMORY_DAMAGED                          82
#define CMERROR_BOSS_FIRMWAREMEMORY_DAMAGED                     83
#define CMERROR_FUMMEMORY_DAMAGED                               84
#define CMERROR_BOSS_FUMMEMORY_DAMAGED                          85
#define CMERROR_FIRMWARE_FUMMEMORY_DAMAGED                      86
#define CMERROR_BOSS_FIRMWARE_FUMMEMORY_DAMAGED                 87
#define CMERROR_SYSTIMEOBSOLETE                                 88
#define CMERROR_BAD_IO_MODE                                     89
#define CMERROR_FULA_UPDATEEB_REACHED                           90
#define CMERROR_FULA_ENCRCOMM_REACHED                           91
#define CMERROR_UNFRAMED_COMMAND_REJECTED                       92
#define CMERROR_COMMAND_PARTIALLY_REJECTED                      93
#define CMERROR_COMMAND_PARTIALLY_UNSUPPORTED                   94
#define CMERROR_BOX_COMMUNICATION_95                            95
#define CMERROR_BOX_COMMUNICATION_96                            96
#define CMERROR_BOX_COMMUNICATION_97                            97
#define CMERROR_BOX_COMMUNICATION_98                            98
#define CMERROR_BOX_LICENSEMEMORY_NOT_FORMATTED                 99

/* management related error codes */
#define CMERROR_NETWORK_FAULT                                   100
#define CMERROR_SERVER_NOT_FOUND                                101
#define CMERROR_SEND_FAULT                                      102
#define CMERROR_RECEIVE_FAULT                                   103
#define CMERROR_INTERNAL_ORGANISATION                           104
#define CMERROR_INVALID_PARAMETER                               105
#define CMERROR_INVALID_HANDLE                                  106
#define CMERROR_NO_MORE_HANDLES                                 107
#define CMERROR_NO_MORE_MEMORY                                  108
#define CMERROR_SECURITY_PROBLEM                                109
#define CMERROR_NO_LOCAL_SERVER_STARTED                         110
#define CMERROR_NO_NETWORK_SERVER                               111
#define CMERROR_BUFFER_OVERFLOW                                 112
#define CMERROR_BAD_ADDRESS                                     113
#define CMERROR_BAD_HANDLE                                      114
#define CMERROR_WRONG_HANDLE_TYPE                               115
#define CMERROR_UNDEFINED_SYSTEM_HANDLE                         116
#define CMERROR_NO_EVENT_OCCURRED                               117
#define CMERROR_CTCS_FAULT                                      118
#define CMERROR_UNKNOWN_OS                                      119
#define CMERROR_NO_SECURITY_OBJECT                              120
#define CMERROR_WRONG_SECURITY_OBJECT                           121
#define CMERROR_NO_FSB_FUNCTIONALITY                            122
#define CMERROR_NO_CTSB_FUNCTIONALITY                           123
#define CMERROR_WRONG_OEMID                                     124
#define CMERROR_SERVER_VERSION_TOO_OLD                          125
/* replaced by CMERROR_LIBRARY_NOT_FOUND in version 4.00:
  #define CMERROR_SUBSYSTEM_NOT_AVAILABLE                       126
*/
#define CMERROR_LIBRARY_NOT_FOUND                               126
#define CMERROR_HANDLE_INCONSISTENCY                            127
#define CMERROR_LICENSE_TYPE_CMACT_DEACTIVATED                  128
#define CMERROR_USER_RUNTIME_VERSION_TOO_OLD                    129
#define CMERROR_REDUNDANCY_SERVER_MISSING                       130
#define CMERROR_TRANSFER_BUFFER_OVERFLOW                        131
#define CMERROR_NAMEDUSERCREDENTIALSWRONG                       132
#define CMERROR_REDUNDANCY_SERVER_EXCLUSIVE_MODE                133
#define CMERROR_COMMAND_NOT_SUPPORTED_BY_TMR                    134
#define CMERROR_COMMUNICATION_MODE_NOT_SUPPORTED_IN_TMR_MODE    135

/* Internal box communication specific error codes */
#define CMERROR_BOX_COMMUNICATION_176                           176
#define CMERROR_BOX_COMMUNICATION_177                           177
#define CMERROR_BOX_COMMUNICATION_178                           178
#define CMERROR_BOX_COMMUNICATION_179                           179
#define CMERROR_BOX_COMMUNICATION_180                           180
#define CMERROR_BOX_COMMUNICATION_181                           181
#define CMERROR_BOX_COMMUNICATION_182                           182
#define CMERROR_BOX_COMMUNICATION_183                           183
#define CMERROR_BOX_COMMUNICATION_184                           184
#define CMERROR_BOX_COMMUNICATION_185                           185
#define CMERROR_BOX_COMMUNICATION_186                           186
#define CMERROR_BOX_COMMUNICATION_187                           187
#define CMERROR_BOX_COMMUNICATION_188                           188
#define CMERROR_BOX_COMMUNICATION_189                           189
#define CMERROR_BOX_COMMUNICATION_190                           190
#define CMERROR_BOX_COMMUNICATION_191                           191

/* I2C error codes */
#define CMERROR_I2C_TIMEOUT_REQUEST                             192
#define CMERROR_I2C_TIMEOUT_ANSWER                              193
#define CMERROR_I2C_ANSWER_CRC                                  194
#define CMERROR_I2C_TWCOMMAND_INVALID                           195
#define CMERROR_I2C_PROTOCOL_VERSION                            196
#define CMERROR_I2C_COMMUNICATION                               197
#define CMERROR_I2C_REQUEST_CRC                                 198
#define CMERROR_I2C_ANSWER_BEFORE_REQUEST                       199

/* box command execution related error codes */
#define CMERROR_ENTRY_NOT_FOUND                                 200
#define CMERROR_BOX_NOT_FOUND                                   201
#define CMERROR_CRYPTION_FAILED                                 202
#define CMERROR_CRC_VERIFY_FAILED                               203
#define CMERROR_NO_ENABLING_EXISTS                              204
#define CMERROR_NO_PIO_SET                                      205
#define CMERROR_FIRMKEY_TOO_SHORT                               206
#define CMERROR_NO_BOXLOCK_NEEDED                               207
#define CMERROR_BOXLOCK_NOT_CHANGED                             208
#define CMERROR_NO_DATA_AVAILABLE                               209
#define CMERROR_SIGNEDLIST_FAILED                               210
#define CMERROR_VALIDATION_FAILED                               211
#define CMERROR_NO_MORE_LICENSES                                212
#define CMERROR_EXCLUSIVE_MODE_CONFLICT                         213
#define CMERROR_RESERVEDFI_MISSED                               214
#define CMERROR_WRONG_CONTENT                                   215
#define CMERROR_FIELDUPDATE_FAILED                              216
#define CMERROR_LICENSEFILE_CREATION_FAILED                     217
#define CMERROR_NO_LICENSE_AVAILABLE                            218
#define CMERROR_CONTEXT_FILE_WRITING_FAILED                     219
#define CMERROR_UPDATE_FILE_READING_FAILED                      220
#define CMERROR_NETINFO_FAILED                                  221
#define CMERROR_FIELDUPDATE_RESET_FAILED                        222
#define CMERROR_WRONG_ACCESS_MODE                               223
#define CMERROR_WRONG_BOX_VERSION                               224
#define CMERROR_INVALID_LICENSE_PARAMETER                       225
#define CMERROR_BORROW_LICENSE_FAILED                           226
#define CMERROR_BORROW_LICENSE_ISENABLED                        227
#define CMERROR_UPDATE_TOO_NEW                                  228
#define CMERROR_UPDATE_TOO_OLD                                  229
#define CMERROR_SEVERAL_REASONS                                 230
#define CMERROR_ACCESS_DENIED                                   231
#define CMERROR_BORROW_TIMEDIFFTOOLARGE                         232
#define CMERROR_UPDATE_FAILED                                   233
#define CMERROR_UPDATE_ABORTED                                  234
#define CMERROR_COMMAND_PENDING                                 235
#define CMERROR_NO_WAN_LICENSE                                  236
#define CMERROR_CMWAN_CREDENTIALS_CHANGED                       237
#define CMERROR_SERVER_START_PENDING                            238
#define CMERROR_REMOTE_ACCESS_DENIED                            239
#define CMERROR_FILE_NOT_FOUND                                  240
#define CMERROR_CHIP_INCOMPATIBLE                               241
#define CMERROR_FIELDUPDATE_DONGLE_FILE_MISFIT                  242
#define CMERROR_FIELDUPDATE_INCOMPLETE                          243

#define CMERROR_INTERNAL                                        254
#define CMERROR_UNKNOWN                                         255

/* CmActLicense related error codes */
#define CMERROR_CMACT_SUBSYSTEM_FAILED                          260
#define CMERROR_CMACT_LICENSE_NOT_ACTIVATED                     261
#define CMERROR_CMACT_LICENSE_ALREADY_ACTIVATED                 262
#define CMERROR_CMACT_LICENSE_REACTIVATION_REQUIRED             263
#define CMERROR_CMACT_LICENSE_INVALID                           264
#define CMERROR_CMACT_COMMAND_NOT_SUPPORTED                     265
#define CMERROR_CMACT_LICENSE_ALREADY_EXISTS                    266
#define CMERROR_CMACT_RUN_AS_SERVICE_REQUIRED                   267
#define CMERROR_CMACT_BINDING_NOT_POSSIBLE                      268
#define CMERROR_CMACT_OPEN_FIRMITEM_TEMPLATE_FAILED             269
#define CMERROR_CMACT_OSVERSION_NOT_PERMITTED                   270
#define CMERROR_CMACT_HOST_ID_NOT_AVAILABLE                     271
#define CMERROR_CMACT_PLUGIN_MALFUNCTION                        272
#define CMERROR_CMACT_VIRTUAL_MACHINE_NOT_ALLOWED               273
#define CMERROR_INCOMPATIBLE_SYSTEM_SOFTWARE_FOUND              274

#define CMERROR_COMBINATION_LICENSINGSYSTEM_BINDINGTYPE_PLATFORM_NOT_SUPPORTED  275

/* basic error codes */
#define CMERROR_ERRORTEXT_NOT_FOUND                             300
#define CMERROR_SEND_PROTECTION_FAILED                          301
#define CMERROR_ANALYSING_DETECTED                              302
#define CMERROR_COMMAND_EXECUTION_FAILED                        303
#define CMERROR_FUNCTION_NOT_IMPLEMENTED                        304
#define CMERROR_OBSOLETE_FUNCTION                               305
#define CMERROR_FUNCTION_NOT_SUPPORTED                          306
#define CMERROR_EXCEPTION_CAUGHT                                307
#define CMERROR_VERIFY_FAILED                                   308
#define CMERROR_REQUEST_OVERLOAD_REJECTED                       309
#define CMERROR_TIME_CONVERSION                                 310
#define CMERROR_OPERATION_NOT_SUPPORTED_FOR_UNIVERSAL_LICENSE   311
#define CMERROR_INTERNAL_COMMAND_FAILED                         312
#define CMERROR_CM_LIB_NOT_INITIALIZED                          313

#define CMERROR_DONGLE_REPLUG                                   407

/* 100..175  --->  700..775 (new higher box error codes, 1st block) */
#define CMERROR_BOX_ASN1_DECODER_FAILED                         700
#define CMERROR_BOX_ASN1_SIGNATURE_FAILED                       701
#define CMERROR_BOX_ASN1_ENCRYPT_DECRYPT_FAILED                 702
#define CMERROR_BOX_ASN1_INPUT_BUFFER_MISMATCH                  703
#define CMERROR_BOX_LTUC_WRONG                                  704
#define CMERROR_BOX_LTSM_ERROR                                  705
#define CMERROR_BOX_LT_ONLY_RESTRICTED_BCIS_ALLOWED             706

#define CMERROR_BOX_MAX_ENC_RATE_REACHED                        711
#define CMERROR_BOX_ENC_RATE_NO_RESOURCES                       712

#define CMERROR_DONGLE_FUNCTIONS_RESTRICTED                     713

#define CMERROR_BOX_INTEGRITY_ERROR                             730
#define CMERROR_BOX_INTEGRITY_WARNING                           731

/* 200..255  --->  780..835 (new higher box error codes, 2nd block) */
#define CMERROR_BOX_REPORT1_TYPE2_UNSUPPORTED                   780


#define CMERROR_WRONG_BOX_TYPE                                    MAKE_CMERROR(0xd0010002)
#define CMERROR_MUTUAL_WAIT                                       MAKE_CMERROR(0xd0010003)
#define CMERROR_LICENSE_TYPE_CMCLOUD_DEACTIVATED                  MAKE_CMERROR(0xd0010004)
#define CMERROR_NAMEDUSER_NO_MORE_ALLOCATION                      MAKE_CMERROR(0xd0010005)
#define CMERROR_NAMEDUSER_NAMELIST_NOT_FOUND                      MAKE_CMERROR(0xd0010006)
#define CMERROR_NAMEDUSER_VERSION_NOT_SUPPORTED                   MAKE_CMERROR(0xd0010007)
#define CMERROR_COUNTRY_SPECIFIC_VERSION_NOT_SUPPORTED            MAKE_CMERROR(0xd0010008)
#define CMERROR_COUNTRY_UNKNOWN                                   MAKE_CMERROR(0xd0010009)
#define CMERROR_COUNTRY_NOT_DETERMINABLE                          MAKE_CMERROR(0xd001000a)
#define CMERROR_LICENSE_IN_COUNTRY_NOT_AVAILABLE                  MAKE_CMERROR(0xd001000b)

#define CMERROR_LT_DOTRANSFER_LICENSE_TRANSFER_DEACTIVATED        MAKE_CMERROR(0xd0010010)
#define CMERROR_LT_DOTRANSFER_MAX_BORROW_DURATION_EXCEEDED        MAKE_CMERROR(0xd0010011)

#define CMERROR_REMOTE_EXECUTION_DENIED                           MAKE_CMERROR(0xd0010086)


#define CMERROR_CMACTID_DOES_NOT_MATCH                            MAKE_CMERROR(0xd0020001)


#define CMERROR_INVALID_LOGIN                                     MAKE_CMERROR(0xd0100088)
#define CMERROR_INVALID_PASSWORD                                  MAKE_CMERROR(0xd0100089)
#define CMERROR_SESSION_EXPIRED                                   MAKE_CMERROR(0xd010008a)

#define CMERROR_LT_DOTRANSFER_UNFINISHED_TRANSFER_FOUND           MAKE_CMERROR(0xd01c0061)
#define CMERROR_LT_DOTRANSFER_INSUFFICIENT_NUMBER_OF_UNITS        MAKE_CMERROR(0xd01c0062)
#define CMERROR_LT_DOTRANSFER_INSUFFICIENT_NUMBER_OF_LICENSES     MAKE_CMERROR(0xd01c0063)
#define CMERROR_LT_DOTRANSFER_TRANSFER_DEPTH                      MAKE_CMERROR(0xd01c0066)
#define CMERROR_LT_DOTRANSFER_TARGET_CMACT_ID_NOT_ALLOWED         MAKE_CMERROR(0xd01c0067)
#define CMERROR_LT_DOTRANSFER_TRANSFER_TYPE_NOT_ALLOWED           MAKE_CMERROR(0xd01c0068)

#define CMERROR_LT_DOTRANSFER_FI_TRANSFER_NOT_ALLOWED             MAKE_CMERROR(0xd01c006c)
#define CMERROR_LT_DOTRANSFER_BORROW_EXPIRATION_TIME_TOO_LARGE    MAKE_CMERROR(0xd01c006d)
#define CMERROR_LT_DOTRANSFER_BORROW_EXPIRATION_TIME_TOO_SMALL    MAKE_CMERROR(0xd01c006e)
#define CMERROR_LT_DOTRANSFER_ALREADY_COMPLETED                   MAKE_CMERROR(0xd01c006f)
#define CMERROR_LT_DOTRANSFER_CREATE_RAU_FAILED                   MAKE_CMERROR(0xd01c0073)
#define CMERROR_LT_DOTRANSFER_INVALID_RAC_CONTENT                 MAKE_CMERROR(0xd01c0074)
#define CMERROR_LT_DOTRANSFER_BORROW_TIMEDIFFTOOLARGE             MAKE_CMERROR(0xd01c0079)

#define CMERROR_LT_RAR_ACT_RECEIPT_CREATION_FAILED                MAKE_CMERROR(0xd01c0102)
#define CMERROR_LT_STATUS_DISABLED                                MAKE_CMERROR(0xd01c0111)
#define CMERROR_LT_STATUS_RETURNED                                MAKE_CMERROR(0xd01c0112)
#define CMERROR_LT_STATUS_EXPIRED                                 MAKE_CMERROR(0xd01c0113)

#define CMERROR_LT_LICENSEQUANTITY_TOO_SMALL                      MAKE_CMERROR(0xd01c0114)
#define CMERROR_LT_NO_PERMISSION_RETURN                           MAKE_CMERROR(0xd01c0151)

#define CMERROR_LT_COMMAND_NOT_SUPPORTED                          MAKE_CMERROR(0xd01c0160)


#define CMERROR_CMCLOUDCONTAINER_NOT_CONNECTED                    MAKE_CMERROR(0xdc010009)
#define CMERROR_CMCLOUDCONTAINER_SEATS_LIMIT_REACHED              MAKE_CMERROR(0xdc010200)


#define CMERROR_CMREADY_DEVICE_FULL                               MAKE_CMERROR(0xd01d0001)


/*****************************************************************************
 **************************** Global definitions *****************************
******************************************************************************/

#define CM_CHALLENGE_LEN              16
#define CM_BLOCK_SIZE                 16
#define CM_BLOCKCIPHER_KEY_LEN        32
#define CM_DIGEST_LEN                 32
#define CM_SHA256_DIGEST_LEN          32
#define CM_SHA384_DIGEST_LEN          48
#define CM_SHA512_DIGEST_LEN          64
#define CM_IP_ADDRESS_LEN             8
#define CM_PRIVATE_KEY_LEN            32
#define CM_PUBLIC_KEY_LEN             64
#define CM_SESSIONID_LEN              8
#define CM_TVB_LEN                    16
#define CM_RSA1024_KEY_LEN            128
#define CM_RSA2048_KEY_LEN            256
#define CM_RSA3072_KEY_LEN            384
#define CM_RSA4096_KEY_LEN            512
#define CM_ECCSECP224R1_KEY_LEN       64
#define CM_ECCSECP256R1_KEY_LEN       64
#define CM_ECCSECP384R1_KEY_LEN       96
#define CM_ECCSECP521R1_KEY_LEN       136
#define CM_UVD_PUBLIC_KEY_LEN         1044
#define CM_RSA_MINIMAL_KEY_LEN        8
#define CM_SYM_KEY_LEN                16
#define CM_SIGNATURE_LEN              64
#define CM_EPHEMERAL_POINT_LEN        64
#define CM_BORROW_SERVERID_LEN        8

#define CM_MAX_COMPANY_LEN            32
#define CM_MAX_STRING_LEN             256
#define CM_MAX_PASSWORD_LEN           64

#define CM_USERNAME_MAXLEN            32
#define CM_DOMAINNAME_MAXLEN          16
#define CM_HOSTNAME_MAXLEN            128
#define CM_ACCESS_CMACT_ID_LEN        4

/*****************************************************************************
 **************************** Macro definitions ******************************
******************************************************************************/

/* macro setting the complete version in a CMULONG value */
#define CM_SET_VERSION(a,b,c) (((a) & 0xff) << 24) | (((b) & 0xff) << 16) \
    | ((c) & 0xffff)
/* macro getting the major version from a complete version value (see CM_SET_VERSION) */
#define CM_GET_MAJOR_VERSION(v) (((v) >> 24) & 0xff)
/* macro getting the minor version from a complete version value (see CM_SET_VERSION) */
#define CM_GET_MINOR_VERSION(v) (((v) >> 16) & 0xff)
/* macro getting the build version from a complete version value (see CM_SET_VERSION) */
#define CM_GET_BUILD_VERSION(v) ((v) & 0xffff)

/*****************************************************************************
 ***************************** Type definitions ******************************
******************************************************************************/

#ifndef HCMSysEntryType
#define HCMSysEntryType void*
#endif

typedef HCMSysEntryType HCMSysEntry;

/***** 
 definition of gobal flags
*****/
/* 
  * used in mflFiCtrl of CMBOXENTRY
  * used in mflFiCtrl of CMBOXENTRY2
  * used in mflCtrl of CMCRYPTSIM
  * used in mflCtrl of CMPIOCK
  * used in musFirmItemTypePlain & musFirmItemTypeEncrypted of CMPROGRAM_ADD_FIRMITEM
  * used in mulFirmItemType of CMINTERNALENTRYINFO
  * used in mulFirmItemType of CMCREATEITEM
*/
#define CM_GF_FILEBASED               0x0000
#define CM_GF_BOXBASED                0x0001
#define CM_GF_TRANSFERBASED           0x0004
#define CM_GF_SEC_MASK                0x0005
#define CM_GF_FI_RESERVED             0x8000

/* 
  * used in flCtrl for CmCreateSequence()
  * used in flCtrl for CmProgram()
*/
#define CM_GF_ADD_FIRMITEM            0x0000
#define CM_GF_UPDATE_FIRMITEM         0x0001
#define CM_GF_DELETE_FIRMITEM         0x0002
#define CM_GF_ADD_PRODUCTITEM         0x0003
#define CM_GF_UPDATE_PRODUCTITEM      0x0004
#define CM_GF_DELETE_PRODUCTITEM      0x0005
#define CM_GF_SET_BOXCONTROL          0x0006
#define CM_GF_SET_BOXLOCK             0x0007
#define CM_GF_SET_FIRMKEY             0x0008
#define CM_GF_SET_USERKEY             0x0009
#define CM_GF_ADD_ENABLEBLOCK         0x000a
#define CM_GF_UPDATE_ENABLEBLOCK      0x000b
#define CM_GF_DELETE_ENABLEBLOCK      0x000c
#define CM_GF_ATTACH_ENABLEBLOCK      0x000d
#define CM_GF_DETACH_ENABLEBLOCK      0x000e
#define CM_GF_SET_ENABLEBLOCK         0x0011
#define CM_GF_ENABLEITEM              0x0012
#define CM_GF_DISABLEITEM             0x0013
#define CM_GF_TEMPENABLEITEM          0x0014
#define CM_GF_SET_BOXPASSWORD         0x002f
#define CM_GF_CHANGE_BOXPASSWORD      0x003f
#define CM_GF_RESET_BOXPASSWORD       0x004f
#define CM_GF_UPDATECHECKOUT_BORROW   0x005f
#define CM_GF_SET_BOXPASSWORD2        0x006f

#define CM_GF_ITEM_MASK               0x007f


/* 
  * used in mflSetPios of CMBOXENTRY
  * used in mflCtrl of CMENTRYDATA
  * used in musPioType of CMSECUREDATA
  * ! not used in CMBOXENTRY2, but most values are the same
*/
#define CM_GF_PRODUCTCODE             0x0001
#define CM_GF_FEATUREMAP              0x0002
#define CM_GF_EXPTIME                 0x0004
#define CM_GF_ACTTIME                 0x0008
#define CM_GF_UNITCOUNTER             0x0010
#define CM_GF_PROTDATA                0x0020
#define CM_GF_EXTPROTDATA             0x0040
#define CM_GF_HIDDENDATA              0x0080
#define CM_GF_SECRETDATA              0x0100
#define CM_GF_USERDATA                0x0200
#define CM_GF_TEXT                    0x0400
#define CM_GF_USAGEPERIOD             0x0800
#define CM_GF_LICENSEQUANTITY         0x1000
#define CM_GF_BORROWCLIENT            0x2000
#define CM_GF_BORROWSERVER            0x4000
#define CM_GF_COLI                    0x8000
#define CM_GF_PIO_MASK                0xffff

/*
  * used in mflTvbCtrl of CMCPIO_PRODUCTCODE
  * used in mflDependency of CMENTRYDATA
  * used in mfbDependencyXX of CMBOXENTRY
*/
#define CM_GF_DATA                    0x0001
#define CM_GF_FUC                     0x0002
#define CM_GF_SERIAL                  0x0004


//! time since the Epoch (00:00:00 UTC, January 1, 1970), measured in seconds
#define CM_TIME_OFFSET_TO_UNIX_TIME 946684800


/*****************************************************************************
 ************************** Structures declarations **************************
******************************************************************************/

#if defined(_WIN32) || defined(_WIN64) || (defined(__GNUC__) && __GNUC__ >= 3)
  /* set 64 Bit = 8 Byte alignment */
  #pragma pack(push, 8)
#elif defined(__GNUC__)
  #pragma pack(8)
#else
  #error "Undefined platform / Compiler. Packing not enabled!"
#endif

/*******************
 1.) Main structures
********************/

/***** CMBOXCONTROL *****/
typedef struct __CMBOXCONTROL {
  CMUSHORT musIndicatorFlags;
  CMUSHORT musSwitchFlags;
  CMULONG mulReserve;
} CMBOXCONTROL;

/***** CMBOXINFO *****/
typedef struct __CMBOXINFO {
  CMBYTE mbMajorVersion;
  CMBYTE mbMinorVersion;
  CMUSHORT musBoxMask;
  CMULONG mulSerialNumber;
  CMUSHORT musBoxKeyId;
  CMUSHORT musUserKeyId;
  CMBYTE mabBoxPublicKey[CM_PUBLIC_KEY_LEN];
  CMBYTE mabSerialPublicKey[CM_PUBLIC_KEY_LEN];
  CMULONG mulExtendedSerial;
} CMBOXINFO;

/***** CMTIME *****/
typedef struct __CMTIME {
  CMUSHORT musYear;
  CMUSHORT musMonth;
  CMUSHORT musDay;
  CMUSHORT musHours;
  CMUSHORT musMinutes;
  CMUSHORT musSeconds;
  CMULONG mulSecondsSince01_01_2000;
} CMTIME;

/*
  flags for midCps in all CMBORROWXXX structures
*/
#define CM_CPS_CODEMETER              0x00000000
#define CM_CPS_CODEMETERACT           0x00000001
#define CM_CPS_CMCLOUD                0x00000002

/***** CMBORROWCLIENT *****/
typedef struct __CMBORROWCLIENT {
  CMULONG mulStatus;
  /* data from EPD 130 */
  CMUSHORT midEnableBlock;
  CMUSHORT midCps;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMBYTE mabUpdateProgSeq[32];
  CMBYTE mabServerID[CM_BORROW_SERVERID_LEN];
  CMBYTE mabReserved1[12];
  /* data from EPD 131 */
  CMUSHORT musReserved;
  CMUSHORT musServerBoxMask;
  CMULONG mulServerBoxSerial;
  char mszServerName[CM_HOSTNAME_MAXLEN];
  CMBYTE mabReserved2[12];
} CMBORROWCLIENT;

/***** CMBORROWDATA *****/
typedef struct __CMBORROWDATA {
  CMULONG mulStatus;
  CMUSHORT midEnableBlock;
  CMUSHORT midCps;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMULONG mulFeatureMap;
  CMUSHORT musReserved;
  CMUSHORT musBoxMask;
  CMULONG mulBoxSerial;
  CMBYTE mabSerialPublicKey[CM_PUBLIC_KEY_LEN];
  CMBYTE mabServerID[CM_BORROW_SERVERID_LEN];
  char mszClientName[CM_HOSTNAME_MAXLEN];
  CMBYTE mabTrailingValidationBlock[CM_TVB_LEN];
  CMULONG mulClientRequestTime;
  CMULONG mulMaintenancePeriodStart;
  CMULONG mulMaintenancePeriodEnd;
} CMBORROWDATA;

/***** CMBORROWITEM *****/
typedef struct __CMBORROWITEM {
  /* data from EPD 132 */
  CMUSHORT midCps;
  CMUSHORT musReserved;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMULONG mulFeatureMap;
  CMULONG mcLicenseQuantity;
  CMULONG mulCheckoutDuration;
  CMBYTE mabServerID[CM_BORROW_SERVERID_LEN];
  CMBYTE mabReserved[28];
  CMUSHORT musUsedLicenses;
  CMUSHORT musFreeLicenses;
} CMBORROWITEM;


/***** CMACCESS *****/
/*
  flags for mflCtrl in CMACCESS
*/
/* flags for kind of access */
#define CM_ACCESS_USERLIMIT           0x00000000
#define CM_ACCESS_NOUSERLIMIT         0x00000100
#define CM_ACCESS_EXCLUSIVE           0x00000200
#define CM_ACCESS_STATIONSHARE        0x00000300
#define CM_ACCESS_CONVENIENT          0x00000400
/* mask for the access modes */
#define CM_ACCESS_STRUCTMASK          0x00000700

/* no validation check of the entry data */
#define CM_ACCESS_FORCE               0x00010000
/* constant for searching a fitting FSB entry */
#define CM_ACCESS_CHECK_FSB           0x00020000
/* constant for searching a fitting CTSB entry */
#define CM_ACCESS_CHECK_CTSB          0x00040000
/* allow normal subsystem access if no CmContainer is found */
#define CM_ACCESS_SUBSYSTEM           0x00080000
/* force FI access to prevent access to a FC:PC=x:0 */
#define CM_ACCESS_FIRMITEM            0x00100000

/* flag access to borrow license */
#define CM_ACCESS_BORROW_ACCESS       0x01000000
/* flag release to borrow license */
#define CM_ACCESS_BORROW_RELEASE      0x02000000
/* flag check borrowed license */
#define CM_ACCESS_BORROW_VALIDATE     0x04000000
/* flag ignore entry state for release to borrow license */
#define CM_ACCESS_BORROW_IGNORESTATE  0x08000000
  
#define CM_ACCESS_BORROW_MASK         0x0f000000
/* flag ignore Linger Time of License */
#define CM_ACCESS_IGNORE_LINGERTIME  0x10000000

/* These option constants are used in Feature Codes which are FSB-internally
   created for a specific FSB operation (mulFeatureCode). */

/* Feature Code for the <FsbEncryptAddFi> FSB operation */
#define CM_FSBFEATURECODE_ADDFI            0x00000001
/* Feature Code for the <FsbHashBoxAlg::DeleteFi> FSB operation */
#define CM_FSBFEATURECODE_DELETEFI         0x00000002
/* Feature Code for the <FsbHashBoxAlg::UpdateFi> FSB operation */
#define CM_FSBFEATURECODE_UPDATEFI         0x00000004
/* Feature Code for the <FsbHashBoxAlg::AddPi> FSB operation */
#define CM_FSBFEATURECODE_ADDPI            0x00000008
/* Feature Code for the <FsbHashBoxAlg::DeletePi> FSB operation */
#define CM_FSBFEATURECODE_DELETEPI         0x00000010
/* Feature Code for the <FsbHashBoxAlg::UpdatePi> FSB operation */
#define CM_FSBFEATURECODE_UPDATEPI         0x00000020
/* Feature Code for the <FsbHashBoxAlg::AddEb> FSB operation */
#define CM_FSBFEATURECODE_ADDEB            0x00000040
/* Feature Code for the <FsbHashBoxAlg::DeleteEb> FSB operation */
#define CM_FSBFEATURECODE_DELETEEB         0x00000080
/* Feature Code for the <FsbHashBoxAlg::UpdateEb> FSB operation */
#define CM_FSBFEATURECODE_UPDATEEB         0x00000100
/* Feature Code for the <FsbHashBoxAlg::AttachEb> FSB operation */
#define CM_FSBFEATURECODE_ATTACHEB         0x00000200
/* Feature Code for the <FsbHashBoxAlg::GetBoxTimeSign> FSB operation */
#define CM_FSBFEATURECODE_GETBOXTIMESIGN   0x00002000
/* Feature Code for the <FsbHashBoxAlg::DeleteFiSign> FSB operation */
#define CM_FSBFEATURECODE_DELETEFISIGN     0x00004000
/* Feature Code for the <FsbHashBoxAlg::ListSign> FSB operation */
#define CM_FSBFEATURECODE_LISTSIGN         0x00008000
/* Feature Code for the <FsbHashBoxAlg::Encrypt> FSB operation */
#define CM_FSBFEATURECODE_ENCRYPT          0x00010000
/* Feature Code for the <FsbPriceDeduct> FSB operation */
#define CM_FSBFEATURECODE_PRICEDEDUCT      0x00020000
/* Feature Code for storing the CmActLicense serial numbers */
#define CMACT_FSBFEATURECODE_SERIALNUMBERS 0x00040000
/* Feature Code for licenses with non binding */
#define CMACT_FSBFEATURECODE_NONELICENSES  0x00080000
/* Feature Code for trial licenses */
#define CMACT_FSBFEATURECODE_TRIALLICENSES 0x00100000
/* Feature Code for the <CreateCloudContainer> FSB operation */
#define CMCLOUD_FSBFEATURECODE_CREATECLOUDCONTAINER 0x00200000

typedef struct __CMACCESS {
  CMULONG mflCtrl;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMULONG mulFeatureCode;
  CMULONG mulUsedRuntimeVersion;
  CMULONG midProcess;
  CMUSHORT musProductItemReference;
  CMUSHORT musSession;
  unsigned char    mabIPv4Address[4];
  CMBOXINFO mcmBoxInfo;
} CMACCESS;

/* These option constants are used in CMCREDENTIAL in combination 
   with a CmGetInfo and CM_GEI_CREDENTIAL or CM_GEI_NETINFO_USER_EXT
   request and are informal (mulAccessCtrl). */

/* Identicate that an access is based on CM_ACCESS_LOCAL */
#define CM_CRED_LICENSE_LOCATION_LOCAL    0x0001 
/* Identicate that an access is based on CM_ACCESS_LAN */
#define CM_CRED_LICENSE_LOCATION_LAN      0x0002 
  /* Identicate that an access is based on CM_ACCESS_WAN */
#define CM_CRED_LICENSE_LOCATION_WAN      0x0004
#define CM_CRED_LICENSE_LOCATION_MASK     0x0007

/* Identicate that a handle is based on a Entry Access */
#define CM_CRED_HANDLE_ACCESS_ENTRY       0x0008 
/* Identicate that a handle is based on a FI Access */
#define CM_CRED_HANDLE_ACCESS_FI          0x0010
/* Identicate that a handle is based on a Box Access */
#define CM_CRED_HANDLE_ACCESS_BOX         0x0018
/* Identicate that a handle is based on a SubSystem Access */
#define CM_CRED_HANDLE_ACCESS_SUBSYSTEM   0x0020
#define CM_CRED_HANDLE_ACCESS_MASK        0x0038 

/* Type of WibuCm client library */
#define CM_LIBTYPE_WIBUCM         1
#define CM_LIBTYPE_WIBUCMJAVA     2
#define CM_LIBTYPE_WIBUCMEMBEDDED 3
#define CM_LIBTYPE_WIBUCMNET      4

/***** CMCREDENTIAL *****/
typedef struct __CMCREDENTIAL {
  CMULONG mulPID;
  CMULONG mulSession;
  CMULONG mulCleanupTime;
  CMULONG mulMaxLifeTime;
  CMULONG mulCreationTime;
  CMULONG mulAccessCtrl;
  CMULONG mulExpirationTime; 
  CMULONG mulUserDefinedID;
  char mszUserDefinedText[128];
  char mszUsername[CM_USERNAME_MAXLEN];
  CMULONG mulOtherBorrowFirmCode;
  CMULONG mulOtherBorrowProductCode;
  CMULONG mulOtherBorrowFeatureMap;
  CMULONG mulOtherBorrowSerial;
  CMUSHORT musOtherBorrowMask;
  CMBYTE mbLibType;
  CMBYTE mbCompatibilitySession;
  CMULONG mulCurrentCmInstanceUid;
  CMULONG mulPreviousCmInstanceUid;
  char mszDomainName[CM_DOMAINNAME_MAXLEN];
  CMULONG mulLibVersion;
} CMCREDENTIAL;

/***** CMACCESS2 *****/
typedef struct __CMACCESS2 {
  CMULONG mflCtrl;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMULONG mulFeatureCode; 
  CMULONG mulUsedRuntimeVersion; 
  CMULONG mulReserved1;
  CMULONG mulProductItemReference;
  CMULONG mulReserved2; 
  CMBYTE  mbMinBoxMajorVersion;
  CMBYTE  mbMinBoxMinorVersion;
  CMUSHORT musBoxMask;
  CMULONG mulSerialNumber;
  CMULONG mulLicenseQuantity; 
  CMULONG mulReserved4; 
  char    mszServername[CM_HOSTNAME_MAXLEN];
  CMTIME  mcmReleaseDate;
  CMBYTE  mabCmActId[CM_ACCESS_CMACT_ID_LEN];
  char    mabReserved[12];
  CMBORROWDATA mcmBorrowData;
  CMCREDENTIAL mcmCredential;
} CMACCESS2;

/***** CMAUTHENTICATE *****/
/*
  flags for mflCtrl in CMAUTHENTICATE
*/
/* Key Source specification */
#define CM_AUTH_FIRMKEY               0x00000000
#define CM_AUTH_HIDDENDATA            0x00000001
#define CM_AUTH_SECRETDATA            0x00000002
#define CM_AUTH_SERIALKEY             0x00000003
#define CM_AUTH_BOXKEY                0x00000004
#define CM_AUTH_PISK                  0x00000005
#define CM_AUTH_UNIVERSALDATA         0x00000006
#define CM_AUTH_KSMASK                0x00000007
/* Authentication mode specification */
#define CM_AUTH_DIRECT                0x00000000
#define CM_AUTH_EXTENDED              0x00010000
#define CM_AUTH_SERIAL                0x00020000
#define CM_AUTH_EXTENDED2             0x00030000
#define CM_AUTH_RSA                   0x00100000
#define CM_AUTH_AMMASK                0x00FF0000

typedef struct __CMAUTHENTICATE {
  CMULONG mflCtrl;
  CMULONG mulKeyExtType;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMULONG mulEncryptionCodeOptions;
  CMULONG mulFeatureCode;
  CMBOXINFO mcmBoxInfo;
  CMBYTE mabDigest[CM_DIGEST_LEN];
} CMAUTHENTICATE;

typedef struct __CMAUTHENTICATE2 {
  CMULONG mflCtrl;
  CMULONG mulKeyExtType;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMULONG mulEncryptionCodeOptions;
  CMULONG mulFeatureCode;
  CMBOXINFO mcmBoxInfo;
  CMBYTE mabDigest[CM_SHA512_DIGEST_LEN];
  CMBYTE mabReserved[64];
} CMAUTHENTICATE2;

/***** CMPUBLICKEY *****/
/*
 values for musKeyType in CMPUBLICKEY and CMPRIVATEKEY
*/
/* RSA key types */
#define CM_RSA2048 3
#define CM_RSA3072 2
#define CM_RSA4096 1
/* ECC key types */
#define CM_ECCSECP224R1 11
#define CM_ECCSECP256R1 10
#define CM_ECCSECP384R1  9
#define CM_ECCSECP521R1  7

typedef struct __CMPUBLICKEY {
  CMUSHORT musKeyType;
  union {
    struct {
      CMBYTE mabModulus[CM_RSA2048_KEY_LEN];
      CMBYTE mabExponent[CM_RSA2048_KEY_LEN];
    } rsa2048;
    struct {
      CMBYTE mabModulus[CM_RSA3072_KEY_LEN];
      CMBYTE mabExponent[CM_RSA3072_KEY_LEN];
    } rsa3072;
    struct {
      CMBYTE mabModulus[CM_RSA4096_KEY_LEN];
      CMBYTE mabExponent[CM_RSA4096_KEY_LEN];
    } rsa4096;
    struct {
      CMBYTE mabPublicKey[CM_ECCSECP224R1_KEY_LEN];
    } eccsecp224r1;
    struct {
      CMBYTE mabPublicKey[CM_ECCSECP256R1_KEY_LEN];
    } eccsecp256r1;
    struct {
      CMBYTE mabPublicKey[CM_ECCSECP384R1_KEY_LEN];
    } eccsecp384r1;
    struct {
      CMBYTE mabPublicKey[CM_ECCSECP521R1_KEY_LEN];
    } eccsecp521r1;
  } mKey;
} CMPUBLICKEY;

/***** CMPRIVATEKEY *****/

typedef struct __CMPRIVATEKEY {
  CMUSHORT musKeyType;
  union {
    struct {
      CMBYTE mabExponent[CM_RSA2048_KEY_LEN];
      CMBYTE mabPrimeP[CM_RSA2048_KEY_LEN];
      CMBYTE mabPrimeQ[CM_RSA2048_KEY_LEN];      
    } rsa2048;
    struct {
      CMBYTE mabExponent[CM_RSA3072_KEY_LEN];
      CMBYTE mabPrimeP[CM_RSA3072_KEY_LEN];
      CMBYTE mabPrimeQ[CM_RSA3072_KEY_LEN];      
    } rsa3072;
    struct {
      CMBYTE mabExponent[CM_RSA4096_KEY_LEN];
      CMBYTE mabPrimeP[CM_RSA4096_KEY_LEN];
      CMBYTE mabPrimeQ[CM_RSA4096_KEY_LEN];      
    } rsa4096;
    struct {
      CMBYTE mabPrivateKey[CM_ECCSECP224R1_KEY_LEN];
    } eccsecp224r1;
    struct {
      CMBYTE mabPrivateKey[CM_ECCSECP256R1_KEY_LEN];
    } eccsecp256r1;
    struct {
      CMBYTE mabPrivateKey[CM_ECCSECP384R1_KEY_LEN];
    } eccsecp384r1;
    struct {
      CMBYTE mabPrivateKey[CM_ECCSECP521R1_KEY_LEN];
    } eccsecp521r1;
  } mKey;
} CMPRIVATEKEY;

/***** CMBOXENTRY *****/
/*
 global flags used for mflFiCtrl, mflSetPios & mfbDependencyXX of CMBOXENTRY
*/
typedef struct __CMBOXENTRY {
  /* Firm Item data */
  CMULONG mflFiCtrl;
  CMULONG mulFirmCode;
  CMUSHORT musFirmAccessCounter;
  CMUSHORT musReserve1;
  CMULONG mulFirmUpdateCounter;
  CMULONG mulFirmPreciseTime;
  CMUSHORT mausFirmItemText[CM_MAX_STRING_LEN];
  /* flag specified the set ProductItem Option */
  CMULONG mflSetPios;
  /* Product Item data */
  CMULONG mulProductCode;
  CMULONG mulFeatureMap;
  CMULONG mulUnitCounter;
  CMTIME mcmExpirationTime;
  CMTIME mcmActivationTime;
  CMBYTE mfbDependencyPC;
  CMBYTE mfbDependencyFM;
  CMBYTE mfbDependencyUC;
  CMBYTE mfbDependencyET;
  CMBYTE mfbDependencyAT;
  CMBYTE mbReserve;
  CMUSHORT musProductItemRef;
} CMBOXENTRY;

/*
 flags for mflFiCtrl of CMBOXENTRY2
*/
#define CM_GF_FI_ALLOWED_TO_PULL      0x4000

/*
 flags for mflSetPios of CMBOXENTRY2
*/
#define CM_BE_PRODUCTCODE             CM_GF_PRODUCTCODE     /* 0x00000001 */
#define CM_BE_FEATUREMAP              CM_GF_FEATUREMAP      /* 0x00000002 */
#define CM_BE_EXPTIME                 CM_GF_EXPTIME         /* 0x00000004 */
#define CM_BE_ACTTIME                 CM_GF_ACTTIME         /* 0x00000008 */
#define CM_BE_UNITCOUNTER             CM_GF_UNITCOUNTER     /* 0x00000010 */
#define CM_BE_PROTDATA                CM_GF_PROTDATA        /* 0x00000020 */
#define CM_BE_EXTPROTDATA             CM_GF_EXTPROTDATA     /* 0x00000040 */
#define CM_BE_HIDDENDATA              CM_GF_HIDDENDATA      /* 0x00000080 */
#define CM_BE_SECRETDATA              CM_GF_SECRETDATA      /* 0x00000100 */
#define CM_BE_USERDATA                CM_GF_USERDATA        /* 0x00000200 */
#define CM_BE_TEXT                    CM_GF_TEXT            /* 0x00000400 */
#define CM_BE_USAGEPERIOD             CM_GF_USAGEPERIOD     /* 0x00000800 */
#define CM_BE_LICENSEQUANTITY         CM_GF_LICENSEQUANTITY /* 0x00001000 */
#define CM_BE_BORROWCLIENT            CM_GF_BORROWCLIENT    /* 0x00002000 */
#define CM_BE_BORROWSERVER            CM_GF_BORROWSERVER    /* 0x00004000 */
#define CM_BE_COLI                    CM_GF_COLI            /* 0x00008000 */
#define CM_BE_MAINTENANCEPERIOD       0x00010000
#define CM_BE_LINGERTIME              0x00020000
#define CM_BE_MINIMUMVERSION          0x00040000
#define CM_BE_MODULE_ITEM             0x00080000
#define CM_BE_BORROWEXPIRATIONTIME    0x00100000
#define CM_BE_HISTORY                 0x00200000
#define CM_BE_ACCESS_PASSWORD         0x00400000
#define CM_BE_TRANSFEROPTIONS         0x00800000
#define CM_BE_RETURNINFO              0x01000000
#define CM_BE_ACCEPTSPULL             0x02000000
#define CM_BE_STATUS                  0x04000000
#define CM_BE_NAMEDUSER               0x08000000
#define CM_BE_MAX_ENCRYPTION_RATE     0x10000000
#define CM_BE_UNIVERSALDATA           0x20000000
#define CM_BE_COUNTRY_SPECIFIC        0x40000000
#define CM_BE_PIO_MASK                0x7fffffff

/*
 definition of the mflLicenseQuantityFlags
*/

#define CM_LQ_DENIEDUSERLIMIT         0x00000001
#define CM_LQ_DENIEDNOUSERLIMIT       0x00000002
#define CM_LQ_DENIEDEXCLUSIVE         0x00000004
#define CM_LQ_DENIEDSTATIONSHARE      0x00000008
#define CM_LQ_ALLOWCMWAN              0x00000100
#define CM_LQ_TRIPLEMODEREDUNDANCY    0x00000200
#define CM_LQ_LOCALACCESSONLY         0x00000400
#define CM_LQ_DENIEDALL               0x0000000f
#define CM_LQ_MAXVALUE                0x0000070f

/*
 definition of the mflTransferStatus
 CM_TRANSFER_STATUS_DELETED (3) has been replaced by
 CM_TRANSFER_STATUS_RETURNED and 
 CM_TRANSFER_STATUS_EXPIRED since CM 6.30
*/
#define CM_TRANSFER_STATUS_ENABLED           1
#define CM_TRANSFER_STATUS_DISABLED          2
#define CM_TRANSFER_STATUS_RETURNED          3
#define CM_TRANSFER_STATUS_EXPIRED           4

  
/***** CMBOXENTRY2 *****/
/*
 global flags used for mflFiCtrl, mflSetPios & mfbDependencyXX of CMBOXENTRY2
*/
typedef struct __CMBOXENTRY2 {
  /* Firm Item data */
  CMULONG mflFiCtrl;
  CMULONG mulFirmCode;
  CMUSHORT musFirmAccessCounter;
  CMUSHORT musReserve1;
  CMULONG mulFirmUpdateCounter;
  CMULONG mulFirmPreciseTime;
  CMUSHORT mausFirmItemText[CM_MAX_STRING_LEN];
  CMULONG mulLicenseTransferUpdateCounter;
  CMUSHORT musBoxMask;
  CMUSHORT musReserve5;
  CMULONG mulBoxSerial;
  CMBYTE mabReserve1[88]; /* aligned to 632 */
  /* flag specified the set ProductItem Option */
  CMULONG mflSetNativePios;
  CMULONG mflSetEffectivePios;
  CMULONG mflSetPios;
  /* Product Item data */
  CMULONG mulProductCode;
  CMULONG mulFeatureMap;
  CMTIME  mcmMaintenancePeriodStart;
  CMTIME  mcmMaintenancePeriodEnd;
  CMULONG mulUnitCounter;
  CMTIME mcmExpirationTime;
  CMTIME mcmActivationTime;
  CMULONG mulUsagePeriodLifeTime;
  CMTIME mcmUsagePeriodStartTime;
  CMULONG mulLicenseQuantity;
  CMULONG mulLingerTime;
  CMULONG mulMinimumRuntimeVersion;
  CMULONG mflLicenseQuantityFlags;
  CMULONG mulLicenseTag;
  CMULONG mflTransferStatus;
  CMBYTE mabProductItemId[10];
  CMUSHORT musMaxEncryptionRate;
  CMBYTE mabReserve2[20]; /* aligned to 796 */
  CMBYTE mfbDependencyPC;
  CMBYTE mfbDependencyFM;
  CMBYTE mfbDependencyMP;
  CMBYTE mfbDependencyUC;
  CMBYTE mfbDependencyET;
  CMBYTE mfbDependencyAT;
  CMBYTE mfbDependencyUP;
  CMBYTE mfbDependencyLQ;
  CMBYTE mfbDependencyLT;
  CMBYTE mfbDependencyMV;
  CMBYTE mabReserve3[12]; /* aligned to 818 */
  CMUSHORT musModuleRef;
  CMUSHORT musProductItemRef;
  CMUSHORT musTransferBoxMask;
  CMULONG mulTransferSerialNumber;
  CMTIME mcmTransferBorrowExpirationTime;
  CMBYTE mabTransferId[10];
  CMBYTE mabReserve4[170]; /* aligned to 1024 */
} CMBOXENTRY2;

/***** CMBOXSECURITY *****/
typedef struct __CMBOXSECURITY {
  CMULONG midOem;
  CMULONG mulFsbFirmCode;
  CMULONG mulCtsbFirmCode;
  CMULONG mulReserve;
} CMBOXSECURITY;

/***** CMBOXTIME *****/
typedef struct __CMBOXTIME {
  CMTIME mcmCertifiedTime;
  CMTIME mcmBoxTime;
  CMTIME mcmSystemTime;
} CMBOXTIME;

/***** CMUSAGEPERIOD *****/
typedef struct __CMUSAGEPERIOD {
  CMULONG mulPeriod;
  CMTIME mcmStartTime;
} CMUSAGEPERIOD;

/***** CMBASECRYPT *****/

/*
  flags for mflCtrl in CMBASECRYPT
*/

/* Key Source specification */
#define CM_CRYPT_FIRMKEY              0x00000000
#define CM_CRYPT_HIDDENDATA           0x00000001
#define CM_CRYPT_SECRETDATA           0x00000002
#define CM_CRYPT_UNIVERSALDATA        0x00000008
#define CM_CRYPT_KSMASK               0x0000000F

/* encryption algorithms */
#define CM_CRYPT_AES                  0x00000000
#define CM_CRYPT_ECIES                0x01000000
#define CM_CRYPT_ECIES_STD            0x02000000
#define CM_CRYPT_AES_DIRECT           0x03000000
#define CM_CRYPT_RSA                  0x05000000
#define CM_CRYPT_ECC                  0x0B000000
#define CM_CRYPT_ALGMASK              0xff000000
/* flags for CRC calculation and checking */
#define CM_CRYPT_CHKCRC               0x00010000
#define CM_CRYPT_CALCCRC              0x00020000

/*
  flags for mulEncryptionCodeOptions in CMBASECRYPT
*/

#define CM_CRYPT_ENC_RATE             0x40000000
#define CM_CRYPT_UCMASK               0x30000000
#define CM_CRYPT_UCCHECK              0x00000000
#define CM_CRYPT_UCIGNORE             0x20000000
#define CM_CRYPT_UCREQUIRED           0x10000000
#define CM_CRYPT_ATMASK               0x0C000000
#define CM_CRYPT_ATCHECK              0x00000000
#define CM_CRYPT_ATIGNORE             0x08000000
#define CM_CRYPT_ATREQUIRED           0x04000000
#define CM_CRYPT_ETMASK               0x03000000
#define CM_CRYPT_ETCHECK              0x00000000
#define CM_CRYPT_ETIGNORE             0x02000000
#define CM_CRYPT_ETREQUIRED           0x01000000
#define CM_CRYPT_MPREQUIRED           0x00100000
#define CM_CRYPT_TOPLAINONLY          0x00200000
#define CM_CRYPT_FACDECREMENT         0x00400000
#define CM_CRYPT_CERTTIME             0x00800000
#define CM_CRYPT_RES2MASK             0x00F00000
#define CM_CRYPT_SAMASK               0x000F0000
#define CM_CRYPT_SAEXCLUSIVE          0x00080000
#define CM_CRYPT_SAUNLIMITED          0x00000000
#define CM_CRYPT_SAUSERLIMIT          0x00040000
#define CM_CRYPT_SASTATIONSHARE       0x00020000
#define CM_CRYPT_RES3MASK             0x0000C000
#define CM_CRYPT_UCDELTAMASK          0x00003FFF
#define CM_CRYPT_MAX                  0xFFFFFFFF

/***** CMBASECRYPT *****/
typedef struct __CMBASECRYPT {
  CMULONG mflCtrl;
  CMULONG mulKeyExtType;
  CMULONG mulEncryptionCode;
  CMULONG mulEncryptionCodeOptions;
  CMULONG mulFeatureCode;
  CMULONG mulCrc;
} CMBASECRYPT;

/***** CMBASECRYPT2 *****/
typedef struct __CMBASECRYPT2 {
  CMULONG mflCtrl;
  CMULONG mulKeyExtType;
  CMULONG mulEncryptionCode;
  CMULONG mulEncryptionCodeOptions;
  CMULONG mulFeatureCode;
  CMTIME  mcmReleaseDate;
  CMULONG mulCrc;
  CMUINT  muLogin;
  CMULONG maulReserved[9];
} CMBASECRYPT2;

/***** CMCRYPT *****/
typedef struct __CMCRYPT {
  CMBASECRYPT mcmBaseCrypt;
  CMBYTE mabInitKey[CM_BLOCK_SIZE];
} CMCRYPT;

/***** CMCRYPT2 *****/
typedef struct __CMCRYPT2 {
  CMBASECRYPT2 mcmBaseCrypt;
  CMBYTE mabDirectAesKey[CM_BLOCK_SIZE];
  CMBYTE mabInitKey[CM_BLOCK_SIZE];
  CMULONG maulReserved[8];
} CMCRYPT2;

/***** CMCRYPTSIM2 *****/
/*
  global flags used in mflCtrl of CMCRYPTSIM2
*/
typedef struct __CMCRYPTSIM2 {
  CMULONG mflCtrl;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMBASECRYPT2 mcmBaseCrypt;
  CMBYTE mabInitKey[CM_BLOCK_SIZE];
  CMULONG mcbExtFirmKey;
  CMBYTE mabExtFirmKey[CM_BLOCKCIPHER_KEY_LEN];
  CMULONG maulReserved[8];
} CMCRYPTSIM2;

/***** CMCRYPTSIM *****/
/*
  global flags used in mflCtrl of CMCRYPTSIM
*/
typedef struct __CMCRYPTSIM {
  CMULONG mflCtrl;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMBASECRYPT mcmBaseCrypt;
  CMBYTE mabInitKey[CM_BLOCK_SIZE];
  CMULONG mcbExtFirmKey;
  CMBYTE mabExtFirmKey[CM_BLOCKCIPHER_KEY_LEN];
} CMCRYPTSIM;

/***** CMSECUREDATA *****/
/*
 global flags also used for musPioType and musKeyExtType of CMSECUREDATA
*/
typedef struct __CMSECUREDATA {
  CMBASECRYPT mcmBaseCrypt;
  CMUSHORT musPioType;
  CMUSHORT musExtType;
  CMBYTE mabPioEncryptionKey[CM_BLOCK_SIZE];
  CMULONG mulReserve;
} CMSECUREDATA;

/***** CMPIOCK *****/
/*
  global flags used in mflCtrl of CMPIOCK
*/
typedef struct __CMPIOCK {
  CMSECUREDATA mcmSecureData;
  CMULONG mflCtrl;
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMULONG mulHiddenDataAccessCode;
  CMULONG mcbExtFirmKey;
  CMBYTE mabExtFirmKey[CM_BLOCKCIPHER_KEY_LEN];
} CMPIOCK;

/***** CMCHIPINFO *****/
typedef struct __CMCHIPINFO {
  CMULONG mulFirmwareBuild;
  CMULONG mulFirmwareBuildTop;
  CMULONG mulMdfaLba;
  CMULONG mulReserved;
  CMUSHORT musChipType;
  CMUSHORT musSiliconRevision;
  CMUSHORT midFactory;
  CMUSHORT musProductYear;
  CMUSHORT musLotNumber;
  CMUSHORT midWafer;
  CMUSHORT midChipOnWafer;
  CMUSHORT musDowngradeCount;
} CMCHIPINFO;

/***** CMUSBCHIPINFO *****/
typedef struct __CMUSBCHIPINFO {
  CMUSHORT musFirmwareMajor;
  CMUSHORT musFirmwareMinor;
  CMULONG  mulFlashSize;
  char mszNodeDescription[CM_MAX_STRING_LEN];
  CMBYTE mszNodes[CM_MAX_STRING_LEN];
} CMUSBCHIPINFO;


/***** CMNETINFOCLUSTER *****/
typedef struct __CMNETINFOCLUSTER {
  CMUINT mulSerial;
  CMUSHORT musMask;
  CMUSHORT musProductItemRef;
  CMUINT mulFirmCode;
  CMUINT mulProductCode;
  CMUINT mulFeatureMap;
  CMUINT mulUserLimitLicenses;
  CMUINT mulNoUserLimitLicenses;
  CMUINT mulExclusiveLicenses;
  CMUINT mulStationShareLicenses;
  CMUINT mulUsedLicenses;
  CMUINT mulFreeLicenses;
  CMUINT mulTotalLicenses;
  CMUINT mulLicenseQuantity;
  CMUINT mulAccessPermissions;
} CMNETINFOCLUSTER;


/***** CMNETINFOUSER *****/
typedef struct __CMNETINFOUSER {
  CMUINT mulSerial;
  CMUSHORT musMask;
  CMUSHORT musProductItemRef;
  CMUINT mulFirmCode;
  CMUINT mulProductCode;
  CMUINT mulFeatureMap;
  CMUINT mulID;
  CMUINT mulAccessMode;
  CMUINT mulCreationTime;
  char mszClientIP[32];
} CMNETINFOUSER;


/***** CMNETINFOUSER_EXT *****/
typedef struct __CMNETINFOUSER_EXT {
  CMUSHORT musMask;
  CMUSHORT mflCtrl;
  CMULONG mulSerial;
  CMULONG mulProductItemRef;
  CMULONG mulFirmCode;
  
  CMULONG mulProductCode;
  CMULONG mulFeatureMap;
  CMULONG mulID;
  CMULONG mulAccessMode;
  
  CMULONG mulLastAccessTime;
  CMULONG mulUsedLicenseQuantity;
  CMULONG mulStationShareSlot;
  CMULONG mulReserved4;
  
  char mszClientAddress[64];
  CMCREDENTIAL mcmCredential;
} CMNETINFOUSER_EXT;


/***** CMINTERNALENTRYINFO *****/
typedef struct __CMINTERNALENTRYINFO {
  CMUSHORT musFirmItemReference;
  CMUSHORT musProductItemReference;
  CMULONG mulFirmItemType;
  CMULONG mulFirmUpdateCounter;
  CMULONG mulReserve;
} CMINTERNALENTRYINFO;

/***** CMENTRYDATA *****/
/*
 global flags used for mflCtrl &  mflDependency of CMENTRYDATA
*/

typedef struct __CMENTRYDATA {
  CMULONG mflCtrl;
  CMULONG mflDependency;
  CMULONG mulReserve;
  CMUINT mcbData;
  CMBYTE mabData[2 * CM_MAX_STRING_LEN];
} CMENTRYDATA;

/***** CMMEMINFO *****/
typedef struct __CMMEMINFO {
  CMUSHORT mcFree4ByteBlock;
  CMUSHORT mcFree8ByteBlock;
  CMUSHORT mcFree16ByteBlock;
  CMUSHORT mcFree32ByteBlock;
  CMUSHORT mcFree64ByteBlock;
  CMUSHORT mcFree128ByteBlock;
  CMUSHORT mcFree256ByteBlock;
  CMUSHORT mcFree512ByteBlock;
} CMMEMINFO;


/***** CMMEMINFO2 *****/
typedef struct __CMMEMINFO2 {
  CMUSHORT mcFree4ByteBlock;
  CMUSHORT mcFree8ByteBlock;
  CMUSHORT mcFree16ByteBlock;
  CMUSHORT mcFree32ByteBlock;
  CMUSHORT mcFree64ByteBlock;
  CMUSHORT mcFree128ByteBlock;
  CMUSHORT mcFree256ByteBlock;
  CMUSHORT mcFree512ByteBlock;
  CMULONG  mulReserved1;
  CMULONG  mulReserved2;
  CMULONG  mulReserved3;
  CMULONG  mulReserved4;
  CMULONG  mulTotalBytesFree;
  CMULONG  mulCapacity;
} CMMEMINFO2;


/***** CMLICENSEINFO *****/
/*
  flags for mflCtrl in CMLICENSEINFO
*/
#define CM_LICENSE_FILEBASED          0x00000001
#define CM_LICENSE_BOXBASED           0x00000002
#define CM_LICENSE_TRANSFERBASED      0x00000004
#define CM_LICENSE_FILEBASEDCREATE    0x00010000
#define CM_LICENSE_FIRMKEY            0x00020000
#define CM_LICENSE_PUBLICFIRMKEY      0x00040000
#define CM_LICENSE_FSBUPDATEKEY       0x00080000
#define CM_LICENSE_ENCRYPTED          0x00100000
#define CM_LICENSE_BOXBASEDFSB        0x00200000

typedef struct __CMLICENSEINFO {
  CMULONG mflCtrl;
  CMULONG mulFirmCode;
  CMULONG midOem;
  CMULONG mulFsbFirmCode;
  CMULONG mulFsbProductCode;
  CMULONG mulReserve;
  char mszDescription[CM_MAX_STRING_LEN];
  char mszFirmItemText[CM_MAX_STRING_LEN];
} CMLICENSEINFO;


/***** CMRESERVEFI *****/
typedef struct __CMRESERVEFI {
  CMUSHORT musFirmItemRef;
  CMBYTE mabSessionId[2 * CM_SESSIONID_LEN];
  CMBYTE mabReserve[6];
} CMRESERVEFI;

/***** CMSIGNEDTIME *****/
typedef struct __CMSIGNEDTIME {
  CMBOXTIME mcmBoxTime;
  CMBYTE mabTrailingValidationBlock[CM_TVB_LEN];
} CMSIGNEDTIME;

/***** CMSYSTEM *****/
/*
  flags for midPlatform in CMSYSTEM
*/
#define CM_SYSTEM_W95                 0x00000000  /* 4.00 */
#define CM_SYSTEM_W98                 0x00000001  /* 4.10 */
#define CM_SYSTEM_WME                 0x00000002  /* 4.90 */
#define CM_SYSTEM_NT4                 0x00000003  /* 4.00 */
#define CM_SYSTEM_W2K                 0x00000004  /* 5.00 */
#define CM_SYSTEM_WXP                 0x00000005  /* 5.1 */
#define CM_SYSTEM_W2003               0x00000006  /* 5.2 */
#define CM_SYSTEM_VISTA               0x00000007  /* 6.0 */
#define CM_SYSTEM_W2008               0x00000008  /* 6.0 */
#define CM_SYSTEM_W7                  0x00000009  /* 6.1 */
#define CM_SYSTEM_W2008R2             0x00000010  /* 6.1 */
#define CM_SYSTEM_W8                  0x00000011  /* 6.2 */
#define CM_SYSTEM_W2012               0x00000012  /* 6.2 */
#define CM_SYSTEM_W81                 0x00000013  /* 6.3 */
#define CM_SYSTEM_W10                 0x00000014  /* 10.0 */
#define CM_SYSTEM_W2016               0x00000015  /* 10.0 */
#define CM_SYSTEM_W2012R2             0x00000016  /* 6.3  */
#define CM_SYSTEM_W2019               0x00000017  /* 10.0 */
#define CM_SYSTEM_W11                 0x00000018  /* 11.0 */
#define CM_SYSTEM_W2022               0x00000019  /* 11.0 */

  /***** CMSYSTEM *****/
  /*
   flags for midPlatform in CMSYSTEM
   */

#define CM_SYSTEM_WINDOWS             0x00000000
#define CM_SYSTEM_MACOSX              0x00000100
#define CM_SYSTEM_SOLARIS             0x00000200
#define CM_SYSTEM_WIN_CE              0x00001000
#define CM_SYSTEM_EMBEDDED            0x00002000
#define CM_SYSTEM_LINUX               0x00010000

#define CM_SYSTEM_PLATFORM_MASK       0x000FFF00
#define CM_SYSTEM_VERSION_MASK        0x000000FF
#define CM_SYSTEM_INVALID_PLATFORM    0xFFFFFFFF

#define CM_SYSTEM_LITTLE_ENDIAN       0x00100000
#define CM_SYSTEM_BIG_ENDIAN          0x00200000
#define CM_SYSTEM_ENDIAN_ORDER_MASK   0x00300000

#define CM_SYSTEM_RUNNING_IN_VM       0x01000000

#define CM_SYSTEM_RUNNING_IN_CONTAINER 0x02000000

typedef struct __CMSYSTEM {
  CMULONG midPlatform;
  CMULONG mulSystemKernelVersion;
  CMUSHORT mausIpAddress[CM_IP_ADDRESS_LEN];
  char mszComputerName[CM_MAX_STRING_LEN];
} CMSYSTEM;


  /*
   flags for mulServerFeatures in CMCOMMUNICATION
   */
  
#define CM_COMMUNICATION_LAN_SERVER             0x00000001
#define CM_COMMUNICATION_WAN_SERVER             0x00000002
#define CM_COMMUNICATION_REMOTE_FSB_ACCESS      0x00000004
  
#define CM_API_COMM_MODE_SHARED_MEM  0x00000002
#define CM_API_COMM_MODE_TCPIP_IPV4  0x00000004
#define CM_API_COMM_MODE_TCPIP_IPV6  0x00000008

  
typedef struct __CMCOMMUNICATION {
  CMULONG mulServerFeatures;
  CMUSHORT musApiCommunicationMode;
  CMBYTE mabReserved[106];
  char mszDomainName[16];
  char mszIpV4Address[32];
  char mszIpV6Address[128];
  char mszComputerName[CM_MAX_STRING_LEN];
} CMCOMMUNICATION;

  
/***** CMVERSION *****/
typedef struct __CMVERSION {
  CMUSHORT musVersion;
  CMUSHORT musSubVersion;
  CMUSHORT musBuild;
  CMUSHORT musCount;
  CMUSHORT musYear;
  CMUSHORT musMonth;
  CMUSHORT musDay;
  CMUSHORT musReserve;
} CMVERSION;

/***** CMSECURITYVERSION *****/
typedef struct __CMSECURITYVERSION {
  CMUSHORT musVersion;
  CMUSHORT musSubVersion;
  CMUSHORT musBuild;
  CMUSHORT musCount;
  CMUSHORT musYear;
  CMUSHORT musMonth;
  CMUSHORT musDay;
  CMUSHORT musType;
  CMUSHORT musReserved1;
  CMUSHORT musReserved2;
  CMULONG  mulReserved1;
  CMULONG  mulReserved2;
  CMULONG  mulReserved3;
} CMSECURITYVERSION;

/***** CMSYSTEMSTATUS *****/
typedef struct __CMSYSTEMSTATUS {
  CMULONG mflStatusFlags;
  CMULONG mulServerStartTimeInSecondsSince01_01_2000;
  CMBYTE mabReserved[72];
} CMSYSTEMSTATUS;

/*****
 definition of the mflStatusFlags flags (CMSYSTEMSTATUS / CM_GEI_SYSTEMSTATUS)
*****/
#define CM_SYSTEMSTATUS_RUNNING_AS_SERVICE  0x00000001
#define CM_SYSTEMSTATUS_CMACT_ENABLED       0x00000002
#define CM_SYSTEMSTATUS_MSD_ENABLED         0x00000004
#define CM_SYSTEMSTATUS_HID_ENABLED         0x00000008
#define CM_SYSTEMSTATUS_CMACT_AVAILABLE     0x00000010
#define CM_SYSTEMSTATUS_CMCLOUD_ENABLED     0x00000020
#define CM_SYSTEMSTATUS_RUNTIME_EXT_ENABLED 0x00000040

/*****
 definition of the mbTransferType flags (CMLTHISTORY / CM_GEI_LTHISTORY)
*****/
#define CM_HISTORY_TYPE_MOVE_COMPLETE           1
#define CM_HISTORY_TYPE_MOVE_UNITS              2
#define CM_HISTORY_TYPE_MOVE_LICENSES           3
#define CM_HISTORY_TYPE_BORROW_LOCAL_LICENSE    4
#define CM_HISTORY_TYPE_BORROW_COMPLETE         5
#define CM_HISTORY_TYPE_MERGE_UNITS             6
#define CM_HISTORY_TYPE_MERGE_LICENSES          7

/*****
 definition of the mflStatus flags (CMLTHISTORY / CM_GEI_LTHISTORY)
*****/
#define CM_HISTORY_STATUS_HIDDEN              0
#define CM_HISTORY_STATUS_IN_PUSH_TRANSIT     1
#define CM_HISTORY_STATUS_IN_RETURN_TRANSIT   2
#define CM_HISTORY_STATUS_IN_PULL_TRANSIT     3
#define CM_HISTORY_STATUS_ACTIVE              4

/***** CMLTHISTORY *****/
typedef struct __CMLTHISTORY {
  CMUSHORT musIndex;
  CMBYTE mbStatus;
  CMBYTE mbTransferType;
  CMULONG mulUnitCounter;
  CMULONG mulLicenseQuantity;
  CMULONG mulReturnedQuantity;
  CMTIME mcmTimeStamp;
  CMTIME mcmBorrowExpirationTime;
  CMBYTE mabTransferId[10];
  CMBYTE mabConfirmedTransferId[10];
  CMULONG mulSerialNumber;
  CMULONG mulLtUpdateCounter;
  CMUSHORT musBoxMask;
  CMBYTE mabReserved[26];
} CMLTHISTORY;


/***** CMLTLICENSEINFO*****/
typedef struct __CMLTLICENSEINFO
{
  CMUSHORT musIndex;
  CMBYTE mbStatus;
  CMBYTE mbTransferType;
  CMULONG mulUnitCounter;
  CMULONG mulLicenseQuantity;
  CMULONG mulReturnedQuantity;
  CMTIME mcmTimeStamp;
  CMTIME mcmBorrowExpirationTime;
  CMBYTE mabTransferId[10];
  CMBYTE mabConfirmedTransferId[10];
  CMULONG mulSerialNumber;
  CMULONG mulLtUpdateCounter;
  CMUSHORT musBoxMask;
  CMBYTE mabReserved[26];
} CMLTLICENSEINFO;




/*****
 definition of the mflFlags flags (CMLTTRANSFEROPTIONS / CM_GEI_LTTRANSFEROPTIONS)
*****/
#define CM_TRANSFER_OPTIONS_MAY_BE_PULLED              1
#define CM_TRANSFER_OPTIONS_MAY_BE_RETURNED            2
#define CM_TRANSFER_OPTIONS_NO_FI_AT_TARGET_REQUIRED   4

/*****
 definition of the mflTransferType flags (CMLTTRANSFEROPTIONS / CM_GEI_LTTRANSFEROPTIONS)
*****/
#define CM_TRANSFER_TYPE_MOVE_COMPLETE           1
#define CM_TRANSFER_TYPE_MOVE_UNITS              2
#define CM_TRANSFER_TYPE_MOVE_LICENSES           3
#define CM_TRANSFER_TYPE_BORROW_LOCAL_LICENSE    4
#define CM_TRANSFER_TYPE_BORROW_COMPLETE         5
#define CM_TRANSFER_TYPE_NONE                    6


/***** CMLTTRANSFEROPTIONS *****/
typedef struct __CMLTTRANSFEROPTIONS {
  CMBYTE mbFlags;
  CMBYTE mbTransferType;
  CMBYTE mcbTargetIds;
  CMBYTE mcbLtkTargetIds;
  CMBYTE mabTargetIds[112];
  CMBYTE mabLtkTargetIds[112];
  CMULONG mulMaxBorrowPeriod;
  CMUSHORT musTransferDepth;
  CMBYTE mabReserved[86];
} CMLTTRANSFEROPTIONS;

/***** CMNAMEDUSER struct and flags ****/

typedef struct __CMNAMEDUSER
{
  CMUSHORT musPioType;
  CMUSHORT musIndex;
  CMULONG mflDependency;
  CMULONG mulReserved;
  CMULONG mcbEntryData;
  CMBYTE mabTag[4];
  CMBYTE mbVersion;
  CMBYTE mbNextIndex;
  CMBYTE mbfModeFlags;
  CMBYTE mbReserved2;
  CMULONG mulCrc32;
  CMULONG mulNameListId;
  CMBYTE mabHash[32];
  CMBYTE mabPlaintext[128];
} CMNAMEDUSER;

#define CM_NMU_MODE_USERNAME 0x01
#define CM_NMU_MODE_USERDOMAIN 0x02
#define CM_NMU_MODE_USERDEFINED 0x03
#define CM_NMU_MODE_MASK 0x0f
#define CM_NMU_FLAG_PLAINTEXT 0x10
#define CM_NMU_FLAG_NAMELIST 0x20
#define CM_NMU_FLAG_SINGLE_ALLOCATION 0x40
#define CM_NMU_FLAG_ALLOW_OVERWRITTEN_USERINFO 0x80
#define CM_NMU_FLAG_MASK 0xf0


/***** CMPIENTRYBASE *****/
typedef struct __CMPIENTRYBASE {
  CMULONG mulProductCode;
  CMULONG mulFeatureMap;
  CMULONG mflSetEffectivePios;
  CMULONG mflSetNativePios;
  CMULONG mulMaintenancePeriodStartSecondsSince01_01_2000;
  CMULONG mulMaintenancePeriodEndSecondsSince01_01_2000;
  CMULONG mulUnitCounter;
  CMULONG mulExpirationTimeSecondsSince01_01_2000;
  CMULONG mulActivationTimeSecondsSince01_01_2000;
  CMULONG mulUsagePeriodLifeTime;
  CMULONG mulUsagePeriodStartTimeSecondsSince01_01_2000;
  CMULONG mulLicenseQuantity;
  CMULONG mulMinimumRuntimeVersion;
  CMULONG mflLicenseQuantityFlags;
  CMNAMEDUSER mcmNamedUserEntry;
  CMUSHORT musProductItemRef;
  CMUSHORT musReserved1;
  CMULONG mflTransferStatus;
} CMPIENTRYBASE;

/**********************
 2.) Special structures
***********************/

/****************************
 2.1.) Programming Structures
*****************************/

/***** CMCREATEITEM *****/
typedef struct __CMCREATEITEM {
  /* must be set for all program commands */
  CMULONG mulFirmCode;
  CMULONG mulFirmItemType;
  CMBOXINFO mcmBoxInfoUser;
  /* must be set for all program commands except AddFi */
  CMULONG mulFirmUpdateCounter;
  /* must be set for UpdatePi & DeletePi */
  CMULONG mulProductCode;
  /* must be set for BoxLock */
  CMULONG midOem;
  /* must be set for ActFieldUpd */
  CMULONG mulFirmwareBuild;
} CMCREATEITEM;

/***** CMCPIO_PRODUCTCODE *****/
/*
 global flags also used for mflTvbCtrl of CMCPIO_PRODUCTCODE
 additional flags for mflTvbCtrl of CMCPIO_PRODUCTCODE
*/
#define CM_PIO_CHANGE_PC              0x00010000

typedef struct __CMCPIO_PRODUCTCODE {
  CMULONG mflTvbCtrl;
  CMUSHORT musFirmItemReference;
  CMUSHORT musProductItemReference;
  CMULONG mulProductCode;
  CMBYTE mbMajorVersion;
  CMBYTE mbMinorVersion;
  CMUSHORT musReserve;
} CMCPIO_PRODUCTCODE;

/***** CMCPIO_EXTPROTDATA *****/
typedef struct __CMCPIO_EXTPROTDATA {
  CMUSHORT musExtType;
  CMUSHORT mcbData;
  CMULONG mulReserve;
  /* variable length, max = 256 */
  CMBYTE mabData[CM_MAX_STRING_LEN];
} CMCPIO_EXTPROTDATA;

/***** CMCPIO_HIDDENDATA *****/
typedef struct __CMCPIO_HIDDENDATA {
  CMUSHORT musExtType;
  CMUSHORT mcbTotal;
  CMUSHORT mcbData;
  CMBYTE mabReserve[6];
  CMULONG mulHiddenDataAccessCode;
  /* variable length, max = 256 */
  CMBYTE mabData[CM_MAX_STRING_LEN];
} CMCPIO_HIDDENDATA;

/***** CMCPIO_PROTDATA *****/
typedef struct __CMCPIO_PROTDATA {
  CMUSHORT mcbData;
  /* variable length, max = 256 */
  CMBYTE mabData[CM_MAX_STRING_LEN];
  CMBYTE mabReserve[6];
} CMCPIO_PROTDATA;

/***** CMCPIO_SECRETDATA *****/
typedef struct __CMCPIO_SECRETDATA {
  CMUSHORT musExtType;
  CMUSHORT mcbTotal;
  CMUSHORT mcbData;
  CMUSHORT musReserve;
  /* variable length, max = 256 */
  CMBYTE mabData[CM_MAX_STRING_LEN];
} CMCPIO_SECRETDATA;

/***** CMCPIO_TEXT *****/
typedef struct __CMCPIO_TEXT {
  CMUSHORT mcchText;
  CMBYTE mabReserve[6];
  /* variable length, max = 256 */
  CMUSHORT mausText[CM_MAX_STRING_LEN];
} CMCPIO_TEXT;

/***** CMCPIO_USERDATA *****/
typedef struct __CMCPIO_USERDATA {
  CMUSHORT mcbData;
  CMBYTE mabReserve[6];
  /* variable length, max = 256 */
  CMBYTE mabData[CM_MAX_STRING_LEN];
} CMCPIO_USERDATA;

/***** CMCPIO_MAINTENANCEPERIOD *****/
typedef struct __CMCPIO_MAINTENANCEPERIOD {
  CMTIME mcmStartPeriod;
  CMTIME mcmEndPeriod;
} CMCPIO_MAINTENANCEPERIOD;

/***** CMPROGRAM_BOXCONTROL *****/
/*
  flags for mflCtrl in CMPROGRAM_BOXCONTROL
*/
#define CM_BC_ABSOLUTE                0x0000
#define CM_BC_SWITCH                  0x0001

typedef struct __CMPROGRAM_BOXCONTROL {
  CMULONG mflCtrl;
  CMUSHORT musIndicatorFlags;
  CMUSHORT musReserve;
} CMPROGRAM_BOXCONTROL;

/***** CMPROGRAM_BOXLOCK *****/
/*
  flags for mflCtrl in CMPROGRAM_BOXLOCK
*/
#define CM_BL_LOCK                    0x0000
#define CM_BL_UNLOCK                  0x0001

typedef struct __CMPROGRAM_BOXLOCK {
  CMULONG mflCtrl;
  CMULONG mulReserve;
  CMBYTE mabUserKey[CM_BLOCKCIPHER_KEY_LEN];
} CMPROGRAM_BOXLOCK;


/***** CMPROGRAM_BOXPASSWORD *****/
typedef struct __CMPROGRAM_BOXPASSWORD {
  char mszOldPassword[CM_MAX_PASSWORD_LEN];
  char mszNewPassword[CM_MAX_PASSWORD_LEN];
} CMPROGRAM_BOXPASSWORD;

/***** CMVALIDATE_DELETEFI *****/
typedef struct __CMVALIDATE_DELETEFI {
  CMULONG mflFiCtrl;
  CMULONG mulFirmCode;
  CMULONG mulSystemTime;
  CMULONG mulFirmUpdateCounter;
  CMBYTE mabTrailingValidationBlock[CM_TVB_LEN];
} CMVALIDATE_DELETEFI;

#if defined(_WIN32) || defined(_WIN64) || (defined(__GNUC__) && __GNUC__ >= 3)
  /* set 1 Byte alignment, only for programming & enabling structures */
  #pragma pack(pop)
  #pragma pack(push, 1)
#elif defined(__GNUC__)
  #pragma pack(1)
#else
  #error "Undefined platform / Compiler. Packing not enabled!"
#endif

/***** CMPROGRAM_ADD_FIRMITEM *****/
/*
 global flags used for musFirmItemTypePlain
 & musFirmItemTypeEncrypted of CMPROGRAM_ADD_FIRMITEM
*/
typedef struct __CMPROGRAM_ADD_FIRMITEM {
  /* plain members */
  CMUSHORT musFirmItemReference;
  CMUSHORT musFirmItemTypePlain;
  CMBYTE mabPublicLicensorKey[CM_PUBLIC_KEY_LEN];
  /* encrypted members */
  CMUSHORT musFirmItemTypeEncrypted;
  CMUSHORT musFirmAccessCounter;
  CMULONG mulFirmUpdateCounter;
  CMULONG mulFirmPreciseTime;
  CMULONG mulReserved;
  CMBYTE mabFirmKey[CM_BLOCKCIPHER_KEY_LEN];
  CMBYTE mabSessionId[2 * CM_SESSIONID_LEN];
  /* plain members */
  CMUSHORT mcchText;
  /* variable length, max = 256 */
  CMUSHORT mausText[CM_MAX_STRING_LEN];
} CMPROGRAM_ADD_FIRMITEM;

/***** CMPROGRAM_UPDATE_FIRMITEM *****/
/*
  flags for musCtrl in CMPROGRAM_UPDATE_FIRMITEM
*/
#define CM_UFI_FAC                    0x0001
#define CM_UFI_FUC                    0x0002
#define CM_UFI_FPT                    0x0004
#define CM_UFI_TEXT                   0x0008
#define CM_UFI_MASK                   0x000f

typedef struct __CMPROGRAM_UPDATE_FIRMITEM {
  CMUSHORT musFirmItemReference;
  CMUSHORT musCtrl;
  CMUSHORT musFirmAccessCounter;
  CMULONG mulFirmUpdateCounter;
  CMULONG mulFirmPreciseTime;
  CMUSHORT mcchText;
  /* variable length, max = 256, expand structure on the stack */
  CMUSHORT mausText[1];
} CMPROGRAM_UPDATE_FIRMITEM;

/***** CMPROGRAM_DELETE_FIRMITEM *****/
typedef struct __CMPROGRAM_DELETE_FIRMITEM {
  CMUSHORT musFirmItemReference;
  CMBYTE mabTrailingValidationBlock[CM_TVB_LEN];
} CMPROGRAM_DELETE_FIRMITEM;

/***** CMPROGRAM_ADD_PRODUCTITEM *****/
typedef struct __CMPROGRAM_ADD_PRODUCTITEM {
  CMUSHORT musFirmItemReference;
  CMUSHORT musProductItemSuccessorReference;
  /* variable length, expand structure on the stack */
  CMBYTE mabProductItemOptionBuffer[1];
} CMPROGRAM_ADD_PRODUCTITEM;

/***** CMPROGRAM_UPDATE_PRODUCTITEM *****/
/*
  flags for musCtrl set internally!!!
*/
typedef struct __CMPROGRAM_UPDATE_PRODUCTITEM {
  CMUSHORT musFirmItemReference;
  CMUSHORT musProductItemReference;
  CMUSHORT musCtrl;
  /* variable length, expand structure on the stack */
  CMBYTE mabProductItemOptionBuffer[1];
} CMPROGRAM_UPDATE_PRODUCTITEM;

/***** CMPROGRAM_DELETE_PRODUCTITEM *****/
typedef struct __CMPROGRAM_DELETE_PRODUCTITEM {
  CMUSHORT musFirmItemReference;
  CMUSHORT musProductItemReference;
  CMBYTE mabTrailingValidationBlock[CM_TVB_LEN];
} CMPROGRAM_DELETE_PRODUCTITEM;

/*************************
 2.2.) Enabling Structures
**************************/

/***** CMENABLING_SIMPLEPIN *****/
typedef struct __CMENABLING_SIMPLEPIN {
  CMBYTE mcbEnableAccessCode;
  /* variable length, max = 16 */
  CMBYTE mabEnableAccessCode[CM_CHALLENGE_LEN];
} CMENABLING_SIMPLEPIN;

/***** CMENABLING_TIMEPIN *****/
typedef struct __CMENABLING_TIMEPIN {
  CMULONG mulDisableTime;
  CMBYTE mcbEnableAccessCode;
  CMUSHORT mcchText;
  /* variable length, AccessCode + Text added, expand structure on the stack */
  CMBYTE mabBuffer[1];
} CMENABLING_TIMEPIN;

/***** CMENABLING_SIMPLEPIN_LIST *****/
typedef struct __CMENABLING_SIMPLEPIN_LIST {
  CMBYTE mcbEnableAccessCode;
} CMENABLING_SIMPLEPIN_LIST;

/***** CMENABLING_TIMEPIN_LIST *****/
typedef struct __CMENABLING_TIMEPIN_LIST {
  CMULONG mulDisableTime;
  CMBYTE mcbEnableAccessCode;
  CMUSHORT mcchText;
  CMUSHORT mausText[CM_MAX_STRING_LEN];
} CMENABLING_TIMEPIN_LIST;

/***** CMENABLING_ENABLEBLOCKITEM *****/
/*
  flags for musOption in CMENABLING_ENABLEBLOCKITEM
*/
#define CM_EBI_VALID                  0x8000
#define CM_EBI_ENABLED                0x0000
#define CM_EBI_DISABLED               0x0001
#define CM_EBI_TEMPENABLED            0x0002
#define CM_EBI_ENABLING_MASK          0x0003
#define CM_EBI_EXPIRED                0x0100

/*
  flags for mbType in CMENABLING_ENABLEBLOCKITEM
*/
#define CM_EBI_SIMPLEPIN              0x0000
#define CM_EBI_TIMEPIN                0x0010
/*
  constants for mulDisableTime
*/
#define CM_EBI_NODISABLETIME          0xFFFFFFFF


typedef struct __CMENABLING_ENABLEBLOCKITEM {
  CMUSHORT musOption;
  CMBYTE mbIndex;
  CMBYTE mbType;
  union {
    /* variable length */
    CMENABLING_SIMPLEPIN mcmSimplePin;
    CMENABLING_TIMEPIN mcmTimePin;
    CMBYTE mabReserved[22];
  } mEnableType;
} CMENABLING_ENABLEBLOCKITEM;

/***** CMENABLING_ENABLEBLOCKITEM_LIST *****/
typedef struct __CMENABLING_ENABLEBLOCKITEM_LIST {
  CMUSHORT musOption;
  CMBYTE mbIndex;
  CMBYTE mbType;
  union {
    /* variable length */
    CMENABLING_SIMPLEPIN_LIST mcmSimplePin;
    CMENABLING_TIMEPIN_LIST mcmTimePin;
  } mEnableType;
} CMENABLING_ENABLEBLOCKITEM_LIST;

/***** CMENABLING_ENABLELOOKUPENTRY *****/
/*
  flags for musOption in CMENABLING_ENABLELOOKUPENTRY
*/
#define CM_ELUE_VALID                 0x8000
#define CM_ELUE_IFI                   0x0001
#define CM_ELUE_REQUIRED              0x0002

/*
  flags for mbEnableLevel & mbDisableLevel in CMENABLING_ENABLELOOKUPENTRY
*/
#define CM_ELUE_LOCATE                0x0000
#define CM_ELUE_READ                  0x0001
#define CM_ELUE_ENCRYPT               0x0003
#define CM_ELUE_UNITUSE               0x0005
#define CM_ELUE_MODIFY                0x0007
#define CM_ELUE_DETACH                0x00ff

typedef struct __CMENABLING_ENABLELOOKUPENTRY {
  CMUSHORT musOption;
  CMBYTE mbEnableBlockIndex;
  CMBYTE mbEnableLevel;
  CMBYTE mbDisableLevel;
  CMBYTE mbType;
  CMUSHORT musStatus;
} CMENABLING_ENABLELOOKUPENTRY;

/***** CMENABLING_WRITEADD *****/
typedef struct __CMENABLING_WRITEADD {
  /* plain members */
  CMULONG mcbLength;
  CMUSHORT musFirmItemReference;
  /* encrypted members */
  CMENABLING_ENABLEBLOCKITEM mcmEnableBlockItem;
} CMENABLING_WRITEADD;

/***** CMENABLING_WRITEUPDATE *****/
/*
  flags for mbEnableBlockIndex in CMENABLING_WRITEUPDATE
*/
#define CM_WU_FIRMKEY                 0x80
/*
  flags for musCtrl in CMENABLING_WRITEUPDATE
*/
// Annotation to Enabling:
// the flag CM_WU_DISABLED has to be set if you want to enable, disable
// or temp enable an EBI.
// If the FirmItem is capable of temp enabling, CM_WU_TEMPENABLED has to be ored
// additional, but musn't if it doesn't.
#define CM_WU_DISABLED                0x0001
#define CM_WU_TEMPENABLED             0x0002
#define CM_WU_ACCESSCODE              0x0004
#define CM_WU_DISABLETIME             0x0008
#define CM_WU_TEXT                    0x0010
#define CM_WU_IDENTITY                0x0020
#define CM_WU_FULL                    0x003f

typedef struct __CMENABLING_WRITEUPDATE {
  /* plain members */
  CMBYTE mbEnableBlockIndex;
  /* encrypted members */
  CMUSHORT musCtrl;
  CMULONG mulFirmUpdateCounter;
  CMENABLING_ENABLEBLOCKITEM mcmEnableBlockItem;
} CMENABLING_WRITEUPDATE;

/***** CMENABLING_WRITEATTACHDETACH *****/
typedef struct __CMENABLING_WRITEATTACHDETACH {
  /* plain members */
  CMBYTE mbEnableBlockIndex;
  /* encrypted members (32 Bytes) */
  CMUSHORT musFirmItemReference;
  CMUSHORT musProductItemReference;
  CMULONG mulFirmUpdateCounter;
  CMENABLING_ENABLELOOKUPENTRY mcmEnableLookupEntry;
  CMBYTE mabTrailingValidationBlock[CM_TVB_LEN];
} CMENABLING_WRITEATTACHDETACH;

/***** CMENABLING_WRITEDELETE *****/
typedef struct __CMENABLING_WRITEDELETE {
  /* plain members */
  CMUSHORT musFirmItemReference;
  CMBYTE mbEnableBlockIndex;
  CMBYTE mabTrailingValidationBlock[CM_TVB_LEN];
} CMENABLING_WRITEDELETE;

#if defined(_WIN32) || defined(_WIN64) || (defined(__GNUC__) && __GNUC__ >= 3)
  /* reset to original alignment */
  #pragma pack(pop)
  #pragma pack(push, 8)
#elif defined(__GNUC__)
  #pragma pack(8)
#else
  #error "Undefined platform / Compiler. Packing not enabled!"
#endif

/***** CMENABLING_APPCONTEXT *****/
/*
  flags for mflCtrl in CMENABLING_APPCONTEXT
*/
#define CM_AC_VALID                   0x8000
#define CM_AC_DISABLED                0x0001
#define CM_AC_TEMPENABLED             0x0002

typedef struct __CMENABLING_APPCONTEXT {
  CMULONG mulFirmCode;
  CMULONG mulProductCode;
  CMBOXINFO mcmBoxInfo;
  CMULONG mflCtrl;
  CMUSHORT midEnableBlockItem;
  CMUSHORT musReserve;
  CMULONG mulFirmUpdateCounter;
  CMULONG midIdentity;
  char mszCompanyName[CM_MAX_COMPANY_LEN];
} CMENABLING_APPCONTEXT;

/***********************************
 2.3.) Remote Programming Structures
************************************/

/***** CMSERIAL *****/
typedef struct __CMSERIAL {
  CMUINT mulSerial;
  CMUSHORT musMask;
  CMUSHORT musReserved;
} CMSERIAL;


/***********************************
 3.0.) CodeMeterAct Structures
************************************/

/*****
 definition of the mulPreferredActivationMethod flags (CMACTLICENSEINFO)
*****/
#define CMACT_ACTIVATION_INVALID      0
#define CMACT_ACTIVATION_PHONE        1
#define CMACT_ACTIVATION_EMAIL        2
#define CMACT_ACTIVATION_PORTAL       3
#define CMACT_ACTIVATION_SERVICE      4
#define CMACT_ACTIVATION_FILE         5

#define CMACT_LO_ALLOW_VM             0x00000001
#define CMACT_LO_ALLOW_REIMPORT       0x00000002
#define CMACT_LO_ALLOW_RESTORE        0x00000004

/*****
 for CmActLicenseControl( CM_GF_ACT_REMOVE, ... )
*****/
#define CM_GF_ACT_UNLOAD_LICENSE      0
#define CM_GF_ACT_REMOVE_LICENSE      1

#define CMACT_MAX_LEN_PRODUCTNAME     64
#define CMACT_MAX_LEN_VENDORTEXT      128
#define CMACT_MAX_LEN_PRODUCT_SERIAL  128
#define CMACT_MAX_LEN_ACTIVATION_CODE 64
#define CMACT_MAX_LEN_LICENSORDATA    4096
#define CMACT_MAX_LEN_PLUGIN_NAME     32

#define CMACT_LEN_PRODUCTIDMAJ        5
#define CMACT_LEN_CLIENTSECKEY        6

#define CMACT_LEN_NAMEHASH            20

#define CMACT_MAX_NUM_PRODUCT_DESCS   2000
#define CMACT_MAX_NUM_DIGESTS         4

#define CMACT_MAX_PLATFORMS           10

#define CMACT_LEN_HOSTINFOFINGERPRINT 4
#define CMACT_LEN_HOSTINFONAME       24
#define CMACT_LEN_DEVICE_ID           64

/***** CMACTPRODUCTDESCRIPTION (CmAct) *****/
typedef struct __CMACTPRODUCTDESCRIPTION {
  CMULONG mulProductCode;
  CMULONG mulFeatureMask;
} CMACTPRODUCTDESCRIPTION;

/***** CMACTLICENSEINFO / CMACTLICENSEINFO2 *****/
typedef struct __CMACTLICENSEINFO2 {
  CMUSHORT musBoxMask;
  CMUSHORT musReserved1;
  CMULONG  mulSerialNumber;
  CMULONG  mulFirmCode;
  CMULONG  mulStatus;
  CMULONG  mflLicenseOptions;
  char mszProductName[CMACT_MAX_LEN_PRODUCTNAME];
  char mszPluginName[CMACT_MAX_LEN_PLUGIN_NAME];
  CMBYTE mabNameHash[CMACT_LEN_NAMEHASH];
  CMULONG mulTelephoneId;
  char mszCmActId[CMACT_LEN_PRODUCTIDMAJ];
  CMBYTE mbReserved2;
  CMUSHORT musNumberOfProductDescriptions;
  CMACTPRODUCTDESCRIPTION maProductDescriptions[CMACT_MAX_NUM_PRODUCT_DESCS];
  char mszProductSerialNumber[CMACT_MAX_LEN_PRODUCT_SERIAL];
  CMBYTE mabDeviceId[CMACT_LEN_DEVICE_ID];
  CMBYTE mabReserved3[192];
} CMACTLICENSEINFO2;

typedef struct __CMACTLICENSEINFO {
  CMUSHORT musBoxMask;
  CMUSHORT musReserved1;
  CMULONG  mulSerialNumber;
  CMULONG  mulFirmCode;
  CMULONG  mulStatus;
  CMULONG  mflLicenseOptions;
  CMULONG  mulReserved2;
  CMULONG  mulReserved3;
  // PHONE = 1, EMAIL = 2, PORTAL = 3, SERVICE = 4, FILE = 5
  CMULONG  mulPreferredActivationMethod; 
  char mszProductName[CMACT_MAX_LEN_PRODUCTNAME];
  char mszProductNameExt[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorName[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorPhone[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorEMail[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorWebPortal[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorWebService[CMACT_MAX_LEN_VENDORTEXT];
  char mszPluginName[CMACT_MAX_LEN_PLUGIN_NAME];
  CMBYTE mabReserved3[96];
  CMULONG mulProductIdMinor;
  CMULONG mulReserved4;
  char mszProductIdMajor[CMACT_LEN_PRODUCTIDMAJ];
  CMBYTE mbReserved1;
  CMUSHORT musNumberOfProductDescriptions;
  CMACTPRODUCTDESCRIPTION maProductDescriptions[CMACT_MAX_NUM_PRODUCT_DESCS];
  char mszProductSerialNumber[CMACT_MAX_LEN_PRODUCT_SERIAL];
} CMACTLICENSEINFO;


/***** CMACTLICENSEDATA (CmAct) *****/

typedef struct __CMACTLICENSEDATA {
  CMULONG mflCtrl;
  CMUSHORT musBoxMask;
  CMBYTE mbSmartBindHeuristic;
  CMBYTE mbSmartBindRedundancyLevel;
  CMULONG mulSerialNumber;
  CMULONG mulFirmCode;
  CMULONG mulBindingFlags;
  CMULONG mnNumberOfRequiredBindingFlags;
  CMULONG mflLicenseOptions;
  CMULONG mulGeneration;
  CMULONG mulReplacesGeneration;
  CMBYTE mabSymKeyPhone[CM_SYM_KEY_LEN];
  CMBYTE mabLicensorPubKey[CM_PUBLIC_KEY_LEN];
  CMBYTE mabSigLicensorPubKey[CM_SIGNATURE_LEN];
  char mszProductName[CMACT_MAX_LEN_PRODUCTNAME];
  CMULONG mulProductIdMinor;
  char mszProductIdMajor[CMACT_LEN_PRODUCTIDMAJ];
  CMBYTE mbReserved2;
  CMUSHORT musReserved4;
  CMULONG mulPreferredActivationMethod;
  char mszProductNameExt[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorName[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorPhone[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorEMail[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorWebPortal[CMACT_MAX_LEN_VENDORTEXT];
  char mszVendorWebService[CMACT_MAX_LEN_VENDORTEXT];
  char mszPluginName[CMACT_MAX_LEN_PLUGIN_NAME];
  CMULONG mulCertifiedTime;
  CMBYTE mabReserved6[88];
  CMUSHORT musMinRequiredRuntimeVer;  
  CMUSHORT musMinRequiredRuntimeSubVer;
  CMUSHORT musNumberOfProductDescriptions;
  CMUSHORT musReserved3;
  CMACTPRODUCTDESCRIPTION maProductDescriptions[CMACT_MAX_NUM_PRODUCT_DESCS];
  CMBYTE mabBoxTemplateKey[CM_SYM_KEY_LEN];
  CMBYTE mabSymKeyBoxMemory[CM_SYM_KEY_LEN];
  CMBYTE mabSymKeyDynData[CM_SYM_KEY_LEN];
  CMULONG mulNumberOfPermittedPlatforms;
  CMULONG maulPermittedPlatforms[CMACT_MAX_PLATFORMS]; // see flags for midPlatform in CMSYSTEM
  CMULONG mulReserved5;
} CMACTLICENSEDATA;

/***** CMACTDIGEST (CmAct) *****/
typedef struct __CMACTDIGEST {
  CMUSHORT musPaPuCount;
  CMUSHORT musDigestCount;
  CMULONG mulReserved1;
  CMULONG mulReserved2;
  CMULONG mulReserved3;
  CMBYTE mabDigest[CMACT_MAX_NUM_DIGESTS][CM_DIGEST_LEN];  //!< one or more digest
} CMACTDIGEST;


/***** CMACTSIGNATURE (CmAct) *****/
typedef struct __CMACTSIGNATURE {
  CMUSHORT musPaPuCount;
  CMUSHORT musSignatureCount;
  CMULONG mulReserved1;
  CMULONG mulReserved2;
  CMULONG mulReserved3;
  CMBYTE  mabSignature[CMACT_MAX_NUM_DIGESTS][CM_SIGNATURE_LEN]; //!< one or more signatures
} CMACTSIGNATURE;


/***** CMACTACTIVATION (CmAct) *****/
typedef struct __CMACTACTIVATION {
  CMULONG mflCtrl;
  char mszInstallationId[CMACT_MAX_LEN_ACTIVATION_CODE];
  char mszProductSerialNumber[CMACT_MAX_LEN_PRODUCT_SERIAL];
  CMBYTE mabSymKeyPhone[CM_SYM_KEY_LEN];
  CMBYTE mabClientSecKey[CMACT_LEN_CLIENTSECKEY];
  CMUSHORT musReserved2;
  CMUSHORT musBoxMask;
  CMUSHORT musReserved1;
  CMULONG mulSerialNumber;
  CMULONG mulProductIdMinor;
  CMULONG mulGeneration;
  CMUSHORT musPaPuCount;
  CMBYTE mabReserved[34];
} CMACTACTIVATION;

/***** CMACTREQUEST *****/
typedef struct __CMACTREQUEST {
  char mszProductSerialNumber[CMACT_MAX_LEN_PRODUCT_SERIAL];
  CMULONG mulProductSubType;
  CMULONG mulReserved1;
  CMBYTE mabLicensorData[CMACT_MAX_LEN_LICENSORDATA];
} CMACTREQUEST;


/***** CMACTSYMKEY (CmAct) *****/

typedef struct __CMACTSYMKEY {
  CMBYTE mabSymKey[CM_SYM_KEY_LEN];
  CMBYTE mabEphemeralPoint[CM_EPHEMERAL_POINT_LEN];
} CMACTSYMKEY;

/*****
 definition of the mulFileType flags (CMACTFILEINFO/CMACTFILEINFO2)
*****/
#define CMACT_FILETYPE_LICENSEINFORMATIONFILE           1
#define CMACT_FILETYPE_LICENSEINFORMATIONFILE_BOXMEMORY 2 
#define CMACT_FILETYPE_LICENSEREQUESTFILE               3
#define CMACT_FILETYPE_LICENSEACTIVATIONFILE            4
#define CMACT_FILETYPE_LICENSEFILE                      5
#define CMACT_FILETYPE_LICENSEREQUESTFILE_BOXMEMORY     6

/***** CMACTFILEINFO / CMACTFILEINFO2 (CmAct) *****/
typedef struct __CMACTFILEINFO2 {
  CMULONG mulFileType;
  CMULONG mulFirmCode;
  char mszProductName[CMACT_MAX_LEN_PRODUCTNAME];
  CMUSHORT musBoxMask;
  CMUSHORT musReserved1;
  CMULONG mulSerialNumber;
  CMULONG mulTelephoneId;
  char mszCmActId[CMACT_LEN_PRODUCTIDMAJ]; 
  CMBYTE mbReserved2;
  CMUSHORT musReserved3;
  CMULONG mulGeneration;
  char mszProductSerialNumber[CMACT_MAX_LEN_PRODUCT_SERIAL];
  CMBYTE mabLicensorInfo[CMACT_MAX_LEN_LICENSORDATA];
  CMULONG mulStatus;
  char mszPluginName[CMACT_MAX_LEN_PLUGIN_NAME];
  CMULONG mflLicenseOptions;
  CMULONG mulPlatform;
  CMUSHORT musNumberOfProductDescriptions;
  CMUSHORT musReserved4;
  CMBYTE mabNameHash[CMACT_LEN_NAMEHASH];
  CMACTPRODUCTDESCRIPTION maProductDescriptions[CMACT_MAX_NUM_PRODUCT_DESCS];
  CMACTSYMKEY mSymKeyBoxMemory;
  CMACTSYMKEY mSymKeyDynData;
  char mszHostInfoName[CMACT_LEN_HOSTINFONAME];
  CMUSHORT mausHostInfoFingerPrint[CMACT_LEN_HOSTINFOFINGERPRINT];
  CMBYTE mabReserved5[224];
} CMACTFILEINFO2;

typedef struct __CMACTFILEINFO {
  CMULONG mulFileType;
  CMULONG mulFirmCode;
  char mszProductName[CMACT_MAX_LEN_PRODUCTNAME];
  CMUSHORT musBoxMask;
  CMUSHORT musReserved1;
  CMULONG mulSerialNumber;
  CMULONG mulProductIdMinor;
  char mszProductIdMajor[CMACT_LEN_PRODUCTIDMAJ]; 
  CMBYTE mbReserved2;
  CMUSHORT musReserved3;
  CMULONG mulGeneration;
  char mszProductSerialNumber[CMACT_MAX_LEN_PRODUCT_SERIAL];
  CMBYTE mabLicensorInfo[CMACT_MAX_LEN_LICENSORDATA];
  CMULONG mulStatus;
  char mszPluginName[CMACT_MAX_LEN_PLUGIN_NAME];
  CMULONG mflLicenseOptions;
  CMULONG mulPlatform;
  CMUSHORT musNumberOfProductDescriptions;
  CMUSHORT musReserved5;
  CMACTPRODUCTDESCRIPTION maProductDescriptions[CMACT_MAX_NUM_PRODUCT_DESCS];
  CMACTSYMKEY mSymKeyBoxMemory;
  CMACTSYMKEY mSymKeyDynData;
} CMACTFILEINFO;

/***** CMACTERRORINFO *****/
typedef struct __CMACTERRORINFO {
  CMULONG mulErrorCode;
  CMULONG mulReserved1;
  char mszAdditionalInfo[512];
} CMACTERRORINFO;

#define CLOUD_URL_SIZE 1024

/***** CMCLOUDLICENSEINFO *****/
typedef struct __CMCLOUDLICENSEINFO {
  CMUSHORT musBoxMask;
  CMUSHORT musReserved1;
  CMULONG mulSerialNumber;
  CMULONG mulExtendedSerial;
  CMULONG mulFirmCode;
  CMULONG mulStatus;
  char mszUrl[CLOUD_URL_SIZE];
  char mszCmActId[CMACT_LEN_PRODUCTIDMAJ];
  CMBYTE mabReserved2[63];
} CMCLOUDLICENSEINFO;

/***********************************
 4.0.) Remote Update Structures
************************************/

/*
** for CMFAS.mflSelect
*/

#define CMFAS_SELECT_MASK             0x0000000F
#define CMFAS_SELECT_WILDCARD         0x00000000
#define CMFAS_SELECT_SERIALNUMBER     0x00000001
#define CMFAS_SELECT_FIRMCODE         0x00000002

#define CMFAS_OPT_MASK                0x10000000
#define CMFAS_OPT_IGNORE              0x10000000

/*
** for CMFAS.mulItemType
*/

#define CMFAS_ITEMTYPE_LICENSE             0x00000001
#define CMFAS_ITEMTYPE_CMACT_LICENSE       0x80000001
#define CMFAS_ITEMTYPE_CMHW_LICENSE        0x40000001
#define CMFAS_ITEMTYPE_CMACTLT_LICENSE     0x20000001
#define CMFAS_ITEMTYPE_CMCLOUD_LICENSE     0x10000001
#define CMFAS_ITEMTYPE_CMFIRM_WBC          0x00000002
#define CMFAS_ITEMTYPE_CMACT_FI_TEMPLATE   0x80000004
#define CMFAS_ITEMTYPE_CMCLOUD_CREDENTIALS 0x00000008

/***** CMFAS *****/
typedef struct __CMFAS
{
  CMULONG mflSelect;
  CMULONG mulItemType;

  // Serial number (CMFAS_CTL_SELECT_SERIALNUMBER)
  CMULONG mulSerial;
  CMUSHORT musMask;
  CMUSHORT musReserved1;

  // FC (CMFAS_CTL_SELECT_FIRMCODE)
  CMULONG mulFirmCode;

  CMBYTE mabDeviceId[CMACT_LEN_DEVICE_ID];
  CMULONG maulReserved5[4];

  // result
  CMULONG mulResult;

  CMULONG maulReserved4[10];
} CMFAS;


#if defined(_WIN32) || defined(_WIN64) || (defined(__GNUC__) && __GNUC__ >= 3)
  /* set 1 Byte alignment, only for programming & enabling structures */
  #pragma pack(pop)
  #pragma pack(push, 1)
#elif defined(__GNUC__)
  #pragma pack(1)
#else
  #error "Undefined platform / Compiler. Packing not enabled!"
#endif


/***** CMDISCCONFIGURATION *****/
typedef struct __CM_DISC_CONFIGURATION {
  CMBYTE mbMajorFirmwareRevision;
  CMBYTE mbMinorFirmwareRevision;
  CMUSHORT musBytesPerSector;
  CMULONG mulLengthOfBlock0;
  CMULONG mulLengthOfBlock1;
  CMULONG mulLengthOfBlock2;
  CMULONG mulLengthOfBlock3;
  
  CMULONG mulFlashSize;
  CMULONG mulSerialNumber;
  CMULONG mulEnablingStatus;
  
  CMBYTE mabReserved[12];
} CM_DISC_CONFIGURATION;

/***** CM_DISC_SECTOR *****/
typedef struct __CM_DISC_SECTOR {
  CMULONG mulNumberOfRequestedSectors;
  CMULONG mulSectorOffset;
  CMBYTE mabReserved[56];
} CM_DISC_SECTOR;

/***** CMDISCSTATUS *****/
typedef struct __CM_DISC_STATUS {
  CMULONG mulErrorCode;
  CMULONG mulEnablingBits;
  CMULONG mulDiscSectorsTotal;
  CMULONG mulWriteOnceCount;
  CMBYTE  mabReserved[64];
} CM_DISC_STATUS;



// Flags for mbItemType of CMENABLING_CONTEXT
  #define CM_EC_PRODUCTITEM     0x01
  #define CM_EC_FIRMITEM        0x02
  #define CM_EC_GLOBALITEM      0x04
  
// musOption of CMENABLING_CONTEXT is the same as musStatus of CMENABLING_ENABLELOOKUPENTRY
//           is the same as musOption of CMENABLING_ENABLEBLOCKITEM
// mbType of CMENABLING_CONTEXT is the same as mbType of CMENABLING_ENABLEBLOCKITEM
// musFlags of CMENABLING_CONTEXT is the same as musOption of CMENABLING_ENABLELOOKUPENTRY
// mbEnableLevel und bDisableLevel entsprechen denen aus CMENABLING_ENABLELOOKUPENTRY

typedef struct __CMENABLING_CONTEXT {
  CMULONG  mulFirmcode;
  CMULONG  mulProductcode;
  CMUSHORT musOption; /*enable / temp enable / disable*/
  CMBYTE   mbIndex;
  CMBYTE   mbType;
  CMULONG  mulDisableTime; 
  CMUSHORT mausText[CM_MAX_STRING_LEN];
  CMUSHORT musFlags;
  CMBYTE   mbItemType;
  CMBYTE   mcbEnableAccessCode;
  CMBYTE   mabEnableAccessCode[CM_CHALLENGE_LEN];
  CMUSHORT mausEnablePassword[CM_MAX_PASSWORD_LEN];
  CMBYTE   mbEnableLevel;
  CMBYTE   mbDisableLevel;
} CMENABLING_CONTEXT;
  
#if defined(_WIN32) || defined(_WIN64) || (defined(__GNUC__) && __GNUC__ >= 3)
  /* reset to original alignment */
  #pragma pack(pop)
#elif defined(__GNUC__)
  /* no reset of alignment available */
#else
  #error "Undefined platform / Compiler. Packing not enabled!"
#endif

/*****************************************************************************
 ************************* Functions predeclarations *************************
******************************************************************************/

#if defined(_CODEMETER_API_STATIC_LIB)

/* initialization routine for the static lib */
CODEMETER_API void CMAPIENTRY CmStaticLibOnInit();

/* cleanup routine for the static lib */
CODEMETER_API void CMAPIENTRY CmStaticLibOnExit();

#endif

/*************************************
 1.) Access functions for CmContainers
**************************************/

/*****
 definition of CmAccess and the belonging options (flCtrl)
*****/
#define CM_ACCESS_LOCAL               0x0000
#define CM_ACCESS_LAN                 0x0001
#define CM_ACCESS_LOCAL_LAN           0x0002
#define CM_ACCESS_LAN_LOCAL           0x0003
#define CM_ACCESS_LANWAN              0x0011
#define CM_ACCESS_LOCAL_LANWAN        0x0012
#define CM_ACCESS_LANWAN_LOCAL        0x0013
#define CM_ACCESS_CMDMASK             0x0013

CODEMETER_API HCMSysEntry CMAPIENTRY CmAccess(CMULONG flCtrl, CMACCESS *pcmAcc);

CODEMETER_API HCMSysEntry CMAPIENTRY CmAccess2(CMULONG flCtrl, CMACCESS2 *pcmAcc);

/*****
 definition of CmRelease 
*****/
CODEMETER_API int CMAPIENTRY CmRelease(HCMSysEntry hcmse);

/**********************
 2.) Security functions
***********************/


/*****
 definition of CmCheckEvents and the belonging events (flEvents)
*****/
#define  CM_CE_BOXREMOVED              0x00000001
#define  CM_CE_BOXREPLACED             0x00000002
#define  CM_CE_BOXADDED                0x00000004
#define  CM_CE_NETWORKLOST             0x00000008
#define  CM_CE_NETWORKREPLACED         0x00000010
#define  CM_CE_ENTRYMODIFIED           0x00000020
#define  CM_CE_THRESHOLD_UNITCOUNTER   0x00000040
#define  CM_CE_THRESHOLD_EXPDATE       0x00000080
#define  CM_CE_SERVER_TERMINATED       0x00000100
#define  CM_CE_BOXENABLED              0x00000200
#define  CM_CE_BOXDISABLED             0x00000400
#define  CM_CE_ENTRYALTERED            0x00000800
#define  CM_CE_RELEASE_EVENT           0x00800000
#define  CM_CE_RESERVED                0x80000000

CODEMETER_API CMULONG CMAPIENTRY CmCheckEvents(CMULONG flEvents);

/*****
 definition of CmCrypt & CmCryptSim and the belonging options (flCtrl)
*****/
/* commands for direct encryption */
#define CM_CRYPT_DIRECT_ENC           0x0000
#define CM_CRYPT_DIRECT_DEC           0x0001
/* commands for indirect encryption */
#define CM_CRYPT_STREAM               0x0003
#define CM_CRYPT_AES_ENC_ECB          0x0004
#define CM_CRYPT_AES_DEC_ECB          0x0005
#define CM_CRYPT_AES_ENC_CFB          0x0006
#define CM_CRYPT_AES_DEC_CFB          0x0007
#define CM_CRYPT_AES_ENC_CBC          0x0008
#define CM_CRYPT_AES_DEC_CBC          0x0009

#define CM_CRYPT_AES_ENC_CBC_DIRECT   0x000a
#define CM_CRYPT_AES_DEC_CBC_DIRECT   0x000b

/* command mask for extracting the upper commands from other flags */
#define CM_CRYPT_CMDMASK              0x000f
/* This flag can be or'ed with all upper commands for indirect encryption */
#define CM_CRYPT_AUTOKEY              0x0100

CODEMETER_API int CMAPIENTRY CmCrypt(HCMSysEntry hcmse, CMULONG flCtrl,
    CMCRYPT *pcmCrypt, void* pvDest, CMUINT cbDest);

CODEMETER_API int CMAPIENTRY CmCryptSim(HCMSysEntry hcmse, CMULONG flCtrl,
    CMCRYPTSIM *pcmCryptSim, void* pvDest, CMUINT cbDest);
    
CODEMETER_API int CMAPIENTRY CmCrypt2(HCMSysEntry hcmse, CMULONG flCtrl,
    CMCRYPT2 *pcmCrypt2, void* pvDest, CMUINT cbDest);

CODEMETER_API int CMAPIENTRY CmCryptSim2(HCMSysEntry hcmse, CMULONG flCtrl,
    CMCRYPTSIM2 *pcmCryptSim2, void* pvDest, CMUINT cbDest);

/***** 
 definition of CmCryptEcies
*****/
CODEMETER_API int CMAPIENTRY CmCryptEcies(CMBYTE* pbPublicKey,
    CMUINT cbPublicKey, CMBYTE* pbData, CMUINT cbData);

CODEMETER_API int CMAPIENTRY CmCryptEcies2(CMULONG flCtrl, const CMPUBLICKEY* pPublicKey,
    const CMBYTE* pbPlainData, CMUINT cbPlainData,
    CMBYTE* pbOut, CMUINT cbOut);

/***** 
 definition of CmCalcualtePioCoreKey
*****/
CODEMETER_API int CMAPIENTRY CmCalculatePioCoreKey(HCMSysEntry hcmse,
    CMPIOCK *pcmPioCK, CMBYTE* pbPioCK, CMUINT cbPioCK);

/***** 
 definition of CmGetSecureData
*****/
CODEMETER_API int CMAPIENTRY CmGetSecureData(HCMSysEntry hcmse, CMSECUREDATA *pcmSecureData,
    CMENTRYDATA *pcmEntryData);

/***** 
 definition of CmGetSecureData
*****/
CODEMETER_API int CMAPIENTRY CmGetPioDataKey(
    CMBYTE* pabPiodkDest, CMUINT cbPiodkDest,
    CMBYTE* pabPioCk, CMUINT cbPioCk,
    CMBYTE* pabPioEk, CMUINT cbPioEk);

/***** 
 definition of CmDecryptPioData
*****/
CODEMETER_API int CMAPIENTRY CmDecryptPioData(CMBYTE* pabDest, CMUINT cbDest,
    CMBYTE* pabPiodkDest, CMUINT cbPiodkDest);

/************************************
 3.) CmContainer management functions
*************************************/

/*****
 definition of CmGetBoxContents and CmGetBoxContents2 and the belonging options (flCtrl)
*****/
#define  CM_GBC_ALLENTRIES             0x0000
#define  CM_GBC_FI                     0x0001
#define  CM_GBC_BOX                    0x0002
#define  CM_GBC_USELOCALTIME           0x1000

CODEMETER_API int CMAPIENTRY CmGetBoxContents(HCMSysEntry hcmse, CMULONG flCtrl,
    CMULONG ulFirmCode, CMBOXINFO *pcmBoxInfo,
    CMBOXENTRY *pcmBoxEntry, CMUINT cbBoxEntry);

CODEMETER_API int CMAPIENTRY CmGetBoxContents2(HCMSysEntry hcmse, CMULONG flCtrl,
    CMULONG ulFirmCode, CMBOXINFO *pcmBoxInfo,
    CMBOXENTRY2 *pcmBoxEntry, CMUINT cbBoxEntry);

/*****
 definition of CmGetBoxes and the belonging ports (idPort)
 CM_GB_DONGLE is the new name for CM_GB_USB (everything that is not CM_ACT)
*****/
#define  CM_GB_ALLPORTS                0x0000
#define  CM_GB_DONGLE                  0x0001
#define  CM_GB_USB                     CM_GB_DONGLE
#define  CM_GB_SIM                     0x0002
#define  CM_GB_ACT                     0x0004
#define  CM_GB_CLOUD                   0x0010
#define  CM_GB_PORT_MASK               0x001F

CODEMETER_API int CMAPIENTRY CmGetBoxes(HCMSysEntry hcmse, CMULONG idPort,
    CMBOXINFO *pcmBoxInfo, CMUINT cbBoxInfo);


#define CM_SEC_LICENSOR 2


/*****
 definition of CmGetInfo and the belonging options (flCtrl)
*****/
#define CM_GEI_BOXCONTROL             0x0000
#define CM_GEI_BOXINFO                0x0001
#define CM_GEI_BOXSECURITY            0x0002
#define CM_GEI_BOXTIME                0x0003
#define CM_GEI_ENTRYDATA              0x0004
#define CM_GEI_ENTRYINFO              0x0005
#define CM_GEI_INTERNALENTRYINFO      0x0006
#define CM_GEI_MEMINFO                0x0007
#define CM_GEI_SIGNEDLIST             0x0008
#define CM_GEI_SIGNEDTIME             0x0009
#define CM_GEI_SYSTEM                 0x000a
#define CM_GEI_VERSION                0x000b
#define CM_GEI_ENABLEBLOCKITEMS       0x000c
#define CM_GEI_ENABLELOOKUPITEMS_FI   0x000d
#define CM_GEI_ENABLELOOKUPITEMS_PI   0x000e
#define CM_GEI_CHIPINFO               0x000f
#define CM_GEI_BOXSTATUS              0x0010
#define CM_GEI_USBCHIPINFO            0x0011
#define CM_GEI_NETINFO_CLUSTER        0x0012
#define CM_GEI_NETINFO_USER           0x0013
#define CM_GEI_CREDENTIAL             0x0014
#define CM_GEI_SECURITYVERSION        0x0015
#define CM_GEI_NETINFO_USER_EXT       0x0016
#define CM_GEI_MEMINFO2               0x0017
#define CM_GEI_ACT_LICENSE_INFO       0x0018
#define CM_GEI_ACT_ERROR_INFO         0x0019
#define CM_GEI_CMACTVERSION           0x0020
#define CM_GEI_BORROWCLIENT           0x0030
#define CM_GEI_BORROWDATA             0x0031
#define CM_GEI_BORROWITEMS            0x0032
#define CM_GEI_ENTRYINFO2             0x0033
#define CM_GEI_ENABLEINFO             0x0034
#define CM_GEI_ACT_LICENSE_INFO2      0x0035
#define CM_GEI_SYSTEMSTATUS           0x0036
#define CM_GEI_COMMUNICATION          0x0037
#define CM_GEI_LTHISTORY              0x0038
#define CM_GEI_LTTRANSFEROPTIONS      0x0039
#define CM_GEI_LISTALLPI              0x003a
#define CM_GEI_NAMEDUSER              0x0040
#define CM_GEI_PITEXT                 0x0041
#define CM_GEI_RESET_TIME             0x0042
#define CM_GEI_SPIDEVICEINFO          0x0043
#define CM_GEI_CLOUD_LICENSE_INFO     0x0044

#define CM_GEI_CMDMASK                0x00ff
#define CM_GEI_USELOCALTIME           0x1000
#define CM_GEI_INDEXMASK              0xff000000


/*****
 definition of the BoxStatus flags (CM_GEI_BOXSTATUS)
*****/
#define CM_BOXSTATUS_LOWMEMORY         0x00000001
#define CM_BOXSTATUS_REPLUG            0x00000002
#define CM_BOXSTATUS_HASFLASH          0x00000004
#define CM_BOXSTATUS_ISCMACT           0x00000008
#define CM_BOXSTATUS_REMOVABLE         0x00000010
#define CM_BOXSTATUS_BATTERY_POWERED_CLOCK_USED 0x00000020
#define CM_BOXSTATUS_USES_HID_COMMUNICATION 0x00000040
#define CM_BOXSTATUS_USES_SPI_COMMUNICATION 0x00000080
#define CM_BOXSTATUS_FEATURE_MASK      0x004000FF

#define CM_ACTSTATUS_FILE              0x00000100
#define CM_ACTSTATUS_LOADED            0x00000200
#define CM_ACTSTATUS_ACTIVATE_BY_CODE  0x00000400
#define CM_ACTSTATUS_ACTIVATE_BY_FILE  0x00000800
#define CM_ACTSTATUS_ACTIVE            0x00001000
#define CM_ACTSTATUS_PROGRAMMABLE      0x00002000
#define CM_ACTSTATUS_INVALID           0x00004000
#define CM_ACTSTATUS_BROKEN            0x00008000
#define CM_ACTSTATUS_PSN_REQUIRED      0x02000000
#define CM_ACTSTATUS_VM_DETECTED       0x04000000 
#define CM_ACTSTATUS_BLACKLISTED       0x10000000 

  /* CM_ACTSTATUS_VM_DETECTED has two meanings: When returned by
     CM_GEI_BOXSTATUS it's an error condition; when returned by
     CM_GF_ACT_GETFILEINFO, the file was created in a VM (which
     was permitted) */
#define CM_BOXSTATUS_CMACT_MASK        0x1F00FF00

#define CM_BOXSTATUS_ENABLED           0x00000000
#define CM_BOXSTATUS_DISABLED          0x00010000
#define CM_BOXSTATUS_TEMPENABLED       0x00020000
#define CM_BOXSTATUS_NOENABLEITEMS     0x00040000
#define CM_BOXSTATUS_AMBIGUOUS         0x00080000
#define CM_BOXSTATUS_MASK              0x000f0000

#define CM_BOXSTATUS_STATE_FUM         0x00100000
#define CM_BOXSTATUS_STATE_LOCKED      0x00200000
#define CM_BOXSTATUS_STATE_MASK        0x00300000
  
#define CM_BOXSTATUS_ISCMCLOUD         0x00400000
#define CM_CLOUDSTATUS_DISCONNECTED    0x00800000


CODEMETER_API int CMAPIENTRY CmGetInfo(HCMSysEntry hcmse, CMULONG flCtrl,
    void *pvDest, CMUINT cbDest);

/*****
 definition of CmGetLicenseInfo 
*****/
CODEMETER_API int CMAPIENTRY CmGetLicenseInfo(HCMSysEntry hcmse,
    CMLICENSEINFO *pcmLicenseInfo, CMUINT cbNumberOfLicenses);

/*****
 definition of CmGetServers and the belonging options (flCtrl) 
*****/
#define CM_GS_LIST                    0x0000
#define CM_GS_SINGLE_FIRST            0x0001
#define CM_GS_SINGLE                  0x0002
#define CM_GS_CMDMASK                 0x0003

#define CM_GS_IPADDR                  0x0200
#define CM_GS_NAME                    0x0010
#define CM_GS_ALL_SERVERS             0x0020
#define CM_GS_REMOTE_SERVERS_ONLY     0x0040
#define CM_GS_IPADDR_IPv6MAPPED       0x0080
#define CM_GS_INCLUDE_CMWAN_SERVERS   0x0100

CODEMETER_API int CMAPIENTRY CmGetServers(CMULONG flCtrl, char *pszServer,
    CMUINT cbServer, CMUINT *pnNumberOfServers);

CODEMETER_API int CMAPIENTRY CmReadProfilingEntry( HCMSysEntry hcmse, CMULONG flCtrl, 
    const char * pszEntryName, char * pszValue, CMUINT cbValue );

/*****
 definition of CmGetVersion 
*****/
CODEMETER_API CMULONG CMAPIENTRY CmGetVersion(HCMSysEntry hcmse);

/******************************
 4.) Error management functions
*******************************/

/*****
 definition of CmGetLastErrorCode
*****/
CODEMETER_API int CMAPIENTRY CmGetLastErrorCode();

/***** 
 definition of CmGetLastErrorText/CmGetLastErrorText2 and the belonging options (flCtrl)
*****/
#define CM_GLET_ERRORTEXT             0x0000
#define CM_GLET_DIALOG                0x0010
#define CM_GLET_MASK                  0x00FF
#define CM_GLET_LOCAL_ENCODING        0x0000
#define CM_GLET_UTF8_ENCODING         0x0100
#define CM_GLET_WCHAR_ENCODING        0x0200
#define CM_GLET_ENCODING_MASK         0x0F00

CODEMETER_API int CMAPIENTRY CmGetLastErrorText(CMULONG flCtrl,
    char *pszErrorText, CMUINT cbErrorText);

CODEMETER_API int CMAPIENTRY CmGetLastErrorText2(CMULONG flCtrl,
    void *pvErrorText, CMUINT cbErrorText);

/***** 
 definition of CmSetLastErrorCode
*****/
CODEMETER_API void CMAPIENTRY CmSetLastErrorCode(int idError);

/*************************************
 5.) CmContainer programming functions
**************************************/

/***** 
 definition of CmReserveFirmItem
  * global flags used for flCtrl
*****/
CODEMETER_API int CMAPIENTRY CmReserveFirmItem(HCMSysEntry hcmse,
    CMULONG ulFirmCode, CMRESERVEFI *pcmReserveFi);

/*****
 definition of CmCreateProductItemOption and the belonging options (flCtrl) 
*****/
/* flags specifing the PIO to add, update or delete */
#define CM_CPIO_PRODUCTCODE           0x00000000
#define CM_CPIO_FEATUREMAP            0x00000001
#define CM_CPIO_EXPTIME               0x00000002
#define CM_CPIO_ACTTIME               0x00000003
#define CM_CPIO_UNITCOUNTER           0x00000004
#define CM_CPIO_PROTDATA              0x00000005
#define CM_CPIO_EXTPROTDATA           0x00000006
#define CM_CPIO_HIDDENDATA            0x00000007
#define CM_CPIO_SECRETDATA            0x00000008
#define CM_CPIO_USERDATA              0x00000009
#define CM_CPIO_TEXT                  0x0000000a
#define CM_CPIO_DELETEALL             0x0000000b
#define CM_CPIO_CHANGEDEPENDENCY      0x0000000c
#define CM_CPIO_USAGEPERIOD           0x0000000d
#define CM_CPIO_MAINTENANCEPERIOD     0x0000000e
#define CM_CPIO_CMDMASK               0x0000000f
/* flags combined only with CM_CPIO_PRODUCTCODE */
#define CM_CPIO_ADD                   0x00010000
#define CM_CPIO_UPDATE                0x00020000
/* flag combined with any CM_CPIO_...-flag */
#define CM_CPIO_DELETE_PIO            0x00100000
#define CM_CPIO_RELATIVE              0x00200000
/* flag combined with any CM_CPIO_...-flag but only called once altogether */
#define CM_CPIO_TERMINATE             0x00400000

CODEMETER_API int CMAPIENTRY CmCreateProductItemOption(HCMSysEntry hcmse,
    CMULONG flCtrl, void *pvPio, CMUINT cbPio);

/***** 
 definition of CmCreateSequence
  * global flags used for flCtrl
*****/
CODEMETER_API int CMAPIENTRY CmCreateSequence(HCMSysEntry hcmse, CMULONG flCtrl,
    CMCREATEITEM *pcmCreateItem, void *pvCtrl, CMUINT cbCtrl);

/***** 
 definition of CmProgram
  * global flags used for flCtrl
*****/
#define CM_PROG_DEFRAGMEM             0x00010000
#define CM_PROG_MASTERPASSWORD        0x00020000

CODEMETER_API int CMAPIENTRY CmProgram(HCMSysEntry hcmse,
    CMULONG flCtrl, void *pvCtrl, CMUINT cbCtrl, 
    void *pvVerify, CMUINT cbVerify);

/***** 
 definition of CmValidateEntry
*****/
#define CM_VAL_SIGNEDLIST             0x00000000
#define CM_VAL_SIGNEDTIME             0x00000001
#define CM_VAL_DELETE_FI              0x00000002
#define CM_VAL_CMDMASK                0x00000003

CODEMETER_API int CMAPIENTRY CmValidateEntry(HCMSysEntry hcmse, CMULONG flCtrl,
    CMBOXINFO *pcmBoxInfo, void *pvValidate, CMUINT cbValidate); 


/********************************
 6.) Remote-Programming functions
*********************************/

/***** 
 definition of CmGetRemoteContext/CmGetRemoteContext2/CmGetRemoteContextBuffer
*****/
#define CM_RMT_OVERWRITE              0x00000000
#define CM_RMT_APPEND                 0x00000001
#define CM_RMT_MASK                   0x000000ff
#define CM_RMT_LOCAL_ENCODING         0x00000000
#define CM_RMT_UTF8_ENCODING          0x00000100
#define CM_RMT_WCHAR_ENCODING         0x00000200
#define CM_RMT_ENCODING_MASK          0x00000f00

CODEMETER_API int CMAPIENTRY CmGetRemoteContext(const char *pszRacFile,
    CMUINT flCtrl, HCMSysEntry hcmBox, CMUINT *pulFirmCodes,
    CMUINT cbFirmCodes);

CODEMETER_API int CMAPIENTRY CmGetRemoteContext2(HCMSysEntry hcmBox, 
    CMUINT flCtrl, void *pvRacFile, CMUINT *pulFirmCodes,
    CMUINT cbFirmCodes);

CODEMETER_API int CMAPIENTRY CmGetRemoteContextBuffer(HCMSysEntry hcmBox, 
    CMUINT flCtrl, CMUINT *pulFirmCodes, CMUINT cbFirmCodes,
    void* pvMemBuffer, CMUINT cbMemBuffer, CMUINT* pcbWritten);

/***** 
 definition of CmSetRemoteUpdate/CmSetRemoteUpdate2/CmSetRemoteUpdateBuffer
*****/
CODEMETER_API int CMAPIENTRY CmSetRemoteUpdate(const char *pszRauFile,
    HCMSysEntry hcmBox);

CODEMETER_API int CMAPIENTRY CmSetRemoteUpdate2(HCMSysEntry hcmBox,
    CMUINT flCtrl, void *pvRauFile);

CODEMETER_API int CMAPIENTRY CmSetRemoteUpdateBuffer(HCMSysEntry hcmBox,
    CMUINT flCtrl, void* pvMemBuffer, CMUINT cbMemBuffer);

/***** 
 definition of CmListRemoteUpdate/CmListRemoteUpdate2/CmListRemoteUpdateBuffer
*****/
CODEMETER_API int CMAPIENTRY CmListRemoteUpdate(const char *pszRauFile,
    CMSERIAL *pcmSerials, CMUINT cbSerials);

CODEMETER_API int CMAPIENTRY CmListRemoteUpdate2(CMUINT flCtrl,
    void *pvRauFile, CMSERIAL *pcmSerials, CMUINT cbSerials);

CODEMETER_API int CMAPIENTRY CmListRemoteUpdateBuffer(CMUINT flCtrl, 
    void* pvMemBuffer, CMUINT cbMemBuffer, CMSERIAL *pcmSerials, CMUINT cbSerials);

/***** 
 definition of CmExecuteRemoteUpdate
*****/

/*
** For flCtrl
*/

#define CMFAS_CTL_CMD_MASK            0x00000003
#define CMFAS_CTL_CMD_LIST            0x00000000
#define CMFAS_CTL_CMD_UPDATE          0x00000001
#define CMFAS_CTL_CMD_UPDATE_EXISTING 0x00000002
#define CMFAS_CTL_CMD_GET_COUNT       0x00000003

#define CMFAS_CTL_OPT_MASK            0x00000010
#define CMFAS_CTL_OPT_ABORT_ON_ERROR  0x00000010

CODEMETER_API int CMAPIENTRY CmExecuteRemoteUpdate(
  HCMSysEntry hcmSubSystem,
  CMULONG flCtrl, void* pMemBuffer, CMUINT cbMemBuffer,
  CMFAS * pCmFasSelect, CMUINT cbCmFasSelect,
  CMFAS * pCmFasResult, CMUINT cbCmFasResult,
  CMUINT *pcbCmFasResultCount
  );

/**********************
 7.) Enabling functions
***********************/

/***** 
 definition of CmEnablingWriteApplicationKey and the belonging options (flCtrl)
*****/
#define CM_EW_ADD                     0x0000
#define CM_EW_UPDATE                  0x0001
#define CM_EW_DELETE                  0x0002
#define CM_EW_ATTACH                  0x0003
#define CM_EW_DETACH                  0x0004
#define CM_EW_CMDMASK                 0x0007

CODEMETER_API int CMAPIENTRY CmEnablingWriteApplicationKey(HCMSysEntry hcmse,
    CMULONG flCtrl, void *pvCtrl, CMUINT cbCtrl);

/***** 
 definition of CmEnablingGetApplicationContext
*****/
CODEMETER_API int CMAPIENTRY CmEnablingGetApplicationContext(HCMSysEntry hcmse,
    CMENABLING_APPCONTEXT *pcmAppContext);

/***** 
 definition of CmEnablingGetChallenge
*****/
CODEMETER_API int CMAPIENTRY CmEnablingGetChallenge(HCMSysEntry hcmse,
    CMENABLING_APPCONTEXT *pcmAppContext, CMBYTE *pbChallenge,
    CMUINT cbChallenge);

/***** 
 definition of CmEnablingSendResponse
*****/
CODEMETER_API int CMAPIENTRY CmEnablingSendResponse(HCMSysEntry hcmse,
    CMENABLING_APPCONTEXT *pcmAppContext, CMBYTE *pbResponse,
    CMUINT cbResponse);

/***** 
 CmEnablingWithdrawAccessRights is obsolete since CM v5.20
*****/

/***************************************************
 8.) Functions for time and date update in a CmContainer 
****************************************************/

/***** 
 definition of CmSetCertifiedTimeUpdate
*****/
CODEMETER_API int CMAPIENTRY CmSetCertifiedTimeUpdate(HCMSysEntry hcmse, const char *pszCtcsServer);


/****************************
 9.) Authentication functions 
*****************************/

/***** 
 definition of CmCalculateDigest
*****/
CODEMETER_API int CMAPIENTRY CmCalculateDigest(const CMBYTE *pbInput, CMUINT cbInput,
    CMBYTE *pbDigest, CMUINT cbDigest);

/***** 
 definition of CmCalculateSignature
*****/
CODEMETER_API int CMAPIENTRY CmCalculateSignature(HCMSysEntry hcmse,
    const CMAUTHENTICATE *pcmAuth, CMBYTE *pbSignature, CMUINT cbSignature);

/***** 
 definition of CmCalculateSignature2
*****/
CODEMETER_API int CMAPIENTRY CmCalculateSignature2(HCMSysEntry hcmse, CMUINT uLogin,
    const CMAUTHENTICATE2 *pcmAuth, CMBYTE *pbSignature, CMUINT cbSignature);

/***** 
 definition of CmGetPublicKey
*****/
CODEMETER_API int CMAPIENTRY CmGetPublicKey(HCMSysEntry hcmse,
    const CMAUTHENTICATE *pcmAuth, CMBYTE *pbPubKey, CMUINT cbPubKey);

CODEMETER_API int CMAPIENTRY CmGetPublicKey2(HCMSysEntry hcmse, CMUINT uLogin,
    const CMAUTHENTICATE2 *pcmAuth, CMPUBLICKEY *pPubKey);

/***** 
 definition of CmValidateSignature
*****/
CODEMETER_API int CMAPIENTRY CmValidateSignature(const CMAUTHENTICATE *pcmAuth,
    const CMBYTE *pbSignature, CMUINT cbSignature,
    const CMBYTE *pbPubKey, CMUINT cbPubKey);

CODEMETER_API int CMAPIENTRY CmValidateSignature2(const CMAUTHENTICATE2 *pcmAuth,
    const CMBYTE *pbSignature, CMUINT cbSignature, const CMPUBLICKEY *pPubKey);

/****************************
10.) Helper functions 
*****************************/

/***** 
 definition of CmConvertString, CmConvertTime, and the belonging options (flCtrl)
*****/
#define CM_CONVERT_UTF8_TO_WCHAR        0x0001
#define CM_CONVERT_UTF8_TO_UCS2LE       0x0002
#define CM_CONVERT_UTF8_TO_ASCII        0x0003
#define CM_CONVERT_UTF8_TO_UTF16        0x0004
#define CM_CONVERT_UTF8_TO_LOCAL        0x0005

#define CM_CONVERT_WCHAR_TO_UCS2LE      0x0010
#define CM_CONVERT_WCHAR_TO_UTF8        0x0011
#define CM_CONVERT_WCHAR_TO_ASCII       0x0012
#define CM_CONVERT_WCHAR_TO_UTF16       0x0013
#define CM_CONVERT_WCHAR_TO_LOCAL       0x0014

#define CM_CONVERT_UCS2LE_TO_WCHAR      0x0020
#define CM_CONVERT_UCS2LE_TO_UTF8       0x0021
#define CM_CONVERT_UCS2LE_TO_ASCII      0x0022
#define CM_CONVERT_UCS2LE_TO_UTF16      0x0023
#define CM_CONVERT_UCS2LE_TO_LOCAL      0x0024

#define CM_CONVERT_ASCII_TO_WCHAR       0x0030
#define CM_CONVERT_ASCII_TO_UCS2LE      0x0031
#define CM_CONVERT_ASCII_TO_UTF8        0x0032
#define CM_CONVERT_ASCII_TO_UTF16       0x0033
#define CM_CONVERT_ASCII_TO_LOCAL       0x0034

#define CM_CONVERT_UTF16_TO_UTF8        0x0040
#define CM_CONVERT_UTF16_TO_UCS2LE      0x0041
#define CM_CONVERT_UTF16_TO_WCHAR       0x0042
#define CM_CONVERT_UTF16_TO_ASCII       0x0043
#define CM_CONVERT_UTF16_TO_LOCAL       0x0044

#define CM_CONVERT_LOCAL_TO_UTF8        0x0050
#define CM_CONVERT_LOCAL_TO_UCS2LE      0x0051
#define CM_CONVERT_LOCAL_TO_WCHAR       0x0052
#define CM_CONVERT_LOCAL_TO_ASCII       0x0053
#define CM_CONVERT_LOCAL_TO_UTF16       0x0054

#define CM_CONVERT_SECONDS_TO_CALENDAR  0x0081
#define CM_CONVERT_CALENDAR_TO_SECONDS  0x0082

#define CM_CONVERT_MASK                 0x00FF

CODEMETER_API int CMAPIENTRY CmConvertString(CMUINT flCtrl,
    const void *pszSrc, void *pszDst, CMUINT cbDst);

CODEMETER_API int CMAPIENTRY CmConvertTime(CMULONG flCtrl,
    CMTIME *pcmtime);

/****************************
11.) CmAct License managment
*****************************/

#define CM_GF_ACT_CREATE_LICENSE_INFO     0x0011
#define CM_GF_ACT_SETUP_LICENSE           0x0012
#define CM_GF_ACT_SETUP_LICENSE_STEP1     0x0013
#define CM_GF_ACT_SETUP_LICENSE_STEP2     0x0014
#define CM_GF_ACT_REGISTER                0x0015
#define CM_GF_ACT_UPDATE_LICENSE_DATA     0x0016
#define CM_GF_ACT_ACTIVATION_CODE         0x0017
#define CM_GF_ACT_LICENSE_BY_CODE         0x0018
#define CM_GF_ACT_REQUEST                 0x0019
#define CM_GF_ACT_CREATE_ACTIVATION_STEP1 0x001a
#define CM_GF_ACT_CREATE_ACTIVATION_STEP2 0x001b
#define CM_GF_ACT_CREATE_ACTIVATION_STEP3 0x001c
#define CM_GF_ACT_REMOVE                  0x001d
#define CM_GF_ACT_GETFILEINFO             0x001e
#define CM_GF_ACT_GET_INSTALLATION_ID     0x001f
#define CM_GF_ACT_SPLIT_INSTALLATION_ID   0x0020
#define CM_GF_ACT_GET_TEMPLATE_KEY        0x0021
#define CM_GF_ACT_SPLIT_ACTIVATION_CODE   0x0022 
#define CM_GF_ACT_GETFILEINFO2            0x0023
#define CM_GF_ACT_CREATE_ACTIVATION_STEP2A 0x0024

#define CM_GF_ACT_MASK                    0x007f

CODEMETER_API int CMAPIENTRY CmActLicenseControl(HCMSysEntry hcmse,
    CMULONG flCtrl, const void *pvData, CMUINT cbData,
    void *pvReturn, CMUINT cbReturn);


/****************************
12.) License Borrowing
*****************************/

CODEMETER_API int CMAPIENTRY CmBorrow(HCMSysEntry hcmse, 
    CMULONG flCtrl, const char *pszServer);

/****************************
13.) Revalidation of boxes and licenses
*****************************/

CODEMETER_API int CMAPIENTRY CmRevalidateBox( HCMSysEntry hcmse, 
    CMULONG flCtrl );

#define CM_REVALIDATE_DEFAULT                 0x00000000

#define CM_REVALIDATE_LICENSOR_SPECIFIC_MASK  0xF0000000
#define CM_REVALIDATE_LICENSOR_BIT28          0x10000000
#define CM_REVALIDATE_LICENSOR_BIT29          0x20000000
#define CM_REVALIDATE_LICENSOR_BIT30          0x40000000
#define CM_REVALIDATE_LICENSOR_BIT31          0x80000000

#define CM_REVALIDATE_WIBU_SYSTEMS_MASK       0x0FFFFFFF
#define CM_REVALIDATE_BASIC                   0x00000001

/****************************
14.) Secure Disc Handling
*****************************/

/*
** Enabling flags for CmSecureDisc feature
*/

#define CM_SECURE_DISC_STATUS             0x00000200
#define CM_SECURE_DISC_READ               0x00000201
#define CM_SECURE_DISC_WRITE              0x00000202
#define CM_GET_DISC_CONFIGURATION         0x00000203

#define CM_SECURE_DISC_VERIFY_MASK        0x00030000
/* These flags may be or'ed with CM_SECURE_DISC_WRITE. 'Auto' is the default */
#define CM_SECURE_DISC_VERIFY_ON          0x00010000
#define CM_SECURE_DISC_VERIFY_OFF         0x00020000
#define CM_SECURE_DISC_VERIFY_AUTO        0x00000000


CODEMETER_API int CMAPIENTRY CmSecureDiscRead(HCMSysEntry hcmse, CMULONG flCtrl, 
    CM_DISC_STATUS *status, const CM_DISC_SECTOR *sector, CMBYTE *data, CMUINT cbData);

CODEMETER_API int CMAPIENTRY CmSecureDiscWrite(HCMSysEntry hcmse, CMULONG flCtrl, 
    CM_DISC_STATUS *status, const CM_DISC_SECTOR *sector, const CMBYTE *data, CMUINT cbData);

CODEMETER_API int CMAPIENTRY CmExtendedDiscControl(HCMSysEntry hcmse, CMULONG flCtrl,
    void *data, CMUINT cbData);


/****************************
15.) License Transfer Global Definitions
*****************************/

/*
  flags for flCtrl of CmLtXyz() functions
*/
#define CM_LT_FSB                           1
#define CM_LT_PUSH                          2
#define CM_LT_RETURN                        3
#define CM_LT_PULL                          4
#define CM_LT_RENEWBORROW                   5
#define CM_LT_INITPULL                      0x00001000
#define CM_LT_CMDMASK_LOW                   (CM_LT_FSB | CM_LT_PUSH | CM_LT_RETURN | CM_LT_PULL | CM_LT_RENEWBORROW)
#define CM_LT_CMDMASK                       (CM_LT_CMDMASK_LOW | CM_LT_INITPULL)


/*
  flags for mflCtrl of CMLTREQUEST
*/
#define CM_LTR_NORECEIPT                    0x00000100

/***** CMLTREQUEST *****/
typedef struct __CMLTREQUEST {
  CMULONG mflCtrl;
  CMULONG mulFirmCode;
  CMBYTE  mabReserved[248];
} CMLTREQUEST;


/*
  flags for mflCtrl of CMLTTRANSFER
*/
#define CM_LTT_MOVECOMPLETE                 1
#define CM_LTT_MOVEUNITS                    2
#define CM_LTT_MOVELICENSES                 3
#define CM_LTT_BORROWLOCALLICENSE           4
#define CM_LTT_BORROWCOMPLETE               5
#define CM_LTT_CMDMASK                      7

/***** CMLTTRANSFER *****/
typedef struct __CMLTTRANSFER {
  CMULONG mflCtrl;
  CMULONG mcUnitsToTransfer;
  CMULONG mcLicenseQuantityToTransfer;
  CMTIME  mcmBorrowExpirationTime;
  CMUSHORT midPIOHistory;
  CMBYTE  mabReserved[234];
} CMLTTRANSFER;


/*
  flags for flCtrl of CmLtCleanup()
*/
#define CM_LTCU_DELETED       0x00000001
#define CM_LTCU_DISABLED      0x00000002
#define CM_LTCU_HIDDENHISTORY 0x00000004
#define CM_LTCU_CONTAINER     0x00000008
#define CM_LTCU_ALL           0xffffffff


CODEMETER_API int CMAPIENTRY CmLtCreateContext(HCMSysEntry hcmse, CMULONG flCtrl,
  const CMLTREQUEST *pcmltRequest, const void *pContextForPull, CMUINT cbContextForPull, void *pContext, CMUINT cbContext );

CODEMETER_API int CMAPIENTRY CmLtDoTransfer(HCMSysEntry hcmse, CMULONG flCtrl,
    CMLTTRANSFER *pcmltTransfer, const void *pRequest, CMUINT cbRequest, void *pUpdate, CMUINT cbUpdate);

CODEMETER_API int CMAPIENTRY CmLtImportUpdate(HCMSysEntry hcmse, CMULONG flCtrl,
    const void *pUpdate, CMUINT cbUpdate);

CODEMETER_API int CMAPIENTRY CmLtCreateReceipt(HCMSysEntry hcmse, CMULONG flCtrl,
    const CMLTREQUEST *pcmltRequest, void *pAcknowledge, CMUINT cbAcknowledge);

CODEMETER_API int CMAPIENTRY CmLtConfirmTransfer(HCMSysEntry hcmse, CMULONG flCtrl,
    const void *pAcknowledge, CMUINT cbAcknowledge);

CODEMETER_API int CMAPIENTRY CmLtCleanup(HCMSysEntry hcmse, CMULONG flCtrl);

CODEMETER_API int CMAPIENTRY CmLtLiveTransfer(HCMSysEntry hcmseFrom, HCMSysEntry hcmseTo,
    CMULONG flCtrl, const CMLTTRANSFER *pcmltTransfer);


/****************************
16.) Universal Data 
*****************************/

/*****
 * definition of PIO UvD Key sub-content type constants
 *****/

#define CM_UVD_SCT_RSA_4096 1
#define CM_UVD_SCT_RSA_3072 2
#define CM_UVD_SCT_RSA_2048 3
#define CM_UVD_SCT_AES_256  5
#define CM_UVD_SCT_AES_128  6

#define CM_UVD_IS_ACTIVE               0xffff0010
#define CM_UVD_IS_GENERATED            0xffff0020
#define CM_UVD_READ_RIGHT              0xffff0030
#define CM_UVD_WRITE_RIGHTS            0xffff0040
#define CM_UVD_USE_RIGHTS              0xffff0050
#define CM_UVD_NO_UPDATE               0xffff0060
#define CM_UVD_NO_READ                 0xffff0070
#define CM_UVD_INTEGRITY               0xffff0080
#define CM_UVD_READ_PW1                0xffff0090
#define CM_UVD_READ_PW2                0xffff00a0
#define CM_UVD_WRITE_PW1               0xffff00b0
#define CM_UVD_WRITE_PW2               0xffff00c0
#define CM_UVD_USE_PW1                 0xffff00d0
#define CM_UVD_USE_PW2                 0xffff00e0
#define CM_UVD_CONTENT_TYPE            0xffff00f0
#define CM_UVD_SUB_CONTENT_TYPE        0xffff0100
#define CM_UVD_DATA_LEN                0xffff0110
#define CM_UVD_DATA                    0xffff0120
#define CM_UVD_IS_KEY                  0xffff0130
#define CM_UVD_IS_PASSWORD             0xffff0140
#define CM_UVD_IS_BLOCKCIPHER          0xffff0150
#define CM_UVD_ALGO_TYPE               0xffff0160
#define CM_UVD_IS_PK_CIPHER            0xffff0170
#define CM_UVD_BLOCK_LEN               0xffff0180
#define CM_UVD_KEY_LEN                 0xffff0190
#define CM_UVD_MESSAGE_LEN             0xffff01a0
#define CM_UVD_MODULUS_LEN             0xffff01b0
#define CM_UVD_EXPONENT_LEN            0xffff01c0
#define CM_UVD_MODULUS                 0xffff01d0
#define CM_UVD_EXPONENT                0xffff01e0
#define CM_UVD_PASSWORD                0xffff01f0


/* 
 * used in flCtrl for CmLoginUvd
 */

#define CM_UVD_VERIFY_PASSWORD         0x00000001
#define CM_UVD_KEEP_PASSWORD           0x00000002

/*
 * used in flCtrl for CmWriteUvd
 */

#define CM_UVD_WRITE_DATA              0x00000001
#define CM_UVD_WRITE_PASSWORD          0x00000002
#define CM_UVD_WRITE_KEY               0x00000004
#define CM_UVD_GENERATE_KEY            0x00000104
#define CM_UVD_DESTROY_KEY             0x00000204
#define CM_UVD_WRITE_SUBTYPE           0x00000010
#define CM_UVD_WRITE_IN_PLACE          0x00000020

CODEMETER_API CMUINT CMAPIENTRY CmLoginUvd(HCMSysEntry hcmse, CMULONG flCtrl, CMUSHORT usIndex, 
    const CMBYTE *pbPassword, CMUINT cbPassword);

CODEMETER_API int CMAPIENTRY CmLogoutUvd(CMUINT uLogin);

CODEMETER_API int CMAPIENTRY CmNewSessionUvd(CMUINT uLogin);

CODEMETER_API int CMAPIENTRY CmReadUvd(HCMSysEntry hcmse, CMULONG flCtrl, CMUSHORT usIndex, CMUINT uLogin, 
                  CMULONG ibPart, CMULONG cbPart, 
                  void *pbDest, CMULONG cbDest);

CODEMETER_API int CMAPIENTRY CmWriteUvd(HCMSysEntry hcmse, CMULONG flCtrl, CMUSHORT usIndex, 
                  CMUINT uLogin, 
                  CMUSHORT usSubtype, const void *pbData, CMULONG cbData);

CODEMETER_API int CMAPIENTRY CmGetUvd(HCMSysEntry hcmse, CMUSHORT usIndex, CMUINT uLogin,
    CMUINT uTag, 
    void* pbDest, CMULONG cbDest);

CODEMETER_API CMBYTE CmGetUvdb(HCMSysEntry hcmse, CMUSHORT usIndex, CMUINT uTag);

CODEMETER_API CMULONG CmGetUvdn(HCMSysEntry hcmse, CMUSHORT usIndex, CMUINT uTag);

CODEMETER_API int CMAPIENTRY CmSetUvd(HCMSysEntry hcmse, CMUSHORT usIndex, CMUINT uLogin,
    CMUINT uTag, 
    const void* pbVal, CMULONG cbVal);

CODEMETER_API int CMAPIENTRY CmCryptUvd(HCMSysEntry hcmse, CMUSHORT usIndex, CMUINT uLogin, 
    CMULONG flCtrl,
    const void* pbSrc, CMULONG cbSrc,
    void* pbDest, CMULONG cbDest);

CODEMETER_API const void *CmAPIInfo(void);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* !defined(_CODEMETER_INCLUDED) */
