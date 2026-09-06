# Connected recording request supplied by Jeremy

Source: user message in the **Document Ring BOM and stack** task on September 6,
2026. The Chinese request below is retained as supplied. Its app-workaround
description predates the current direct-write implementation; see the
[recording and PTT review](../../ring-recording-and-ptt.md).

各位好，我们已经定位到 Ring 在连接手机时无法生成录音文件的问题，想请你们确认并协助修复。

当前设备信息：

- 硬件：603V1.23.2
- 固件：6.0.3.3Z62
- 固件目标：标准 `1.23.2`，不是 `1.23.2_one_sec`

我们在源码中发现：

1. Ring 连接 BLE 时，双击会调用 `app_pdm_touch_start()`，进入 `PDM_MODE_ONLINE`。
2. 在该模式下，音频只通过 BLE 发送，并不会写入 Flash。
3. 在线模式写 Flash 的代码只在以下宏中启用：

```c
HANDWARE_1_23_3
HANDWARE_1_23_2_ONE_SEC
```

但标准 1.23.2 固件没有启用这两个宏，因此连接手机录音时不会产生 Flash 文件。

4. Ring 未连接时，会调用 `app_pdm_recording_start()`，进入离线模式并正常创建 LittleFS 录音文件。
5. 固件中还存在 `app_pdm_switch_online_to_offline()` 和命令 `0x71 / 0xFD`，但目前似乎没有正式使用，也没有可靠的成功/失败响应，并且 BCL SDK 没有公开对应接口。

因此，目前的实际行为是：

- 未连接手机：录音会保存到 Flash，可以之后同步。
- 已连接手机：音频只在线发送，不创建 Flash 文件，因此 App 后续查询文件列表时没有任何文件可以下载。

我们现在的 App 临时方案是先停止在线音频，再调用 `ringStartRecording(true)` 开启离线 Flash 录音；第二次双击后调用 `ringStartRecording(false)` 并同步文件。但 `controlADPCMFormatAudio(false)` 似乎没有可靠 ACK，因此这里可能存在命令时序或状态竞争问题。

想请你们确认：

1. 标准 1.23.2 固件在 BLE 连接状态下不保存 Flash，是否是预期设计？
2. App 当前“停止在线录音后再调用 `ringStartRecording(true)`”的方式是否安全？两条命令之间是否需要等待特定状态或时间？
3. 能否提供以下任一正式解决方案？
   - 连接手机录音时，同时将完整音频保存到 Flash；
   - 或提供一个正式、带 ACK 的“在线转离线录音”命令；
   - 或更新 BCL SDK，提供可靠的 Flash 录音开始、停止及状态查询接口。
4. 开始录音的响应最好明确返回成功或失败；停止后最好能返回文件名/文件 ID、文件大小和完成状态，方便 App 验证文件确实已经保存。
5. 如果 BLE 中断、App 被系统关闭或 Ring 重启，录音文件也应能够安全关闭并在下次连接时恢复同步。

我们的验收标准是：Ring 保持连接手机，双击开始录音，再双击停止；随后文件列表中必须出现一个新的非零大小文件，并且 App 可以完整下载和播放。

麻烦你们确认问题原因，并提供推荐的协议流程、固件补丁或新版固件/SDK。谢谢！
