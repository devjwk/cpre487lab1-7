#pragma once

#include <cstddef>

#include "Types.h"

#ifdef ZEDBOARD
#include "xil_io.h"
#include "xllfifo_hw.h"
#include "xparameters.h"
#define MAC_FIFO_BASE XPAR_AXI_FIFO_MM_S_0_BASEADDR
#endif

namespace ML {

constexpr std::size_t MAC_MAX_WORDS = 256;  // 한 패킷에 보내는 최대 word 수
constexpr int MAC_BITS = 8;                 // 8비트 MAC (4비트, 2비트 MAC이면 4 또는 2로 변경)

// word 안에서 pos 위치의 bits 비트를 부호 있는 수로 꺼냄
inline i32 macField(ui32 word, int pos, int bits) {
    const i32 signBit = 1 << (bits - 1);
    return (static_cast<i32>((word >> pos) & ((1u << bits) - 1)) ^ signBit) - signBit;
}

#ifdef ZEDBOARD

// 보드: FIFO로 word들을 보내고 MAC 결과 1개를 받음
inline i32 macSend(const ui32* words, std::size_t n) {
    for (std::size_t i = 0; i < n; i++) {
        Xil_Out32(MAC_FIFO_BASE + XLLF_TDFD_OFFSET, words[i]);
    }
    Xil_Out32(MAC_FIFO_BASE + XLLF_TLF_OFFSET, n * 4);  // 길이(바이트) 기록 = TLAST

    while (Xil_In32(MAC_FIFO_BASE + XLLF_RDFO_OFFSET) == 0) {}
    ui32 data = 0;
    while (true) {
        ui32 length = Xil_In32(MAC_FIFO_BASE + XLLF_RLF_OFFSET);
        for (ui32 i = 0; i < (length & 0x7FFFFFFFUL); i += 4) {
            data = Xil_In32(MAC_FIFO_BASE + XLLF_RDFD_OFFSET);
        }
        if (!(length & (1u << 31))) break;
    }
    return static_cast<i32>(data);
}

#else

// PC: 같은 계산을 소프트웨어로 수행하는 MAC 모델
inline i32 macSend(const ui32* words, std::size_t n) {
    i32 acc = 0;
    for (std::size_t i = 0; i < n; i++) {
        acc += macField(words[i], MAC_BITS, MAC_BITS) * macField(words[i], 0, MAC_BITS);
    }
    return acc;
}

#endif

// sum(weights[i] * acts[i]) 를 MAC으로 계산. 256개를 넘으면 패킷을 나눠 부분합을 더함
inline i32 macGroup(const i8* weights, const i8* acts, std::size_t n) {
    static ui32 words[MAC_MAX_WORDS];  // 보드 스택이 작아서 static
    const ui32 mask = (1u << MAC_BITS) - 1;
    i32 acc = 0;
    for (std::size_t done = 0; done < n; done += MAC_MAX_WORDS) {
        const std::size_t count = (n - done < MAC_MAX_WORDS) ? n - done : MAC_MAX_WORDS;
        for (std::size_t i = 0; i < count; i++) {
            words[i] = ((static_cast<ui32>(weights[done + i]) & mask) << MAC_BITS) |
                        (static_cast<ui32>(acts[done + i]) & mask);
        }
        acc += macSend(words, count);
    }
    return acc;
}

}  // namespace ML