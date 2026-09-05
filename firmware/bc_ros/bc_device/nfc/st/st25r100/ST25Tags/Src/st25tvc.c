/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2022 STMicroelectronics, all rights reserved
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/
/*
 *      PROJECT:   st25tags library
 *      Revision:
 *      LANGUAGE:  ISO C99
 */

/*! \file
 *
 *  \author STMicroelectronics MMY Ecosystem
 *
 *  \brief st25tvc higher level interface
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "st25tvc.h"
#include "rfal_nfcv.h"
#include "rfal_st25xv.h"
#include "rfal_utils.h"


/*
 ******************************************************************************
 * LOCAL DEFINES
 ******************************************************************************
 */
#define ST25TAGS_TVC_CONFIG_PWD_LEN             4U

#define ST25TAGS_TVC_ANDEF_CFG_UID_EN_MASK      0x01
#define ST25TAGS_TVC_ANDEF_CFG_CUS_EN_MASK      0x02
#define ST25TAGS_TVC_ANDEF_CFG_UTC_EN_MASK      0x04
#define ST25TAGS_TVC_ANDEF_CFG_TD_EN_MASK       0x10
#define ST25TAGS_TVC_ANDEF_CFG_SEP_EN_MASK      0x20

#define ST25TAGS_TVC_ANDEF_CFG_UID_BIT          0U
#define ST25TAGS_TVC_ANDEF_CFG_CUS_BIT          1U
#define ST25TAGS_TVC_ANDEF_CFG_UTC_BIT          2U
#define ST25TAGS_TVC_ANDEF_CFG_TD_BIT           4U
#define ST25TAGS_TVC_ANDEF_CFG_SEP_BIT          5U

#define ST25TAGS_TVC_REQ_FLAG           (RFAL_NFCV_REQ_FLAG_DEFAULT | RFAL_NFCV_REQ_FLAG_ADDRESS)

#define ST25TAGS_TVC_ISO_MAGIC_NUM_IDX  (RFAL_NFCV_UID_LEN - 1)
#define ST25TAGS_TVC_MFR_IDX            (ST25TAGS_TVC_ISO_MAGIC_NUM_IDX - 1)
#define ST25TAGS_TVC_PRODUCT_CODE_IDX   (ST25TAGS_TVC_MFR_IDX - 1)

/*
 ******************************************************************************
 * SHARED VARIABLES
 ******************************************************************************
 */
char ST25TAGS_TVC_tagName[][16] = {
    "UNKNOWN_TVC_TAG",
    "ST25TV512C",
    "ST25TV02KC"
};


/*
 ******************************************************************************
 * LOCAL METHODS
 ******************************************************************************
 */
static ReturnCode readAndefCfg(const uint8_t *uid, uint8_t *regValue);
static bool isByteValueSafe(uint8_t data);


/*
 ******************************************************************************
 * CONFIGURATION
 ******************************************************************************
 */
ReturnCode ST25TAGS_TVC_ReadConfiguration( uint8_t flags, const uint8_t* uid, uint8_t fid,
        uint8_t pid, uint8_t* regValue, uint8_t* regLen )
{
    uint8_t            data[2]; // FID + PID
    uint8_t            dataLen;
    uint16_t           rcvLen;
    rfalNfcvGenericRes res;
    ReturnCode         errCode;

    dataLen = 0U;

    /* Compute Request Data */
    data[dataLen++] = fid;
    data[dataLen++] = pid;

    if ( regValue == NULL || regLen == NULL )
    {
        return RFAL_ERR_PARAM;
    }

    errCode = rfalNfcvPollerTransceiveReq( RFAL_NFCV_CMD_READ_CONFIGURATION, flags, RFAL_NFCV_ST_IC_MFG_CODE,
            uid, data, dataLen, (uint8_t*)&res, sizeof(rfalNfcvGenericRes), &rcvLen );
    if ( errCode == RFAL_ERR_NONE )
    {
        *regLen = rcvLen - 1;   // Remove flag byte from response
        if ( rcvLen < 2U )
        {
            errCode = RFAL_ERR_PROTO;
        }
        else
        {
            for (uint8_t i = 0; i < rcvLen - 1; i++)
            {
                *(regValue + i) = res.data[i];
            }
        }
    }
    return errCode;
}

ReturnCode ST25TAGS_TVC_WriteConfiguration( uint8_t flags, const uint8_t* uid, uint8_t fid, uint8_t pid,
        uint8_t* regValue, uint8_t regLen )
{
    uint8_t            data[2 + 8]; // FID + PID + up to 8 bytes of data
    uint8_t            dataLen;
    uint16_t           rcvLen;
    rfalNfcvGenericRes res;

    dataLen = 0U;

    data[dataLen++] = fid;
    data[dataLen++] = pid;
    for (uint8_t i = 0; i < regLen; i++) {
        data[dataLen++] = regValue[i];
    }

    return rfalNfcvPollerTransceiveReq( RFAL_NFCV_CMD_WRITE_CONFIGURATION, flags, RFAL_NFCV_ST_IC_MFG_CODE,
            uid, data, dataLen, (uint8_t*)&res, sizeof(rfalNfcvGenericRes), &rcvLen );
}


ReturnCode ST25TAGS_TVC_OpenSecuritySession(const uint8_t * uid, ST25TAG_TVC_SecuritySession_t sessionType,
        const uint8_t * pPassword)
{
    uint8_t     pwdNumber;
    uint8_t     pwdLengthInBytes = 4;
    ReturnCode  errCode;
    uint8_t     rxBuf[ 1 + 2 + 2 ];    /* Flags + Random Number Data + CRC */;
    uint16_t    rcvLen;
    uint8_t     updatedPassword[8];

    switch (sessionType)
    {
        case CONFIGURATION_SECURITY_SESSION:
            pwdNumber = 0;
            break;
        case USER_AREA1_SECURITY_SESSION:
            pwdNumber = 1;
            break;
        case USER_AREA2_SECURITY_SESSION:
            pwdNumber = 2;
            break;
        case SINGLE_USER_AREA_SECURITY_SESSION:
            pwdLengthInBytes = 8;
            pwdNumber = 1;
            break;
        case UNTRACEABLE_SECURITY_SESSION:
            pwdNumber = 3;
            break;
        default:
            return RFAL_ERR_PARAM;
    }

    // Encrypt password
    // 1. Get Random Number
    errCode = rfalST25xVPollerGetRandomNumber( ST25TAGS_TVC_REQ_FLAG, uid, rxBuf, sizeof(rxBuf), &rcvLen );

    if ( errCode == RFAL_ERR_NONE )
    {
        // 2. XOR random number with given password located in rxBuf[1] and rxBuf[2]
        for (uint8_t i = 0; i < (pwdLengthInBytes * 2); i++)
        {
            updatedPassword[i] = pPassword[i] ^ rxBuf[2 - (i + 1) % 2];
        }

        // Present password
        errCode = rfalST25xVPollerPresentPassword( ST25TAGS_TVC_REQ_FLAG, uid,
                pwdNumber, updatedPassword, pwdLengthInBytes);
    }

    // Same ReturnCode definitions used in st_errno.h of RFAL
    return errCode;
}


/*******************************************************************************/
bool ST25TAGS_TVC_IsTagSt25TvC(const uint8_t* uid)
{
    if (    (uid[ST25TAGS_TVC_ISO_MAGIC_NUM_IDX] == 0xE0)
         && (uid[ST25TAGS_TVC_MFR_IDX] == ST25TAGS_ST_MFR_CODE) )
    {
        // STMicroelectronics Iso15693 tag recognized
        if ( uid[ST25TAGS_TVC_PRODUCT_CODE_IDX] == (ST25TAGS_TVC_PRODUCT_CODE) )
        {
            return true;
        }
    }

    return false;
}

/*******************************************************************************/
ST25TAGS_TVC_TagType_t ST25TAGS_TVC_GetSt25TvCTagType(const uint8_t* uid)
{
    ReturnCode      errCode;
    uint8_t         rxBuf[ST25TAGS_TVC_SYSINFO_MAX_LEN];
    uint16_t        rcvLen;
    uint8_t         endMem;
    uint8_t         icRef;

    errCode = rfalNfcvPollerGetSystemInformation( ST25TAGS_TVC_REQ_FLAG, uid, rxBuf,
                                                  (uint16_t)sizeof(rxBuf), &rcvLen );

    if( (errCode != RFAL_ERR_NONE) || (rcvLen < 2) || (rcvLen > (ST25TAGS_TVC_SYSINFO_MAX_LEN)) )
    {
        return UNKNOWN_TVC_TAG;
    }

    if (    (uid[ST25TAGS_TVC_ISO_MAGIC_NUM_IDX] == 0xE0)
         && (uid[ST25TAGS_TVC_MFR_IDX] == ST25TAGS_ST_MFR_CODE) )
    {
        /* An STMicroelectronics Iso15693 tag was recognized.
           Check the tag memory size by parsing the content of the getSystemInfo command. */
        endMem = rxBuf[ST25TAGS_TVC_SYSINFO_USERBLOCKS_INDEX];
        icRef = rxBuf[ST25TAGS_TVC_SYSINFO_ICREF_INDEX];

        if (    (icRef == (ST25TAGS_TVC_PRODUCT_CODE))
             && (uid[ST25TAGS_TVC_PRODUCT_CODE_IDX] == (ST25TAGS_TVC_PRODUCT_CODE)) )
        {
            if (endMem > ST25TAGS_TVC_TV512C_END_MEM) {
                return ST25TV02KC;
            }
            else
            {
                return ST25TV512C;
            }
        }
    }

    return UNKNOWN_TVC_TAG;
}


/*
 ******************************************************************************
 * AREA MANAGEMENT
 ******************************************************************************
 */

ReturnCode ST25TAGS_TVC_GetNumberOfUserAreas(const uint8_t* uid, uint8_t * numberOfUserAreas)
{
    ReturnCode  errCode;
    uint8_t     regEndA1Value;
    uint8_t     rxBuf[ST25TAGS_TVC_SYSINFO_MAX_LEN];
    uint16_t    rcvLen;
    uint8_t     regEndMemValue;
    uint8_t     regLen;

    // Read END_A1 from Configuration Register @FID=00h, PID=01h
    errCode = ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                                ST25TAGS_TVC_FID_AREA1, ST25TAGS_TVC_PID_END_Ax,
                                                &regEndA1Value, &regLen );
    if ( errCode != RFAL_ERR_NONE )
    {
        return errCode;
    }

    // Read END_MEM data
    errCode = rfalNfcvPollerGetSystemInformation( ST25TAGS_TVC_REQ_FLAG, uid, rxBuf,
                                                    (uint16_t)sizeof(rxBuf), &rcvLen );
    if ( errCode != RFAL_ERR_NONE )
    {
        return errCode;
    }

    if ( (uid[ST25TAGS_TVC_ISO_MAGIC_NUM_IDX] == 0xE0) && (uid[ST25TAGS_TVC_MFR_IDX] == ST25TAGS_ST_MFR_CODE) )
    {
        /* An STMicroelectronics Iso15693 tag was recognized.
           Check the tag memory size by parsing the content of the getSystemInfo command. */
        regEndMemValue = rxBuf[ST25TAGS_TVC_SYSINFO_USERBLOCKS_INDEX];

        // If END_A1 < END_MEM then dual user area configuration
        // Else we are in a single area configuration
        if ( regEndA1Value < regEndMemValue ) {
            *numberOfUserAreas = 2;
        } else {
            *numberOfUserAreas = 1;
        }

    }
    return errCode;
}

ReturnCode ST25TAGS_TVC_SetSizeOfUserAreaOne(const uint8_t* uid, const uint8_t sizeOfAreaOne)
{
    ReturnCode  errCode;
    uint8_t     regValue;
    uint16_t    rcvLen;
    uint8_t     rxBuf[ST25TAGS_TVC_SYSINFO_MAX_LEN];
    uint8_t     regEndMemValue;


    // Read END_MEM data
    errCode = rfalNfcvPollerGetSystemInformation( ST25TAGS_TVC_REQ_FLAG, uid, rxBuf, (uint16_t) sizeof(rxBuf), &rcvLen );
    if ( errCode != RFAL_ERR_NONE )
    {
        return errCode;
    }

    regValue = sizeOfAreaOne;

    if ( (uid[RFAL_NFCV_UID_LEN - 1] == 0xE0) && (uid[RFAL_NFCV_UID_LEN - 2] == ST25TAGS_ST_MFR_CODE) )
    {
        /* An STMicroelectronics Iso15693 tag was recognized.
           Check the tag memory size by parsing the content of the getSystemInfo command. */
        regEndMemValue = rxBuf[ST25TAGS_TVC_SYSINFO_USERBLOCKS_INDEX];

        if (sizeOfAreaOne > regEndMemValue)
        {
            // Area 1 size should not exceed the max amount of memory available on the tag
            regValue = regEndMemValue;
        }

    }

    errCode = ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
            ST25TAGS_TVC_FID_AREA1, ST25TAGS_TVC_PID_END_Ax, &regValue, 1 );

    return errCode;
}


/*******************************************************************************/

ReturnCode ST25TAGS_TVC_ProtectUserArea(const uint8_t* uid, ST25TAG_TVC_UserAreaName_t areaName,
        ST25TAG_TVC_AccessProtection_t accessControl)
{
    ReturnCode  errCode;
    uint8_t     regFeatureID;
    uint8_t     regValue;

    switch (areaName)
    {
        case AREA_USER_1:
            regFeatureID = ST25TAGS_TVC_FID_AREA1;
            break;
        case AREA_USER_2:
            regFeatureID = ST25TAGS_TVC_FID_AREA2;
            break;
        default:
            return RFAL_ERR_PARAM;
    }

    switch (accessControl)
    {
        case READABLE_AND_WRITEABLE :
            regValue = 0b00;
            break;
        case READABLE_AND_WRITE_PROTECTED_BY_PWD :
            regValue = 0b01;
            break;
        case READ_AND_WRITE_PROTECTED_BY_PWD :
            regValue = 0b10;
            break;
        case READ_PROTECTED_BY_PWD_AND_WRITE_FORBIDDEN :
            regValue = 0b11;
            break;
        default:
            return RFAL_ERR_PARAM;
    }

    // Sets content of register RW_PROTECTION_A1 or RW_PROTECTION_A2 to the wanted value
    errCode = ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                                regFeatureID, ST25TAGS_TVC_PID_RW_PROTECTION_Ax,
                                                &regValue, 1 );
    return errCode;
}

/*******************************************************************************/
ReturnCode ST25TAGS_TVC_GetUniqueTapCode(const uint8_t *uid, uint8_t utcValue[3])
{
    ReturnCode  errCode;
    uint8_t     regValue[3];
    uint8_t     regLen = 0;

    if ( (uid == NULL) || (utcValue == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    // Read UTC from Configuration Register @FID=02h, PID=01h
    errCode = ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                              ST25TAGS_TVC_FID_UNIQUE_TAP_CODE,
                                              ST25TAGS_TVC_PID_UTC, regValue, &regLen );

    if ( (errCode == RFAL_ERR_NONE) && (regLen != 0) ) {
        RFAL_MEMCPY( utcValue, regValue, regLen );
    }

    return errCode;
}

ReturnCode ST25TAGS_TVC_IsTamperEventUpdateEnabled(const uint8_t *uid, bool *isEnabled)
{
    ReturnCode  errCode;
    uint8_t     regValue;
    uint8_t     regLen;

    if ( (uid == NULL) || (isEnabled == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    // Read TD_EVENT_UPDATE_EN from Configuration Register @FID=03h, PID=00h
    errCode = ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                              ST25TAGS_TVC_FID_TAMPER_DETECT,
                                              ST25TAGS_TVC_PID_TD_EVENT_UPDATE_EN,
                                              &regValue, &regLen );

    *isEnabled = (bool) (regValue & 0x01);
    return errCode;
}

ReturnCode ST25TAGS_TVC_GetTamperDetectStatus(const uint8_t *uid, uint8_t *tamperStatus)
{
    ReturnCode  errCode;
    uint8_t     regValue[3];
    uint8_t     regLen = 0;

    if ( (uid == NULL) || (tamperStatus == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    // Read TD Status from Configuration Register @FID=03h, PID=06h
    errCode = ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                              ST25TAGS_TVC_FID_TAMPER_DETECT,
                                              ST25TAGS_TVC_PID_TD_STATUS, regValue, &regLen );

    if ( (errCode == RFAL_ERR_NONE) && (regLen != 0) ) {
        RFAL_MEMCPY( tamperStatus, regValue, regLen );
    }

    return errCode;
}


/*
 ******************************************************************************
 * AUGMENTED NDEF
 ******************************************************************************
 */

/*******************************************************************************/
ReturnCode ST25TAGS_TVC_EnableAndef(const uint8_t *uid, bool isEnabled)
{
    ReturnCode  errCode;
    uint8_t     regValue;

    if (uid == NULL) {
        return RFAL_ERR_PARAM;
    }

    regValue = isEnabled ? 0x01 : 0x00;

    // Sets content of register ANDEF_EN to the wanted value
    errCode = ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                               ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                               ST25TAGS_TVC_PID_ANDEF_EN, &regValue, 1 );
    return errCode;
}

ReturnCode ST25TAGS_TVC_IsAndefEnabled(const uint8_t *uid, bool *isEnabled)
{
    ReturnCode  errCode;
    uint8_t     regValue;
    uint8_t     regLen;

    if ( (uid == NULL) || (isEnabled == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    // Read ANDEF_EN from Configuration Register @FID=04h, PID=00h
    errCode = ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                              ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                              ST25TAGS_TVC_PID_ANDEF_EN, &regValue, &regLen );

    *isEnabled = (bool) (regValue & 0x01);
    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefIsUidEnabled(const uint8_t *uid, bool *isEnabled)
{
    ReturnCode  errCode;
    uint8_t     regValue;

    if ( (uid == NULL) || (isEnabled == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    errCode = readAndefCfg( uid, &regValue);

    *isEnabled = (bool) ((regValue & ST25TAGS_TVC_ANDEF_CFG_UID_EN_MASK) >> ST25TAGS_TVC_ANDEF_CFG_UID_BIT);
    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefIsCustomMessageEnabled(const uint8_t *uid, bool *isEnabled)
{
    ReturnCode  errCode;
    uint8_t     regValue;

    if ( (uid == NULL) || (isEnabled == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    errCode = readAndefCfg( uid, &regValue);

    *isEnabled = (bool) ((regValue & ST25TAGS_TVC_ANDEF_CFG_CUS_EN_MASK) >> ST25TAGS_TVC_ANDEF_CFG_CUS_BIT);
    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefIsUtcEnabled(const uint8_t *uid, bool *isEnabled)
{
    ReturnCode  errCode;
    uint8_t     regValue;

    if ( (uid == NULL) || (isEnabled == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    errCode = readAndefCfg( uid, &regValue);

    *isEnabled = (bool) ((regValue & ST25TAGS_TVC_ANDEF_CFG_UTC_EN_MASK) >> ST25TAGS_TVC_ANDEF_CFG_UTC_BIT);
    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefIsTamperDetectEnabled(const uint8_t *uid, bool *isEnabled)
{
    ReturnCode  errCode;
    uint8_t     regValue;

    if ( (uid == NULL) || (isEnabled == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    errCode = readAndefCfg( uid, &regValue);

    *isEnabled = (bool) ((regValue & ST25TAGS_TVC_ANDEF_CFG_TD_EN_MASK) >> ST25TAGS_TVC_ANDEF_CFG_TD_BIT);
    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefIsSeparatorEnabled(const uint8_t *uid, bool *isEnabled)
{
    ReturnCode  errCode;
    uint8_t     regValue;

    if ( (uid == NULL) || (isEnabled == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    errCode = readAndefCfg( uid, &regValue);

    *isEnabled = (bool) ((regValue & ST25TAGS_TVC_ANDEF_CFG_SEP_EN_MASK) >> ST25TAGS_TVC_ANDEF_CFG_SEP_BIT);
    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefGetSeparatorValue(const uint8_t *uid, uint8_t *separator)
{
    uint8_t     regLen;

    if ( (uid == NULL) || (separator == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    // Read ANDEF_SEP from Configuration Register @FID=04h, PID=02h
    return ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                           ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                           ST25TAGS_TVC_PID_ANDEF_SEP, separator, &regLen );

}

ReturnCode ST25TAGS_TVC_AndefGetCustomMessage(const uint8_t *uid, uint8_t customMessage[8])
{
    ReturnCode  errCode;
    uint8_t     regValue[4];
    uint8_t     regLen = 0;

    if ( (uid == NULL) || (customMessage == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    // Read ANDEF_CUSTOM from Configuration Register @FID=04h, PID=03h and 04h
    errCode = ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                              ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                              ST25TAGS_TVC_PID_ANDEF_CUSTOM_LSB, regValue, &regLen );
    if (errCode != RFAL_ERR_NONE) {
        return errCode;
    }

    if (regLen != 0) {
        RFAL_MEMCPY( customMessage, regValue, regLen );
    }

    errCode = ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                              ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                              ST25TAGS_TVC_PID_ANDEF_CUSTOM_MSB, regValue, &regLen );
    if ( (errCode == RFAL_ERR_NONE) && (regLen != 0) ) {
        RFAL_MEMCPY( &customMessage[regLen], regValue, regLen );
    }

    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefGetAndefLengthInBytes(const uint8_t *uid, uint8_t *andefLength)
{
    ReturnCode  errCode;
    bool isEnabled;
    bool isUidEnabled;
    bool isCustomMsgEnabled;
    bool isUtcEnabled;
    bool isTamperDetectEnabled;


    if ( (uid == NULL) || (andefLength == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    errCode = ST25TAGS_TVC_IsAndefEnabled(uid, &isEnabled);
    if (errCode != RFAL_ERR_NONE || !isEnabled)
    {
        *andefLength = 0;
        return 0;
    }

    errCode = ST25TAGS_TVC_AndefIsUidEnabled(uid, &isUidEnabled);
    errCode |= ST25TAGS_TVC_AndefIsCustomMessageEnabled(uid, &isCustomMsgEnabled);
    errCode |= ST25TAGS_TVC_AndefIsUtcEnabled(uid, &isUtcEnabled);
    errCode |= ST25TAGS_TVC_AndefIsTamperDetectEnabled(uid, &isTamperDetectEnabled);
    errCode |= ST25TAGS_TVC_AndefIsSeparatorEnabled(uid, &isEnabled);

    *andefLength = isUidEnabled * 16 + isCustomMsgEnabled * 8 + isUtcEnabled * 3 + isTamperDetectEnabled * 3;
    *andefLength += ( isEnabled * (isUidEnabled + isCustomMsgEnabled + isUtcEnabled + isTamperDetectEnabled - 1) );

    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefConfigure(const uint8_t *uid, ST25TAG_TVC_AndefConfig_t andefConfig)
{
    ST25TAGS_TVC_TagType_t  tagType;
    uint8_t     regValue[2];

    if ( (uid == NULL) || (andefConfig.andefByteOffset > 3) ) {
        return RFAL_ERR_PARAM;
    }

    /* Test block address value */
    tagType = ST25TAGS_TVC_GetSt25TvCTagType(uid);
    switch (tagType)
    {
        case ST25TV512C:
            if ( andefConfig.andefBlockAddress > 0x0F)
            {
                return RFAL_ERR_PARAM;
            }
            break;

        case ST25TV02KC:
            if ( andefConfig.andefBlockAddress > 0x4F)
            {
                return RFAL_ERR_PARAM;
            }
            break;

        default:
            return RFAL_ERR_PARAM;
    }

    // Program ANDEF_CFG register with data passed as parameter
    regValue[0] = andefConfig.uidEnabled
             |  ( andefConfig.customMsgEnabled << 1 )
             |  ( andefConfig.utcEnabled << 2 )
             |  ( andefConfig.tamperDetectEnabled << 4 )
             |  ( andefConfig.separatorEnabled << 5 );

    // Keep last 2 bits if andefByteOffset
    regValue[0] = regValue[0] | ( (andefConfig.andefByteOffset & 0x03) << 6 );

    regValue[1] = andefConfig.andefBlockAddress;

    return ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                            ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                            ST25TAGS_TVC_PID_ANDEF_CFG, regValue, 2 );
}

ReturnCode ST25TAGS_TVC_AndefSetSeparatorValue(const uint8_t *uid, const uint8_t separator)
{
    uint8_t regValue;

    if ( (uid == NULL) || !isByteValueSafe(separator) ) {
        return RFAL_ERR_PARAM;
    }

    regValue = separator;

    return ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                            ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                            ST25TAGS_TVC_PID_ANDEF_SEP, &regValue, 1 );
}


ReturnCode ST25TAGS_TVC_AndefSetCustomMessage(const uint8_t *uid, const char customMessage[8])
{
    ReturnCode errCode;
    uint8_t regValue[4];

    if ( (uid == NULL) || (customMessage == NULL) ) {
        return RFAL_ERR_PARAM;
    }

    for (int32_t i = 7; i >= 0; i--)
    {
        if ( !isByteValueSafe(customMessage[i]) ) {
            return RFAL_ERR_PARAM;
        }
    }

    RFAL_MEMCPY( regValue, customMessage, 4 );

    // Write Configuration Register @FID=04h, PID=03h and 04h
    errCode = ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                               ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                               ST25TAGS_TVC_PID_ANDEF_CUSTOM_LSB, regValue, 4 );


    RFAL_MEMCPY( regValue, &customMessage[4], 4 );
    errCode = ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                               ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                               ST25TAGS_TVC_PID_ANDEF_CUSTOM_MSB, regValue, 4 );


    return errCode;
}

ReturnCode ST25TAGS_TVC_AndefSetTamperMessages(const uint8_t *uid, const ST25TAG_TVC_TamperMessages_t tamperMessages)
{
    ReturnCode errCode;
    uint8_t regValue[2];

    if (   (uid == NULL)
        || !isByteValueSafe(tamperMessages.tdShortMessage)
        || !isByteValueSafe(tamperMessages.tdOpentMessage)) {
        return RFAL_ERR_PARAM;
    }

    for (int32_t i = 1; i >= 0; i--)
    {
        if (    !isByteValueSafe(tamperMessages.tdSealMessage[i])
             || !isByteValueSafe(tamperMessages.tdUnsealMessage[i])
             || !isByteValueSafe(tamperMessages.tdResealMessage[i])) {
            return RFAL_ERR_PARAM;
        }
    }

    regValue[0] = tamperMessages.tdShortMessage;
    errCode = ST25TAGS_TVC_WriteConfiguration(  ST25TAGS_TVC_REQ_FLAG, uid,
                                                ST25TAGS_TVC_FID_TAMPER_DETECT,
                                                ST25TAGS_TVC_PID_TD_SHORT_MSG, &regValue[0], 1 );

    regValue[0] = tamperMessages.tdOpentMessage;
    errCode |= ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                                ST25TAGS_TVC_FID_TAMPER_DETECT,
                                                ST25TAGS_TVC_PID_TD_OPEN_MSG, &regValue[0], 1 );

    RFAL_MEMCPY( regValue, tamperMessages.tdSealMessage, 2 );
    errCode |= ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                                ST25TAGS_TVC_FID_TAMPER_DETECT,
                                                ST25TAGS_TVC_PID_TD_SEAL_MSG, regValue, 2 );

    RFAL_MEMCPY( regValue, tamperMessages.tdUnsealMessage, 2 );
    errCode |= ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                                ST25TAGS_TVC_FID_TAMPER_DETECT,
                                                ST25TAGS_TVC_PID_TD_UNSEAL_MSG, regValue, 2 );

    RFAL_MEMCPY( regValue, tamperMessages.tdResealMessage, 2 );
    errCode |= ST25TAGS_TVC_WriteConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                                ST25TAGS_TVC_FID_TAMPER_DETECT,
                                                ST25TAGS_TVC_PID_TD_RESEAL_MSG, regValue, 2 );
    return errCode;
}

/*
 ******************************************************************************
 * HELPERS
 ******************************************************************************
 */
bool isByteValueSafe(uint8_t data)
{
    const uint8_t forbiddenChar[] = {'"', '&', '\'', '@', '[', ']', '^', '`', '{', '|', '}'};

    if ( (data < '!') || (data > '~') ) {
        return false;
    }

    for (uint8_t i = 0; i <= sizeof(forbiddenChar) - 1; i++)
    {
        if ( data == forbiddenChar[i] ) {
            return false;
        }
    }

    return true;
}

ReturnCode readAndefCfg(const uint8_t *uid, uint8_t *regValue) {
    uint8_t     regLen;

    // Read ANDEF_EN from Configuration Register @FID=04h, PID=00h
    return ST25TAGS_TVC_ReadConfiguration( ST25TAGS_TVC_REQ_FLAG, uid,
                                           ST25TAGS_TVC_FID_AUGMENTED_NDEF,
                                           ST25TAGS_TVC_PID_ANDEF_CFG, regValue, &regLen );
}
