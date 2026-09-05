nrfutil settings generate --family NRF52840 --application .\project\mdk5\Objects\app.hex --application-version 1 --bootloader-version 1 --bl-settings-version 1 .\settings.hex
if %time:~0,2% leq 9 (set hour=0%time:~1,1%) else (set hour=%time:~0,2%)
mergehex.exe -m .\components\softdevice\s140\hex\s140_nrf52_7.2.0_softdevice.hex .\dfu\boot.hex .\settings.hex .\project\mdk5\Objects\app.hex -o bcl603m5_all_%date:~0,4%%date:~5,2%%date:~8,2%_%hour%%time:~3,2%%time:~6,2%.hex
del .\settings.hex