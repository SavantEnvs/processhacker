// Fuzz the vendored Zydis x86 decoder + formatter (tools/thirdparty/zydis).
#include <stdint.h>
#include <string.h>

#include "Zydis.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2)
        return -1;

    ZydisMachineMode mode;
    ZydisStackWidth stack;
    switch (data[0] % 3)
    {
    case 0: mode = ZYDIS_MACHINE_MODE_LONG_64; stack = ZYDIS_STACK_WIDTH_64; break;
    case 1: mode = ZYDIS_MACHINE_MODE_LEGACY_32; stack = ZYDIS_STACK_WIDTH_32; break;
    default: mode = ZYDIS_MACHINE_MODE_LEGACY_16; stack = ZYDIS_STACK_WIDTH_16; break;
    }
    ZydisFormatterStyle style = (data[0] >> 4) & 1 ? ZYDIS_FORMATTER_STYLE_INTEL : ZYDIS_FORMATTER_STYLE_ATT;
    data++;
    size--;

    ZydisDecoder decoder;
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, mode, stack)))
        return -1;
    ZydisFormatter formatter;
    if (!ZYAN_SUCCESS(ZydisFormatterInit(&formatter, style)))
        return -1;

    ZydisDecodedInstruction insn;
    ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
    char text[256];
    size_t off = 0;
    ZyanU64 addr = 0x400000;
    while (off < size)
    {
        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, data + off, size - off, &insn, ops)))
            break;
        (void)ZydisFormatterFormatInstruction(&formatter, &insn, ops, insn.operand_count_visible, text,
                                              sizeof(text), addr, ZYAN_NULL);
        off += insn.length;
        addr += insn.length;
    }
    return 0;
}
