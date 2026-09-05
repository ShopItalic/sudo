if %time:~0,2% leq 9 (set hour=0%time:~1,1%) else (set hour=%time:~0,2%)
del s2l_boot_dfu*.zip

@REM 软件版本文件路径
set VERSION_FILE_PATH=..\..\..\..\bc_ros\bc_config\ring_config.h

@REM 软件版本字符串的格式
set SOFTWARE_VERSION="#define RING_SOFTWARE_VERSION"

@REM 硬件版本字符串的格式
set HARDWARE_VERSION="#define RING_HARDWARE_VERSION"

@REM 获取软件版本
for /f "tokens=3 delims= " %%i in ('findstr /C:%SOFTWARE_VERSION% %VERSION_FILE_PATH%') do set software_version=%%i
set software_version=%software_version:~1,-1%

@REM 获取软件版本
for /f "tokens=3 delims= " %%i in ('findstr /C:%HARDWARE_VERSION% %VERSION_FILE_PATH%') do set hardware_version=%%i
set hardware_version=%hardware_version:~6,-1%


nrfutil pkg generate --bootloader ..\softdevice_boot\s2p_mac_fd3_boot.hex --bootloader-version %software_version% --hw-version 52 --sd-req 0x0101 --key-file ..\softdevice_boot\private.key S2l_boot_dfu_HV%hardware_version%_SV%software_version%.zip