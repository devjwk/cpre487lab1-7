#include <iostream>
#include "Types.h"

#ifdef ZEDBOARD
#include "xparameters.h"
#include "xllfifo_hw.h"
#include "xtime_l.h"
#endif


i32 mac_sw(const i8* w, const i8* a, size n){
	i32 acc =0;
	for (size i=0;i<n;i++){
		acc = acc + ((i32)w[i]*(i32)a[i]);
	}
	return acc;
}

ui32 pack(i8 w, i8 a){
	return (((ui32)(ui8)w) << 8) | (ui32)(ui8)a;
}
#ifdef ZEDBOARD
i32 mac_hw(const i8* w,const i8* a, size n){
	for (size i=0; i<n;i++){
		Xil_Out32(XPAR_AXI_FIFO_0_BASEADDR + XLLF_TDFD_OFFSET, pack(w[i],a[i]));
	}
	Xil_Out32(XPAR_AXI_FIFO_0_BASEADDR + XLLF_TLF_OFFSET, n * 4);

	while (Xil_In32(XPAR_AXI_FIFO_0_BASEADDR + XLLF_RDFO_OFFSET) == 0);
	Xil_In32(XPAR_AXI_FIFO_0_BASEADDR + XLLF_RLF_OFFSET);

	return (i32)Xil_In32(XPAR_AXI_FIFO_0_BASEADDR + XLLF_RDFD_OFFSET);
}
#endif

void check(const char* name, const i8* w, const i8* a, size n) {
    i32 expected = mac_sw(w, a, n);

#ifdef ZEDBOARD
    XTime start, end;

    XTime_GetTime(&start);
    i32 got = mac_hw(w, a, n);
    XTime_GetTime(&end);

    double elapsed_us =
        (double)(end - start) * 1000000.0 / COUNTS_PER_SECOND;

    std::cout << name
              << ": hw=" << got
              << " sw=" << expected
              << (got == expected ? " PASS" : " FAIL")
              << " time=" << elapsed_us << " us"
              << std::endl;
#else
    std::cout << name << ": sw=" << expected << std::endl;
#endif
}


void run_tests(){
	i8 w1[] = {1};  i8 a1[] = {1};
	check("T1",w1,a1,1);
	i8 w2[] = {1,2,3};  i8 a2[] = {2,3,1};
	check("T2",w2,a2,3);
	i8 w4[] = {-2};  i8 a4[] = {3};
	check("T4",w4,a4,1);
	i8 w5a[] = {127};  i8 a5a[] = {127};
	check("T5a",w5a,a5a,1);
	i8 w5b[] = {-128};  i8 a5b[] = {-128};
	check("T5b",w5b,a5b,1);
	i8 wT[] = {2};  i8 aT[] = {3};
	check("TEAM",wT,aT,1);

	i8 w7[75];
	i8 a7[75];

	for (size i = 0;i < 75;i++){
		w7[i] =2;
		a7[i] =3;
	}
	check("T7",w7,a7,75);
}

#ifdef ZEDBOARD
int main() {
	std::cout << "Running on the ZEDBoard..." << std::endl;	
	run_tests();

	//repeat the TEAM transactions so the ILA can be armed at any time
	i8 w[] ={2}; i8 a[]={3};
	while (true){
		mac_hw(w,a,1);
	}
	return 0;
}
#else
int main() {
	std::cout << "Running on the Lab computer..." << std::endl;
	run_tests();
	return 0;
}
#endif

