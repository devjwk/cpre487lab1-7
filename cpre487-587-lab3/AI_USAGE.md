# AI Usage Disclosure – Lab 3 Resubmission

Per the CPRE 4870 syllabus GenAI policy (item 2): tool, model, how it was
used, prompts, and how I updated or used each response.

- **Tools:** Claude Code (command-line interface in my terminal), model Claude Opus 5.5 (`claude-opus-5-5`); Gemini, once, for a reference diagram in 3.4(a) (Entry 22)
- **Starting point:** official course repository (git.ece.iastate.edu), commit `f13d859`
- **Commands:** Claude Code did not run any command in this repository or on
  the lab computers. All builds, simulations, synthesis, board runs and git
  commands were typed and run by me. On my own laptop only, Claude Code ran
  read-only commands: reading my GitHub repository to check my commits and
  files, copying my screenshots into one folder, and pulling earlier drafts
  of this log from our conversation history.
- **When this log was written:** Entries 1–4 were written on 2026-11-09.
  Entries 5–27 were written on 2026-11-11 from my conversation log and are
  dated by when the work was done. Entries 28 onward were written the same
  day as the work.

---

## Entry 1 – Milestone plan (2026-11-09)

- **How it was used:** planning. I asked for a one-day plan that maps the
  handout's highlighted requirements to milestones and separates what AI may
  help with from what I must do myself.
- **Prompts:**
  - "Lab3부터 실제 진행과정을 실험실과 ai policy 기준에서 새로 시작하고 싶어 하루만에 끝낼수 있게 마일스톤 정리해줘"
    (I want to restart Lab 3 following the lab process and the AI policy;
    summarize milestones so I can finish in one day.)
  - "정확하게 어느 부분에서 ai 도움을 받을수 있고 어느 부분에서 도움을 받지않고 직접 해야되는거야?"
    (Exactly which parts can I get AI help with, and which must I do myself?)
- **AI output (summary):** a milestone table M0–M8 with time estimates,
  deliverables and example commit messages, and a table of allowed and
  not-allowed AI use.
- **How I updated / used it:** I checked the milestones against the
  handout's highlighted items and use them as my schedule. The design,
  code, tool runs and analysis in each milestone are done by me.

## Entry 2 – Starter file structure (2026-11-09)

- **How it was used:** explanation of the starter code layout.
- **Prompts:**
  - "m0부터 설명해줘" (Explain from M0.)
- **AI output (summary):** what each starter folder is for (staged_mac,
  piped_mac, simple_interface, software_testing) and why to start a new
  repository from the course starter.
- **How I updated / used it:** I opened the folders and printed the
  staged_mac port list myself to check the explanation.

## Entry 3 – Git setup questions and error fixes (2026-11-09)

- **How it was used:** git setup help and error explanations.
- **Prompts:**
  - "m0에서 현재 mkdir로 새로운 디렉토리를 만들고 git clone으로 학교 iastate.edu온라인에서 파일들을 가져왔어"
    (I made a new directory and cloned the files from the school repository.)
  - "내 git에다가 repository를 만들면서 동시에 거기에 업로드 할수 있게 설정하는 명령어를 간단하게 작성해줘"
    (Give me a short command to create a repository on my GitHub and upload to it.)
  - "origin이라는 remote가 없다는데" (It says there is no remote named origin.)
  - "gh라는 코맨드가 없다는데" (It says the gh command does not exist.)
  - "remote origin이 이제 있다고 뜨네" (Now it says remote origin already exists.)
  - "주소가 잘못됬음 다시 변경 가능?" (The URL is wrong; can I change it?)
  - "주소 변경 완료했어 근데 write access granted래" (I changed the URL, but it
    says write access not granted.)
- **AI output (summary):** git commands to rename the school remote to
  `course`, add my GitHub repository as `origin`, change a remote URL, and an
  explanation that my access token did not include the new repository.
- **How I updated / used it:** I ran each command myself on the lab computer,
  created the repository on the GitHub website because `gh` was not
  installed, corrected the remote URL, added the new repository to my token,
  and pushed.

## Entry 4 – This disclosure file (2026-11-09)

- **How it was used:** formatting this log.
- **Prompts:**
  - "ai usage에 작성할 내용: git 초기 세팅에 대한 질문과 오류 해결, 기본적인 파일 구조에 대한 설명과 실험 진행에 대한 마일스톤정리"
    (What to write in the AI usage log: git setup questions and error fixes,
    an explanation of the file structure, and the milestone plan.)
  - "규격을 정확하게 해서 작성해줘" (Write it in the exact required format.)
- **AI output (summary):** this file's structure and English wording.
- **How I updated / used it:** I checked every entry against what actually
  happened, filled in the commit hash, and committed it myself.

## Entry 5 – AXI-Stream ports (2026-11-09)

- **How it was used:** explanation of the AXI-Stream signals in the starter staged_mac entity.
- **Prompts:**
  - "ARESETN : IN, SD_AXIS_TREADY :OUT, ... Vivado 버전 2020.1" (my port list, asking what comes next)
- **AI output (summary):** a table of what each signal means and which side drives it, and the TVALID/TREADY handshake rule.
- **How I updated / used it:** I read handout 3.1 myself, marked the transfer cycles in the 3.1.7 example, and answered the check questions in my own words.

## Entry 6 – AXI-Stream check questions (2026-11-09)

- **How it was used:** checking my answers about the AXI-Stream handshake.
- **Prompts:** my answers to three check questions (what happens if the sender changes data while TREADY=0; what the MAC does after TLAST; what to do with SD_AXIS_TREADY when MO_AXIS_TREADY=0).
- **AI output (summary):** corrections: data must stay stable until the transfer; the last value is added before the result is sent and the accumulator is cleared after it is accepted; the MAC must drop SD_AXIS_TREADY (back-pressure).
- **How I updated / used it:** I will use these rules when I write the state machine in M3 and the tests for it.

## Entry 7 – Overflow proof method (2026-11-09)

- **How it was used:** explanation of how to bound the MAC sum for section 3.3.
- **Prompts:** "프로젝트 열었어 다음 단계 알려줘" (Project opened; tell me the next step.)
- **AI output (summary):** the method only: largest int8 product, worst case sum of N products, N per conv/dense layer, compare with a 32-bit signed accumulator. No numbers were given.
- **How I updated / used it:** I looked up every layer size in my Lab 2 code and did all calculations and the conclusion myself.

## Entry 8 – Reading the starter testbench (2026-11-09)

- **How it was used:** explanation of the starter staged_mac_tb.tcl and check questions.
- **Prompts:** "테스트밴치 내용을 어떻게 확인함?" (How do I view the testbench?) and the starter testbench text with my answers.
- **AI output (summary):** how to open the file, what add_force/run do, TDATA byte layout, what each test checks; feedback that the back-pressure section does not apply back-pressure (TREADY values are reversed).
- **How I updated / used it:** I read the testbench myself, found that results must be checked in the waveform and that values can be lost when TREADY=0; I will write my own tests in M3.

## Entry 9 – Overflow proof (2026-11-09)

- **How it was used:** explanation of the method for section 3.3, and checking my calculations.
- **Prompts:** questions about why the layer sizes matter, rounding log2, and my results
  ("conv2 800, conv3 288, conv4,5,6 576, dense1 2048 ... 10+11+1 < 32").
- **AI output (summary):** the method (largest product 2^14, sum of N products, compare with 32-bit), a worked conv1 example, and a correction: my bit count should start from 14 (product = 2^14), so dense1 needs 27 bits, not 22.
- **How I updated / used it:** I read the layer sizes from my Lab 2 code, computed N for every layer myself, found dense1 is the largest, and fixed my bit count to 27 bits < 32.

## Entry 10 – Section 3.3.1(a) (2026-11-09)

- **How it was used:** explanation of 3.3.1(a), then feedback on my own answers.
- **Prompts:** "3.3.1(a) 설명해줘" (Explain 3.3.1(a)) and my three answers (functionality, when to clear the accumulator, why a non-zero initial value).
- **AI output (summary):** the accumulator lifecycle (reset, add on transfer, hold, output, clear) and feedback: bias is loaded first through TUSER; my answer 2 was missing reset, hold and back-pressure cases; my answer 3 (partial sums) is correct and bias is the basic reason.
- **How I updated / used it:** I will rewrite the three answers in English myself, adding the missing cases.

## Entry 11 – English draft of 3.3.1(a) (2026-11-09)

- **How it was used:** translating my own Korean answers for 3.3.1(a) into an English report paragraph (a larger AI-generated block).
- **Prompts:**
  - "1.가중치와 활성값을 받고 ... 최종 합산 결과물을 output으로 내보낸다. 2.누산기를 비우는 시점은 최종 output이 나왔을때 다음 가산 과정을 거치기 시작하기전, 3. ... 0이 아닌 초기값도 필요함" (my answers)
  - "이건 영어로 초안으로 작성해줄래?" (Can you write this as an English draft?)
- **AI output (summary):** an English paragraph based on my three answers plus the points from the previous feedback (bias loaded first via TUSER; reset, hold and back-pressure cases).
- **How I updated / used it:** I will rewrite the draft in my own words when I write the final report, and record the changes here.

## Entry 12 – Section 3.3.1(b) test cases (2026-11-09)

- **How it was used:** guidance on what kinds of test cases to include, hex / two's complement conversion, and checking my values.
- **Prompts:** "2번부터 하나씩 같이 채워보자" (let's fill in from case 2 one by one), then my values for each case (e.g. "0102, 0203, 0301 → 2+6+3 = 11", "FE03 → -2×3 = -6", "7F7F → 16129, 8080 → 16384", "0005, 0203 → 5 + 75×6 = 455").
- **AI output (summary):** the list of behaviors to cover, conversion rules, hex results I had not converted (0xFFFF_FFFA, 0x3F01, 0x01C7), and a note that cases 2 and 3 have the same expected value.
- **How I updated / used it:** I chose all input values and computed every product and sum myself; I will use this table as my testbench cases in M3.

## Entry 13 – Section 3.3.1(c) port list (2026-11-09)

- **How it was used:** how to read port direction and width from the VHDL entity, and checking my answers port by port.
- **Prompts:** "포트 폭 하나씩 같이 채워보자", "ARESETN부터 같이 읽어보자" (let's fill in the port widths one by one) and my answers for each port.
- **AI output (summary):** the rule for widths (std_logic = 1, (hi downto lo) = hi - lo + 1), corrections (SD_AXIS_TID is an input), and why TID is returned with the result.
- **How I updated / used it:** I worked out the direction and width of every port myself from the entity and will draw the symbol myself.

## Entry 14 – Section 3.3.1(d) design decisions (2026-11-09)

- **How it was used:** step-by-step questions to decide my datapath and state machine before drawing the schematic.
- **Prompts:** "(d) 내부 회로도 설명해줘", "1번부터 하나씩 같이 풀어보자", "하나씩 같이 채워보자" and my answers (product width, sign extension, bias option B, enable condition, state outputs and transitions).
- **AI output (summary):** the list of parts, questions for each decision, corrections (product is 16 bits, not 15), and diagrams summarizing my answers.
- **How I updated / used it:** I made each design decision myself (option B for bias, enable = SD_TVALID and SD_TREADY, two states COLLECT/SEND with my output values) and will draw the schematic myself.

## Entry 15 – M3 VHDL step 1 (2026-11-09)

- **How it was used:** explanation of the starter template structure and of VHDL declaration syntax.
- **Prompts:** "M3 VHDL 작성 시작하자" (Let's start writing the VHDL for M3.)
- **AI output (summary):** which parts of the template map to my design, and the declaration formats (numeric_std, adding a state, signal syntax). No code for my design was given.
- **How I updated / used it:** I wrote the declarations myself.

## Entry 16 – staged_mac VHDL (2026-11-09)

- **How it was used:** step-by-step explanations of VHDL syntax and concepts (types, concurrent vs. process assignments, signed/resize, case/if), and review of the code I wrote.
- **Prompts:** "다음 설명해줘" (explain the next step) for each step, and my code for each part.
- **AI output (summary):** explanations, short syntax templates, and corrections (missing semicolons/end if, "else then", MO_AXIS_TREADY vs MO_AXIS_TDATA, tid_reg type, missing "acc +", weight/activation must be outside the process).
- **How I updated / used it:** I wrote every line of staged_mac.vhd myself and fixed each issue myself.

## Entry 17 – Starter testbench results (2026-11-10)

- **How it was used:** analysis of my simulation output.
- **Prompts:** my add_condition output from the starter testbench.
- **AI output (summary):** a results table and explanations: 3b is dropped because the testbench does not wait for SD_AXIS_TREADY (the MAC needs one idle cycle between groups); TID follows the last transfer of a group; back-pressure is not exercised.
- **How I updated / used it:** I ran the simulation and extracted the results myself; I will write my own testbench with my 8 cases and a real back-pressure test.

## Entry 18 – Testbench skeleton (2026-11-10)

- **How it was used:** the testbench setup section and empty test headers (larger AI-generated block).
- **Prompts:** "준비 부분 하나씩 같이 써보자", "일단 기본 골격은 너가 써줘" (Write the basic skeleton for me.)
- **AI output (summary):** restart, result printing (add_condition), clock, default inputs, reset, and comment headers for my 8 test cases with their expected values from my table.
- **How I updated / used it:** I chose the values and expected results for all 8 tests and wrote tests 1–3 myself. For tests 4–8, AI assembled the add_force/run lines from my values; I checked every line and ran the testbench.

## Entry 19 – Waveform analysis of my testbench (2026-11-10)

- **How it was used:** reading my simulation waveforms and checking them against my expected values.
- **Prompts:** "파형 분석해줄래?" (Can you analyze the waveforms?) with five screenshots of my simulation.
- **AI output (summary):** a pass/fail table for my 9 results, the back-pressure behavior seen in test 8, notes on UU before reset and on MO_AXIS_TDATA showing partial sums while TVALID=0, and a table mapping tests to behaviors for 3.3.1(f).
- **How I updated / used it:** I ran the simulation and captured the waveforms myself; I will write the 3.3.1(f) justification in my own words.

## Entry 20 – M4 staged_mac synthesis, 3.3.1(g) (2026-11-10)

- **How it was used:** explaining the synthesis reports, and errors.
- **Prompts:** "m4합성파트 해보자" (Let's do the M4 synthesis part.), "implemenstaion도 끝났음" (Implementation is done too.), and my grep output of the reports.
- **AI output (summary):** the timing failure is a pulse-width check caused by the xdc clock waveform {0 0.05}; the MAC path starts at the input ports with no input delay, so Vivado does not time it; the command to report it (`report_timing -from [all_inputs]`); an English draft of (g).
- **How I updated / used it:** I ran synthesis, implementation and all reports. I calculated the conv1 numbers (8,640,000 MACs, 77 cycles/output, 8,870,400 cycles, 107 ms) and answered the PE and reasonableness questions. I removed unmeasured statements from the draft.

## Entry 21 – 3.3.1(c)(d) drawing review (2026-11-10)

- **How it was used:** checking my hand drawings against my VHDL.
- **Prompts:** "c,d에 대한 그림은 이걸로 할거야" (I'll use these drawings for c and d.) and photos of the drawings.
- **AI output (summary):** errors found: TID width written as 18 (should be 8), "TDAAT" typo, one sign extender shared by bias and product (should be two), missing ARESETN/ACLK on registers.
- **How I updated / used it:** I fixed every listed error myself and redrew the final (c) symbol and (d) schematic by hand.

## Entry 22 – 3.4(a) pipeline register placement (2026-11-10)

- **How it was used:** guiding questions, and review of a reference diagram. Tools: Claude; Gemini (I asked Gemini to make a draft diagram so I could understand the pipeline and how each part works).
- **Prompts:** "m5 piped_mac 시작하자" (Let's start M5 piped_mac.), "1번부터 가장 간단하게 회로를 그리는 방법부터 알려줘" (Show me the simplest way to draw the circuit.), "이런 회로도가 맞음?" (Is this diagram right?)
- **AI output (summary):** Claude: questions on where to cut the path, what happens inside the feedback loop, and which control signals must follow the data; errors in the Gemini draft diagram (mo_data must come from sum, wrong acc_clr condition, stall missing); an English draft of (a).
- **How I updated / used it:** I chose the cut points (input register and product register, nothing inside the acc loop), concluded 3 stages maximum, and proposed the MUX before stage 2 for the bias. The Gemini diagram was only for understanding; I drew the final figure myself by hand.

## Entry 23 – 3.4(b) piped_mac.vhd (2026-11-10)

- **How it was used:** step plan and explanations, and review of the code I typed.
- **Prompts:** "3.4(b) VHDL 하나씩 같이 써보자" (Let's write the VHDL step by step.), "아까 쓴거랑 이거랑 차이가 뭐야?" (What's the difference from what I wrote before?), "이게 지금 acc_en logic인거지?" (Is this the acc_en logic?)
- **AI output (summary):** why a sum wire outside the process, an output register and a global stall are needed; errors in my code: std_logic_vector should be signed for s2_val/acc, missing `end if`, `&&` should be `and`, `signed()` should be `std_logic_vector()`, `else if` should be `elsif`, OUPUT_STAGE typo.
- **How I updated / used it:** I wrote every line of piped_mac.vhd myself and fixed each error.

## Entry 24 – piped_mac testbench T1–T11 (2026-11-10)

- **How it was used:** error diagnosis and waveform analysis.
- **Prompts:** the Simtcl 6-179 error, my console output, "T9 하나씩 같이 써보자" (Let's write T9 step by step.), "T11 하나씩 같이 써보자" (Let's write T11 step by step.)
- **AI output (summary):** the starter TB forces 64-bit TDATA into a 16-bit port; T8 failed because the TB dropped TVALID while SD_TREADY = 0 (an AXI rule violation); waveform analysis; a sed command to copy my staged TB; add_force lines for T9–T11 assembled from my values.
- **How I updated / used it:** I chose all inputs and expected values (T9 6/14, T10 8 or 33 if wrong, T11 6/14), ran every simulation and captured the waveforms.

## Entry 25 – 3.4(c) piped_mac synthesis (2026-11-10)

- **How it was used:** report interpretation.
- **Prompts:** screenshots of the timing summary and my grep output.
- **AI output (summary):** the critical path moved to the stage-2 multiplier (5.88 ns); 127 FFs matches the design. AI also proposed an estimated staged Fmax (≈138 MHz) that was not measured.
- **How I updated / used it:** I ran implementation and the reports, and calculated 169 MHz, 76 cycles/output and 51.8 ms. I rejected the estimate and kept only measured values.

## Entry 26 – 3.4(d) hazards (2026-11-10)

- **How it was used:** concept explanation.
- **Prompts:** my answers to "what if the adder takes 2 cycles?"
- **AI output (summary):** the RAW hazard example (10+3+5 → 15), the k× slowdown calculation, the interleaving idea, and an English draft of (d).
- **How I updated / used it:** I answered that p2 would read the old acc and that conv1 has 32 independent filters per pixel. I edited the draft.

## Entry 27 – Test-sufficiency paragraphs and this log (2026-11-10)

- **How it was used:** guiding questions, checking the drafts against the handout, and drafting my analysis in more professional technical English.
- **Prompts:** "하나씩 같이 채워보자" (Let's fill it in one by one.), "초안에서 뺄 내용이나 줄일것을 진행해보자" (Let's remove or shorten parts of the drafts.), "AI_USAGE 밀린 항목 정리하자" (Let's catch up the AI_USAGE entries.)
- **AI output (summary):** wrong-answer values for each test; tables and paragraphs for 3.3.1(f) and 3.4(b); trimmed drafts; drafts of Entries 5–27 of this file, written from our conversation.
- **How I updated / used it:** I stated what each test checks and gave the analysis. I asked AI to write it up in technical English as a draft, then rewrote the text based on my own understanding. I reviewed and corrected these AI_USAGE entries.

## Entry 28 – M6: block design and ILA (2026-11-11)

- **How it was used:** step-by-step guidance for handout section 4, and checks of my screenshots.
- **Prompts:** "오케이 2단계 고" (Go to step 2.), screenshots of my block design, "ila추가를 이렇게 하는게 실험 instruction에 맞게 진행한거야?" (Is adding the ILA this way consistent with the instructions?)
- **AI output (summary):** the order of steps from the handout; settings for the Subset Converter (4 bytes → 2 bytes, tdata[15:0]); checking my wiring in the screenshots; how to add a System ILA through Debug and Connection Automation with depth 4096 (Appendix A); a check that the saved .bd file had no ILA yet and the XSA was old.
- **How I updated / used it:** I built the block design, ran validation, generated the bitstream twice (once without and once with the ILA) and exported the XSA myself.

## Entry 29 – M7: board software and ILA capture (2026-11-11)

- **How it was used:** step-by-step guidance for main.cpp, error explanations, and help with the ILA trigger.
- **Prompts:** "하나씩 같이 써보자" (Let's write it step by step.), "검수해줘" (Please review it.), my board output (all FAIL), "트리거 해도 6이 안나오는데" (Even with the trigger, 6 doesn't appear.)
- **AI output (summary):** the function structure (mac_sw, pack, mac_hw, check); hints for each line; errors in my code: for-loop clauses in the wrong order, `i` not declared, a declaration written where a call was needed, a missing comma, a fixed length of 32 instead of n * 4, and the receive line reading RLF instead of RDFD (the hw values 0x80000004, 0x80000008, … were packet lengths); the idea of repeating the TEAM transaction so the ILA can be armed; trigger settings. AI also showed my finished main.cpp in order so I could compare it with my file.
- **How I updated / used it:** I typed every line of main.cpp myself, chose all test values and expected results (T1, T2, T4, T5a, T5b, TEAM = 6, T7 = 450), ran it on the lab computer and on the ZedBoard (all PASS after the RDFD fix, which is in commit 3ee32cf), and captured the ILA screenshot with TDATA = 6. Bias (TUSER) is not tested on the board because the FIFO stream has no TUSER.

## Entry 30 – Integrity check of the repository (2026-11-11)

- **How it was used:** I asked AI to check the whole repository for anything artificial or over-interpreted.
- **Prompts:** "현재 lab3이 무결성 체크해줘 인위적이거나 오버해석해서 실험이 진행된 부분들이 있는지 체크해줘" (Check the integrity of Lab 3 for anything artificial or over-interpreted.)
- **AI output (summary):** a list of record problems (placeholder commit hash, missing Gemini in the header, missing Entries 28–29, a template author line changed by mistake, a TB header copied from the staged TB) and wording to avoid over-claiming in the report. AI also corrected its own earlier claim: conv1 needs 22 bits, not 23. AI prepared the full updated text of this file for me to paste.
- **How I updated / used it:** I fixed each item myself (commit 152d623 and this update), checked this file, and committed it.
