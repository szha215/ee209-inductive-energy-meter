library IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_unsigned.all;

entity segment_decoder is
  port(
		clk		:	in std_logic;										-- Clock
		b			:	in std_logic_vector (7 downto 0);			-- Received data
		b_done	:	in std_logic;										-- Whether the data is ready
		
		decoded_seg 	:	out std_logic_vector(6 downto 0);	-- Decoded 7 segment data
		DP_en				:	out std_logic;								-- DP on/off
		decoded_digit	: 	out std_logic_vector(3 downto 0)		-- Display digit selection
);
end entity;

architecture behaviour of segment_decoder is
signal temp : std_logic_vector(7 downto 0):= "00000000";	
begin

b_updater: process (clk, b_done, b(7 downto 0))			-- Decrypt b and store in temp when newest byte is ready
 begin
	if (clk'event and clk='1') then
		if (b_done = '1') then
			temp <= b xor "00010100";
			
			-- Check parity
			if (temp(0) = (not((((((temp(7) xor temp(6)) xor temp(5)) xor temp(4)) xor temp(3)) xor temp(2)) xor temp(1)))) then
				temp <= "00010100";		
			end if;
		end if;
	end if;
end process;

------------------------------------------------------------------------
digit_select_process: process (clk)			 -- Select display digit
 begin
	if (clk'event and clk='1') then		-- If data = GROUP_ID ("00000000"), turn off display
		if (temp = "00010100") then
			decoded_digit <= "0000";
		else
			case temp(7 downto 6) is 
				when "00" => decoded_digit <= "0001";
				when "01" => decoded_digit <= "0010";
				when "10" => decoded_digit <= "0100";
				when "11" => decoded_digit <= "1000";
			end case;
		end if;
	end if;
end process;

------------------------------------------------------------------------
segment_decoder_process: process (clk) 	 -- Decode value bits into 7 segment, active high
 begin
  if (clk'event and clk='1') then
		case  temp(5 downto 2) is										
			when "0000" => decoded_seg <= "0111111"; --'0'
			when "0001" => decoded_seg <= "0000110"; --'1'
			when "0010" => decoded_seg <= "1011011"; --'2'
			when "0011" => decoded_seg <= "1001111"; --'3'
			when "0100" => decoded_seg <= "1100110"; --'4'
			when "0101" => decoded_seg <= "1101101"; --'5'
			when "0110" => decoded_seg <= "1111101"; --'6'
			when "0111" => decoded_seg <= "0000111"; --'7'
			when "1000" => decoded_seg <= "1111111"; --'8'
			when "1001" => decoded_seg <= "1101111"; --'9'
			when "1010" => decoded_seg <= "0111110"; --'U (V)'
			when "1011" => decoded_seg <= "1110111"; --'A'
			when "1100" => decoded_seg <= "1110011"; --'P'
			when "1101" => decoded_seg <= "1111001"; --'E'
			when others => decoded_seg <= "0000000"; --' '
		end case;
  end if;
 end process;
 
 ------------------------------------------------------------------------
 DP_process: process (clk)			-- Decode DP bit, active high
 begin
	if (clk'event and clk='1') then
		if (temp(1)='1') then
			DP_en <= '1';
		else
			DP_en <= '0';
		end if;
	end if;
 end process;
 
end behaviour;