-- Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2017.4 (win64) Build 2086221 Fri Dec 15 20:55:39 MST 2017
-- Date        : Thu Feb 22 19:41:13 2018
-- Host        : DESKTOP-PIC5S0G running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode synth_stub
--               d:/Pynq/PYNQ_V2.1/boards/Pynq-Z1/base/base/base.srcs/sources_1/bd/system/ip/system_color_swap_0_0/system_color_swap_0_0_stub.vhdl
-- Design      : system_color_swap_0_0
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z020clg400-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity system_color_swap_0_0 is
  Port ( 
    hsync_in : in STD_LOGIC;
    hsync_out : out STD_LOGIC;
    pixel_in : in STD_LOGIC_VECTOR ( 23 downto 0 );
    pixel_out : out STD_LOGIC_VECTOR ( 23 downto 0 );
    vde_in : in STD_LOGIC;
    vde_out : out STD_LOGIC;
    vsync_in : in STD_LOGIC;
    vsync_out : out STD_LOGIC
  );

end system_color_swap_0_0;

architecture stub of system_color_swap_0_0 is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "hsync_in,hsync_out,pixel_in[23:0],pixel_out[23:0],vde_in,vde_out,vsync_in,vsync_out";
attribute X_CORE_INFO : string;
attribute X_CORE_INFO of stub : architecture is "color_swap,Vivado 2017.4";
begin
end;
