----------------------------------------------------------------------------------
-- Output Storage Unit
--
-- Gregory Ling, 2024
-- Implemented for CprE 487/587 Lab 7
----------------------------------------------------------------------------------

library work;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity output_storage is
    generic(
        DATA_WIDTH : integer := 32;
        BRAM_DATA_WIDTH : integer := 32;
        ADDR_WIDTH : integer := 32;
        BRAM_ADDR_WIDTH : integer := 32;
        DIM_WIDTH : integer := 8;
        C_TID_WIDTH : integer := 1
    );
    port(
        S_AXIS_TREADY : out std_logic;
        S_AXIS_TDATA  : in  std_logic_vector(DATA_WIDTH-1 downto 0);
        S_AXIS_TLAST  : in  std_logic;
        S_AXIS_TID    : in  std_logic_vector(C_TID_WIDTH-1 downto 0);
        S_AXIS_TVALID : in  std_logic;

        BRAM_addr : out std_logic_vector(32-1 downto 0);
        BRAM_din : out std_logic_vector(BRAM_DATA_WIDTH-1 downto 0);
        BRAM_dout : in std_logic_vector(BRAM_DATA_WIDTH-1 downto 0);
        BRAM_en : out std_logic;
        BRAM_we : out std_logic_vector((BRAM_DATA_WIDTH/8)-1 downto 0);
        BRAM_rst : out std_logic;
        BRAM_clk : out std_logic;

        max_pooling : in std_logic;
        elements_per_channel : in std_logic_vector(ADDR_WIDTH-1 downto 0);
        output_w : in std_logic_vector(DIM_WIDTH-1 downto 0);
        output_h : in std_logic_vector(DIM_WIDTH-1 downto 0);
        initial_offset : in std_logic_vector(ADDR_WIDTH-1 downto 0);
        
        conv_complete : out std_logic;
        conv_idle : in std_logic;
        clk : in std_logic;
        rst : in std_logic
    );
end output_storage;

architecture Behavioral of output_storage is

    type state_type is (ST_IDLE, ST_READ_WAIT, ST_WRITE);
    signal r_state : state_type;

    -- Coordinate tracking across (oh, ow)
    signal r_ow           : unsigned(DIM_WIDTH-1 downto 0);
    signal r_oh           : unsigned(DIM_WIDTH-1 downto 0);
    signal r_unpooled_idx : unsigned(ADDR_WIDTH-1 downto 0);
    signal r_pool_row_base: unsigned(ADDR_WIDTH-1 downto 0);

    -- Latched transaction signals
    signal r_data         : signed(DATA_WIDTH-1 downto 0);
    signal r_tid          : unsigned(C_TID_WIDTH-1 downto 0);
    signal r_do_max       : std_logic;
    signal r_byte_lane    : unsigned(1 downto 0);
    signal r_word_addr    : std_logic_vector(31 downto 0);
    signal r_is_final     : std_logic;
    signal r_complete     : std_logic;

begin

    BRAM_clk      <= clk;
    BRAM_rst      <= rst;
    conv_complete <= r_complete;

                        S_AXIS_TREADY <= '1' when (r_state = ST_IDLE and rst = '0' and conv_idle = '0' and r_complete = '0' and r_is_final = '0') else '0';

    process(clk)
        variable v_pixel_offset : unsigned(ADDR_WIDTH-1 downto 0);
        variable v_chan_offset  : unsigned(ADDR_WIDTH-1 downto 0);
        variable v_total_addr   : unsigned(ADDR_WIDTH-1 downto 0);
        variable v_word_addr    : std_logic_vector(31 downto 0);
        variable v_old_word     : std_logic_vector(BRAM_DATA_WIDTH-1 downto 0);
        variable v_new_word     : std_logic_vector(BRAM_DATA_WIDTH-1 downto 0);
        variable v_old_byte     : signed(DATA_WIDTH-1 downto 0);
        variable v_write_byte   : signed(DATA_WIDTH-1 downto 0);
        variable v_tid_int      : integer;
        variable v_lane_int     : integer;
    begin
        if rising_edge(clk) then
            if (rst = '1' or conv_idle = '1') then
                r_state         <= ST_IDLE;
                r_ow            <= (others => '0');
                r_oh            <= (others => '0');
                r_unpooled_idx  <= (others => '0');
                r_pool_row_base <= (others => '0');
                r_data          <= (others => '0');
                r_tid           <= (others => '0');
                r_do_max        <= '0';
                r_byte_lane     <= (others => '0');
                r_word_addr     <= (others => '0');
                r_is_final      <= '0';
                r_complete      <= '0';
                BRAM_en         <= '0';
                BRAM_we         <= (others => '0');
                BRAM_addr       <= (others => '0');
                BRAM_din        <= (others => '0');
            else
                case r_state is
                    when ST_IDLE =>
                        BRAM_en <= '0';
                        BRAM_we <= (others => '0');

                        if (r_is_final = '1') then
                            -- Previous cycle's ST_WRITE has now been latched by BRAM on this rising edge!
                            r_complete <= '1';
                            r_is_final <= '0';
                        elsif (S_AXIS_TVALID = '1' and r_complete = '0') then
                            r_data <= signed(S_AXIS_TDATA);
                            r_tid  <= unsigned(S_AXIS_TID);

                            -- Calculate pixel offset within channel without multipliers
                            if (max_pooling = '1') then
                                v_pixel_offset := r_pool_row_base + resize(r_ow(DIM_WIDTH-1 downto 1), ADDR_WIDTH);
                            else
                                v_pixel_offset := r_unpooled_idx;
                            end if;

                            -- Calculate channel offset: TID * elements_per_channel (TID is 0..3)
                            v_tid_int := to_integer(unsigned(S_AXIS_TID));
                            case v_tid_int is
                                when 0 => v_chan_offset := (others => '0');
                                when 1 => v_chan_offset := unsigned(elements_per_channel);
                                when 2 => v_chan_offset := shift_left(unsigned(elements_per_channel), 1);
                                when others => v_chan_offset := shift_left(unsigned(elements_per_channel), 1) + unsigned(elements_per_channel);
                            end case;

                            v_total_addr := unsigned(initial_offset) + v_pixel_offset + v_chan_offset;
                            r_byte_lane  <= v_total_addr(1 downto 0);

                            v_word_addr := (others => '0');
                            v_word_addr(ADDR_WIDTH-1 downto 2) := std_logic_vector(v_total_addr(ADDR_WIDTH-1 downto 2));
                            r_word_addr <= v_word_addr;

                            if (max_pooling = '1' and (r_oh(0) = '1' or r_ow(0) = '1')) then
                                r_do_max <= '1';
                            else
                                r_do_max <= '0';
                            end if;

                            if (v_tid_int = 3 and (r_ow + 1 = unsigned(output_w)) and (r_oh + 1 = unsigned(output_h))) then
                                r_is_final <= '1';
                            else
                                r_is_final <= '0';
                            end if;

                            -- Issue BRAM read for Read-Modify-Write
                            BRAM_addr <= v_word_addr;
                            BRAM_en   <= '1';
                            BRAM_we   <= "0000";
                            r_state   <= ST_READ_WAIT;
                        end if;

                    when ST_READ_WAIT =>
                        -- Wait 1 cycle for BRAM synchronous read data to appear on BRAM_dout
                        BRAM_en <= '0';
                        BRAM_we <= "0000";
                        r_state <= ST_WRITE;

                    when ST_WRITE =>
                        v_old_word := BRAM_dout;
                        v_lane_int := to_integer(r_byte_lane);
                        v_old_byte := signed(v_old_word((v_lane_int + 1) * DATA_WIDTH - 1 downto v_lane_int * DATA_WIDTH));

                        if (r_do_max = '1' and v_old_byte > r_data) then
                            v_write_byte := v_old_byte;
                        else
                            v_write_byte := r_data;
                        end if;

                        v_new_word := v_old_word;
                        v_new_word((v_lane_int + 1) * DATA_WIDTH - 1 downto v_lane_int * DATA_WIDTH) := std_logic_vector(v_write_byte);

                        BRAM_addr <= r_word_addr;
                        BRAM_din  <= v_new_word;
                        BRAM_en   <= '1';
                        BRAM_we   <= "1111";

                        -- Advance pixel coordinates after 4th channel (TID = 3)
                        if (to_integer(r_tid) = 3) then
                            r_unpooled_idx <= r_unpooled_idx + 1;
                            if (r_ow + 1 < unsigned(output_w)) then
                                r_ow <= r_ow + 1;
                            else
                                r_ow <= (others => '0');
                                if (r_oh + 1 < unsigned(output_h)) then
                                    r_oh <= r_oh + 1;
                                    -- Advance pooled row base by (output_w / 2) every 2 output rows
                                    if (r_oh(0) = '1') then
                                        r_pool_row_base <= r_pool_row_base + resize(unsigned(output_w(DIM_WIDTH-1 downto 1)), ADDR_WIDTH);
                                    end if;
                                end if;
                            end if;
                        end if;

                        r_state <= ST_IDLE;
                end case;
            end if;
        end if;
    end process;

end Behavioral;
