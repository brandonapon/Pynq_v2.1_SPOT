-- Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2017.4 (win64) Build 2086221 Fri Dec 15 20:55:39 MST 2017
-- Date        : Thu Feb 22 19:52:09 2018
-- Host        : DESKTOP-PIC5S0G running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode synth_stub
--               d:/Pynq/PYNQ_V2.1/boards/Pynq-Z1/base/base/base.srcs/sources_1/bd/system/ip/system_mb3_timer_capture_0_0/system_mb3_timer_capture_0_0_stub.vhdl
-- Design      : system_mb3_timer_capture_0_0
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z020clg400-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity system_mb3_timer_capture_0_0 is
  Port ( 
    Din : in STD_LOGIC_VECTOR ( 7 downto 0 );
    Dout : out STD_LOGIC_VECTOR ( 0 to 0 )
  );

end system_mb3_timer_capture_0_0;

architecture stub of system_mb3_timer_capture_0_0 is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "Din[7:0],Dout[0:0]";
attribute X_CORE_INFO : string;
attribute X_CORE_INFO of stub : architecture is "xlslice_v1_0_1_xlslice,Vivado 2017.4";
begin
end;
