// Leak detection is off by design (SPEC §6.2 item 15): ASan memory-safety checks and UBSan stay on.
extern "C" int __lsan_is_turned_off() { return 1; }
