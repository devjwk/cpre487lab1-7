-------------------------------------------------------------------------
-- Matthew Dwyer
-- Department of Electrical and Computer Engineering
-- Iowa State University
-------------------------------------------------------------------------


-- piped_mac.vhd
-------------------------------------------------------------------------
-- DESCRIPTION: This file contains a basic piplined axi-stream mac unit. It
-- multiplies two integer values togeather and accumulates them.
--
-- NOTES:
-- 10/25/21 by MPD::Inital template creation
-- 9/5/25 by CWS::Minor changes to remove Qx.x
-------------------------------------------------------------------------

library work;
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity piped_mac is
  generic(
      -- Parameters of mac
      C_DATA_WIDTH : integer := 4
    );
	port (
        ACLK	: in	std_logic;
		ARESETN	: in	std_logic;       

        -- AXIS slave data interface
		SD_AXIS_TREADY	: out	std_logic;
		SD_AXIS_TDATA	: in	std_logic_vector(C_DATA_WIDTH*2-1 downto 0);  -- Packed data input
		SD_AXIS_TLAST	: in	std_logic;
        SD_AXIS_TUSER   : in    std_logic;  -- Should we treat this first value in the stream as an inital accumulate value?
		SD_AXIS_TVALID	: in	std_logic;
        SD_AXIS_TID     : in    std_logic_vector(7 downto 0);

        -- AXIS master accumulate result out interface
		MO_AXIS_TVALID	: out	std_logic;
		MO_AXIS_TDATA	: out	std_logic_vector(31 downto 0);
		MO_AXIS_TLAST	: out	std_logic;
		MO_AXIS_TREADY	: in	std_logic;
		MO_AXIS_TID     : out   std_logic_vector(7 downto 0)
    );

attribute SIGIS : string; 
attribute SIGIS of ACLK : signal is "Clk"; 

end piped_mac;


architecture behavioral of piped_mac is
    
    -- Internal Signals
    -- Internal Signlas for stage1
	signal s1_tdata : std_logic_vector(C_DATA_WIDTH*2-1 downto 0);
    signal s1_user  : std_logic;
    signal s1_last  : std_logic;
    signal s1_valid : std_logic;
    signal s1_tid   : std_logic_vector(7 downto 0);
    -- Internal Signals for stage2
    signal s2_val   : signed(C_DATA_WIDTH*2-1 downto 0);
    signal s2_user  : std_logic;
    signal s2_last  : std_logic;
    signal s2_valid : std_logic;
    signal s2_tid   : std_logic_vector(7 downto 0);
    -- Internal Signals for stage3
    signal acc      : signed(31 downto 0);
    signal sum      : signed(31 downto 0);
    -- Internal Signals for output stage
    signal mo_data  : std_logic_vector(31 downto 0);
    signal mo_valid : std_logic;
    signal mo_tid   : std_logic_vector(7 downto 0);
    -- Signals for entire stall when output stuck
    signal stall: std_logic;
	-- Mac stages
    type PIPE_STAGES is (STAGE1,STAGE2,STAGE3,OUTPUT_STAGE);

	
	-- Debug signals, make sure we aren't going crazy
    signal mac_debug : std_logic_vector(31 downto 0);

begin

    -- Interface signals
    MO_AXIS_TDATA <= mo_data;
    MO_AXIS_TVALID <= mo_valid;
    MO_AXIS_TLAST <= mo_valid;
    MO_AXIS_TID <= mo_tid;
    SD_AXIS_TREADY <= not stall;
    -- Internal signals
	sum <= resize(s2_val ,32) when s2_user='1' else acc+resize(s2_val,32);
    stall <= mo_valid and not MO_AXIS_TREADY;
	
	-- Debug Signals
    mac_debug <= x"00000000";  -- Double checking sanity
   
   process (ACLK) is
   begin 
    if rising_edge(ACLK) then  -- Rising Edge

      -- Reset values if reset is low
      if ARESETN = '0' then  -- Reset
      s1_valid <= '0';
      s2_valid <= '0';
      mo_valid <= '0';
      acc <= (others => '0');
      elsif stall = '0' then
        for i in PIPE_STAGES'left to PIPE_STAGES'right loop
            case i is  -- Stages
                when STAGE1 =>
					-- STAGE1 INPUT REGISTER
                    s1_tdata <= SD_AXIS_TDATA;
                    s1_user <= SD_AXIS_TUSER;
                    s1_last <= SD_AXIS_TLAST;
                    s1_valid <= SD_AXIS_TVALID;
                    s1_tid <= SD_AXIS_TID;
                when STAGE2 =>
                    --STAGE2 MULTIPLY AND BIAS SELECT
                    s2_user <= s1_user;
                    s2_last <= s1_last;
                    s2_valid <= s1_valid;
                    s2_tid   <= s1_tid;
                    if s1_user = '1' then
                        s2_val <= signed(s1_tdata);--bias used total bits from s1_tdata
                    else 
                        s2_val <= signed(s1_tdata(C_DATA_WIDTH*2-1 downto C_DATA_WIDTH)) * signed(s1_tdata(C_DATA_WIDTH-1 downto 0));
                    end if;
                when STAGE3 =>
                    --STAGE3 SIGN EXTEND AND ACC LOOP
                    if s2_valid = '1' then
                        if s2_last = '1' then
                            acc <= (others => '0');
                        else 
                            acc <= sum;
                        end if;
                    end if;
                when OUTPUT_STAGE =>
                    --last stage for output
                    if s2_valid = '1' and s2_last='1' then
                        mo_data <= std_logic_vector(sum);
                        mo_tid  <= s2_tid;
                        mo_valid <= '1';
                    elsif MO_AXIS_TREADY = '1' then
                        mo_valid <='0';
                    end if;
            end case;  -- Stages
		end loop;  -- Stages
      end if;  -- Reset

    end if;  -- Rising Edge
   end process;
end architecture behavioral;
