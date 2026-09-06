# Sudo 固件需求文档（会前预读）
# Sudo Firmware Requirements — Pre-Read Before Our Call

**会议 / Meeting:** 周六 2026-06-13 上午 9:30（北京时间）· 微信视频会议
Saturday 2026-06-13, 9:30 AM China time · WeChat video

**发送方 / From:** Sudo — 蔡杰瑞 Jeremy, waisoon（固件/APP 开发）, Vivien Xie
**接收方 / To:** 飞扬科技 张总, James, Tan, U, 廖楷华, 韩zj（Bravechip）

**测试环境 / Test setup:** 主板 1.23.2 · 固件 6.0.1.8Z62 · MCU Nordic nRF52840 · 样品电池 12 mAh
**参考资料 / References:**
- Swift 框架 SDK 文档（蓝牙指令的高层封装）High-level Swift framework SDK docs: https://yongxin.gitbook.io/yongxin-docs/documentation
- APP SDK: https://github.com/BravechipSpace/ChipletRing-APPSDK
- 底层蓝牙协议文档 Low-level Bluetooth protocol docs: 【AN6034】BCL603M1 智能戒指蓝牙通信协议 V1.9.pdf
- 邮件资料 Emailed package: BCL603S2P 1.23.2 SDK V1.0

请各位在会前阅读本文档。对第 3 部分的每一项，请准备好以下四种回答之一：
Please read before the call. For each item in Section 3, please come prepared with one of four answers:

> **(A)** 现有固件已支持——请告诉我们正确用法 / Already supported — show us the correct usage
> **(B)** 固件可修改——承诺完成日期 / Firmware change — committed completion date
> **(C)** 需要定制固件 / Requires custom firmware
> **(D)** 硬件限制——请说明原因 / Hardware limitation — please explain

---

## 1. 产品定位 / Product Goal

Sudo 是一款**语音 / AI 交互戒指**，不是健康监测戒指。我们不使用 SDK 中的健康功能（心率、睡眠、血氧、运动等）。整个产品依赖一个核心交互：

**长按录音，松开停止。**

戒指只负责录音、本地缓存和蓝牙传输。语音转文字全部在手机 APP 端完成，戒指端不需要任何 ASR 算力。

Sudo is a **voice / AI interaction ring**, not a health tracker. We do not use the health features in the SDK (heart rate, sleep, SpO2, exercise, etc.). The entire product depends on one interaction: **long press to record, release to stop.** The ring only records, buffers locally, and transfers over BLE. All speech-to-text runs on the phone.

### 目标交互流程 / Target interaction flow

**独立模式（未连接手机）/ Standalone mode (phone not connected):**
1. 长按 → 激活麦克风，绿色 LED + 一次短震动
   Long press → mic on, green LED + one short haptic
2. 松开 → 停止录音，音频保存至本地存储
   Release → recording stops, audio saved to local storage
3. 重新连接 APP 后自动同步；**APP 确认收到后**戒指才删除本地文件
   On reconnect, sync to app; ring deletes local files **only after app confirms receipt**

**连接模式（已连接 APP）/ Connected mode:**
1. 长按 → 激活麦克风，绿色 LED + 一次短震动
   Long press → mic on, green LED + one short haptic
2. 实时音频流发送至 APP（APP 端转文字），连接时**不写本地存储**
   Real-time audio stream to app (app transcribes); **no local storage** while connected
3. 松开 → 麦克风关闭，音频流停止；APP 约 1 秒未收到数据即判定结束
   Release → mic closes, stream stops; app treats ~1s of no data as end of transmission

---

## 2. 核心决策：定制固件路径 / The Decision: Custom Firmware Path

廖楷华已确认长按/松开交互"需要按照这个需求定制固件"。本次会议需要确定走哪条路径：
廖楷华 has confirmed the long-press/release interaction "requires custom firmware." This call needs to decide which path we take:

**路径 A（我们的优先选择）/ Path A (our preference):**
Sudo 获得可编译的固件源代码 + 编译工具链说明 + 签名/烧录路径。我们自己修改，所有改动通过 repo 完整回传给 Bravechip 审核。我们的目的不是绕开或转售你们的软件——只是因为时差和沟通成本，这样双方都更快。
Sudo receives buildable firmware source + toolchain instructions + a signing/flashing path. We make the changes ourselves and share everything back via a repo for Bravechip review. Our intent is not to sidestep or resell your software — with the time zone difference, this is simply faster for both sides.

**路径 B（备选）/ Path B (fallback):**
Bravechip/飞扬按本文档第 3 部分实现修改，提供书面排期和报价。
Bravechip/Feiyang implements Section 3 on a written schedule with pricing.

### 会前请准备回答 / Please come prepared to answer:

| # | 问题 / Question |
|---|---|
| 2.1 | 邮件发送的 BCL603S2P 1.23.2 SDK V1.0 资料，是完整可编译的固件源代码，还是二次开发 SDK？编译工具链是什么？<br>Is the emailed package complete, compilable firmware source for board 1.23.2, or a secondary-development SDK? What is the build toolchain? |
| 2.2 | 我们已拿到文件，但**烧录被固件签名阻止**。可行方案是哪种：(a) 提供签名流程/密钥；(b) 你们为我们的固件签名；(c) 你们代为烧录？<br>We have the files but **flashing is blocked by your firmware signature.** Which is possible: (a) signing process/keys; (b) you sign our builds; (c) you flash our builds? |
| 2.3 | 定制固件能否通过 OTA 服务只推送给 Sudo 的设备（参考文档：[OTA升级服务](https://yongxin.gitbook.io/yongxin-docs/documentation/sheng-ji-fu-wu/ota-sheng-ji-fu-wu)、[获取固件列表](https://yongxin.gitbook.io/yongxin-docs/documentation/sheng-ji-fu-wu/huo-qu-gu-jian-lie-biao)）？我们的设备能否锁定固件版本？<br>Can custom Sudo firmware be pushed via your OTA service to Sudo units only? Can our units be pinned to our firmware version? |
| 2.4 | 背景：6.0.0.6 升级到 6.0.1.8 后，松开时音频流停止的行为被移除，且无变更说明。量产设备上不能发生未经我们确认的固件行为变更。今后能否提供版本变更说明（changelog）？<br>Context: the 6.0.0.6 → 6.0.1.8 upgrade removed release-stops-stream behavior with no changelog. Production units cannot receive behavior changes without our sign-off. Can you provide changelogs going forward? |
| 2.5 | 定制固件的费用和商务条款是什么？这个决策由飞扬还是 Bravechip 确定？<br>What are the cost and commercial terms for custom firmware work? Does Feiyang or Bravechip own this decision? |

---

## 3. 需求清单 / Change Request List

### 3.1 核心交互（最高优先级）/ Core Interaction (Highest Priority)

| # | 现状 / Current | 需求 / Required | 答复 A/B/C/D |
|---|---|---|---|
| 1 | 当前固件双击触发录音和停止<br>Double-tap starts/stops recording | **长按激活麦克风，松开停止**<br>**Long press activates mic, release stops** | |
| 2 | [HID手势指令](https://yongxin.gitbook.io/yongxin-docs/documentation/zhi-neng-jie-zhi-sdk-shi-yong/hid-shou-shi-zhi-ling) 文档定义键值 0x0(长按)~0x7(右滑)，但键值**只在触摸时上报，没有松开事件**。6.0.0.6 中松开会停止 PCM 音频流（可作为松开信号），6.0.1.8 移除了该行为<br>The HID gesture docs define key events 0x0 (long press) – 0x7 (right swipe), but events **only fire on touch — there is no release event.** On 6.0.0.6, release stopped the PCM stream (usable as a release signal); 6.0.1.8 removed this | **戒指上报松开事件**，或松开时可靠停止音频流。注：你们的[语音录制](https://yongxin.gitbook.io/yongxin-docs/documentation/zhi-neng-jie-zhi-sdk-shi-yong/yu-yin-lu-zhi)文档本身就写了"1 秒未收到数据默认传输结束"——我们正是要依赖这个机制，但前提是松开时流必须停止<br>**Ring sends a release event,** or reliably stops the audio stream on release. Note: your own voice-recording docs describe the "1s without data = transmission ends" convention — we want to rely on exactly that, but it only works if the stream actually stops on release | |

### 3.2 LED 灯光

| # | 现状 / Current | 需求 / Required | 答复 |
|---|---|---|---|
| 3 | LED 颜色固件写死，SDK 无控制接口<br>LED colors fixed in firmware; no SDK control | **API 控制 LED 开关和颜色**<br>**API control of LED on/off and color** | |
| 4 | [语音录制](https://yongxin.gitbook.io/yongxin-docs/documentation/zhi-neng-jie-zhi-sdk-shi-yong/yu-yin-lu-zhi)文档写"录音=绿灯"，但 6.0.1.8 实测：双击=绿灯，APP 下发录音=紫灯，demo APP PCM/ADPCM 音频流=红灯/蓝灯<br>Voice-recording docs say recording = green, but on 6.0.1.8: double-tap = green, app-initiated = purple, demo-app PCM/ADPCM streaming = red/blue | **统一：录音时绿灯，与触发方式无关**（与文档一致）<br>**Consistent green LED whenever recording, regardless of trigger** (matching your own docs) | |
| 5 | LED 状态含义无完整文档（例：蓝灯=充电仓电量低，是口头告知的）<br>No complete LED reference (e.g., blue = case battery low was told verbally) | **完整的 LED 状态/颜色说明文档**，覆盖基础固件全部状态<br>**Complete LED state/color reference** covering all base-firmware states | |

### 3.3 震动 / Haptics

| # | 现状 / Current | 需求 / Required | 答复 |
|---|---|---|---|
| 6 | [震动、闹钟](https://yongxin.gitbook.io/yongxin-docs/documentation/zhi-neng-jie-zhi-sdk-shi-yong/zhen-dong-nao-zhong)文档提供 `SET_MOTOR(time, type)` / `linearMotorImmediateVibration(type:)`，3 种震动类型，但**实测触发行为与文档不一致**<br>Vibration docs provide `SET_MOTOR(time, type)` / `linearMotorImmediateVibration(type:)` with 3 types, but **observed behavior does not match the docs** | 修复固件或修正文档，两者必须一致；明确**强度和时长**的实际可控范围<br>Fix firmware or docs so they match; clarify the actual controllable range for **strength and duration** | |
| 7 | 触觉反馈在 APP 打开（走 APP SDK）与未打开（走基础固件）时表现不一致；独立模式震动不稳定，有时不震<br>Haptics differ with app open (App SDK) vs closed (base firmware); standalone haptic is unreliable — sometimes no vibration | **所有模式下激活录音时一次性短震动，稳定一致**<br>**One consistent short haptic on recording activation, in all modes** | |
| 8 | 基础固件的震动参数是固件预设的，无配置接口（廖楷华 6/8 已问是否需要单独配置接口）<br>Base-firmware haptic parameters are pre-set; no config interface (廖楷华 asked on 6/8 whether we need one) | **是，需要配置接口**：震动强度和时长可由 APP 配置并在基础固件中生效<br>**Yes, we need the config interface:** strength and duration configurable from the app, applied in base firmware | |

### 3.4 触摸与手势 / Touch & Gestures

| # | 现状 / Current | 需求 / Required | 答复 |
|---|---|---|---|
| 9 | 手势灵敏度过高，频繁误触右滑（0x7）<br>Gesture sensitivity too high; frequent accidental right-swipes (0x7) | 降低灵敏度或提供灵敏度配置<br>Lower sensitivity or make it configurable | |
| 10 | HID 模式可设为禁用（Android -1 / iOS 255），但请确认这是否**完全**禁用所有手势（包括误触发的滑动）<br>HID mode can be set to disabled (Android -1 / iOS 255), but confirm this **fully** disables all gestures including accidental swipes | **完全禁用手势的选项**，只保留长按<br>**Full gesture-disable option,** keeping only long press | |

### 3.5 连接与传输稳定性 / Connection & Transfer Stability

| # | 现状 / Current | 需求 / Required | 答复 |
|---|---|---|---|
| 11 | 测试中样品戒指连接反复断开<br>Repeated connection drops during testing | 排查断连根因；提供稳定连接的配置建议<br>Root-cause the drops; provide configuration guidance for stable connections | |
| 12 | 数据传输出现多种错误代码，SDK 未暴露完整错误代码列表<br>Transfer bugs return various error codes; the SDK does not expose the full list | 修复传输可靠性；**提供完整错误代码列表及含义**<br>Fix transfer reliability; **provide the complete error-code list with meanings** | |

### 3.6 电池与功耗 / Battery & Power

| # | 现状 / Current | 需求 / Required | 答复 |
|---|---|---|---|
| 13 | 电量读取不稳定（在 60% 和 0% 之间跳动，多次读取结果不同）<br>Battery % unstable (jumps between 60% and 0%; inconsistent reads) | **准确、单调的电量上报**<br>**Accurate, monotonic battery reporting** | |
| 14 | 充电时只能读取充电状态，无法读取电量（韩zj：当前硬件做不到）<br>While charging, only charging status is available, not battery level (韩zj: current hardware cannot) | 确认**量产主板**是否同样受限；若固件可解决请解决<br>Confirm whether the **MP board** has the same limit; if firmware-fixable, fix it | |
| 15 | 样品（12 mAh）耗电很快；量产电池 18.5/22.5 mAh 容量接近<br>Sample (12 mAh) drains fast; MP batteries (18.5/22.5 mAh) are not much bigger | 提供**功耗预算**：录音电流、待机电流、LED/震动功耗、各尺码预计续航<br>Provide a **power budget:** recording current, standby current, LED/haptic draw, projected battery life per size | |
| 16 | 充电仓只有蓝灯=低电提示，无电量显示<br>Charging case only shows blue LED = low battery | 充电仓能否显示电量？请说明充电仓完整状态逻辑<br>Can the case show charge level? Document the full case state model | |

### 3.7 音频格式 / Audio Formats

| # | 现状 / Current | 需求 / Required | 答复 |
|---|---|---|---|
| 17 | SDK 混用多种格式：PCM 16-bit、单声道 ADPCM（8K）、立体声 ADPCM（16K，仅 Z5J 双麦固件），文件系统中还有 Opus 类型。1.23.2 主板实际支持哪些不明确<br>The SDK mixes formats: PCM 16-bit, mono ADPCM (8K), stereo ADPCM (16K, Z5J only), plus an Opus file type in the file system. Unclear what board 1.23.2 actually supports | **明确清单**：1.23.2 硬件支持的格式、采样率，以及每个 API 对应的格式；Sudo 量产固件应使用哪种<br>**Definitive list:** formats and sample rates supported by 1.23.2, which API uses which, and which Sudo MP firmware should use | |

---

## 4. 文档需求 / Documentation Requests

| # | 需求 / Request | 说明 / Notes |
|---|---|---|
| D1 | 完整 SDK 错误代码列表 / Complete SDK error-code list | 当前未暴露 / Not currently exposed |
| D2 | 底层蓝牙协议中的文件系统说明 / File-system protocol in the low-level Bluetooth documentation | 你们提供了两套文档：(1) Swift 框架 SDK（GitBook，蓝牙指令的高层封装），其中有文件系统 API；(2) 底层蓝牙通信协议文档（AN6034 PDF），其中**没有**文件系统协议。如果我们做固件二次开发或非 Swift 实现，需要底层协议细节（指令、报文格式）<br>Two documentation sets were provided: (1) the Swift framework SDK (GitBook — high-level wrapper over the Bluetooth commands), which includes the file-system APIs; (2) the low-level Bluetooth protocol documentation (AN6034 PDF), which has **no** file-system protocol. For firmware-level work or non-Swift implementations we need the low-level protocol detail (commands, packet formats) |
| D3 | 电量上报机制说明 / Battery reporting spec | 上报频率、精度、充电时行为 / Update rate, accuracy, charging behavior |
| D4 | 充电仓状态模型 / Charging-case state model | LED 含义、仓体电量、多尺码充电座兼容性 / LED meanings, case battery, multi-size dock |
| D5 | 基础固件 LED 与震动状态表 / Base-firmware LED & haptic state table | 不连接 APP 时的全部行为 / All behavior when app is not connected |
| D6 | 固件版本变更说明 / Firmware changelogs | 从 6.0.0.6 → 6.0.1.8 开始补发 / Starting retroactively with 6.0.0.6 → 6.0.1.8 |

---

## 5. 时间节点与验收 / Timeline & Acceptance

**关键问题 / Key question:** 包装物料约 7/9–7/11 到厂，目标 7/27 出货。**固件最晚什么时候定稿，才能保证 400 台量产设备全部烧录定稿固件？**
Packaging arrives ~July 9–11; target ship July 27. **What is the last date firmware can be finalized and still be flashed on all 400 MP units?**

**验收标准（样品戒指现场演示）/ Acceptance (live demo on a sample ring):**

1. 长按 → 绿灯 + 一次短震动 + 麦克风开启 / Long press → green LED + one short haptic + mic on
2. 松开 → 麦克风关闭（松开事件或音频流 1 秒内停止）/ Release → mic off (release event, or stream stops within 1s)
3. 连续 10 分钟会话连接稳定、无断连 / Stable connection through a 10-minute session, no drops
4. 离线录音保存本地，重连后同步至 APP，确认后删除 / Offline recording saved locally, synced on reconnect, deleted after confirmation

**会议输出 / Meeting output:** 每项承诺明确负责人 + 完成日期，会后邮件书面确认。
Every commitment gets an owner + date, confirmed by email after the call.

---

*Sudo 非常重视与飞扬和 Bravechip 的合作。我们的团队会把所有固件改动通过 repo 完整回传，目标是让双方都能更快推进。期待周六的会议。*
*Sudo values the partnership with Feiyang and Bravechip. All our firmware changes would be shared back in full via a repo — the goal is speed for both sides. We look forward to Saturday's call.*
