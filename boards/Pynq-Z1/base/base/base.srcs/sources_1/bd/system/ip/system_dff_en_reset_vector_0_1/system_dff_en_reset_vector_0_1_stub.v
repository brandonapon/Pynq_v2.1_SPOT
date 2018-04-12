// Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2017.4 (win64) Build 2086221 Fri Dec 15 20:55:39 MST 2017
// Date        : Thu Feb 22 19:55:11 2018
// Host        : DESKTOP-PIC5S0G running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub -rename_top system_dff_en_reset_vector_0_1 -prefix
//               system_dff_en_reset_vector_0_1_ system_dff_en_reset_vector_0_2_stub.v
// Design      : system_dff_en_reset_vector_0_2
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7z020clg400-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* X_CORE_INFO = "dff_en_reset_vector,Vivado 2017.4" *)
module system_dff_en_reset_vector_0_1(d, clk, en, reset, q)
/* synthesis syn_black_box black_box_pad_pin="d[0:0],clk,en,reset,q[0:0]" */;
  input [0:0]d;
  input clk;
  input en;
  input reset;
  output [0:0]q;
endmodule
