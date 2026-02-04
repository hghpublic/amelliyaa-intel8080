#include <fstream>
#include <iostream>
#include <format>
#include <string>
#include <string_view>
#include <chrono>
#include "Intel8080.h"

using Memory = std::array<std::uint8_t, 0x10000>;

static constexpr std::string testDirectory {"tests/binaries/"};
static constexpr std::string disassambleTable[256] = {
        "nop", "lxi b,#", "stax b", "inx b",
        "inr_ b", "dcr_ b", "mvi b,#", "rlc", "ill", "dad b", "ldax b", "dcx b",
        "inr_ c", "dcr_ c", "mvi c,#", "rrc", "ill", "lxi d,#", "stax d", "inx d",
        "inr_ d", "dcr_ d", "mvi d,#", "ral", "ill", "dad d", "ldax d", "dcx d",
        "inr_ e", "dcr_ e", "mvi e,#", "rar", "ill", "lxi h,#", "shld", "inx h",
        "inr_ h", "dcr_ h", "mvi h,#", "daa", "ill", "dad h", "lhld", "dcx h",
        "inr_ l", "dcr_ l", "mvi l,#", "cma", "ill", "lxi sp,#", "sta $", "inx sp",
        "inr_ M", "dcr_ M", "mvi M,#", "stc", "ill", "dad sp", "lda $", "dcx sp",
        "inr_ a", "dcr_ a", "mvi a,#", "cmc", "mov b,b", "mov b,c", "mov b,d",
        "mov b,e", "mov b,h", "mov b,l", "mov b,M", "mov b,a", "mov c,b", "mov c,c",
        "mov c,d", "mov c,e", "mov c,h", "mov c,l", "mov c,M", "mov c,a", "mov d,b",
        "mov d,c", "mov d,d", "mov d,e", "mov d,h", "mov d,l", "mov d,M", "mov d,a",
        "mov e,b", "mov e,c", "mov e,d", "mov e,e", "mov e,h", "mov e,l", "mov e,M",
        "mov e,a", "mov h,b", "mov h,c", "mov h,d", "mov h,e", "mov h,h", "mov h,l",
        "mov h,M", "mov h,a", "mov l,b", "mov l,c", "mov l,d", "mov l,e", "mov l,h",
        "mov l,l", "mov l,M", "mov l,a", "mov M,b", "mov M,c", "mov M,d", "mov M,e",
        "mov M,h", "mov M,l", "hlt", "mov M,a", "mov a,b", "mov a,c", "mov a,d",
        "mov a,e", "mov a,h", "mov a,l", "mov a,M", "mov a,a", "add_ b", "add_ c",
        "add_ d", "add_ e", "add_ h", "add_ l", "add_ M", "add_ a", "adc_ b", "adc_ c",
        "adc_ d", "adc_ e", "adc_ h", "adc_ l", "adc_ M", "adc_ a", "sub_ b", "sub_ c",
        "sub_ d", "sub_ e", "sub_ h", "sub_ l", "sub_ M", "sub_ a", "sbb_ b", "sbb_ c",
        "sbb_ d", "sbb_ e", "sbb_ h", "sbb_ l", "sbb_ M", "sbb_ a", "ana_ b", "ana_ c",
        "ana_ d", "ana_ e", "ana_ h", "ana_ l", "ana_ M", "ana_ a", "xra_ b", "xra_ c",
        "xra_ d", "xra_ e", "xra_ h", "xra_ l", "xra_ M", "xra_ a", "ora_ b", "ora_ c",
        "ora_ d", "ora_ e", "ora_ h", "ora_ l", "ora_ M", "ora_ a", "cmp_ b", "cmp_ c",
        "cmp_ d", "cmp_ e", "cmp_ h", "cmp_ l", "cmp_ M", "cmp_ a", "rnz", "pop b",
        "jnz $", "jmp $", "cnz $", "push b", "adi #", "rst 0", "rz", "ret", "jz $",
        "ill", "cz $", "call $", "aci #", "rst 1", "rnc", "pop d", "jnc $", "out p",
        "cnc $", "push d", "sui #", "rst 2", "rc", "ill", "jc $", "in p", "cc $",
        "ill", "sbi #", "rst 3", "rpo", "pop h", "jpo $", "xthl", "cpo $", "push h",
        "ani_ #", "rst 4", "rpe", "pchl", "jpe $", "xchg", "cpe $", "ill", "xri #",
        "rst 5", "rp", "pop psw", "jp $", "di", "cp $", "push psw", "ori #",
        "rst 6", "rm", "sphl", "jm $", "ei", "cm $", "ill", "cpi #", "rst 7"};

bool testRunning {true}; // flag needed by two functions could pass by reference but im lazy

void log (Intel8080& intel8080, const Memory& memory, unsigned long long currentCycle)
{
    std::cout << std::format(
            "PC: {:0>4X}, AF: {:0>4X}, BC: {:0>4X}, DE: {:0>4X}, HL: {:0>4X}, SP: {:0>4X}, CYC: {:d}\t({:0>2X} {:0>2X} {:0>2X} {:0>2X}) - {:s}\n",
            intel8080.pc,
            intel8080.getReg(Intel8080::A) << 8U | intel8080.getReg(Intel8080::F),
            intel8080.getPair(Intel8080::BC),
            intel8080.getPair(Intel8080::DE),
            intel8080.getPair(Intel8080::HL),
            intel8080.getPair(Intel8080::SP),
            currentCycle - 1,
            memory[intel8080.pc],
            memory[intel8080.pc + 1U],
            memory[intel8080.pc + 2U],
            memory[intel8080.pc + 3U],
            disassambleTable[memory[intel8080.pc]]);
}

int loadFile(Memory& memory, const std::string& path, int addr)
{
    std::ifstream file {path, std::ios::binary};

    if (file.fail()) {
        std::cerr << std::format("error: failure when opening file '{:s}'.\n", path);
        return -1;
    }

    if (!file.is_open()) {
        std::cerr << std::format("error: can't open file '{:s}'. Ensure you're in the correct directory: 'Intel8080/'.\n", path);
        return -2;
    }
    file.read(reinterpret_cast<char*>(&memory[addr]), memory.size());

    return 0;
}

std::string t(std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end)
{
    auto ms {std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)};

    if (ms.count() >= 60 * 1e3L) {
        std::string min {std::to_string(std::chrono::duration_cast<std::chrono::minutes>(ms).count())};
        std::string sec {std::to_string(std::chrono::duration_cast<std::chrono::seconds>(ms).count())};
        return  min + '.' + sec.erase(0, 1) + " min";
    } else if (ms.count() >= 1e3L) {
        std::string sec {std::to_string(std::chrono::duration_cast<std::chrono::seconds>(ms).count())};
        return  sec + '.' + std::to_string(ms.count()).erase(0, 1) + " sec";
    } else {
        return std::to_string(ms.count()) + " ms";
    }
}

void onDataInput(Intel8080& intel8080, Memory& memory, bool debug, bool verbose)
{
    if (intel8080.status == Intel8080::instructionFetch) {
        intel8080.setDBus(memory[intel8080.getABus()]);
        if (debug and verbose)
            std::cout << std::format(
                    "\tFETCH CYCLE\t[{:s}]: abus={:0>4X}, dbus={:0>2X}\n",
                    disassambleTable[memory[intel8080.getABus()]],
                    intel8080.getABus(),
                    intel8080.getDBus());
    } else if (intel8080.status == Intel8080::memoryRead or intel8080.status == Intel8080::stackRead) {
        intel8080.setDBus(memory[intel8080.getABus()]);
        if (debug and verbose)
            std::cout << std::format(
                    "\tREAD CYCLE\t[{:s}]: abus={:0>4X}, dbus={:0>2X}\n",
                    disassambleTable[intel8080.ir],
                    intel8080.getABus(),
                    intel8080.getDBus());
    } else if (intel8080.status == Intel8080::inputRead) {
        intel8080.setDBus(uint_fast64_t(0ULL));
    } else {
        std::cout << std::format("ERROR: unrecognized status word with DBIN pin high '{:b}' - {:s}\n",
                                 intel8080.status, disassambleTable[intel8080.ir]);
    }
}

void onDataOutput(Intel8080& intel8080, Memory& memory, bool debug, bool verbose)
{
    if (intel8080.status == Intel8080::memoryWrite or intel8080.status == Intel8080::stackWrite) {
        memory[intel8080.getABus()] = intel8080.getDBus();
        if (debug and verbose)
            std::cout << std::format(
                    "\tWRITE CYCLE\t[{:s}]: abus={:0>4X}, dbus={:0>2X}, mem={:0>2X}\n",
                    disassambleTable[intel8080.ir],
                    intel8080.getABus(),
                    intel8080.getDBus(),
                    memory[intel8080.getABus()]);
    } else if (intel8080.status == Intel8080::outputWrite) {
        std::uint16_t port{intel8080.getABus()};
        if (port == 0) {
            testRunning = false;
        } else if (port == 1) {
            const std::uint8_t operation{intel8080.getReg(Intel8080::C)};
            if (operation == 9) {
                // print from memory at (DE) until '$' char
                std::uint16_t addr{intel8080.getPair(Intel8080::DE)};
                do {
                    std::cout << (char) memory[addr++];
                } while (memory[addr] != '$');
            } else if (operation == 2 or operation == 5) {
                // print a character stored in E
                std::cout << (char) intel8080.getReg(Intel8080::E);
            }
        }
    } else {
        std::cout << std::format("ERROR: unrecognized status word with WR pin high '{:b}' - {:s}\n",
                                 intel8080.status, disassambleTable[intel8080.ir]);
    }
}

void test(Intel8080& intel8080, const std::string& testName, unsigned long long expectedCycles, bool debug, bool verbose)
{
    Memory memory {};
    if (loadFile(memory, testDirectory + testName, 0x100) < 0)
        return;
    std::cout << std::format("*** TEST: {:s}\n", testName);

    // inject "out 0,a" at 0x0000 (signal to stop the test)
    memory[0x0000] = 0xD3;
    memory[0x0001] = 0x00;

    // inject "out 1,a" at 0x0005 (signal to output some characters)
    memory[0x0005] = 0xD3;
    memory[0x0006] = 0x01;
    memory[0x0007] = 0xC9;

    // reset the cpu for the next test
    intel8080.reset();
    intel8080.pc = 0x100U;
    testRunning = true;

    unsigned long long executedCycles {0};
    unsigned long instructions {0};
    std::chrono::steady_clock::time_point begin {std::chrono::steady_clock::now()};

    while (testRunning) {
        intel8080.tick();
        ++executedCycles;

        if (intel8080.pins & Intel8080::SYNC and intel8080.status == Intel8080::instructionFetch) {
            ++instructions;
            if (debug)
                log(intel8080, memory, executedCycles);
        } else if (intel8080.pins & Intel8080::DBIN) {
            onDataInput(intel8080, memory, debug, verbose);
        } else if (intel8080.pins & Intel8080::WR) {
            onDataOutput(intel8080, memory, debug, verbose);
        }
    }
    // need to tick() one more time because the test ended before the cpu could finish its last cycle
    intel8080.tick();
    ++executedCycles;

    unsigned long long diff {expectedCycles > executedCycles ? expectedCycles - executedCycles : executedCycles - expectedCycles};
    std::cout << std::format("\n*** {:d} instructions executed on {:d} cycles (expected={:d}, diff={:d}) in {:s}\n\n",
           instructions, executedCycles, expectedCycles, diff,
                             t(begin, std::chrono::steady_clock::now()));
}

int main(int argc, char** argv)
{
    bool debug {false};
    bool verbose {false};

    // simple command line parsing
    using namespace std::string_view_literals;
    for (int i {1}; i < argc; ++i) {
        if (argv[i] == "-debug"sv) {
            debug = true;
        } else if (argv[i] == "-v"sv) {
            verbose = true;
        } else {
            std::cout << std::format("Unrecognized command line argument '{:s}'.\nAvailable arguments are:\n\tenable logging: -debug\n\tenable verbose logging: -v\n\n", argv[i]);
        }
    }
    verbose = (verbose and debug); // verbose only makes sense if debug is also enabled

    Intel8080 intel8080 {};
    std::chrono::steady_clock::time_point begin {std::chrono::steady_clock::now()};

    test(intel8080, "TST8080.COM", 4924ULL, debug, verbose);
    test(intel8080, "8080PRE.COM", 7817ULL, debug, verbose);
    test(intel8080, "CPUTEST.COM", 255653383ULL, debug, verbose);
    test(intel8080, "8080EXM.COM", 23803381171ULL, debug, verbose);

    std::cout << "Done. Total time elapsed " << t(begin, std::chrono::steady_clock::now()) << std::endl;

    return 0;
}