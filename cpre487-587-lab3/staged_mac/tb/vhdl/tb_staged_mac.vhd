library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use std.env.all;

entity tb_staged_mac is
end tb_staged_mac;

architecture test of tb_staged_mac is
    signal clk    : std_logic := '0';
    signal resetn : std_logic := '0';

    signal s_data  : std_logic_vector(15 downto 0) := x"0000";
    signal s_valid : std_logic := '0';
    signal s_ready : std_logic;
    signal s_last  : std_logic := '0';
    signal s_user  : std_logic := '0';
    signal s_id    : std_logic_vector(7 downto 0) := x"00";

    signal m_data  : std_logic_vector(31 downto 0);
    signal m_valid : std_logic;
    signal m_ready : std_logic := '0';
    signal m_last  : std_logic;
    signal m_id    : std_logic_vector(7 downto 0);

begin
    clk <= not clk after 5 ns;

    dut : entity work.staged_mac
        generic map (
            C_DATA_WIDTH => 8
        )
        port map (
            ACLK           => clk,
            ARESETN        => resetn,

            SD_AXIS_TREADY => s_ready,
            SD_AXIS_TDATA  => s_data,
            SD_AXIS_TLAST  => s_last,
            SD_AXIS_TUSER  => s_user,
            SD_AXIS_TVALID => s_valid,
            SD_AXIS_TID    => s_id,

            MO_AXIS_TVALID => m_valid,
            MO_AXIS_TDATA  => m_data,
            MO_AXIS_TLAST  => m_last,
            MO_AXIS_TREADY => m_ready,
            MO_AXIS_TID    => m_id
        );

    stimulus : process

        -- Apply synchronous active-low reset.
        procedure reset_mac is
        begin
            wait until falling_edge(clk);
            resetn  <= '0';
            s_valid <= '0';
            s_last  <= '0';
            s_user  <= '0';
            m_ready <= '0';

            wait until rising_edge(clk);
            wait for 1 ns;

            assert m_valid = '0'
                report "FAIL: VALID after reset"
                severity failure;

            assert m_data = x"00000000"
                report "FAIL: accumulator after reset"
                severity failure;

            assert m_id = x"00"
                report "FAIL: ID after reset"
                severity failure;

            assert s_ready = '1' and m_last = '0'
                report "FAIL: state after reset"
                severity failure;

            wait until falling_edge(clk);
            resetn <= '1';
        end procedure;

        -- Hold each input until it is accepted.
        procedure send_input (
            constant data_value : std_logic_vector(15 downto 0);
            constant last_value : std_logic := '0';
            constant user_value : std_logic := '0';
            constant id_value   : std_logic_vector(7 downto 0) := x"00"
        ) is
        begin
            wait until falling_edge(clk);
            s_data  <= data_value;
            s_last  <= last_value;
            s_user  <= user_value;
            s_id    <= id_value;
            s_valid <= '1';

            loop
                wait until rising_edge(clk);
                exit when s_ready = '1';
            end loop;

            s_valid <= '0';
            s_last  <= '0';
            s_user  <= '0';
        end procedure;

        -- Check the result, optionally stall it, then accept it.
        procedure check_output (
            constant expected    : integer;
            constant test_name   : string;
            constant expected_id : std_logic_vector(7 downto 0) := x"00";
            constant stall_cycles : natural := 0
        ) is
            procedure verify is
            begin
                assert m_valid = '1'
                    report "FAIL " & test_name & ": VALID"
                    severity failure;

                assert signed(m_data) = to_signed(expected, 32)
                    report "FAIL " & test_name & ": result"
                    severity failure;

                assert m_last = '1'
                    report "FAIL " & test_name & ": LAST"
                    severity failure;

                assert m_id = expected_id
                    report "FAIL " & test_name & ": ID"
                    severity failure;

                assert s_ready = '0'
                    report "FAIL " & test_name & ": input READY during output"
                    severity failure;
            end procedure;
        begin
            -- m_ready remains 0 until checks are complete.
            loop
                wait until rising_edge(clk);
                exit when m_valid = '1';
            end loop;

            verify;

            for i in 1 to stall_cycles loop
                wait until rising_edge(clk);
                verify;
            end loop;

            wait until falling_edge(clk);
            m_ready <= '1';

            wait until rising_edge(clk);
            verify;  -- Check the value actually transferred.

            wait for 1 ns;

            assert m_valid = '0' and s_ready = '1'
                report "FAIL " & test_name & ": return to input state"
                severity failure;

            assert m_data = x"00000000"
                report "FAIL " & test_name & ": accumulator not cleared"
                severity failure;

            wait until falling_edge(clk);
            m_ready <= '0';

            report "PASS: " & test_name severity note;
        end procedure;

    begin
        reset_mac;

        -- Case 1: single product
        send_input(x"0101", '1');
        check_output(1, "Case 1");

        -- Case 2: multiple products
        send_input(x"0102");
        send_input(x"0203");
        send_input(x"0301", '1');
        check_output(11, "Case 2");

        -- Case 3: bias + product
        send_input(x"0007", '0', '1');
        send_input(x"0203", '1');
        check_output(13, "Case 3");

        -- Case 4: negative product
        send_input(x"FE03", '1');
        check_output(-6, "Case 4");

        -- Case 5: signed 8-bit limits
        send_input(x"7F7F", '1');
        check_output(16129, "Case 5a");

        send_input(x"8080", '1');
        check_output(16384, "Case 5b");

        -- Case 6: separate calculations without resetting
        send_input(x"0203", '1');
        check_output(6, "Case 6a");

        send_input(x"0304", '1');
        check_output(12, "Case 6b");

        -- Case 7: bias + 75 products
        send_input(x"0005", '0', '1');

        for i in 1 to 75 loop
            if i = 75 then
                send_input(x"0203", '1');
            else
                send_input(x"0203");
            end if;
        end loop;

        check_output(455, "Case 7");

        -- Negative bias: -7 + 6 = -1
        send_input(x"FFF9", '0', '1');
        send_input(x"0203", '1');
        check_output(-1, "Negative bias");

        -- Minimum signed 16-bit bias + zero product
        send_input(x"8000", '0', '1');
        send_input(x"0000", '1');
        check_output(-32768, "Minimum bias");

        -- Maximum signed 16-bit bias + zero product
        send_input(x"7FFF", '0', '1');
        send_input(x"0000", '1');
        check_output(32767, "Maximum bias");

        -- Most negative signed 8-bit product
        send_input(x"807F", '1');
        check_output(-16256, "Minimum product");

        -- Zero product
        send_input(x"007F", '1');
        check_output(0, "Zero product");

        -- Input gap: invalid data must not affect accumulator.
        send_input(x"0203");  -- acc = 6

        wait until falling_edge(clk);
        s_valid <= '0';
        s_data  <= x"7FFF";
        s_user  <= '1';
        s_last  <= '1';
        s_id    <= x"FF";

        for i in 1 to 3 loop
            wait until rising_edge(clk);
            wait for 1 ns;

            assert m_data = x"00000006" and m_valid = '0'
                report "FAIL: invalid input changed calculation"
                severity failure;

            assert m_id = x"00"
                report "FAIL: invalid input changed ID"
                severity failure;
        end loop;

        send_input(x"0304", '1');
        check_output(18, "Input gap");

        -- Output stall: result and metadata must remain stable.
        -- Current implementation forwards the last accepted ID.
        send_input(x"0102", '0', '0', x"11");
        send_input(x"0203", '1', '0', x"22");

        -- Present another input while MAC is not ready.
        wait until falling_edge(clk);
        s_data  <= x"7FFF";
        s_user  <= '1';
        s_last  <= '1';
        s_id    <= x"FF";
        s_valid <= '1';

        check_output(8, "Output stall and ID", x"22", 4);

        -- Remove the pending input before another rising edge.
        s_valid <= '0';
        s_user  <= '0';
        s_last  <= '0';

        -- Reset during accumulation.
        send_input(x"0203");
        reset_mac;
        send_input(x"0304", '1');
        check_output(12, "Reset during accumulation");

        -- Reset while a result is waiting to be accepted.
        send_input(x"0203", '1');

        wait until rising_edge(clk);
        assert m_valid = '1'
            report "FAIL: expected pending output"
            severity failure;

        reset_mac;
        send_input(x"0101", '1');
        check_output(1, "Reset during output");

        report "ALL TESTS PASSED" severity note;
        stop;
        wait;
    end process;

    -- Prevent an infinite wait if the design gets stuck.
    process
    begin
        wait for 100 us;
        assert false
            report "FAIL: simulation timeout"
            severity failure;
        wait;
    end process;

end test;