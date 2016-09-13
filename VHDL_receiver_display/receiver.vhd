library IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_unsigned.all;

entity receiver is
 port (
	clk		:	in std_logic;								-- Clock
	rx			:	in std_logic;								-- Input (received signal)
	reset		:	in std_logic;								-- Reset
	
	b			:	out std_logic_vector (7 downto 0);	-- Output received 8 bits
	b_done	:	out std_logic								-- Receive completetion signal
 );
end entity;

Architecture behaviour of receiver is
type my_states is (idle, start, resetS, pause, data, stop);
signal CS, NS : my_states:= idle;
signal Scounter_en, Ncounter_en, reset_S, reset_N, cmp7, cmpn7, cmp14, shift_en, reset_b : std_logic:= '0';
signal S, N : std_logic_vector (3 downto 0) := "0000";
signal b_temp : std_logic_vector (7 downto 0) := "00000000";

begin
----------------------------
--VHDL code for Moore FSM
----------------------------

Synchronous_process: process (reset, clk)		-- State updater
 begin
	if (reset = '1') then
		CS <= idle;
	elsif (clk'event and clk = '1') then
		CS <= NS;
	end if;
 end process;
 
------------------------------------------------------------------------
NextState_logic: process (CS, NS, rx, cmp7, cmp14, cmpn7)		-- State transition logic, Moore machine
 begin
 case CS is
	when idle =>
		if (rx = '0') then		-- When a low signal is received, go to start
			NS <= start;
		else
			NS <= idle;
		end if;
		
	when start =>					-- Count to 7 then start capturing data
		if (rx = '1') then		-- If a '1' is received within the start bit, go back to idle
			NS <= idle;
		elsif(cmp7 = '1') then
			NS <= resetS;
		else
			NS <= start;
		end if;
		
	when resetS =>					-- Go to next state
		NS <= pause;
		
	when pause =>					-- Count to 15 then go to data
		if (cmp14 = '1') then
			NS <= data;
		else
			NS <= pause;
		end if;
		
	when data =>					
		if (cmpn7 = '1') then	-- If all bits have been received, go to stop
				NS <= stop;
		else
				NS <= pause;		-- Else continue receiving
		end if;

	when stop =>					-- Count to 15 then go back to idle
		if (cmp14 = '1') then
			NS <= idle;
		else
			NS <= stop;
		end if;
end case;
 end process;
 
------------------------------------------------------------------------
Output_logic: process (CS, rx, cmp7, cmpn7, cmp14)		-- Outputs of each state
 begin
 reset_S <= '0';				-- Initialise all registers
 reset_N <= '0';
 Scounter_en <= '0';
 Ncounter_en <= '0';
 shift_en <= '0';
 reset_b <= '0';
 b_done <= '0';
 
 case CS is
	when idle =>				-- Reset S counter, N counter, b
		reset_S <= '1';
		reset_N <= '1';
		reset_b <= '1';
		Scounter_en <= '0';
		Ncounter_en <= '0';
		shift_en <= '0';
		
	when start =>				-- Enable S counter
		Scounter_en <= '1';
		reset_S <= '0';
		b_done <= '0';
		
	when resetS =>				-- Reset S counter
		reset_S <= '1';
	
	when pause =>				-- Enable S counter
		Scounter_en <= '1';
		
	when data =>				-- Enable b shift, N counter, reset S counter
		shift_en <= '1';
		Ncounter_en <= '1';
		reset_S <= '1';
		
	when stop =>				-- Reset N counter and send a receive complete signal
		Scounter_en <= '1';
		reset_N <= '1';
		b_done <= '1';
		
 end case;
 end process;

----------------------------
--VHDL code for datapath
----------------------------
Scounter_process: process(clk, reset_S, Scounter_en)		-- S counter, position of the signal
variable temp : std_logic_vector(3 downto 0);
begin
	if (clk'event and clk = '1') then		-- Reset S if asked
		if (reset_S = '1') then
			temp := "0000";
		elsif (Scounter_en = '1') then		-- Increment S by 1 if asked
			temp := temp + "0001";
		end if;
	end if;
	
	S <= temp;		-- Update S
end process;

------------------------------------------------------------------------
Ncounter_process: process (clk, reset_N, Ncounter_en)		-- N counter, number of bits received
variable temp : std_logic_vector(3 downto 0);
begin
  if (clk'event and clk = '1') then			-- Reset N if asked
		if (reset_N = '1') then
			temp := "0000";
		elsif (Ncounter_en = '1') then		-- Increment N by 1 if asked
			temp := temp + "0001";
		end if;
  end if;
  
  N <= temp;		-- Update N
end process;

------------------------------------------------------------------------
cmp7_process: process(S)		-- Compares if S is 7
begin
	if (S = "0111") then			-- 0111 is 7, if S=7, signal true
		cmp7 <= '1';
	else
		cmp7 <= '0';
	end if;
end process;

------------------------------------------------------------------------
cmpn7_process: process(N)		-- Compares if N is 7
begin
	if (N = "0111") then			-- if N=7, signal true
		cmpn7 <= '1';
	else
		cmpn7 <= '0';
	end if;
end process;

------------------------------------------------------------------------
cmp14_process: process(S)		-- Compares if S is 14, because going back to pause state takes 1 clk, 15-1=14
begin
	if (S = "1110") then			-- 1110 is 14, if S=14, signal true
		cmp14 <= '1';
	else
		cmp14 <= '0';
	end if;
end process;

------------------------------------------------------------------------
ShiftRegister_process: process(clk, rx, shift_en, b_temp(7 downto 0), reset_b) -- Shift b right by 1 bit when asked
begin
 if (clk'event and clk = '1') then			-- Clear b if asked
	if (reset_b = '1') then
		b_temp <= "00000000";
	elsif (shift_en = '1') then				-- Right shift b by 1 bit if asked
		b_temp <= rx & b_temp(7 downto 1);
	end if;
end if;
end process;

------------------------------------------------------------------------
b <= b_temp;		-- Update b as soon as it changes

end architecture;