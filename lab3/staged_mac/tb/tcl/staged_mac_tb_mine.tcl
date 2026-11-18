# ---------------------------------------------------------------
# staged_mac testbench (my cases from report section 3.3.1 b)
# Run from staged_mac/vivado:  source ../tb/tcl/staged_mac_tb_mine.tcl
# ---------------------------------------------------------------

# 1) restart the simulation from 0 ns
relaunch_sim

# 2) print every result that is actually transferred (TVALID and TREADY both 1)
add_condition -notrace {/staged_mac/ACLK == 1 && /staged_mac/MO_AXIS_TVALID == 1 && /staged_mac/MO_AXIS_TREADY == 1} {puts "[current_time]  TDATA=0x[get_value -radix hex /staged_mac/MO_AXIS_TDATA]  TID=[get_value -radix hex /staged_mac/MO_AXIS_TID]"}

# 3) clock: 10 ns period
add_force {/staged_mac/ACLK} -radix hex {0 0ns} {1 5000ps} -repeat_every 10000ps

# 4) default inputs: nothing valid, receiver ready
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TUSER}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0000 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {00 0ns}
add_force {/staged_mac/MO_AXIS_TREADY} -radix hex {1 0ns}

# 5) reset for 30 ns, then release
add_force {/staged_mac/ARESETN} -radix hex {0 0ns}
run 30ns
add_force {/staged_mac/ARESETN} -radix hex {1 0ns}
run 10ns

# ---------------------------------------------------------------
# Test 1: one pair, 1 x 1                         expect 0x0000_0001
# ---------------------------------------------------------------
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0101 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TUSER}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {01 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
run 10ns
# ---------------------------------------------------------------
# Test 2: three pairs, 1x2 + 2x3 + 3x1             expect 0x0000_000B
# ---------------------------------------------------------------
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0102 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TUSER}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {02 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0203 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0301 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
run 20ns
# ---------------------------------------------------------------
# Test 3: bias 7, then 2 x 3                       expect 0x0000_000D
# ---------------------------------------------------------------
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0007 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TUSER}  -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {03 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0203 0ns}
add_force {/staged_mac/SD_AXIS_TUSER}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
run 20ns
# Test 4: negative product, (-2) x 3               expect 0xFFFF_FFFA
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {FE03 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TUSER}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {04 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
run 20ns

# Test 5a: 127 x 127                               expect 0x0000_3F01
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {7F7F 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {05 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
run 20ns

# Test 5b: (-128) x (-128)                         expect 0x0000_4000
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {8080 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {06 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
run 20ns

# ---------------------------------------------------------------
# Test 6: two groups in a row, 2x3 then 3x4       expect 0x6, then 0xC
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0203 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {07 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0304 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {08 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
run 20ns

# ---------------------------------------------------------------
# Test 7: conv1 size, bias 5 + 75 x (2 x 3)        expect 0x0000_01C7
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0005 0ns}
add_force {/staged_mac/SD_AXIS_TUSER}  -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {09 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0203 0ns}
add_force {/staged_mac/SD_AXIS_TUSER}  -radix hex {0 0ns}
for {set i 0} {$i < 74} {incr i} {
    run 10ns
}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}
run 20ns
# ---------------------------------------------------------------
# Test 8: back-pressure (MO_AXIS_TREADY = 0 first, then 1)
#         expect 0x0000_0008, TID 0A, printed only after TREADY = 1
# ---------------------------------------------------------------
# step 1: receiver is busy before the result exists
add_force {/staged_mac/MO_AXIS_TREADY} -radix hex {0 0ns}

# step 2: one pair, 2 x 4, last pair of the group
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA}  -radix hex {0204 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TID}    -radix hex {0A 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
add_force {/staged_mac/SD_AXIS_TLAST}  -radix hex {0 0ns}

# step 3: hold for 3 cycles (result must stay, nothing printed)
run 30ns

# step 4: receiver ready, result goes out once
add_force {/staged_mac/MO_AXIS_TREADY} -radix hex {1 0ns}
run 20ns
