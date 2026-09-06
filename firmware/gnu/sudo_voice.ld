/*
 * Candidate GNU layout for the Sudo Voice nRF52840 application.
 *
 * The application is deliberately bounded below the FDS/custom/bootloader
 * area recorded by the factory artifact.  This file is used only by the
 * diagnostic GNU build; it does not alter the Keil project or its scatter
 * settings.
 */
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

MEMORY
{
    /* 0x27000..0xE0000; bootloader begins at 0xF8000. */
    FLASH (rx)  : ORIGIN = 0x00027000, LENGTH = 0x000B9000
    /* 0x20004758..0x20040000; the supplier RAM claim is 0x3BE70. */
    RAM   (rwx) : ORIGIN = 0x20004758, LENGTH = 0x0003B8A8
}

/* S140's documented minimum RAM start is below the supplier-owned region. */
__softdevice_ram_start__ = 0x20002260;
__app_flash_start__ = ORIGIN(FLASH);
__app_flash_end__ = ORIGIN(FLASH) + LENGTH(FLASH);
__app_ram_start__ = ORIGIN(RAM);
__app_ram_end__ = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : ALIGN(4)
    {
        __isr_vector_start__ = .;
        KEEP(*(.isr_vector))
        KEEP(*(.vectors))
        KEEP(*(.Vectors))
        __isr_vector_end__ = .;
    } > FLASH

    .text : ALIGN(4)
    {
        __text_start__ = .;
        *(.text*)
        /* ArmCC supplier members use i.<symbol> sections. */
        *(i.*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *(.rodata*)
        *(.constdata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame*)
        . = ALIGN(4);
        __text_end__ = .;
    } > FLASH

    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    __exidx_start = .;
    .ARM.exidx : ALIGN(4)
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        . = ALIGN(4);
    } > FLASH
    __exidx_end = .;

    /* Keep every Nordic section-variable collection explicit. */
    .sdh_ant_observers : ALIGN(4)
    {
        __start_sdh_ant_observers = .;
        KEEP(*(SORT(.sdh_ant_observers*)))
        __stop_sdh_ant_observers = .;
    } > FLASH
    .sdh_ble_observers : ALIGN(4)
    {
        __start_sdh_ble_observers = .;
        KEEP(*(SORT(.sdh_ble_observers*)))
        __stop_sdh_ble_observers = .;
    } > FLASH
    .sdh_soc_observers : ALIGN(4)
    {
        __start_sdh_soc_observers = .;
        KEEP(*(SORT(.sdh_soc_observers*)))
        __stop_sdh_soc_observers = .;
    } > FLASH
    .sdh_req_observers : ALIGN(4)
    {
        __start_sdh_req_observers = .;
        KEEP(*(SORT(.sdh_req_observers*)))
        __stop_sdh_req_observers = .;
    } > FLASH
    .sdh_state_observers : ALIGN(4)
    {
        __start_sdh_state_observers = .;
        KEEP(*(SORT(.sdh_state_observers*)))
        __stop_sdh_state_observers = .;
    } > FLASH
    .sdh_stack_observers : ALIGN(4)
    {
        __start_sdh_stack_observers = .;
        KEEP(*(SORT(.sdh_stack_observers*)))
        __stop_sdh_stack_observers = .;
    } > FLASH
    .nrf_queue : ALIGN(4)
    {
        __start_nrf_queue = .;
        KEEP(*(SORT(.nrf_queue*)))
        __stop_nrf_queue = .;
    } > FLASH
    .nrf_balloc : ALIGN(4)
    {
        __start_nrf_balloc = .;
        KEEP(*(SORT(.nrf_balloc*)))
        __stop_nrf_balloc = .;
    } > FLASH
    .cli_command : ALIGN(4)
    {
        __start_cli_command = .;
        KEEP(*(SORT(.cli_command*)))
        __stop_cli_command = .;
    } > FLASH
    .crypto_data : ALIGN(4)
    {
        __start_crypto_data = .;
        KEEP(*(SORT(.crypto_data*)))
        __stop_crypto_data = .;
    } > FLASH
    .pwr_mgmt_data : ALIGN(4)
    {
        __start_pwr_mgmt_data = .;
        KEEP(*(SORT(.pwr_mgmt_data*)))
        __stop_pwr_mgmt_data = .;
    } > FLASH
    .log_const_data : ALIGN(4)
    {
        __start_log_const_data = .;
        KEEP(*(SORT(.log_const_data*)))
        __stop_log_const_data = .;
    } > FLASH
    .log_backends : ALIGN(4)
    {
        __start_log_backends = .;
        KEEP(*(SORT(.log_backends*)))
        __stop_log_backends = .;
    } > FLASH
    .svc_data : ALIGN(4)
    {
        __start_svc_data = .;
        KEEP(*(SORT(.svc_data*)))
        __stop_svc_data = .;
    } > FLASH
    .dfu_trans : ALIGN(4)
    {
        __start_dfu_trans = .;
        KEEP(*(SORT(.dfu_trans*)))
        __stop_dfu_trans = .;
    } > FLASH
    .nrf_sections : ALIGN(4)
    {
        __start_nrf_sections = .;
        KEEP(*(SORT(.nrf_sections*)))
        KEEP(*(SORT(.nrf_section*)))
        __stop_nrf_sections = .;
    } > FLASH

    /* q_initcall_t sections intentionally remain separate for stage order. */
    q_initcall0init : ALIGN(4)
    {
        __start_q_initcall0init = .;
        KEEP(*(q_initcall0init))
        __stop_q_initcall0init = .;
    } > FLASH
    q_initcall1init : ALIGN(4)
    {
        __start_q_initcall1init = .;
        KEEP(*(q_initcall1init))
        __stop_q_initcall1init = .;
    } > FLASH
    q_initcall2init : ALIGN(4)
    {
        __start_q_initcall2init = .;
        KEEP(*(q_initcall2init))
        __stop_q_initcall2init = .;
    } > FLASH
    q_initcall3init : ALIGN(4)
    {
        __start_q_initcall3init = .;
        KEEP(*(q_initcall3init))
        __stop_q_initcall3init = .;
    } > FLASH

    __etext = .;

    .data : AT (__etext)
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(vtable)
        *(.data*)
        __start_fs_data = .;
        KEEP(*(.fs_data*))
        __stop_fs_data = .;
        __start_cli_sorted_cmd_ptrs = .;
        KEEP(*(.cli_sorted_cmd_ptrs*))
        __stop_cli_sorted_cmd_ptrs = .;
        __start_log_dynamic_data = .;
        KEEP(*(SORT(.log_dynamic_data*)))
        __stop_log_dynamic_data = .;
        __start_log_filter_data = .;
        KEEP(*(SORT(.log_filter_data*)))
        __stop_log_filter_data = .;
        KEEP(*(SORT(.privileged_data*)))

        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        *(privileged_data*)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    .noinit (NOLOAD) : ALIGN(4)
    {
        __noinit_start__ = .;
        *(.noinit*)
        *(NoInit)
        . = ALIGN(4);
        __noinit_end__ = .;
    } > RAM

    .heap (COPY) : ALIGN(8)
    {
        __HeapBase = .;
        __end__ = .;
        PROVIDE(end = .);
        KEEP(*(.heap*))
        __HeapLimit = .;
    } > RAM

    .stack_dummy (COPY) : ALIGN(8)
    {
        KEEP(*(.stack*))
        KEEP(*(privileged_functions*))
    } > RAM

    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    ASSERT(ORIGIN(FLASH) == 0x00027000, "application FLASH origin changed")
    ASSERT(ORIGIN(FLASH) + LENGTH(FLASH) == 0x000E0000, "application FLASH end overlaps reserved area")
    ASSERT(ORIGIN(RAM) >= __softdevice_ram_start__, "application RAM precedes SoftDevice minimum")
    ASSERT(ORIGIN(RAM) + LENGTH(RAM) == 0x20040000, "application RAM exceeds nRF52840 physical end")
    ASSERT(ADDR(.isr_vector) == ORIGIN(FLASH), "vector table is not at application origin")
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
    ASSERT(__etext <= ORIGIN(FLASH) + LENGTH(FLASH), "region FLASH overflowed before data")
    ASSERT(LOADADDR(.data) + SIZEOF(.data) <= ORIGIN(FLASH) + LENGTH(FLASH), "region FLASH overflowed with data")
}
