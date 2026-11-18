----------------------------------------------------------------------------------
-- Convolutional MAC Unit
--
-- Gregory Ling, 2024
-- Implemented for CprE 487/587 Lab 7
----------------------------------------------------------------------------------

library work;
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity conv_mac is
  generic(
      C_DATA_WIDTH : integer := 32;
      C_OUTPUT_DATA_WIDTH : integer := 32
    );
    port (  
        S_AXIS_TREADY : out std_logic;
        S_AXIS_TDATA  : in  std_logic_vector(C_DATA_WIDTH*2-1 downto 0);
        S_AXIS_TLAST  : in  std_logic;
        S_AXIS_TVALID : in  std_logic;

        bias : in std_logic_vector(C_OUTPUT_DATA_WIDTH-1 downto 0);

        M_AXIS_TREADY : in  std_logic;
        M_AXIS_TDATA  : out std_logic_vector(C_OUTPUT_DATA_WIDTH-1 downto 0);
        M_AXIS_TLAST  : out std_logic;
        M_AXIS_TVALID : out std_logic;

        rst : in std_logic;
        clk : in std_logic
    );

end conv_mac;


architecture behavioral of conv_mac is

    -- Stage 1 registers (Multiply stage)
    signal s1_valid : std_logic;
    signal s1_last  : std_logic;
    signal s1_first : std_logic;
    signal s1_prod  : signed(C_DATA_WIDTH*2-1 downto 0);
    signal s1_bias  : signed(C_OUTPUT_DATA_WIDTH-1 downto 0);

    -- Stage 2 registers (Accumulate & Output stage)
    signal r_first_in : std_logic;
    signal r_acc      : signed(C_OUTPUT_DATA_WIDTH-1 downto 0);
    signal r_m_valid  : std_logic;
    signal r_m_last   : std_logic;
    signal r_m_data   : std_logic_vector(C_OUTPUT_DATA_WIDTH-1 downto 0);

    signal s_stall    : std_logic;
    signal s_ready    : std_logic;
    signal s_sum      : signed(C_OUTPUT_DATA_WIDTH-1 downto 0);

begin

    -- Stall pipeline only if output holds valid data and downstream is not ready
    s_stall <= r_m_valid and (not M_AXIS_TREADY);
    s_ready <= not s_stall;

    S_AXIS_TREADY <= s_ready;
    M_AXIS_TVALID <= r_m_valid;
    M_AXIS_TLAST  <= r_m_last;
    M_AXIS_TDATA  <= r_m_data;

    -- Combinational sum for stage 2: on first element of packet, add bias instead of previous r_acc
    s_sum <= (s1_bias + resize(s1_prod, C_OUTPUT_DATA_WIDTH)) when s1_first = '1'
             else (r_acc + resize(s1_prod, C_OUTPUT_DATA_WIDTH));

    process(clk)
        variable v_in_a : signed(C_DATA_WIDTH-1 downto 0);
        variable v_in_b : signed(C_DATA_WIDTH-1 downto 0);
    begin
        if rising_edge(clk) then
            if (rst = '1') then
                s1_valid   <= '0';
                s1_last    <= '0';
                s1_first   <= '1';
                s1_prod    <= (others => '0');
                s1_bias    <= (others => '0');
                r_first_in <= '1';
                r_acc      <= (others => '0');
                r_m_valid  <= '0';
                r_m_last   <= '0';
                r_m_data   <= (others => '0');
            elsif (s_stall = '0') then
                -- Stage 1: Input latch & multiplication
                if (S_AXIS_TVALID = '1') then
                    v_in_a   := signed(S_AXIS_TDATA(C_DATA_WIDTH*2-1 downto C_DATA_WIDTH));
                    v_in_b   := signed(S_AXIS_TDATA(C_DATA_WIDTH-1 downto 0));
                    s1_prod  <= v_in_a * v_in_b;
                    s1_bias  <= signed(bias);
                    s1_valid <= '1';
                    s1_last  <= S_AXIS_TLAST;
                    s1_first <= r_first_in;

                    if (S_AXIS_TLAST = '1') then
                        r_first_in <= '1';
                    else
                        r_first_in <= '0';
                    end if;
                else
                    s1_valid <= '0';
                end if;

                -- Stage 2: Accumulate & Output latch
                if (s1_valid = '1') then
                    r_acc <= s_sum;
                    if (s1_last = '1') then
                        r_m_data  <= std_logic_vector(s_sum);
                        r_m_valid <= '1';
                        r_m_last  <= '1';
                    elsif (M_AXIS_TREADY = '1') then
                        r_m_valid <= '0';
                        r_m_last  <= '0';
                    end if;
                elsif (M_AXIS_TREADY = '1') then
                    r_m_valid <= '0';
                    r_m_last  <= '0';
                end if;
            end if;
        end if;
    end process;

end architecture behavioral;
