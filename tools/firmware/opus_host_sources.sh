# Sourced by test.sh: builds the pinned libopus 1.6.1 fixed-point subset once
# for host suites. The same defines select the same code paths as the target
# profile (portable C, no float API, non-thread-safe pseudostack, Sudo
# allocation hook); sanitizer flags match the other host suites.
opus_root=firmware/bc_ros/bc_module/opus/opus-1.6.1
opus_support=firmware/bc_ros/bc_module/recording/opus_support
# GLOBAL_STACK_SIZE must reach arch.h before its 120000-byte default; the
# profile header is the single source of truth and custom_support.h asserts it.
opus_scratch_bytes=$(sed -n 's/^#define BC_OPUS_SCRATCH_BYTES \([0-9]*\)U.*/\1/p' firmware/bc_ros/bc_module/recording/bc_opus_profile.h)
# Host suites run every supported rate/bitrate, including 48 kHz fixtures that
# need more temporary memory than the 16 kHz target profile; the codec suite
# asserts the profile's own high-water mark against $opus_scratch_bytes.
opus_host_scratch_bytes=65536
opus_defines="-DOPUS_BUILD -DFIXED_POINT -DDISABLE_FLOAT_API -DNONTHREADSAFE_PSEUDOSTACK -DCUSTOM_SUPPORT"
opus_includes="-I$opus_root/include -I$opus_root/celt -I$opus_root/silk -I$opus_root/silk/fixed -I$opus_root/src -I$opus_support -Ifirmware/bc_ros/bc_module/recording"
opus_sources() {
  # Exactly the upstream OPUS/CELT/SILK/SILK_FIXED lists minus multistream and
  # projection wrappers, which the Sudo profile never references.
  for list in "$opus_root/opus_sources.mk:OPUS_SOURCES" \
              "$opus_root/celt_sources.mk:CELT_SOURCES" \
              "$opus_root/silk_sources.mk:SILK_SOURCES" \
              "$opus_root/silk_sources.mk:SILK_SOURCES_FIXED"; do
    file=${list%%:*}; name=${list##*:}
    sed -n "/^$name = /,/^\$/p" "$file" | grep '/' | tr -d '\\' | tr -d ' ' |
      grep -v 'multistream\|projection\|mapping_matrix' |
      while read -r source; do printf '%s/%s\n' "$opus_root" "$source"; done
  done
}
# Two archives: libopus_host.a uses the enlarged host scratch for every
# supported rate; libopus_profile.a is built with exactly the target scratch
# bound so the capture adapter suite exercises the real pseudostack size.
build_one_opus_library() {
  name=$1; scratch=$2
  lib=build/firmware/tests/opus/$name.a
  stamp=build/firmware/tests/opus/$name.stamp
  key="$firmware_host_cc|$opus_defines|$scratch|$(cat "$opus_root/sudo-import-manifest.json" | shasum -a 256 | cut -c1-16)|$(cat tools/firmware/opus_host_sources.sh | shasum -a 256 | cut -c1-16)"
  if [ -f "$lib" ] && [ -f "$stamp" ] && [ "$(cat "$stamp")" = "$key" ]; then return 0; fi
  rm -rf "build/firmware/tests/opus/$name.obj" "$lib" "$stamp"
  mkdir -p "build/firmware/tests/opus/$name.obj"
  for source in $(opus_sources); do
    object="build/firmware/tests/opus/$name.obj/$(basename "${source%.c}").o"
    "$firmware_host_cc" -std=gnu99 -O2 -g -fsanitize=address,undefined \
      -fno-sanitize=shift-base,shift-exponent,signed-integer-overflow \
      $opus_defines -DGLOBAL_STACK_SIZE=$scratch $opus_includes -c "$source" -o "$object" || return 1
  done
  ar rcs "$lib" "build/firmware/tests/opus/$name.obj"/*.o || return 1
  printf '%s' "$key" > "$stamp"
}
build_opus_host_library() {
  mkdir -p build/firmware/tests/opus
  build_one_opus_library libopus_host "$opus_host_scratch_bytes" || return 1
  build_one_opus_library libopus_profile "$opus_scratch_bytes" || return 1
}
