library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library STD;
use STD.env.all;

entity tb_piped_mac is
end entity;

architecture test of tb_piped_mac is

    signal clk     : std_logic := '0';
    signal resetn  : std_logic := '0';

    signal s_ready : std_logic;
    signal s_data  : std_logic_vector(15 downto 0) := (others => '0');
    signal s_last  : std_logic := '0';
    signal s_user  : std_logic := '0';
    signal s_valid : std_logic := '0';
    signal s_tid   : std_logic_vector(7 downto 0) := (others => '0');

    signal m_ready : std_logic := '1';
    signal m_data  : std_logic_vector(31 downto 0);
    signal m_last  : std_logic;
    signal m_valid : std_logic;
    signal m_tid   : std_logic_vector(7 downto 0);

    signal output_count : natural := 0;

    type integer_array is array (natural range <>) of integer;

    -- Expected results in output order.
    constant expected_data : integer_array := (
        1,          -- T1
        11,         -- T2
        13,         -- T3
        -6,         -- T4
        16129,      -- T5a
        16384,      -- T5b
        6, 12,      -- T6
        455,        -- T7
        8,          -- T8
        6, 14,      -- T9
        8,          -- T10
        6, 14       -- T11
    );

    constant expected_tid : integer_array := (
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    );

    function test_name(index : natural) return string is
    begin
        case index is
            when 0  => return "T1";
            when 1  => return "T2";
            when 2  => return "T3";
            when 3  => return "T4";
            when 4  => return "T5a";
            when 5  => return "T5b";
            when 6  => return "T6a";
            when 7  => return "T6b";
            when 8  => return "T7";
            when 9  => return "T8";
            when 10 => return "T9a";
            when 11 => return "T9b";
            when 12 => return "T10";
            when 13 => return "T11a";
            when 14 => return "T11b";
            when others => return "Unexpected output";
        end case;
    end function;

begin

    clk <= not clk after 5 ns;

    dut : entity work.piped_mac
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
            SD_AXIS_TID    => s_tid,
            MO_AXIS_TVALID => m_valid,
            MO_AXIS_TDATA  => m_data,
            MO_AXIS_TLAST  => m_last,
            MO_AXIS_TREADY => m_ready,
            MO_AXIS_TID    => m_tid
        );

    -- Check each transferred output.
    monitor : process(clk)
        variable index       : natural := 0;
        variable was_stalled : boolean := false;
        variable held_data   : std_logic_vector(31 downto 0);
        variable held_tid    : std_logic_vector(7 downto 0);
        variable held_last   : std_logic;
    begin
        if rising_edge(clk) then
            if resetn = '0' then
                index := 0;
                output_count <= 0;
                was_stalled := false;
            else
                -- An unaccepted result must remain unchanged.
                if was_stalled then
                    assert m_valid = '1' and
                           m_data = held_data and
                           m_tid = held_tid and
                           m_last = held_last
                        report "FAIL: Output changed during stall"
                        severity failure;
                end if;

                if m_valid = '1' and m_ready = '0' then
                    assert s_ready = '0'
                        report "FAIL: Input READY remained high during stall"
                        severity failure;

                    held_data := m_data;
                    held_tid := m_tid;
                    held_last := m_last;
                    was_stalled := true;
                else
                    was_stalled := false;
                end if;

                if m_valid = '1' and m_ready = '1' then
                    if index >= expected_data'length then
                        assert false
                            report "FAIL: Unexpected extra output"
                            severity failure;
                    else
                        assert m_data =
                               std_logic_vector(to_signed(expected_data(index), 32))
                            report "FAIL: " & test_name(index) & " data"
                            severity failure;

                        assert m_tid =
                               std_logic_vector(to_unsigned(expected_tid(index), 8))
                            report "FAIL: " & test_name(index) & " TID"
                            severity failure;

                        assert m_last = '1'
                            report "FAIL: " & test_name(index) & " TLAST"
                            severity failure;

                        report "PASS: " & test_name(index)
                            severity note;

                        index := index + 1;
                        output_count <= index;
                    end if;
                end if;
            end if;
        end if;
    end process;

    stimulus : process

        -- Hold an input until the DUT accepts it.
        procedure send_input(
            data_value : std_logic_vector(15 downto 0);
            id_value   : natural;
            last_value : std_logic := '0';
            user_value : std_logic := '0'
        ) is
        begin
            wait until falling_edge(clk);
            s_data  <= data_value;
            s_tid   <= std_logic_vector(to_unsigned(id_value, 8));
            s_last  <= last_value;
            s_user  <= user_value;
            s_valid <= '1';

            loop
                wait until rising_edge(clk);
                exit when s_ready = '1';
            end loop;
        end procedure;

        procedure idle(cycles : positive := 1) is
        begin
            wait until falling_edge(clk);
            s_valid <= '0';
            s_last  <= '0';
            s_user  <= '0';

            for i in 1 to cycles loop
                wait until rising_edge(clk);
            end loop;
        end procedure;

    begin
        -- Reset.
        wait for 30 ns;
        wait until falling_edge(clk);
        resetn <= '1';

        -- T1: 1 x 1 = 1.
        send_input(x"0101", 1, '1');
        idle(3);

        -- T2: 1x2 + 2x3 + 3x1 = 11.
        send_input(x"0102", 2);
        send_input(x"0203", 2);
        send_input(x"0301", 2, '1');
        idle(3);

        -- T3: bias 7 + 2x3 = 13.
        send_input(x"0007", 3, '0', '1');
        send_input(x"0203", 3, '1');
        idle(3);

        -- T4: (-2) x 3 = -6.
        send_input(x"FE03", 4, '1');
        idle(3);

        -- T5a: 127 x 127 = 16129.
        send_input(x"7F7F", 5, '1');
        idle(3);

        -- T5b: (-128) x (-128) = 16384.
        send_input(x"8080", 6, '1');
        idle(3);

        -- T6: Separate groups produce 6 and 12.
        send_input(x"0203", 7, '1');
        idle(3);
        send_input(x"0304", 8, '1');
        idle(3);

        -- T7: bias 5 + 75 x (2x3) = 455.
        send_input(x"0005", 9, '0', '1');

        for i in 1 to 74 loop
            send_input(x"0203", 9);
        end loop;

        send_input(x"0203", 9, '1');
        idle(4);

        -- T8: Hold one result while the receiver is not ready.
        wait until falling_edge(clk);
        m_ready <= '0';

        send_input(x"0204", 10, '1');
        idle(6);

        assert m_valid = '1' and m_data = x"00000008"
            report "FAIL: T8 result not held"
            severity failure;

        wait until falling_edge(clk);
        m_ready <= '1';
        idle(4);

        -- T9: Consecutive groups without an input gap.
        -- Group 1 = 6; group 2 = 12 + 2 = 14.
        send_input(x"0203", 11, '1');
        send_input(x"0304", 12);
        send_input(x"0102", 12, '1');
        idle(5);

        -- T10: Ignore 0505 while TVALID = 0.
        send_input(x"0203", 13);

        wait until falling_edge(clk);
        s_valid <= '0';
        s_data  <= x"0505";
        s_last  <= '0';
        wait until rising_edge(clk);

        send_input(x"0102", 13, '1');
        idle(4);

        -- T11: Stall with valid data still in the pipeline.
        wait until falling_edge(clk);
        m_ready <= '0';

        send_input(x"0203", 14, '1');
        send_input(x"0101", 15);
        send_input(x"0202", 15);

        -- Present the next input and keep it stable during stall.
        wait until falling_edge(clk);
        s_data  <= x"0303";
        s_tid   <= x"0F";
        s_last  <= '1';
        s_user  <= '0';
        s_valid <= '1';

        for i in 1 to 4 loop
            wait until rising_edge(clk);

            assert s_ready = '0'
                report "FAIL: T11 pipeline did not stall"
                severity failure;
        end loop;

        -- Release the stall and accept the held input.
        wait until falling_edge(clk);
        m_ready <= '1';

        loop
            wait until rising_edge(clk);
            exit when s_ready = '1';
        end loop;

        idle(8);

        assert output_count = expected_data'length
            report "FAIL: Missing output"
            severity failure;

        report "ALL TESTS PASSED" severity note;
        stop;
        wait;
    end process;

    -- Prevent an unnoticed deadlock.
    timeout : process
    begin
        wait for 100 us;
        assert false
            report "FAIL: Simulation timeout"
            severity failure;
        stop;
        wait;
    end process;

end architecture;