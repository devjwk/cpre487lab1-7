library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tb_staged_4bit is
end entity;

architecture test of tb_staged_4bit is
    signal clk, resetn, valid, last : std_logic := '0';
    signal ready, result_valid : std_logic;
    signal data : std_logic_vector(7 downto 0) := (others => '0');
    signal result : std_logic_vector(31 downto 0);
begin
    clk <= not clk after 5 ns;

    dut : entity work.staged_mac
        port map (
            ACLK => clk, ARESETN => resetn,
            SD_AXIS_TREADY => ready, SD_AXIS_TVALID => valid,
            SD_AXIS_TDATA => data, SD_AXIS_TLAST => last,
            SD_AXIS_TUSER => '0', SD_AXIS_TID => x"00",
            MO_AXIS_TDATA => result, MO_AXIS_TVALID => result_valid,
            MO_AXIS_TREADY => '1', MO_AXIS_TLAST => open,
            MO_AXIS_TID => open
        );

    process
    begin
        wait for 20 ns;
        resetn <= '1';
        wait until falling_edge(clk);
        data <= x"D2";  -- -3 * 2
        valid <= '1';
        wait until rising_edge(clk) and ready = '1';

        wait until falling_edge(clk);
        data <= x"78";  -- 7 * -8
        last <= '1';
        wait until rising_edge(clk) and ready = '1';
        valid <= '0';

        wait until result_valid = '1' for 100 ns;
        assert result_valid = '1' and signed(result) = to_signed(-62, 32)
            report "FAIL: expected -62" severity failure;
        report "PASS: staged 4-bit MAC returned -62";
        wait;
    end process;
end architecture;