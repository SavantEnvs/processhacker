// The Windows-adapted json-c seeds its hash tables from phlib's RNG. Supply a fixed value so that
// fuzz inputs reproduce identically from run to run.
unsigned long long PhGenerateRandomNumber64(void)
{
    return 0x9e3779b97f4a7c15ull;
}
