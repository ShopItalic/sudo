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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef ST25TVC_H
#define ST25TVC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "rfal_utils.h"


/* Exported types ------------------------------------------------------------*/
/** @addtogroup ST25Tags
 *  @{
 */

typedef enum {
    UNKNOWN_TVC_TAG = 0,
    ST25TV512C,
    ST25TV02KC,
} ST25TAGS_TVC_TagType_t;

typedef enum {
    CONFIGURATION_SECURITY_SESSION,
    SINGLE_USER_AREA_SECURITY_SESSION, /*!< AREA1 + AREA2 are merged in a single user area */
    USER_AREA1_SECURITY_SESSION,
    USER_AREA2_SECURITY_SESSION,
    UNTRACEABLE_SECURITY_SESSION
} ST25TAG_TVC_SecuritySession_t;

typedef enum {
    AREA_USER_1,
    AREA_USER_2,
} ST25TAG_TVC_UserAreaName_t;

typedef enum {
    READABLE_AND_WRITEABLE = 0U,
    READABLE_AND_WRITE_PROTECTED_BY_PWD = 1U,
    READ_AND_WRITE_PROTECTED_BY_PWD = 2U,
    READ_PROTECTED_BY_PWD_AND_WRITE_FORBIDDEN = 3U
} ST25TAG_TVC_AccessProtection_t;

typedef enum {
    ST25TAGS_TVC_FID_AREA1 = 0U,
    ST25TAGS_TVC_FID_AREA2,
    ST25TAGS_TVC_FID_UNIQUE_TAP_CODE,
    ST25TAGS_TVC_FID_TAMPER_DETECT,
    ST25TAGS_TVC_FID_AUGMENTED_NDEF,
    ST25TAGS_TVC_FID_PRIVACY,
    ST25TAGS_TVC_FID_AFI_PROT,
    ST25TAGS_TVC_FID_DEVICE_ID = 0xFEU,
    ST25TAGS_TVC_FID_LOCK = 0xFFU
} ST25TAG_TVC_FeatureId_t;

typedef struct {
    bool uidEnabled;
    bool customMsgEnabled;
    bool utcEnabled;
    bool tamperDetectEnabled;
    bool separatorEnabled;
    uint8_t andefByteOffset;
    uint8_t andefBlockAddress;
} ST25TAG_TVC_AndefConfig_t;

typedef struct {
    char tdShortMessage;
    char tdOpentMessage;
    char tdSealMessage[2];
    char tdUnsealMessage[2];
    char tdResealMessage[2];
} ST25TAG_TVC_TamperMessages_t;

/* Exported constants --------------------------------------------------------*/
#define ST25TAGS_ST_MFR_CODE                    0x02U

#define ST25TAGS_TVC_SYSINFO_USERBLOCKS_INDEX   12U
#define ST25TAGS_TVC_SYSINFO_BLOCKSIZE_INDEX    13U
#define ST25TAGS_TVC_SYSINFO_ICREF_INDEX        14U

#define ST25TAGS_TVC_PRODUCT_CODE               0x08U
#define ST25TAGS_TVC_TV512C_END_MEM             0x0F
#define ST25TAGS_TVC_TV02KC_END_MEM             0x4F

#define ST25TAGS_TVC_CCFILE_ADDRESS             0x00

#define ST25TAGS_TVC_SYSINFO_MAX_LEN            17U

/*****************/
/* Parameter IDs */
/*****************/
//ST25TAGS_TVC_FID_AREAx
#define ST25TAGS_TVC_PID_RW_PROTECTION_Ax       0x00
#define ST25TAGS_TVC_PID_END_Ax                 0x01
//ST25TAGS_TVC_FID_UNIQUE_TAP_CODE
#define ST25TAGS_TVC_PID_UTC_EN                 0x00
#define ST25TAGS_TVC_PID_UTC                    0x01
//ST25TAGS_TVC_FID_TAMPER_DETECT
#define ST25TAGS_TVC_PID_TD_EVENT_UPDATE_EN     0x00
#define ST25TAGS_TVC_PID_TD_SEAL_MSG            0x01
#define ST25TAGS_TVC_PID_TD_UNSEAL_MSG          0x02
#define ST25TAGS_TVC_PID_TD_RESEAL_MSG          0x03
#define ST25TAGS_TVC_PID_TD_SHORT_MSG           0x04
#define ST25TAGS_TVC_PID_TD_OPEN_MSG            0x05
#define ST25TAGS_TVC_PID_TD_STATUS              0x06
//ST25TAGS_TVC_FID_AUGMENTED_NDEF
#define ST25TAGS_TVC_PID_ANDEF_EN               0x00
#define ST25TAGS_TVC_PID_ANDEF_CFG              0x01
#define ST25TAGS_TVC_PID_ANDEF_SEP              0x02
#define ST25TAGS_TVC_PID_ANDEF_CUSTOM_LSB       0x03
#define ST25TAGS_TVC_PID_ANDEF_CUSTOM_MSB       0x04
//ST25TAGS_TVC_FID_PRIVACY
#define ST25TAGS_TVC_PID_PRIVACY                0x00
//ST25TAGS_TVC_FID_AFI_PROT
#define ST25TAGS_TVC_PID_AFI_PROT               0x00
//ST25TAGS_TVC_FID_DEVICE_ID
#define ST25TAGS_TVC_PID_DEVICE_ID_REV          0x00
#define ST25TAGS_TVC_PID_DEVICE_ID_UID          0x01
//ST25TAGS_TVC_FID_LOCK
#define ST25TAGS_TVC_PID_LCK_CONFIG             0x00

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern char ST25TAGS_TVC_tagName[][16];

/* Exported functions --------------------------------------------------------*/

/**
 *****************************************************************************
 * \brief Determine if the UID parameter belongs to a ST25TVC product
 *
 * \param[in]  uid : pointer to UID byte array. The expected length is 8 bytes
 *                   and the last byte of the array must be equal to 0xE0.
 *
 * \return true  if the UID belongs to a ST25TVC product
 * \return false if the UID does not belong to a ST25TVC product
 *****************************************************************************
 */
bool ST25TAGS_TVC_IsTagSt25TvC(const uint8_t *uid);

/**
 *****************************************************************************
 * \brief Get Tag Type
 *
 * Returns the type of ST25TVC tag matching the given UID
 *
 * \param[in]  uid : pointer to UID byte array. The expected length is 8 bytes
 *                   and the last byte of the array must be equal to 0xE0.
 *
 * \return ST25TAGS_TVC_TagType_t value corresponding to the tag's reference
 *****************************************************************************
 */
ST25TAGS_TVC_TagType_t ST25TAGS_TVC_GetSt25TvCTagType(const uint8_t *uid);

/**
 *****************************************************************************
 * \brief Open a security session
 *
 * Sends presentPassword RF command through rfalST25xVPollerPresentPassword() in
 * addressed mode using the uid provided as the first parameter.
 *
 * The security session will be closed upon taking the tag away from the RF field, or
 * when presenting an incorrect password.
 *
 * When AREA1 and AREA2 user areas are merged, using the command
 * ST25TAGS_TVC_SetNumberOfUserAreas(uid, 1), the password for the session must be set
 * to 64-bit length.
 *
 * Beware that opening a security session will cause a RF field reset.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[in]  sessionType    : Value taken from ST25TAG_DVPWM_SecuritySession_t
 * \param[in]  pPassword      : pointer to a 4-byte or 8-byte char array
 *                              pPassword length used is 8 bytes for SINGLE_USER_AREA_SECURITY_SESSION
 *                              pPassword length used is 4 bytes for all other sessions
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_OpenSecuritySession(const uint8_t *uid,
        ST25TAG_TVC_SecuritySession_t securitySession,
        const uint8_t *pConfigPassword);

/*!
 *****************************************************************************
 * \brief  ST25TVC Read Configuration
 *
 * Reads static configuration registers at the feature identifier (fid) and
 * parameter identifier (pid) location
 *
 * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
 *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
 * \param[in]  uid            : UID of the device to be put to be read
 *                              if not provided Select mode will be used
 * \param[in]  fid            : Feature IDentifier
 * \param[in]  pid            : Parameter IDentifier
 * \param[out] regValue       : Register value
 * \param[out] regLen         : Register value's length, could be 1, 2, 3, 4 or 8-byte long
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_ReadConfiguration(uint8_t flags, const uint8_t *uid,
        uint8_t fid, uint8_t pid, uint8_t *regValue, uint8_t *regLen);

/*!
 *****************************************************************************
 * \brief  ST25TVC Write Configuration
 *
 * Writes static configuration registers at the feature identifier (fid) and
 * parameter identifier (pid) location
 *
 * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
 *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
 * \param[in]  uid            : UID of the device to be put to be read
 *                              if not provided Select mode will be used
 * \param[in]  fid            : Feature IDentifier
 * \param[in]  pid            : Parameter IDentifier
 * \param[in]  regValue       : Pointer to byte-array register value to write
 * \param[in]  regLen         : Register length to write (in bytes)
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_WriteConfiguration(uint8_t flags, const uint8_t *uid,
        uint8_t fid, uint8_t pid, uint8_t *regValue, uint8_t regLen);

/**
 *****************************************************************************
 * \brief Set the number and size of user areas
 *
 * Configures the number and size of user areas. It is possible to set 1 or 2 user areas.
 *
 * NOTE: A CONFIG security session must be active for this command to execute properly.
 *       If ST25TAGS_TVC_OpenSecuritySession() has not been called prior to this
 *       command, the method may return ERR_REQUEST (unless no write is required).
 *
 * \param[in] uid              : pointer to UID byte array. The expected length is 8 bytes
 *                               and the last byte of the array must be equal to 0xE0.
 * \param[in] sizeOfAreaOne    : Number of the last block belonging to AREA1.
 *                               When sizeOfAreaOne is lower than END_MEM, user memory is split in two areas:
 *                                 - AREA1 (blocks 00h to END_A1)
 *                                 - AREA2 (blocks END_A1 + 1 to END_MEM).
 *                               Otherwise user memory contains a single area:
 *                                 - AREA1 (blocks 00h to END_MEM)
 *                               END_MEM is the address of the last block available from the user memory
 *                               (0Fh and 4Fh on ST25TV512C and ST25TV02KC devices, respectively)
 *
 * \return ERR_WRONG_STATE     : RFAL not initialized or incorrect mode
 * \return ERR_PARAM           : Invalid parameters
 * \return ERR_IO              : Generic internal error
 * \return ERR_CRC             : CRC error detected
 * \return ERR_FRAMING         : Framing error detected
 * \return ERR_PROTO           : Protocol error detected
 * \return ERR_TIMEOUT         : Timeout error
 * \return ERR_REQUEST         : Cannot be executed at the moment
 * \return ERR_NONE            : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_SetSizeOfUserAreaOne(const uint8_t *uid,
        const uint8_t sizeOfAreaOne);

/**
 *****************************************************************************
 * \brief Set area access protection
 *
 * Sets Read/Write protection for a given area.
 * Note that Block 00h is always accessible for reading.
 *
 * NOTE: A TVC_CTRL_SECURITY_SESSION security session must be active for this command
 *       to execute correctly.
 *       If ST25TAGS_TVC_OpenSecuritySession() has not been called prior to this
 *       command, the method may return ERR_REQUEST (unless no write is required).
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[in]  areaName       : Area to protect
 * \param[in]  accessControl  : Level of protection to apply to the given area
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_ProtectUserArea(const uint8_t *uid,
        ST25TAG_TVC_UserAreaName_t areaName,
        ST25TAG_TVC_AccessProtection_t accessControl);

/**
 *****************************************************************************
 * \brief Get the number of User Areas configured in memory
 *
 * Returns the number of user areas by comparing the value of the END_A1 register
 * with the END_MEM info given by the GetSystemInfo command.
 *
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] numberOfUserAreas : Number of user areas configured in the current tag
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_GetNumberOfUserAreas(const uint8_t *uid,
        uint8_t *numberOfUserAreas);

/**
 *****************************************************************************
 * \brief Enable/Disable ANDEF feature of ST25TVC
 *
 * When enabled, the ANDEF feature will add dynamic content to the existing NDEF message.
 * This setting will take effect at the next tag reboot (POR).
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[in] isEnabled       : Set to true to enable the ANDEF feature, false to disable
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_EnableAndef(const uint8_t *uid, bool isEnabled);

/**
 *****************************************************************************
 * \brief Read Enable/Disable value from the ANDEF_EN register of ST25TVC
 *
 * Returns the value of the ANDEF_EN register, indicating if the ANDEF feature is
 * enabled or not.
 *
 * \param[in] uid             : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] *isEnabled     : True if the ANDEF feature is enabled, false otherwise
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_IsAndefEnabled(const uint8_t *uid, bool *isEnabled);

/**
 *****************************************************************************
 * \brief Get the tag's Unique Tap Code
 *
 * Returns the byte array containing the three ASCII characters of the Unique Tap Code.
 * If the return code is different from ERR_NONE, the UTC value should be considered
 * as undefined.
 *
 * The content of the UTC register is updated internally during the RF boot sequence
 * when UTC_EN is set to 1b
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] utcValue       : Byte array containing the UTC value, allocated by the caller
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_GetUniqueTapCode(const uint8_t *uid, uint8_t utcValue[3]);

/**
 *****************************************************************************
 * \brief Reads the Tamper Detection Update enable content
 *
 * Returns the status of the Tamper Event Update Enable register.
 * Only relevant on ST25TV02KC-T devices.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] isEnabled      : Byte containing the Tamper Event Update Enable value
 *                              Set to 1 if the updates are enabled, 0 if they are disabled.
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_IsTamperEventUpdateEnabled(const uint8_t *uid, bool *isEnabled);

/**
 *****************************************************************************
 * \brief Get the tag's Tamper Detect Status
 *
 * Returns the byte containing the status of the Tamper Detect register.
 * Only relevant on ST25TV02KC-T devices.
 * If the return code is different from ERR_NONE, the tamper stratus value should
 * be considered as undefined.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] tamperStatus   : Byte containing the Tamper Detection value
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_GetTamperDetectStatus(const uint8_t *uid, uint8_t *tamperStatus);

/**
 *****************************************************************************
 * \brief Get the tag's ANDEF separator value
 *
 * Returns the byte containing the value of the optional separator used to separate
 * fields of the Augmented NDEF URL (ASCII character)
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] utcValue       : Byte containing an ASCII value representing the ANDEF field separator
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefGetSeparatorValue(const uint8_t *uid, uint8_t *separator);

/**
 *****************************************************************************
 * \brief Get the tag's Augmented NDEF Custom Message
 *
 * Returns a byte array containing the 8 ASCII characters composing the Custom message.
 * If the return code is different from ERR_NONE, the Custom message value should be
 * considered as undefined.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] customMessage  : Byte array containing the Custom Message, allocated by the caller
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefGetCustomMessage(const uint8_t *uid, uint8_t customMessage[8]);

/**
 *****************************************************************************
 * \brief Read the ANDEF_UID_EN bit of ST25TVC
 *
 * Returns the value of the ANDEF_UID_EN bit of the ANDEF_CFG register, indicating
 * if the UID is appended to the augmented URL or not.
 *
 * \param[in] uid             : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] *isEnabled     : True if the UID is appened to the URL, false otherwise
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefIsUidEnabled(const uint8_t *uid, bool *isEnabled);

/**
 *****************************************************************************
 * \brief Read the ANDEF_CUS_EN bit of ST25TVC
 *
 * Returns the value of the ANDEF_CUS_EN bit of the ANDEF_CFG register, indicating
 * if a custom message is appended to the augmented URL or not.
 *
 * \param[in] uid             : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] *isEnabled     : True if a custom message is appened to the URL, false otherwise
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefIsCustomMessageEnabled(const uint8_t *uid, bool *isEnabled);

/**
 *****************************************************************************
 * \brief Read the ANDEF_UTC_EN bit of ST25TVC
 *
 * Returns the value of the ANDEF_UTC_EN bit of the ANDEF_CFG register, indicating
 * if the Unique Tap Code is appended to the augmented URL or not.
 *
 * \param[in] uid             : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] *isEnabled     : True if the UTC is appened to the URL, false otherwise
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefIsUtcEnabled(const uint8_t *uid, bool *isEnabled);

/**
 *****************************************************************************
 * \brief Read the ANDEF_TD_EN bit of ST25TVC
 *
 * Returns the value of the ANDEF_TD_EN bit of the ANDEF_CFG register, indicating
 * if the Tamper Detection status is appended to the augmented URL or not.
 * Only relevant on ST25TV02KC-T devices.
 *
 * \param[in] uid             : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] *isEnabled     : True if the TD status is appened to the URL, false otherwise
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefIsTamperDetectEnabled(const uint8_t *uid, bool *isEnabled);

/**
 *****************************************************************************
 * \brief Read the ANDEF_SEP_EN bit of ST25TVC
 *
 * Returns the value of the ANDEF_SEP_EN bit of the ANDEF_CFG register, indicating
 * if an ASCII separator character is appended to the augmented URL or not.
 *
 * \param[in] uid             : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] *isEnabled     : True if a separator character is appened to the URL, false otherwise
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefIsSeparatorEnabled(const uint8_t *uid, bool *isEnabled);

/**
 *****************************************************************************
 * \brief Get Augmented NDEF Length
 *
 * Returns the length of the Augmented NDEF data (to be added to the NDEF message size).
 * Its value is 0 if the ANDEF feature is not enabled.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[out] andefLength    : length of the Augmented NDEF data
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefGetAndefLengthInBytes(const uint8_t *uid, uint8_t *andefLength);

/**
 *****************************************************************************
 * \brief Configures the ANDEF fields
 *
 * The ANDEF must be configured before writing the NDEF message on tag.
 * Note: a field reset must occur in order tfor the ANDEF configuration to be taken into account.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[in] enableFeatureMask : byte containing
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefConfigure(const uint8_t *uid,
        ST25TAG_TVC_AndefConfig_t andefConfig);

/**
 *****************************************************************************
 * \brief Sets the ANDEF Separator value
 *
 * The Separator byte is inserted between ANDEF fields when there are several of them.
 * ANDEF_SEP must contain an ASCII value.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[in] separator       : ASCII value of the separator byte
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefSetSeparatorValue(const uint8_t *uid, const uint8_t separator);

/**
 *****************************************************************************
 * \brief Sets the ANDEF Custom Message
 *
 * The 8-char array customMessage will be concatenated into CUSTOM_LSB and CUTOM_MSB registers
 * in this order.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[in] customMessage   : ASCII value of the custom message on 8 bytes
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefSetCustomMessage(const uint8_t *uid, const char customMessage[8]);

/**
 *****************************************************************************
 * \brief Sets the various Tamper Detect Messages
 *
 * The ST25TAG_TVC_TamperMessages_t structure must be filled with values for
 * TD_SEAL_MSG, TD_UNSEAL_MSG and TD_RESEAL_MSG on 2 bytes and
 * TD_SHORT_MSG and TD_OPEN_MSG on 1 byte.
 *
 * Note: The tamper detection field is available on ST25TV02KC-T part number.
 *
 * \param[in]  uid            : pointer to UID byte array. The expected length is 8 bytes
 *                              and the last byte of the array must be equal to 0xE0.
 * \param[in] tamperMessages  : Structure containing the values of the different Tamper Detection messages
 *
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_REQUEST        : Cannot be executed at the moment
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode ST25TAGS_TVC_AndefSetTamperMessages(const uint8_t *uid,
        const ST25TAG_TVC_TamperMessages_t tamperMessages);

#ifdef __cplusplus
}
#endif

#endif /* ST25TVC_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
