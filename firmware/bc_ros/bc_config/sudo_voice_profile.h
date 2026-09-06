#ifndef SUDO_VOICE_PROFILE_H
#define SUDO_VOICE_PROFILE_H

/* Engineering candidate only: the supplier assigns the DFU release counter. */
/* Both legacy version replies copy exactly ten bytes before the hardware
 * field. Keep the candidate identifiable without truncating its suffix. */
#define RING_1232_SOFTWARE_VERSION "6.0.3.3S02"
typedef char sudo_voice_version_fits_legacy_field[(sizeof(RING_1232_SOFTWARE_VERSION) == 11U) ? 1 : -1];
#if !defined(HANDWARE_1_23_2) || defined(HANDWARE_1_23_3) || defined(USE_OPUS)
#error "Sudo voice profile requires the 603V1.23.2 ADPCM production board"
#endif

/* Preserve BLE HID service identity, bonding, IMU and recording gestures.
 * Remove optional phone/media/presentation action capabilities. */
#define ANDROID_TOUCH_SHORT_VIDEO_HID 0
#define ANDROID_TOUCH_PHOTOGRAPH_HID 0
#define ANDROID_TOUCH_MUSIC_HID 0
#define ANDROID_TOUCH_PPT_HID 0
#define ANDROID_GESTURE_SHORT_VIDEO_HID 0
#define ANDROID_GESTURE_PHOTOGRAPH_HID 0
#define ANDROID_GESTURE_MUSIC_HID 0
#define ANDROID_GESTURE_PPT_HID 0
#define ANDROID_GESTURE_SNAP_HID 0
#define IOS_TOUCH_SHORT_VIDEO_HID 0
#define IOS_TOUCH_PHOTOGRAPH_HID 0
#define IOS_TOUCH_MUSIC_HID 0
#define IOS_TOUCH_PPT_HID 0
#define IOS_GESTURE_SHORT_VIDEO_HID 0
#define IOS_GESTURE_PHOTOGRAPH_HID 0
#define IOS_GESTURE_MUSIC_HID 0
#define IOS_GESTURE_PPT_HID 0
#define IOS_GESTURE_SNAP_HID 0
#define PPG_ENABLED 0
/* Temperature, power and motion support stay until supplier validation can
 * establish whether they participate in safety or recording decisions. */
#endif
