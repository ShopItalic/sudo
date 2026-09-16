/* Appended to the existing controls fixture by test_factory_disable_swipes.py. */
int main(void) {
    unsigned before;
    const uint32_t magics[] = {LEGACY_MAGIC, LIGHT_MAGIC, HOLD_MAGIC, SWIPE_MAGIC, MAGIC};
    for (unsigned m = 0; m < 5; ++m) {
        for (unsigned mask = 0; mask < 16; ++mask) {
            uint32_t values = 0x00020201U; /* PTT, both tap mappings, haptics off */
            if (m >= 1) values |= LIGHT_MUTED;
            if (m >= 2) values |= 6U << 26;
            if (m >= 3) values |= (~mask << 4) & SWIPE_MUTED_MASK;
            before = writes;
            seed(magics[m], values);
            CHECK(ready && !storage_fault && writes == before);
            CHECK(disk[0] == magics[m] && disk[2] == values); /* no boot writes */
            CHECK(current[1] == 17 && current[3] == crc(current));
            CHECK((current[2] & ~SWIPE_MUTED_MASK) == (values & ~SWIPE_MUTED_MASK));
            CHECK(app_factory_controls_action(0) == 1 && app_factory_controls_action(1) == 2 && app_factory_controls_action(2) == 2);
            CHECK(!app_factory_controls_haptics());
            CHECK(app_factory_controls_recording_light() == (m == 0));
            CHECK(app_factory_controls_hold_delay_ms() == (m >= 2 ? 3500 : 500));
            for (unsigned d = 0; d <= 4; ++d) CHECK(!app_factory_controls_swipe_enabled(d));
            for (unsigned schema = 1; schema <= 5; ++schema) {
                get(schema);
                CHECK(answer[9] == OK && answer_length == (schema == 1 ? 18 : schema == 2 ? 19 : 20));
                if (schema >= 4) CHECK((answer[19] & 0xf0U) == 0);
            }
            for (unsigned schema = 4; schema <= 5; ++schema) {
                for (unsigned requested = 1; requested < 16; ++requested) {
                    set(schema, requested, 17);
                    CHECK(answer[9] == INVALID && !saving && writes == before);
                    CHECK((answer[19] & 0xf0U) == 0);
                }
            }
            /* Saving an unrelated preference persists Off with the same ABI. */
            uint8_t p[19] = {0,8,0x84,0x11,5,6,7,8,9};
            put32(p + 9, 17);
            put32(p + 13, (current[2] & LEGACY_VALUES_MASK & ~SWIPE_MUTED_MASK) | 0x01000000U);
            p[17] = m == 0; p[18] = m >= 2 ? 7 : 1;
            CHECK(app_factory_controls_command(p, sizeof(p)) && saving);
            CHECK(!app_factory_controls_haptics()); /* still awaiting FDS */
            complete(0);
            CHECK(answer[9] == OK && current[1] == 18 && app_factory_controls_haptics());
            CHECK((disk[2] & SWIPE_MUTED_MASK) == SWIPE_MUTED_MASK);
            boot(); CHECK(ready && current[1] == 18);
            for (unsigned d = 0; d < 4; ++d) CHECK(!app_factory_controls_swipe_enabled(d));
        }
    }
    seed(MAGIC, DEFAULTS & ~SWIPE_MUTED_MASK);
    disk[3] ^= 1; before = writes; boot(); CHECK(!ready && storage_fault && writes == before);
    /* Even invalid in-memory mask bits cannot reopen the HID input path. */
    ready = true; active_values = 0;
    for (unsigned d = 0; d < 4; ++d) CHECK(!app_factory_controls_swipe_enabled(d));
    printf("PASS swipe retirement: %u checks\n", checks);
    return 0;
}
