if %time:~0,2% leq 9 (set hour=0%time:~1,1%) else (set hour=%time:~0,2%)
del s2l_boot_dfu_HV4.1.2*.zip
set /p software_version=Enter the software version number:
nrfutil pkg generate --bootloader .\dfu\boot.hex --bootloader-version %software_version% --hw-version 52 --sd-req 0x0101 --key-file .\dfu\private.key S2l_boot.zip