# CprE 487/587 – Lab 3 Report Draft (Team 06 – Jongwoo Kim)

---

## 3.3.1 Show that such a MAC unit will never need to saturate or handle overflow for our model

The weights and activations are signed 8-bit integers, ranging from −128 to 127. The largest product is (−128) × (−128) = 16,384, which also bounds the absolute value of any product. In our model, the first Dense layer has the largest accumulation count, with 2,048 products per output. Starting from zero, the worst-case accumulation magnitude is:

$$2{,}048 \times 128^2 = 2^{11} \times 2^{14} = 2^{25} = 33{,}554{,}432$$

This is within the signed 32-bit range:

$$-2^{31} < -2^{25} \le S \le 2^{25} < 2^{31} - 1$$

Therefore, the Dense layer does not overflow when the accumulator starts from zero. The other layers accumulate fewer products, so they also remain within the signed 32-bit range and do not require saturation.

In addition, if the bias is used as the initial accumulator value and represented as a signed 16-bit integer, its range is:

$$-2^{15} \le \text{Bias} \le 2^{15} - 1$$

Including this bias, the accumulated result is bounded by:

$$-2^{15} - 2{,}048 \times 128^2 \le S \le (2^{15} - 1) + 2{,}048 \times 128^2$$

Comparing these bounds with the signed 32-bit range gives:

$$-2^{31} < -33{,}587{,}200 \le S \le 33{,}587{,}199 < 2^{31} - 1$$

Thus, even with a signed 16-bit bias, the accumulated result remains within the signed 32-bit range. Intermediate sums are also safe because they contain no more than 2,048 products. Under these conditions, neither the Dense layer nor the other layers require overflow handling or saturation.

---

## 3.3.1 (a–g) Staged MAC Design & Evaluation

### a. Describe the module's functionality, including how to control the accumulator state. Would you ever need to put a non-zero value in? Why?

The module (MAC) takes a weight and an activation, each represented as a signed 8-bit integer, multiplies them, and adds each product to the accumulator. For the conv2d layer I selected, it accumulates a total of 75 products to produce one output value.

The accumulator is initialized to zero when the reset signal is active. When an input is actually transferred (`TVALID = 1` and `TREADY = 1`), it accumulates the product. When no input is transferred, it keeps the current accumulated value. When the last input with `TLAST = 1` is transferred, it adds the last product and then outputs the result. It keeps the result unchanged until the receiving device accepts it. Once the result is transferred, the accumulator is reset to zero for the next accumulation.

A non-zero initial value is needed to include the bias of the convolution layer. Starting from the bias, the MAC accumulates the products of the weights and activations. In my design, when an input with `TUSER = 1` is transferred, the MAC does not multiply it. Instead, it reads the entire 16-bit input as a signed bias and sets it as the initial accumulator value.

### b. Identify a few example inputs and corresponding outputs that you expect for this module

| Case | Input Sequence — TDATA (hex) | Function Tested | Expected Calculation | Expected `MO_AXIS_TDATA` |
| :---: | :--- | :--- | :--- | :---: |
| 1 | `0x0101` (`TLAST = 1`) | Single multiplication | $1 \times 1 = 1$ | `0x00000001` |
| 2 | `0x0102`, `0x0203`, `0x0301` (`TLAST = 1` on last) | Accumulation of multiple products | $(1\times2) + (2\times3) + (3\times1) = 11$ | `0x0000000B` |
| 3 | `0x0007` (`TUSER = 1`), then `0x0203` (`TLAST = 1`) | Bias initialization + MAC | $7 + (2 \times 3) = 13$ | `0x0000000D` |
| 4 | `0xFE03` (`TLAST = 1`) | Signed multiplication & sign extension | $(-2) \times 3 = -6$ | `0xFFFFFFFA` |
| 5 | `0x7F7F` (`TLAST = 1`); `0x8080` (`TLAST = 1`) | Signed 8-bit input limits | $127\times127 = 16129$; $(-128)\times(-128) = 16384$ | `0x00003F01`; `0x00004000` |
| 6 | `0x0203` (`TLAST = 1`); `0x0304` (`TLAST = 1`) | Accumulator clearing between sequences | $2\times3 = 6$, then $3\times4 = 12$ | `0x00000006`, `0x0000000C` |
| 7 | `0x0005` (`TUSER = 1`), then 75× `0x0203` (`TLAST = 1` on last) | Bias init + 75 MACs for one conv2d output | $5 + 75 \times (2 \times 3) = 455$ | `0x000001C7` |

### c. Define the module interface
*(Insert Figure 1: Interface of the State Machine MAC Module)*

### d. Create a schematic diagram of your design with signal names, widths, and ports defined
*(Insert Figure 2: Schematic of the State Machine MAC Module)*

### e. Calculate (and report) all of the required bit-widths you will need based on the specific layer you implemented

For my selected `conv2d` layer, one output requires 75 MAC operations: $N = 5 \times 5 \times 3 = 75$.
Both the weight and activation are signed 8-bit values, so input width = 16 bits and product width = 16 bits.
Using the textbook formula:

$$\text{Accumulator width} = 8 + 8 + \lceil \log_2(75) \rceil = 23\text{ bits}$$

Including a signed 16-bit initial bias:
- Minimum result $= -2^{15} + 75 \times (-128 \times 127) = -1{,}251{,}968$
- Maximum result $= (2^{15} - 1) + 75 \times (-128 \times -128) = 1{,}261{,}567$

Both bounds fit easily within the signed 32-bit range ($-2^{31}$ to $2^{31} - 1$).

| Signal or Component | Bit Width | Reason |
| :--- | :---: | :--- |
| Weight | 8 | Specified signed input precision |
| Activation | 8 | Specified signed input precision |
| `SD_AXIS_TDATA` | 16 | Two packed 8-bit operands |
| Initial bias | 16 | Full input interpreted as a signed value |
| Multiplication result | 16 | 8-bit × 8-bit multiplication |
| Extended product and bias | 32 | Sign-extended to match the accumulator |
| Adder inputs and result | 32 | Match the accumulator width |
| MUX data inputs and output | 32 | Select zero, bias, or accumulated sum |
| Accumulator | 32 | Satisfies calculated width requirement |
| `MO_AXIS_TDATA` | 32 | Defined by the template |
| `SD_AXIS_TID`, ID reg, `MO_AXIS_TID` | 8 each | Defined by the template |
| MUX selection (`sel`) | 2 | $\lceil \log_2(3) \rceil = 2$ bits |
| `acc_en`, `id_en` | 1 each | Enable or hold |
| `TVALID`, `TREADY`, `TLAST`, `TUSER` | 1 each | Boolean control signals |
| `ACLK`, `ARESETN` | 1 each | Clock and reset |
| FSM state | Min 1 | $\lceil \log_2(2) \rceil = 1$ bit (2 states) |

### f. Justify why your tests were sufficient

I chose these tests to check that my MAC calculates correctly and updates its values only when it should. I used signed input and bias limits to check sign extension, and 75 products to check one complete conv2d output. I also added input gaps and output stalls to check that values stay unchanged while waiting. Finally, I tested resets and consecutive calculations to make sure the previous result does not affect the next calculation. All 18 tests passed, so I considered these tests sufficient to verify these behaviors.

### g. Report the maximum frequency, resource utilization, and critical path of the design. Calculate maximum throughput and PE scaling.

- **Utilization**: 119 LUTs, 41 Flip-Flops, 0 DSPs, 0 BRAMs.
- **Estimated $F_{\max}$**: **257.14 MHz**.
- **Critical Path**: `acc_reg[1]` through the adder to `acc_reg[31]` (**3.753 ns**).
- **Throughput (First Conv2D Layer: $60 \times 60 \times 32$ outputs, 75 MACs/output)**:
  - Total MAC operations $= 60 \times 60 \times 32 \times 75 = 8{,}640{,}000$
  - Cycles per output $= 75 + 1\text{ (bias)} + 1\text{ (output/clear)} = 77$
  - Total cycles $= 60 \times 60 \times 32 \times 77 = 8{,}870{,}400$
  - Time per layer $= 8{,}870{,}400 / 257.14 \times 10^6 \approx \mathbf{34.50\text{ ms}}$
  - Maximum throughput $\approx \mathbf{28.99\text{ layer executions/sec}}$
- **PE Scaling**: With $P$ PEs, different outputs can be calculated in parallel, ideally scaling throughput by $P\times$ when work is divided evenly. This estimate is reasonable when ignoring memory/transfer overhead, though in practice memory bandwidth and data transfer delays will reduce effective scaling.

---

## 3.4 Pipelined MAC Design & Evaluation

### a. Identify and report "natural" spots for pipeline registers, overall approach (with figure), and maximum number of stages
*(Insert Figure 3: Pipelined MAC Schematic)*

I placed registers at the input and after the multiplier, separating multiplication from accumulation into **three stages**: (1) Input registers, (2) Multiply / bias registers, and (3) Accumulate / final result. Control signals (`valid`, `user`, `last`, `tid`) propagate alongside the data. Three stages are the natural split while keeping multiplication and addition as single-cycle operations; further splitting accumulation would require extra feedback handling due to loop-carried dependencies.

### b. Justify why your tests were sufficient

I implemented the pipelined MAC in VHDL and verified it in Vivado. All tests passed, covering multiplication, accumulation, bias loading, negative signed values, 75-MAC Conv2D outputs, consecutive sequences, input gaps (`TVALID = 0`), and back-pressure stalls (`MO_AXIS_TREADY = 0`), confirming no data loss or mixing across pipeline stages.

### c. Report the maximum frequency and throughput of a chosen layer

- **Estimated $F_{\max}$**: $1000 / (5 - (-1.567)) \approx \mathbf{152.28\text{ MHz}}$
- **Throughput (First Conv2D Layer)**:
  - 75 MAC inputs + 1 bias input = 76 cycles per output
  - Total cycles $= 60 \times 60 \times 32 \times 76 = 8{,}755{,}200$
  - Estimated maximum throughput $= 152{,}280{,}000 / 8{,}755{,}200 \approx \mathbf{17.4\text{ layers/sec}}$

---

## 3.5 Software Testing on ZedBoard (`[TODO: Add ZedBoard Test Output]`)

- **[TODO]** Run `lab3/software_testing/src/main.cpp` on the ZedBoard with `test_hardware.xsa` and paste the verified AXI-FIFO MAC output and latency summary here.
