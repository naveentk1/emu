//
// Created by Naveen T K on 29/12/25.
//

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <random>

class Chip8 {
public:
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t I;
    uint16_t pc;
    uint8_t sp;
    uint16_t stack[16];
    uint8_t delay_timer;
    uint32_t display[64 * 32];

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;

    Chip8() : randGen(std::random_device()()), randByte(0, 255) {
        pc = 0x200;

        // Load fonts (0-F) into memory
        uint8_t fonts[80] = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };
        memcpy(memory, fonts, 80);
    }

    void LoadROM(const char* filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streampos size = file.tellg();
            char* buffer = new char[size];
            file.seekg(0, std::ios::beg);
            file.read(buffer, size);
            file.close();

            for (long i = 0; i < size; ++i) {
                memory[0x200 + i] = buffer[i];
            }
            delete[] buffer;
        }
    }

    void Cycle() {
        // Fetch
        uint16_t opcode = memory[pc] << 8 | memory[pc + 1];
        pc += 2;

        // Decode and execute
        uint16_t nnn = opcode & 0x0FFF;
        uint8_t n = opcode & 0x000F;
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;
        uint8_t kk = opcode & 0x00FF;

        switch (opcode & 0xF000) {
            case 0x0000:
                if (opcode == 0x00E0) {
                    memset(display, 0, sizeof(display));
                } else if (opcode == 0x00EE) {
                    pc = stack[--sp];
                }
                break;
            case 0x1000: pc = nnn; break;
            case 0x2000: stack[sp++] = pc; pc = nnn; break;
            case 0x3000: if (V[x] == kk) pc += 2; break;
            case 0x4000: if (V[x] != kk) pc += 2; break;
            case 0x5000: if (V[x] == V[y]) pc += 2; break;
            case 0x6000: V[x] = kk; break;
            case 0x7000: V[x] += kk; break;
            case 0x8000:
                switch (n) {
                    case 0x0: V[x] = V[y]; break;
                    case 0x1: V[x] |= V[y]; break;
                    case 0x2: V[x] &= V[y]; break;
                    case 0x3: V[x] ^= V[y]; break;
                    case 0x4: {
                        uint16_t sum = V[x] + V[y];
                        V[0xF] = sum > 255 ? 1 : 0;
                        V[x] = sum & 0xFF;
                        break;
                    }
                    case 0x5: V[0xF] = V[x] > V[y]; V[x] -= V[y]; break;
                    case 0x6: V[0xF] = V[x] & 0x1; V[x] >>= 1; break;
                    case 0x7: V[0xF] = V[y] > V[x]; V[x] = V[y] - V[x]; break;
                    case 0xE: V[0xF] = (V[x] & 0x80) >> 7; V[x] <<= 1; break;
                }
                break;
            case 0x9000: if (V[x] != V[y]) pc += 2; break;
            case 0xA000: I = nnn; break;
            case 0xB000: pc = nnn + V[0]; break;
            case 0xC000: V[x] = randByte(randGen) & kk; break;
            case 0xD000: {
                uint8_t xPos = V[x] % 64;
                uint8_t yPos = V[y] % 32;
                V[0xF] = 0;

                for (unsigned int row = 0; row < n; ++row) {
                    uint8_t spriteByte = memory[I + row];
                    for (unsigned int col = 0; col < 8; ++col) {
                        uint8_t spritePixel = spriteByte & (0x80 >> col);
                        uint32_t* screenPixel = &display[(yPos + row) * 64 + (xPos + col)];

                        if (spritePixel) {
                            if (*screenPixel == 0xFFFFFFFF) {
                                V[0xF] = 1;
                            }
                            *screenPixel ^= 0xFFFFFFFF;
                        }
                    }
                }
                break;
            }
            case 0xF000:
                switch (kk) {
                    case 0x07: V[x] = delay_timer; break;
                    case 0x15: delay_timer = V[x]; break;
                    case 0x1E: I += V[x]; break;
                    case 0x29: I = V[x] * 5; break;
                    case 0x33:
                        memory[I] = V[x] / 100;
                        memory[I + 1] = (V[x] / 10) % 10;
                        memory[I + 2] = V[x] % 10;
                        break;
                    case 0x55:
                        for (uint8_t i = 0; i <= x; ++i) {
                            memory[I + i] = V[i];
                        }
                        break;
                    case 0x65:
                        for (uint8_t i = 0; i <= x; ++i) {
                            V[i] = memory[I + i];
                        }
                        break;
                }
                break;
        }

        if (delay_timer > 0) --delay_timer;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
        return 1;
    }

    Chip8 chip8;
    chip8.LoadROM(argv[1]);

    // Run a few cycles and print register state
    for (int i = 0; i < 10; ++i) {
        chip8.Cycle();
        std::cout << "Cycle " << i << " - PC: 0x" << std::hex << chip8.pc << std::endl;
    }

    return 0;
}