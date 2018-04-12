-- Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2017.4 (win64) Build 2086221 Fri Dec 15 20:55:39 MST 2017
-- Date        : Thu Feb 22 19:45:58 2018
-- Host        : DESKTOP-PIC5S0G running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode funcsim
--               d:/Pynq/PYNQ_V2.1/boards/Pynq-Z1/base/base/base.srcs/sources_1/bd/system/ip/system_constant_tkeep_tstrb_0/system_constant_tkeep_tstrb_0_sim_netlist.vhdl
-- Design      : system_constant_tkeep_tstrb_0
-- Purpose     : This VHDL netlist is a functional simulation representation of the design and should not be modified or
--               synthesized. This netlist cannot be used for SDF annotated simulation.
-- Device      : xc7z020clg400-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity system_constant_tkeep_tstrb_0 is
  port (
    dout : out STD_LOGIC_VECTOR ( 7 downto 0 )
  );
  attribute NotValidForBitStream : boolean;
  attribute NotValidForBitStream of system_constant_tkeep_tstrb_0 : entity is true;
  attribute CHECK_LICENSE_TYPE : string;
  attribute CHECK_LICENSE_TYPE of system_constant_tkeep_tstrb_0 : entity is "system_constant_tkeep_tstrb_0,xlconstant_v1_1_3_xlconstant,{}";
  attribute DowngradeIPIdentifiedWarnings : string;
  attribute DowngradeIPIdentifiedWarnings of system_constant_tkeep_tstrb_0 : entity is "yes";
  attribute X_CORE_INFO : string;
  attribute X_CORE_INFO of system_constant_tkeep_tstrb_0 : entity is "xlconstant_v1_1_3_xlconstant,Vivado 2017.4";
end system_constant_tkeep_tstrb_0;

architecture STRUCTURE of system_constant_tkeep_tstrb_0 is
  signal \<const1>\ : STD_LOGIC;
begin
  dout(7) <= \<const1>\;
  dout(6) <= \<const1>\;
  dout(5) <= \<const1>\;
  dout(4) <= \<const1>\;
  dout(3) <= \<const1>\;
  dout(2) <= \<const1>\;
  dout(1) <= \<const1>\;
  dout(0) <= \<const1>\;
VCC: unisim.vcomponents.VCC
     port map (
      P => \<const1>\
    );
end STRUCTURE;
