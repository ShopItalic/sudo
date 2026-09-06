# Sudo × Feiyang / Bravechip — Firmware Alignment Call
# Sudo × 飞扬 / Bravechip — 固件对齐会议

**Time / 时间:** Saturday 2026-06-13, 9:30 AM China (Friday 2026-06-12, 9:30 PM New York) — WeChat video
**Sudo:** Jeremy (蔡杰瑞), waisoon (firmware/app dev), Vivien Xie (translation/ops)
**Feiyang / Bravechip:** Mr. Zhang (飞扬科技), James, Tan, U, 廖楷华, 韩zj

**Hardware context / 硬件背景:** Board 主板 1.23.2 · Firmware 固件 6.0.1.8Z62 · MCU Nordic nRF52840 · Flash 16MB · Sample battery 样品电池 12 mAh (MP: 18.5 mAh for #9/10, 22.5 mAh for #11/12)

---

## AGENDA / 会议议程

### 1. Product goal framing (5 min) / 产品定位说明

Sudo is a **voice / AI interaction ring**, not a health-tracking ring. The entire product depends on one interaction: **long-press to record, release to stop.** The ring only does storage and BLE transfer — all transcription happens on the phone.

Sudo 是一款**语音 / AI 交互戒指**，不是健康监测戒指。整个产品依赖一个核心交互：**长按录音，松开停止**。戒指只负责存储和蓝牙传输，语音转文字全部在手机端完成。

### 2. THE DECISION: custom firmware path (20 min) / 核心决策：定制固件路径

**Question 1: Can Sudo work on custom firmware ourselves? / Sudo 能否自己开发定制固件？**

Three things to resolve, in order:

1. **Is what we received buildable source?** The email (BCL603S2P 1.23.2 SDK V1.0) — is this the complete, compilable firmware source for board 1.23.2, or only SDK/secondary-development material? Walk through the build toolchain on the call.
   收到的邮件资料是完整可编译的固件源代码，还是只有 SDK / 二次开发资料？请在会上确认编译工具链。
2. **Flashing is blocked by your signature.** We built/obtained the files but cannot flash because the firmware image is signed. We need either (a) the signing key/process, (b) a signing service where you sign our builds, or (c) you flash our builds for us. Which is possible?
   烧录被你们的签名机制阻止了。我们需要：(a) 签名密钥/流程，或 (b) 你们帮我们的固件签名，或 (c) 你们代为烧录。哪种可行？
3. **OTA path & version control.** Overseas OTA works through the public app — can custom Sudo firmware be pushed through OTA to our units only? And can our units be pinned to our firmware version? (The 6.0.0.6 → 6.0.1.8 upgrade silently removed release-detection behavior we depended on — this cannot happen on production units.)
   定制固件能否通过 OTA 只推送给我们的设备？能否锁定固件版本？（6.0.0.6 升级到 6.0.1.8 时，我们依赖的松开检测行为被移除了——量产设备上不能再发生这种情况。）

**Question 2: If not source access, will you implement our changes? / 如果不开放源代码，你们能否按需求修改固件？**

If Bravechip keeps the source closed, we need a written commitment: which items from the change list below will be implemented, by what date, at what cost. 廖楷华 already said the long-press requirement "需要按照这个需求定制固件" (requires custom firmware) and that it needs commercial alignment — that is exactly what this call decides.

如果 Bravechip 不开放源代码，我们需要书面承诺：下方需求清单中哪些项目会实现、完成日期、费用。

**Also clarify / 同时确认:**
- Who owns this decision — Feiyang or Bravechip? Is the right decision-maker on this call? / 这个决策由飞扬还是 Bravechip 拍板？决策人是否在会上？
- Cost and commercial terms for custom firmware work. / 定制固件的费用和商务条款。

### 3. Walk the change list, item by item (25 min) / 逐项过需求清单

Go through Section B below. For each item the answer must be one of:
**(a) works today — show us how · (b) firmware change, will do by [date] · (c) needs custom firmware · (d) hardware limitation — explain**

逐项确认下方清单。每项需明确：**(a) 现有固件已支持——请演示 · (b) 固件修改，承诺完成日期 · (c) 需定制固件 · (d) 硬件限制——请解释原因**

### 4. Documentation gaps (5 min) / 文档缺口

See Section C. Commit to delivery dates. / 见 C 部分，确认提供日期。

### 5. Timeline & acceptance (5 min) / 时间节点与验收标准

- **Deadline question:** assembly starts mid-June production window, packaging arrives ~July 9–11, target ship 7/27. **What is the last date firmware can be finalized and still be flashed on all 400 MP units?**
  关键问题：固件最晚什么时候定稿，才能保证 400 台量产设备全部烧录新固件？
- **Acceptance test / 验收标准:** demo on a sample ring showing — long press → green LED + short haptic + mic on; release → mic off (release event or stream stops within 1s); stable app connection through a 10-minute session; offline recording saved and synced on reconnect.
  在样品戒指上演示：长按 → 绿灯 + 短震动 + 麦克风开启；松开 → 麦克风关闭（松开事件或音频流 1 秒内停止）；连续 10 分钟连接稳定；离线录音保存并在重连后同步。

### 6. Wrap-up (5 min) / 总结

Every commitment gets an owner + date, confirmed in writing (email, not only WeChat). Schedule follow-up check-in.
每项承诺明确负责人和日期，并通过邮件书面确认（不只是微信）。约定下次跟进时间。

---

## B. CONSOLIDATED CHANGE REQUEST LIST / 固件需求清单

### B1. Core interaction model — highest priority / 核心交互模型（最高优先级）

| # | Current behavior 现状 | Required behavior 需求 |
|---|---|---|
| 1 | Double-tap starts/stops recording 双击触发录音和停止 | **Long press activates mic; release stops** 长按激活麦克风，松开停止 |
| 2 | No release event — SDK key events (0–7) only fire on touch; 6.0.0.6 stopped the PCM stream on release (usable as release signal), 6.0.1.8 removed this 没有松开事件——键值只在触摸时上报；6.0.0.6 松开时音频流会停止（可作为松开信号），6.0.1.8 移除了该行为 | **Ring sends a release event** (or reliably stops the audio stream on release so the app can detect ~1s of silence) 戒指上报松开事件（或松开时可靠停止音频流，app 检测约 1 秒无数据即停止） |
| 3 | — | **Standalone mode 独立模式:** long press → mic on, green LED + short haptic; release → save audio to local storage 长按 → 麦克风开启，绿灯 + 短震动；松开 → 音频保存至本地存储 |
| 4 | — | **Storage full → delete oldest recordings first** 存储满时从最旧的录音开始清理 |
| 5 | — | **App-connected mode 连接模式:** long press → mic on, green LED + short haptic; release → mic closes; real-time audio stream to app (app transcribes); **no local storage while connected** 长按 → 麦克风开启，绿灯 + 短震动；松开 → 麦克风关闭；实时音频流发送至 app（app 转文字）；连接时不写本地存储 |
| 6 | — | **Reconnect sync:** local files sync to app on reconnection; ring deletes local audio **only after app confirms receipt** 重连后本地文件同步至 app；app 确认收到后戒指才删除本地音频 |
| 7 | — | **Keyboard mode 键盘模式:** long press for voice-to-text input; (related) can the ring receive UTF-8 text over a BLE write characteristic and output it as HID keyboard reports to the host? 长按语音输入文字；（相关）戒指能否通过 BLE 写特征接收 UTF-8 文本并以 HID 键盘形式输出到主机？ |

### B2. LED / 灯光

| # | Current behavior 现状 | Required behavior 需求 |
|---|---|---|
| 8 | LEDs not controllable from app SDK; colors fixed in firmware LED 无法通过 SDK 控制，颜色固件写死 | **Software control: LED on/off and color configurable via API** 通过 API 控制 LED 开关和颜色 |
| 9 | Inconsistent colors by trigger source: double-tap = green, app-initiated recording = purple, demo-app PCM/ADPCM streaming = red/blue 触发来源不同颜色不同：双击=绿，app 下发=紫，demo app 音频流=红/蓝 | **Consistent: recording = green LED regardless of how it was triggered** 统一：录音时绿灯，与触发方式无关 |
| 10 | LED meanings undocumented (e.g., blue = charging case battery low) LED 含义无文档（蓝灯=充电仓电量低） | **Full LED state/color reference document** 完整的 LED 状态颜色说明文档 |

### B3. Haptics / 震动

| # | Current behavior 现状 | Required behavior 需求 |
|---|---|---|
| 11 | Haptic strength/duration pre-set in firmware, no API 震动强度/时长固件预设，无接口 | **API control for haptic strength and duration (config interface — yes, we need it)** API 控制震动强度和时长（需要单独的配置接口） |
| 12 | Haptic behavior differs with app open (App SDK) vs closed (base firmware); standalone haptic is unreliable — sometimes no vibration 打开 app 与不打开 app 震动表现不一致；独立模式震动不稳定，有时不震 | **Consistent, reliable single short haptic on activation in all modes** 所有模式下激活时一次性短震动，稳定一致 |
| 13 | Haptic trigger behavior does not match SDK docs 震动触发行为与 SDK 文档不符 | **Fix firmware or fix docs — they must match** 修正固件或文档，两者必须一致 |

### B4. Touch / gestures / 触摸与手势

| # | Current behavior 现状 | Required behavior 需求 |
|---|---|---|
| 14 | Gesture sensitivity too high — accidental right-swipe triggers 手势灵敏度过高，出现误触右滑 | **Reduce sensitivity / make it configurable** 降低灵敏度或可配置 |
| 15 | No way to disable gestures 无法禁用手势 | **Full option to disable gestures** 提供完全禁用手势的选项 |
| 16 | HID mode 4 ("upload real-time audio" per GitBook docs) does not work as documented HID 模式 4（文档写的上传实时音频）实测无效 | **Fix or document actual behavior** 修复或说明实际行为 |

### B5. Connection & transfer stability / 连接与传输稳定性

| # | Current behavior 现状 | Required behavior 需求 |
|---|---|---|
| 17 | Connection to sample ring drops repeatedly during testing 测试中样品戒指连接反复断开 | **Stable BLE connection; root-cause the drops** 稳定的蓝牙连接；排查断连原因 |
| 18 | Data transfer bugs with various opaque error codes 数据传输 bug，出现各种不明错误代码 | **Fix transfer reliability; expose the complete error-code list** 修复传输可靠性；提供完整错误代码列表 |

### B6. Battery & power / 电池与功耗

| # | Current behavior 现状 | Required behavior 需求 |
|---|---|---|
| 19 | Battery % unstable — jumps between values (e.g., 60% ↔ 0%), inconsistent across reads 电量显示不稳定（60% 和 0% 之间跳动），多次读取结果不同 | **Accurate, monotonic battery reporting** 准确稳定的电量上报 |
| 20 | No battery level while charging — only charging status (claimed hardware limit) 充电时无法读取电量，只有充电状态（据称硬件限制） | **Confirm whether this is truly a hardware limit on MP board; if firmware-fixable, fix it** 确认量产主板是否确实是硬件限制；若固件可解决请解决 |
| 21 | Sample (12 mAh) depletes very quickly 样品电池消耗很快 | **Provide power budget: recording current, standby current, LED/haptic draw, projected battery life per size (18.5 / 22.5 mAh)** 提供功耗数据：录音电流、待机电流、LED/震动功耗、各尺寸预计续航 |
| 22 | Charging case has no charge-level display; blue LED = case low 充电仓无电量显示 | **Can the case show charge level? Clarify case state model** 充电仓能否显示电量？说明充电仓状态逻辑 |

### B7. Audio formats / 音频格式

| # | Current behavior 现状 | Required behavior 需求 |
|---|---|---|
| 23 | SDK mixes ADPCM (mono/stereo), PCM, Opus — unclear what hardware actually supports SDK 混用 ADPCM/PCM/Opus，不清楚硬件实际支持哪些 | **Definitive list of supported formats, sample rates, and which API uses which** 明确支持的格式、采样率，以及各 API 对应的格式 |

---

## C. DOCUMENTATION REQUESTS / 文档需求

1. **Complete SDK error-code list** / 完整的 SDK 错误代码列表
2. **File-system / file-transfer protocol** — missing from the AN6034 BCL603M1 BLE protocol PDF V1.9; U pointed to GitBook but waisoon confirmed file-system detail is not there. We need the actual protocol for listing, reading, deleting files on the ring. / 文件系统/文件传输协议——蓝牙协议 PDF 里没有；需要列出、读取、删除戒指文件的完整协议
3. **Battery reporting behavior spec** (update rate, accuracy, charging behavior) / 电量上报机制说明
4. **Charging-case state model** (LED meanings, case battery, multi-size dock behavior) / 充电仓状态模型说明
5. **LED & haptic state reference** for base firmware / 基础固件的 LED 与震动状态说明
6. **Firmware changelog** between versions (6.0.0.6 → 6.0.1.8 broke behavior we used, with no notice) / 固件版本变更说明

---

## D. DESIRED OUTCOME / 期望结果

**Path A (preferred 优先):** Sudo receives buildable firmware source + build toolchain + signing/flashing path. Sudo modifies, shares all changes back via repo. Bravechip reviews/signs.
Sudo 获得可编译固件源码 + 工具链 + 签名/烧录路径。Sudo 修改后通过 repo 全部回传，Bravechip 审核/签名。

**Path B (fallback 备选):** Bravechip/Feiyang implements the B-list on a written schedule with cost, with the B1 items (long press / release / LED / haptic consistency) committed before MP flashing deadline.
Bravechip/飞扬按书面计划和报价实现需求清单，其中 B1 核心交互项必须在量产烧录截止日期前完成。

Either path requires: acceptance demo on sample ring (Section 5 criteria) + email confirmation of all commitments.
两种路径都需要：样品戒指验收演示 + 邮件书面确认所有承诺。
