#!/usr/bin/env python3
"""Render the recorded September 10 ArmCC software qualification as standalone HTML."""

import argparse
import hashlib
import html
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]


def read(path):
    return json.loads(path.read_text())


def esc(value):
    return html.escape(str(value))


def table(headers, rows):
    return '<div class="scroll"><table><thead><tr>' + ''.join('<th>' + esc(h) + '</th>' for h in headers) + '</tr></thead><tbody>' + ''.join('<tr>' + ''.join('<td>' + str(c) + '</td>' for c in row) + '</tr>' for row in rows) + '</tbody></table></div>'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    base = args.evidence.resolve()
    build = read(base / "result.json")
    runtime = read(base / "runtime-regression.json")
    host = read(base / "host-tests.json")
    contract = read(base / "source-contract.json")
    baseline = read(base.parent / "vendor-baseline-verification.json")
    assert build["errors"] == 0 and host["exitCode"] == 0 and baseline["factoryByteIdentical"]
    assert build["changed_inputs"] == [] and contract["source_inputs_unchanged_after_build"]
    assert len(runtime["results"]) >= 33 and len(runtime["stack_inventory"]["tasks"]) == 13
    for item in build["outputs"]:
        path = Path(item["path"])
        assert hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], path
    assert runtime["bin_sha256"] == next(x["sha256"] for x in build["outputs"] if x["path"].endswith('.bin'))
    assert runtime["elf_sha256"] == next(x["sha256"] for x in build["outputs"] if x["path"].endswith('.axf'))

    stack_rows = []
    for s in runtime["stack_inventory"]["tasks"]:
        stack_rows.append([f'<code>{esc(s["function"])}</code>', f'{s["configured_bytes"]:,}',
                           f'{s["linker_direct_depth_bytes"]:,}',
                           '—' if s['largest_listed_cpu_probe_bytes'] is None else f'{s["largest_listed_cpu_probe_bytes"]:,}',
                           f'{s["remaining_after_screen_bytes"]:,}'])
    probe_rows = []
    for r in runtime["results"]:
        name = r['name'] + (f' · epoch {r["epoch"]}' if 'epoch' in r else '')
        outcome = 'Expected diagnostic reset' if r.get('expected_reset') else ('Returned' if r['returned'] else 'Reached stated stop boundary')
        probe_rows.append([esc(name), r['ipsr'], r['stack_bytes'], esc(outcome)])
    output_rows = []
    for item in build['outputs']:
        path = Path(item['path']).relative_to(ROOT)
        output_rows.append([f'<a href="../../{esc(path)}">{esc(path.name)}</a>', f'{item["bytes"]:,}', f'<code class="hash">{esc(item["sha256"])}</code>'])

    dispositions = [
        ('BLE connection and Peer Manager logging reset', 'Fixed for selected runtime',
         'Restored Arm Microlib and removed all BC log formatting/argument evaluation from interrupts. The complete connection callback returns with actual QWR initialization, registered application callback and real kernel queues/timers; the security-start callback also returns.',
         'IPSR 38 CPU probes; no printf, vsnprintf, localtime or task-only tick API reached. Pairing/radio remain physical checks.'),
        ('RTC eighth-tick calendar reset', 'Fixed',
         'The interrupt compares hours, minutes and seconds using integer arithmetic. Actual alarm configuration and callback registration succeed; clock-of-day results match the linked Arm library across midnight, 2038 and uint32 wrap.',
         'Host coverage of all 86,400 seconds; nine linked-library/alarm comparisons. The interrupt alarm path uses 56 B in the model.'),
        ('BLE receive, PWM and motion log resets', 'Fixed for selected paths',
         'The Sudo macros gate before formatting or argument evaluation, including the separate hex logger. Direct helper calls also reject interrupt context. PWM is tested in its already-idle stopped state.',
         'Actual IPSR 38/44/22 callbacks return. BLE receive is also exercised with a real queue; largest tested callback depth is 616 B.'),
        ('Motion GPIO task-only timer command', 'Fixed',
         'Uses xTimerStartFromISR and yields when needed. The re-enable timer is one-shot and is created before GPIO enable. A full command queue leaves GPIO enabled for a later retry.',
         'Real linked timer creation and full-queue execution; no task tick, allocator or blocking task API reached in the IRQ. Host tests cover initialization failure and ordering.'),
        ('BLE callback task-only tick read', 'Additional defect fixed',
         'The registered connection callback now uses xTaskGetTickCountFromISR. Its timer helper handles an absent timer and honors the ISR yield request.',
         'The expanded test rejected the first restored candidate for calling xTaskGetTickCount in IPSR 38; the final binary passes the same complete callback path.'),
        ('Motor / hardware-check / motion stack overruns', 'Known failures fixed',
         'Each worker now has a 2 KiB stack. Task logs use a serialized 240-byte static buffer. Motor and hardware-check first logs use 232 B each; motion timer creation uses 248 B, before parent frames.',
         'Original GNU observations were 664 / 664 / 672 B against 512 B. New measurements execute the linked Arm library; task probes use real task creation/selection.'),
        ('All worker and timer stack budgets', 'Software budget review complete',
         'All thirteen budgets are read from actual kernel task creation. Touch and BLE receive now have 3 KiB, shared timers 3 KiB, idle 1 KiB. Timer screening includes the largest indirect callback, rather than treating it as a zero-depth call.',
         'Every known-path screen fits with a 256 B Cortex-M4F context reserve. Static unknown calls/cycles and full workload high-water marks remain explicitly unmeasured.'),
        ('Disabled allocation / stack diagnostics', 'Enabled and tested',
         'Sudo enables the allocation-failure hook and mode-2 stack checking. Hooks record debugger-visible flags; the stack hook records pointers and resets without formatting or allocation.',
         'A real failed allocation sets its flag. Corrupting a real allocated stack guard makes vTaskSwitchContext invoke the hook and request the expected reset. Flags clear on reboot.'),
        ('Startup, source identity and supplier ABI', 'Restored and verified',
         'The baseline matches the factory application byte for byte. Candidate ARM startup, system source, heap_4 and all three original ARM/CMSIS port sources are unchanged from the supplier. The original algorithm archive is unchanged.',
         'Actual dependency hashes, vectors, C scatter initialization, mc_w.l / mf_w.l map entries and zero linker warnings. GNU Newlib files are excluded; its original guard and forensic reproducer remain intact.'),
        ('Flash, RAM, boot and DFU boundary', 'Software placement verified; hardware deferred',
         'The application ends at 0x77b98, below 0xe0000. Actual RAM ends at 0x20039048, leaving 28,600 B. The original oversized region declarations are independently checked against real boundaries.',
         'ELF load bytes match BIN. The 140 KiB RTOS heap and 8 KiB MSP stack are included in 215,280 B RAM; the unused C heap is discarded. No installed-flash or recovery claim.'),
        ('Recording / Opus / storage', 'Preserved; software regressions pass',
         'All 123 Opus 1.6.1 sources remain selected. Actual encoder allocation, initialization and silent-frame self-test succeed both alone and with the modeled task/queue allocations present. Recording, codec fixtures and storage fault suites pass.',
         'Opus helper uses 4,840 B in the CPU model; full worker direct linker depth is 5,288 B against 12,288 B. No physical audio, timing, power or sustained encoding claim.'),
        ('BLE/HID/queues, drivers, clock and watchdog', 'Prior software coverage retained',
         'The full host suite passes, including BLE event routing, transfer/storage fault matrices, driver tests, 54 Opus fixtures and 84,292 codec checks. One pre-existing Python test remains skipped.',
         'Actual bonding, reconnect, notifications, watchdog timing, sensors, buses and power behavior remain recoverable-device checks. No line-by-line proof of all upstream or closed supplier code is claimed.'),
    ]
    disposition_rows = [[f'<strong>{esc(name)}</strong><br><span class="status">{esc(status)}</span>', esc(change), esc(evidence)] for name, status, change, evidence in dispositions]
    compact = {"status": "Software qualification checks passed; physical qualification not performed",
               "date": "2026-09-10", "build": build, "baseline": baseline,
               "runtime": runtime, "host": host, "source_contract": contract}
    # Make embedded evidence portable without exposing installed-tool paths.
    for item in compact['build']['outputs']:
        item['path'] = str(Path(item['path']).relative_to(ROOT))
    compact['build']['project'] = str(Path(compact['build']['project']).relative_to(ROOT))
    evidence_json = json.dumps(compact, ensure_ascii=False).replace('</', '<\\/')
    count = len(runtime['results'])
    free = runtime['heap_accounting']['free_heap_bytes_after_modeled_allocations']
    content = f'''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Ring ArmCC 5 software qualification · 10 September 2026</title>
<style>
:root{{color-scheme:light;--ink:#17252e;--muted:#536572;--line:#d8e1e5;--green:#166b4d;--soft:#edf7f2;--accent:#a85611}}*{{box-sizing:border-box}}body{{margin:0;background:#f4f6f7;color:var(--ink);font:16px/1.65 system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}}main{{max-width:1240px;margin:40px auto;padding:40px 46px;background:white;border:1px solid var(--line);border-radius:12px}}h1{{font-size:clamp(30px,4vw,46px);line-height:1.15;letter-spacing:-.035em;max-width:940px;margin:12px 0 20px}}h2{{font-size:24px;line-height:1.3;margin:38px 0 14px}}p{{max-width:1050px;margin:14px 0}}a{{color:#14659a;text-underline-offset:3px}}code{{font:12px/1.6 ui-monospace,SFMono-Regular,Consolas,monospace;overflow-wrap:anywhere}}.eyebrow{{font-size:12px;letter-spacing:.1em;text-transform:uppercase;color:var(--muted)}}.lead{{font-size:19px}}.callout{{border-left:4px solid var(--green);background:var(--soft);padding:15px 20px;margin:22px 0}}.boundary{{border-color:var(--accent);background:#fff7ed}}.cards{{display:grid;grid-template-columns:repeat(4,1fr);gap:14px;margin:26px 0}}.card{{border:1px solid var(--line);border-radius:8px;padding:18px}}.card strong{{display:block;font-size:28px;line-height:1.25}}.card span{{font-size:13px;color:var(--muted)}}.scroll{{overflow:auto;border:1px solid var(--line);border-radius:8px;margin:18px 0}}table{{border-collapse:collapse;width:100%;font-size:13px;line-height:1.55}}th{{text-align:left;background:#edf2f4;color:#334956;font-weight:650}}th,td{{padding:12px 14px;border-bottom:1px solid var(--line);vertical-align:top}}tr:last-child td{{border-bottom:0}}td:first-child{{min-width:175px}}.status{{display:block;font-size:12px;color:var(--green);margin-top:6px}}.muted{{color:var(--muted);font-size:13px}}.hash{{font-size:11px}}details{{margin:20px 0}}summary{{cursor:pointer;font-weight:650;padding:10px 0}}footer{{border-top:1px solid var(--line);padding-top:20px;margin-top:35px;font-size:13px;color:var(--muted)}}@media(max-width:720px){{main{{margin:0;padding:24px 18px;border:0;border-radius:0}}.cards{{grid-template-columns:repeat(2,1fr)}}th,td{{padding:10px}}}}@media print{{body{{background:white}}main{{border:0;margin:0;padding:10px}}details{{display:block}}.scroll{{overflow:visible}}}}
</style></head><body><main>
<div class="eyebrow">Production Ring · 603V1.23.2 · MBP-M5 · 10 September 2026</div>
<h1>Factory baseline reproduced.<br>Restored candidate passes software checks.</h1>
<p class="lead">The original Arm Compiler 5.06 update 7 build 960 runtime is restored with recording and Opus intact. The supplier baseline is byte-for-byte identical to the preserved factory application. The repaired candidate compiles, links and passes the host and linked ARM regressions.</p>
<div class="cards"><div class="card"><strong>184,132 B</strong><span>Factory baseline · exact byte match</span></div><div class="card"><strong>0 errors</strong><span>Both baseline and candidate builds</span></div><div class="card"><strong>{count} scenarios</strong><span>Linked ARM regression checks</span></div><div class="card"><strong>13 tasks</strong><span>Recorded kernel allocations and budgets</span></div></div>
<div class="callout boundary"><strong>Software qualification only.</strong> No image was signed, flashed or published. The failed Ring has not been recovered, and no physical boot, version readback, BLE, audio or stack/heap high-water result is claimed. The <a href="ring-s05-failure-audit.html">original S05 failure audit</a> remains incident evidence.</div>
<h2>Build identity</h2>
{table(['Build','Compiler / runtime','Result','Application'],[
['Supplier baseline','ArmCC 5.06u7 for Certification, build 960 · original startup, Microlib and ARM/CMSIS FreeRTOS port','339 objects · 0 errors · 543 warnings','184,132 B · <strong>factory-identical</strong>'],
['Repaired Sudo candidate','Same compiler and runtime · recording plus 123 Opus 1.6.1 units','351 objects · 0 errors · 588 warnings · no linker warnings','330,648 B · unsigned software candidate']])}
<p>The working build uses µVision in the installed <code>Bravechip-ArmCC5-Trial</code> CrossOver bottle. MDK Professional evaluation licensing was active with 30 days remaining on September 10. CMSIS 5.7.0 and Nordic DeviceFamilyPack 8.35.0 are installed. The Linux Docker host executes x86 tools, but its unconfigured compiler license prevents that separate route from building firmware.</p>
<p class="muted">The final build records 759 actual compiler/header inputs, all 351 object hashes, the compiler installation hashes and unchanged before/after source manifests. Six original startup/system/allocator/port source files and the supplier algorithm archive match the supplier baseline. One prepared baseline RTE header was regenerated with comment/formatting-only changes; all other 7,055 original files remained identical.</p>
<p><strong>Factory BIN SHA-256:</strong> <code class="hash">{baseline['applicationSha256']}</code><br><strong>Candidate BIN SHA-256:</strong> <code class="hash">{runtime['bin_sha256']}</code></p>
<h2>Disposition of every S05 audit area</h2>
{table(['Finding / area','Change or disposition','Evidence and remaining boundary'],disposition_rows)}
<h2>Task and interrupt stack accounting</h2>
<p>The budgets below come from actual <code>xTaskCreate</code> calls in the CPU model, including the real scheduler creation of IDLE and Tmr Svc. The last column subtracts the larger listed known path and a 256-byte Cortex-M4F context reserve. For Tmr Svc, it also includes the largest linked timer callback: 1,240 B. Positive space in this table is a software screen, not a measured worst-case margin.</p>
{table(['Task entry','Budget B','Direct linker B','Listed CPU probe B','After screen + reserve B'],stack_rows)}
<p class="muted">The 256 B reserve covers the extended 104 B hardware exception frame, the ARM port’s 100 B register save and alignment. Nested exceptions also use the separate 8 KiB MSP stack. The largest tested callback uses 616 B. Linker analysis reports unknown function pointers and cycles; driver/error paths, sustained activity and interrupt nesting still need physical high-water measurements. Motor/hardware probes stop after the first log; the motion helper measurement excludes parent frames.</p>
<h2>Memory and retained features</h2>
<p>Load bytes span <code>0x27000–0x77b98</code>, below the reserved <code>0xe0000</code> boundary. RAM spans <code>0x20004758–0x20039048</code>: 215,280 B including the 140 KiB RTOS heap and 8 KiB MSP stack, with 28,600 B left before physical SRAM ends. The unused C-heap reservation is discarded by the linker.</p>
<p>Actual allocation of thirteen tasks, BLE queues, voice command/touch queues, seven linked timers and the Opus state leaves <strong>{free:,} B</strong> in the RTOS heap. The Opus silent-frame self-test passes in this allocation state. Driver/event-group/storage allocations, sustained fragmentation and scheduling are excluded; this is not a device heap low-water measurement.</p>
<p>The complete host suite passed with its existing single Python skip. It includes 84,292 Opus checks, 54 committed codec fixtures, storage fault matrices, BLE routing and driver coverage, plus new logging/RTC/motion tests. Inherited source diagnostics remain visible: 399 double-promotion warnings, 103 unused-variable/function warnings and 41 missing-return warnings, among others. These are retained for follow-up where reachable; zero build errors is not a claim that every warning is harmless.</p>
<details><summary>Inspect all {count} linked ARM scenarios</summary>{table(['Scenario','IPSR','Stack B','Observed result'],probe_rows)}<p class="muted">The expected reset occurs only in deliberate kernel stack-guard fault injection. Other probes return or reach their declared stop boundary. C startup executes the real Arm compressed-data loader. No target instruction is patched, bypassed or replaced with a host function.</p></details>
<details><summary>Exact output hashes and local evidence</summary>{table(['Output','Bytes','SHA-256'],output_rows)}<p><a href="../../{esc(base.relative_to(ROOT))}/runtime-regression.json">Structured runtime evidence</a> · <a href="../../{esc(base.relative_to(ROOT))}/actual-compiler-inputs.json">Actual compiler inputs</a> · <a href="../../{esc(base.relative_to(ROOT))}/source-contract.json">Source/runtime contract</a> · <a href="../../{esc(base.relative_to(ROOT))}/host-tests.log">Host test log</a> · <a href="../../{esc(base.parent.relative_to(ROOT))}/vendor-baseline-verification.json">Factory baseline evidence</a></p></details>
<h2>Reproduce and continue</h2>
<p>The <a href="../how-to/qualify-firmware.md#repeat-the-recorded-software-build">build and qualification guide</a> gives the commands for a fresh unsigned rebuild. Use <a href="../../tools/firmware/qualify_armcc5_runtime.py">the new runtime regression</a> for repaired artifacts. Keep the original hash-pinned failure reproducer unchanged.</p>
<p>Hardware work remains in the <a href="../backlog.md#armcc-5-software-qualification--2026-09-10">backlog</a>: establish a verified recovery route and identified recoverable board, then check boot/version, bonding/reconnect, recording and transfer, physical stack/heap margins, timing and power before any signed OTA release.</p>
<footer>Evidence applies to the exact hashes shown above. The local build/evidence files are intentionally ignored by Git; this standalone report embeds its core measurements and dispositions for review.</footer>
<script id="qualification-evidence" type="application/json">{evidence_json}</script>
</main></body></html>'''
    args.output.write_text(content)
    print(f"Wrote {args.output} ({len(content.encode()):,} bytes)")


if __name__ == "__main__":
    main()
