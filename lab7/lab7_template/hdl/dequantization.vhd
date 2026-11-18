----------------------------------------------------------------------------------
-- Dequantization Unit
--
-- Gregory Ling, 2024
-- Implemented for CprE 487/587 Lab 7
----------------------------------------------------------------------------------

library work;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity dequantization is
    generic(
        C_DATA_WIDTH : integer := 32;
        C_TID_WIDTH : integer := 1;
        C_OUT_WIDTH : integer := 8
    );
    port(
        S_AXIS_TREADY : out std_logic;
        S_AXIS_TDATA  : in  std_logic_vector(C_DATA_WIDTH-1 downto 0);
        S_AXIS_TLAST  : in  std_logic;
        S_AXIS_TID    : in  std_logic_vector(C_TID_WIDTH-1 downto 0);
        S_AXIS_TVALID : in  std_logic;

        relu : in std_logic;
        q_scale : in std_logic_vector(C_DATA_WIDTH-1 downto 0);
        q_zero : in std_logic_vector(C_OUT_WIDTH-1 downto 0);

        M_AXIS_TREADY : in  std_logic;
        M_AXIS_TDATA  : out std_logic_vector(C_OUT_WIDTH-1 downto 0);
        M_AXIS_TLAST  : out std_logic;
        M_AXIS_TID    : out std_logic_vector(C_TID_WIDTH-1 downto 0);
        M_AXIS_TVALID : out std_logic;

        clk : in std_logic;
        rst : in std_logic
    );
end dequantization;

architecture Behavioral of dequantization is

    -- Pipeline Stage 1: Multiply (out * q_scale) >> 32
    signal s1_valid  : std_logic;
    signal s1_last   : std_logic;
    signal s1_tid    : std_logic_vector(C_TID_WIDTH-1 downto 0);
    signal s1_scaled : signed(C_DATA_WIDTH-1 downto 0);

    -- Pipeline Stage 2: ReLU + Zero point + Saturation to int8
    signal r_m_valid : std_logic;
    signal r_m_last  : std_logic;
    signal r_m_tid   : std_logic_vector(C_TID_WIDTH-1 downto 0);
    signal r_m_data  : std_logic_vector(C_OUT_WIDTH-1 downto 0);

    signal s_stall   : std_logic;

begin

    s_stall       <= r_m_valid and (not M_AXIS_TREADY);
    S_AXIS_TREADY <= not s_stall;

    M_AXIS_TVALID <= r_m_valid;
    M_AXIS_TLAST  <= r_m_last;
    M_AXIS_TID    <= r_m_tid;
    M_AXIS_TDATA  <= r_m_data;

    process(clk)
        variable v_in_val   : signed(C_DATA_WIDTH-1 downto 0);
        variable v_scale_u  : signed(C_DATA_WIDTH downto 0);
        variable v_prod     : signed(C_DATA_WIDTH*2 downto 0);
        variable v_relued   : signed(C_DATA_WIDTH-1 downto 0);
        variable v_zeroed   : signed(C_DATA_WIDTH-1 downto 0);
        variable v_sat      : signed(C_OUT_WIDTH-1 downto 0);
        constant MAX_VAL    : signed(C_DATA_WIDTH-1 downto 0) := to_signed((2**(C_OUT_WIDTH-1)) - 1, C_DATA_WIDTH);
        constant MIN_VAL    : signed(C_DATA_WIDTH-1 downto 0) := to_signed(-(2**(C_OUT_WIDTH-1)), C_DATA_WIDTH);
    begin
        if rising_edge(clk) then
            if (rst = '1') then
                s1_valid  <= '0';
                s1_last   <= '0';
                s1_tid    <= (others => '0');
                s1_scaled <= (others => '0');
                r_m_valid <= '0';
                r_m_last  <= '0';
                r_m_tid   <= (others => '0');
                r_m_data  <= (others => '0');
            elsif (s_stall = '0') then
                -- Stage 1: 32-bit signed * 32-bit unsigned scale -> arithmetic >> 32
                if (S_AXIS_TVALID = '1') then
                    v_in_val  := signed(S_AXIS_TDATA);
                    v_scale_u := signed('0' & q_scale);
                    v_prod    := v_in_val * v_scale_u;
                    s1_scaled <= v_prod(C_DATA_WIDTH*2-1 downto C_DATA_WIDTH);
                    s1_valid  <= '1';
                    s1_last   <= S_AXIS_TLAST;
                    s1_tid    <= S_AXIS_TID;
                else
                    s1_valid  <= '0';
                end if;

                -- Stage 2: ReLU, Add Zero Point, and Saturate
                if (s1_valid = '1') then
                    if (relu = '1' and s1_scaled < 0) then
                        v_relued := (others => '0');
                    else
                        v_relued := s1_scaled;
                    end if;

                    v_zeroed := v_relued + resize(signed(q_zero), C_DATA_WIDTH);

                    if (v_zeroed > MAX_VAL) then
                        v_sat := to_signed((2**(C_OUT_WIDTH-1)) - 1, C_OUT_WIDTH);
                    elsif (v_zeroed < MIN_VAL) then
                        v_sat := to_signed(-(2**(C_OUT_WIDTH-1)), C_OUT_WIDTH);
                    else
                        v_sat := resize(v_zeroed, C_OUT_WIDTH);
                    end if;

                    r_m_data  <= std_logic_vector(v_sat);
                    r_m_tid   <= s1_tid;
                    r_m_last  <= s1_last;
                    r_m_valid <= '1';
                elsif (M_AXIS_TREADY = '1') then
                    r_m_valid <= '0';
                end if;
            end if;
        end if;
    end process;

end Behavioral;
