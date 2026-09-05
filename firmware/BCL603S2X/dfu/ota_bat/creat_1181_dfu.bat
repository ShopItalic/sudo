@echo off

@REM 删除BCL开头的.hex文件
del ..\..\..\dfu\ota_output\BCL*.hex
del ..\..\..\dfu\ota_output\BCL*.zip

@REM check boot文件路径
set CHECK_BOOT_FILE_PATH=..\..\user\inc\sdk_config.h

@REM 校验DFU标志字符串的格式
set CHECK_BOOT_VALID="#define APP_CHECK_BOOT_VALID"

@REM 读取文件内容查找宏定义
for /f "tokens=3 delims= " %%i in ('findstr /C:%CHECK_BOOT_VALID% %CHECK_BOOT_FILE_PATH%') do set check=%%i

@REM 判断宏定义值是否等于字符串 "1"
if "%check%"=="1" (
    echo APP_CHECK_BOOT_VALID is defined as 1
) else (
    echo APP_CHECK_BOOT_VALID is not defined as 1
    echo %check%
    exit
)

@REM 硬件板卡文件路径
set HARDWARE_BATH_FILE_PATH=..\..\..\..\bc_ros\bc_config\ring_config.h

@REM 校验DFU标志字符串的格式
set HARDWARE_BATH_VALID="#define HARDWARE_1181_ENABLED"

@REM 读取文件内容查找宏定义
for /f "tokens=3 delims= " %%i in ('findstr /C:%HARDWARE_BATH_VALID% %HARDWARE_BATH_FILE_PATH%') do set check=%%i

@REM 判断宏定义值是否等于字符串 "1"
if "%check%"=="1" (
    echo HARDWARE_1181_ENABLED is defined as 1
) else (
    echo HARDWARE_1181_ENABLED is not defined as 1
    echo %check%
    exit
)

@REM 版本号文件路径
set VERSION_FILE_PATH=..\..\..\..\bc_ros\bc_config\ring_config.h

@REM 软件版本字符串的格式
set SOFTWARE_VERSION="#define RING_1181_SOFTWARE_VERSION"

@REM 硬件版本字符串的格式
set HARDWARE_VERSION="#define RING_1181_HARDWARE_VERSION"

@REM 获取软件版本
for /f "tokens=3 delims= " %%i in ('findstr /C:%SOFTWARE_VERSION% %VERSION_FILE_PATH%') do set software_version=%%i
set software_version=%software_version:~1,-1%

@REM 获取软件版本
for /f "tokens=3 delims= " %%i in ('findstr /C:%HARDWARE_VERSION% %VERSION_FILE_PATH%') do set hardware_version=%%i
set hardware_version=%hardware_version:~6,-1%

nrfutil settings generate --family NRF52840 --application .\Objects\app.hex --application-version 1 --bootloader-version 1 --bl-settings-version 1 ..\..\..\dfu\ota_bat\settings.hex

mergehex.exe -m ..\..\..\dfu\softdevice_boot\s140_nrf52_7.2.0_softdevice.hex ..\..\..\dfu\softdevice_boot\s2p_mac_fd3_boot.hex ..\..\..\dfu\ota_bat\settings.hex .\Objects\app.hex -o ..\..\..\dfu\ota_output\BCL603S2P_%software_version%.hex

del ..\..\..\dfu\ota_bat\settings.hex

nrfutil pkg generate --application .\Objects\app.hex --application-version 1 --hw-version 52 --sd-req 0x0100 --key-file ..\..\..\dfu\softdevice_boot\private.key ..\..\..\dfu\ota_output\BCL603S2P_%software_version%.zip


