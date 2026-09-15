"""Remove live microphone transport from the P08 factory-derived application.

Patch the fresh, verified supplier source, leaving historical releases intact.
The local recorder, completion events and saved-file transfer are preserved.
"""

def function_span(source, signature):
    start = source.index(signature + '\n{')
    return start, source.index('\n}', start) + 2


def replace_function(source, signature, body):
    start, end = function_span(source, signature)
    return source[:start] + signature + '\n{\n' + body + '\n}' + source[end:]


def patch_commands(source):
    # Keep the factory failure response shape, including the request ID and
    # original opcode. No rejected command may acquire/cancel the recorder.
    start, end = function_span(source, 'static uint8_t app_cmd_pdm(struct app_cmd_package * cmd_package)')
    body = source[start:end]
    first = body.index('\t\tcase 0:')
    local = body.index('    case 5:')
    body = body[:first] + '''        case 0: /* PCM stream */
        case 1: /* ADPCM stream */
        case 2: /* select pushed-audio encoding */
        case 3: /* query pushed-audio encoding */
        case 0xFD: /* switch a live stream to flash */
            cmd_package->data[0] = 0;
            app_package_send_enqueue(cmd_package, 5);
            break;
''' + body[local:]
    first = body.index('    case 0xFD:', body.index('    case 5:'))
    last = body.index('    case 0xFC:', first)
    body = body[:first] + body[last:]
    source = source[:start] + body + source[end:]
    start, end = function_span(source, 'static uint8_t app_cmd_ipc_event(struct app_cmd_package * cmd_package)')
    body = source[start:end]
    first = body.index('    case 0x01:')
    last = body.index('    case 0x03:', first)
    body = body[:first] + '''    case 0x01: /* retired touch-audio start */
    case 0x02: /* retired touch-audio stop */
      cmd_package->data[0] = 0;
      app_package_send_enqueue(cmd_package, 5);
      break;
''' + body[last:]
    return source[:start] + body + source[end:]


def patch_pdm(source):
    # Remove the sender itself as well as all routes that enter online mode.
    # Compatibility symbols are inert: an old stop cannot stop local capture.
    start, end = function_span(source, 'static void app_pdm_handler_thread(void *thread_handler)')
    body = source[start:end]
    first = body.index('        case PDM_MODE_ONLINE:')
    last = body.index('        default:', first)
    body = body[:first] + body[last:]
    source = source[:start] + body + source[end:]
    for signature, statement in (
        ('void app_pdm_start(struct app_cmd_package * pack)', '    (void)pack;'),
        ('void app_pdm_stop(struct app_cmd_package * pack)', '    (void)pack;'),
        ('void app_pdm_touch_start(void)', ''),
        ('void app_pdm_touch_stop(void)', ''),
        ('bool app_pdm_switch_online_to_offline(void)', '    return false;'),
    ):
        source = replace_function(source, signature,
            '    /* Live microphone transport was removed. Local recording is unchanged. */\n' + statement)
    return source
