// Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2017.4 (win64) Build 2086221 Fri Dec 15 20:55:39 MST 2017
// Date        : Thu Feb 22 19:19:06 2018
// Host        : DESKTOP-PIC5S0G running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub
//               d:/Pynq/PYNQ_V2.1/boards/Pynq-Z1/base/base/base.srcs/sources_1/bd/system/ip/system_concat_interrupts_0/system_concat_interrupts_0_stub.v
// Design      : system_concat_interrupts_0
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7z020clg400-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* X_CORE_INFO = "xlconcat_v2_1_1_xlconcat,Vivado 2017.4" *)
module system_concat_interrupts_0(In0, In1, In2, In3, In4, In5, dout)
/* synthesis syn_black_box black_box_pad_pin="In0[5:0],In1[0:0],In2[0:0],In3[2:0],In4[0:0],In5[0:0],dout[12:0]" */;
  input [5:0]In0;
  input [0:0]In1;
  input [0:0]In2;
  input [2:0]In3;
  input [0:0]In4;
  input [0:0]In5;
  output [12:0]dout;
endmodule
