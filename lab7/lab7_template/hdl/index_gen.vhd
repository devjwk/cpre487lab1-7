----------------------------------------------------------------------------------
-- Input & Filter Index Generator
--
-- Gregory Ling, 2024
-- Implemented for CprE 487/587 Lab 7
----------------------------------------------------------------------------------

library work;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity index_gen is
    generic(
        DIM_WIDTH : integer := 32;
        INPUT_ADDR_WIDTH : integer := 32;
        FILTER_ADDR_WIDTH : integer := 32;
        OUTPUT_ADDR_WIDTH : integer := 32
    );
    port(
        filter_w : in std_logic_vector(DIM_WIDTH-1 downto 0); -- Filter dimension width (Filter width)
        filter_h : in std_logic_vector(DIM_WIDTH-1 downto 0); -- Filter dimension height (Filter height)
        filter_c : in std_logic_vector(DIM_WIDTH-1 downto 0); -- Filter dimension channels (Filter channels == Input channels)
        output_w : in std_logic_vector(DIM_WIDTH-1 downto 0); -- Output dimension width (Output width)
        output_h : in std_logic_vector(DIM_WIDTH-1 downto 0); -- Output dimension height (Output height)
        input_end_diff_fw : in std_logic_vector(INPUT_ADDR_WIDTH-1 downto 0); -- Amount to add to addr when completing a filter row
        input_end_diff_fh : in std_logic_vector(INPUT_ADDR_WIDTH-1 downto 0); -- Amount to add to addr when completing a filter column
        input_end_diff_fc : in std_logic_vector(INPUT_ADDR_WIDTH-1 downto 0); -- Amount to add to addr when completing a filter channel
        input_end_diff_ow : in std_logic_vector(INPUT_ADDR_WIDTH-1 downto 0); -- Amount to add to addr when completing an output row
        
        M_AXIS_TREADY : in std_logic;
        M_AXIS_TDATA_input_addr : out std_logic_vector(INPUT_ADDR_WIDTH-1 downto 0);
        M_AXIS_TDATA_filter_addr : out std_logic_vector(FILTER_ADDR_WIDTH-1 downto 0);
        M_AXIS_TLAST : out std_logic;
        M_AXIS_TVALID : out std_logic;

        conv_idle : in std_logic; -- When the convolution is idle, reset the index generator. Starts counting on falling edge
        rst : in std_logic;
        clk : in std_logic
    );
end index_gen;

architecture Behavioral of index_gen is

    signal r_fw : unsigned(DIM_WIDTH-1 downto 0);
    signal r_fh : unsigned(DIM_WIDTH-1 downto 0);
    signal r_fc : unsigned(DIM_WIDTH-1 downto 0);
    signal r_ow : unsigned(DIM_WIDTH-1 downto 0);
    signal r_oh : unsigned(DIM_WIDTH-1 downto 0);

    signal r_input_addr  : signed(INPUT_ADDR_WIDTH-1 downto 0);
    signal r_filter_addr : unsigned(FILTER_ADDR_WIDTH-1 downto 0);
    signal r_done        : std_logic;

    signal s_fw_last : std_logic;
    signal s_fh_last : std_logic;
    signal s_fc_last : std_logic;
    signal s_ow_last : std_logic;
    signal s_oh_last : std_logic;
    signal s_tvalid  : std_logic;

begin

    s_fw_last <= '1' when (r_fw + 1 = unsigned(filter_w)) else '0';
    s_fh_last <= '1' when (r_fh + 1 = unsigned(filter_h)) else '0';
    s_fc_last <= '1' when (r_fc + 1 = unsigned(filter_c)) else '0';
    s_ow_last <= '1' when (r_ow + 1 = unsigned(output_w)) else '0';
    s_oh_last <= '1' when (r_oh + 1 = unsigned(output_h)) else '0';

    s_tvalid <= '1' when (rst = '0' and conv_idle = '0' and r_done = '0') else '0';

    M_AXIS_TVALID            <= s_tvalid;
    M_AXIS_TDATA_input_addr  <= std_logic_vector(r_input_addr);
    M_AXIS_TDATA_filter_addr <= std_logic_vector(r_filter_addr);
    M_AXIS_TLAST             <= s_fw_last and s_fh_last and s_fc_last;

    process(clk)
    begin
        if rising_edge(clk) then
            if (rst = '1' or conv_idle = '1') then
                r_fw          <= (others => '0');
                r_fh          <= (others => '0');
                r_fc          <= (others => '0');
                r_ow          <= (others => '0');
                r_oh          <= (others => '0');
                r_input_addr  <= (others => '0');
                r_filter_addr <= (others => '0');
                r_done        <= '0';
            elsif (s_tvalid = '1' and M_AXIS_TREADY = '1') then
                if (s_fw_last = '0') then
                    -- Step within filter row
                    r_fw          <= r_fw + 1;
                    r_input_addr  <= r_input_addr + 1;
                    r_filter_addr <= r_filter_addr + 1;
                elsif (s_fh_last = '0') then
                    -- Completed a filter row (fw), step to next filter row (fh)
                    r_fw          <= (others => '0');
                    r_fh          <= r_fh + 1;
                    r_input_addr  <= r_input_addr + signed(input_end_diff_fw);
                    r_filter_addr <= r_filter_addr + 1;
                elsif (s_fc_last = '0') then
                    -- Completed a filter channel slice (fh), step to next channel (fc)
                    r_fw          <= (others => '0');
                    r_fh          <= (others => '0');
                    r_fc          <= r_fc + 1;
                    r_input_addr  <= r_input_addr + signed(input_end_diff_fh);
                    r_filter_addr <= r_filter_addr + 1;
                else
                    -- Completed full 3D filter volume for one output pixel (TLAST)
                    r_fw          <= (others => '0');
                    r_fh          <= (others => '0');
                    r_fc          <= (others => '0');
                    r_filter_addr <= (others => '0');

                    if (s_ow_last = '0') then
                        -- Step to next output column (ow)
                        r_ow         <= r_ow + 1;
                        r_input_addr <= r_input_addr + signed(input_end_diff_fc);
                    elsif (s_oh_last = '0') then
                        -- Completed an output row (ow), step to next output row (oh)
                        r_ow         <= (others => '0');
                        r_oh         <= r_oh + 1;
                        r_input_addr <= r_input_addr + signed(input_end_diff_ow);
                    else
                        -- Completed all output pixels for the current 4-filter pass
                        r_done <= '1';
                    end if;
                end if;
            end if;
        end if;
    end process;

end Behavioral;
