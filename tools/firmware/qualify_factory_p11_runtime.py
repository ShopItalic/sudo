#!/usr/bin/env python3
"""Execute the exact ArmCC P11 image on a CPU model; never device I/O.

Actual scatter loader, RTOS allocators, codec and setjmp overflow recovery.
No simulated timing result may be reported as real MCU time or BLE capacity.
"""
import argparse
import json
from pathlib import Path
import struct
from qualify_armcc5_runtime import Artifact, Probe, SCRATCH, require
from verify_factory_ptt_v11 import task_descriptors


def qualify(source):
    receipt = json.loads((source / "p11-preparation.json").read_text())
    require(receipt.get("packageRevision") == 2 and receipt.get("mode") in
            ("opus", "adpcm-control-not-for-release"),
            "This codec probe requires P11-r2; P11-r1 keeps the factory ADPCM path")
    stem = source / "firmware/BCL603S2X/app/project/mdk5/Objects/app"
    artifact = Artifact(stem.with_suffix(".axf"), stem.with_suffix(".bin"))
    p = Probe(artifact)
    results = [p.call("bc_queue_init")]
    # Allocate every linked vendor descriptor, including optional/unused ones:
    # this is a conservative allocation scenario, not a claim about boot order.
    descriptors = task_descriptors(artifact)
    for descriptor in descriptors:
        p.create_task(descriptor)
    free_before = p.call("xPortGetFreeHeapSize")["return_r0"]
    # Reserve another 8 KiB for scheduler/system timers and driver allocations.
    reserve = p.call("pvPortMalloc", args=(8192,))["return_r0"]
    require(reserve != 0, "No 8 KiB system-allocation reserve")
    result = p.call("prepare_codec", limit=6000000)
    results.append(result)
    require(result["return_r0"] == 1, "Actual ArmCC codec self-test failed")
    names = ("workspace_bytes", "pcm_bytes", "queue_bytes", "scratch_high_water",
             "heap_before", "heap_after", "max_encode_cycles", "max_queue_records",
             "faults", "allocations", "completed_blocks")
    diagnostic = dict(zip(names, struct.unpack("<11I", p.cpu.mem_read(artifact.sym("p11_audio_diagnostics"), 44))))
    control = receipt["mode"] != "opus"
    require(diagnostic["allocations"] == (0 if control else 1), "Wrong allocation count")
    audio = artifact.sym("audio")
    if not control:
        scratch = p.read_u32("scratch")
        state = p.read_u32("workspace")
        size = p.call("bc_opus_encoder_state_size", args=(1,))["return_r0"]
        require(0 < size <= 16384 and diagnostic["workspace_bytes"] == ((size + 7) & ~7) + 20480 + 64,
                "Invalid fixed-profile workspace: " + str((size, diagnostic)))
        require(p.call("p11_audio_init", args=(audio, state, size, artifact.sym("discard_test"), 0))["return_r0"] == 1,
                "Standalone exact-binary codec init failed")
    else:
        # Control uses the real supplier IMA library; override only its sink.
        # discard_test may be linker-removed, so retain production sink and
        # initialize the factory capture ledger and task handles instead.
        require(p.call("factory_capture_begin", args=(10,))["return_r0"] == 1, "Control ledger start failed")
        require(p.call("factory_capture_p11_hold")["return_r0"] == 1, "Control producer hold failed")
        p.call("factory_capture_enable")
        writer = next(d for d in descriptors if d["function"] == "app_pdm_handler_thread")
        handle = struct.unpack("<I", p.cpu.mem_read(int(writer["address"], 16) + 44, 4))[0]
        p.write_u32("writer_task", handle)
    for scenario in range(3):
        require(p.call("p11_audio_begin", args=(audio,))["return_r0"] == 1, "Codec begin failed")
        # Silence, alternating full scale, deterministic high-entropy input.
        values = [0 if scenario == 0 else (32767 if i % 2 else -32768) if scenario == 1
                  else ((i * 7919 + i*i * 31) & 65535) - 32768 for i in range(880)]
        p.cpu.mem_write(SCRATCH, struct.pack("<880h", *values))
        for _ in range(2):
            result = p.call("p11_audio_feed", args=(audio, SCRATCH, 880), limit=6000000)
            require(result["return_r0"] == 1, "Exact-binary feed failed")
            results.append(result)
        require(p.call("p11_audio_finish", args=(audio,), limit=6000000)["return_r0"] == 1, "Codec finish failed")
        if not control:
            require(bytes(p.cpu.mem_read(scratch + 20480, 64)) == bytes([0xc5])*64, "Scratch canary changed")
    if not control:
        require(p.call("p11_audio_begin", args=(audio,))["return_r0"] == 1, "Guard-test begin failed")
        p.write_u32("global_stack", scratch + 20476)
        p.write_u32("scratch_ptr", scratch)
        result = p.call("p11_audio_feed", args=(audio, SCRATCH, 880), limit=6000000)
        require(result["return_r0"] == 0, "Pre-write overflow did not unwind")
        results.append(result)
        require(bytes(p.cpu.mem_read(scratch + 20480, 64)) == bytes([0xc5])*64, "Overflow wrote canary")
        require(p.read_u32("guard_failures") == 1, "Guard count mismatch")
        p.call("p11_audio_finish", args=(audio,))
        require(p.call("p11_audio_begin", args=(audio,))["return_r0"] == 1, "Codec cannot restart after guarded failure")
        require(p.call("p11_audio_feed", args=(audio, SCRATCH, 880), limit=6000000)["return_r0"] == 1, "Restart feed failed")
        require(p.call("p11_audio_finish", args=(audio,), limit=6000000)["return_r0"] == 1, "Restart finish failed")
        p.call("measure")
        diagnostic = dict(zip(names, struct.unpack("<11I", p.cpu.mem_read(artifact.sym("p11_audio_diagnostics"), 44))))
    return {"status": "pass-exact-armcc-cpu-probes", **artifact.identity, "packageRevision": 2,
            "mode": "adpcm-control" if control else "opus",
            "startup": artifact.startup, "layout": artifact.layout, "allocated_descriptors": descriptors,
            "heap_after_all_descriptors_and_queues": free_before, "additional_system_reserve": 8192,
            "diagnostics": diagnostic, "encoder_state_bytes": 0 if control else size,
            "max_observed_call_stack": max(r["stack_bytes"] for r in results),
            "cpu_probes": results, "overflow_prewrite_unwind_tested": not control,
            "limits": ["CPU-only, no scheduler, SoftDevice, peripherals, battery or elapsed-time model.",
                       "Allocations are a conservative synthetic scenario, not measured physical heap high-water.",
                       "Hardware recording/transfer/charging/recovery qualification is still required."],
            "signed": False, "flashed": False, "physicallyQualified": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Use fresh evidence")
    result = qualify(args.source.resolve())
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
