-- Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2017.4 (win64) Build 2086221 Fri Dec 15 20:55:39 MST 2017
-- Date        : Thu Feb 22 19:25:59 2018
-- Host        : DESKTOP-PIC5S0G running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode synth_stub -rename_top system_xbar_7 -prefix
--               system_xbar_7_ system_xbar_7_stub.vhdl
-- Design      : system_xbar_7
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z020clg400-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity system_xbar_7 is
  Port ( 
    aclk : in STD_LOGIC;
    aresetn : in STD_LOGIC;
    s_axi_awaddr : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_awprot : in STD_LOGIC_VECTOR ( 2 downto 0 );
    s_axi_awvalid : in STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_awready : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_wdata : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_wstrb : in STD_LOGIC_VECTOR ( 3 downto 0 );
    s_axi_wvalid : in STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_wready : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_bresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_bvalid : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_bready : in STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_araddr : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_arprot : in STD_LOGIC_VECTOR ( 2 downto 0 );
    s_axi_arvalid : in STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_arready : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_rdata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_rresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_rvalid : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_rready : in STD_LOGIC_VECTOR ( 0 to 0 );
    m_axi_awaddr : out STD_LOGIC_VECTOR ( 319 downto 0 );
    m_axi_awprot : out STD_LOGIC_VECTOR ( 29 downto 0 );
    m_axi_awvalid : out STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_awready : in STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_wdata : out STD_LOGIC_VECTOR ( 319 downto 0 );
    m_axi_wstrb : out STD_LOGIC_VECTOR ( 39 downto 0 );
    m_axi_wvalid : out STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_wready : in STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_bresp : in STD_LOGIC_VECTOR ( 19 downto 0 );
    m_axi_bvalid : in STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_bready : out STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_araddr : out STD_LOGIC_VECTOR ( 319 downto 0 );
    m_axi_arprot : out STD_LOGIC_VECTOR ( 29 downto 0 );
    m_axi_arvalid : out STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_arready : in STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_rdata : in STD_LOGIC_VECTOR ( 319 downto 0 );
    m_axi_rresp : in STD_LOGIC_VECTOR ( 19 downto 0 );
    m_axi_rvalid : in STD_LOGIC_VECTOR ( 9 downto 0 );
    m_axi_rready : out STD_LOGIC_VECTOR ( 9 downto 0 )
  );

end system_xbar_7;

architecture stub of system_xbar_7 is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "aclk,aresetn,s_axi_awaddr[31:0],s_axi_awprot[2:0],s_axi_awvalid[0:0],s_axi_awready[0:0],s_axi_wdata[31:0],s_axi_wstrb[3:0],s_axi_wvalid[0:0],s_axi_wready[0:0],s_axi_bresp[1:0],s_axi_bvalid[0:0],s_axi_bready[0:0],s_axi_araddr[31:0],s_axi_arprot[2:0],s_axi_arvalid[0:0],s_axi_arready[0:0],s_axi_rdata[31:0],s_axi_rresp[1:0],s_axi_rvalid[0:0],s_axi_rready[0:0],m_axi_awaddr[319:0],m_axi_awprot[29:0],m_axi_awvalid[9:0],m_axi_awready[9:0],m_axi_wdata[319:0],m_axi_wstrb[39:0],m_axi_wvalid[9:0],m_axi_wready[9:0],m_axi_bresp[19:0],m_axi_bvalid[9:0],m_axi_bready[9:0],m_axi_araddr[319:0],m_axi_arprot[29:0],m_axi_arvalid[9:0],m_axi_arready[9:0],m_axi_rdata[319:0],m_axi_rresp[19:0],m_axi_rvalid[9:0],m_axi_rready[9:0]";
attribute X_CORE_INFO : string;
attribute X_CORE_INFO of stub : architecture is "axi_crossbar_v2_1_16_axi_crossbar,Vivado 2017.4";
begin
end;
