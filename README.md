GHI CHÚ : Đây là file viết từ AI đã qua chỉnh sửa của sinh viên

================================================================
  TIC-TAC-TOE CONSOLE GAME  —  README
  Version: 0.4.25022808
================================================================

MỤC LỤC
--------
  1. Yêu cầu hệ thống
  2. Biên dịch (Compile)
  3. Chạy game thường (Interactive Mode)
  4. Judge Mode & cờ dòng lệnh
  5. Chạy grader.py (tự động test)
  6. Cấu trúc testcase
  7. Output files
  8. Các tính năng game
  9. Ghi chú


================================================================
1. YÊU CẦU HỆ THỐNG
================================================================

  - Compiler: g++ (GCC 10+) hoặc clang++ 12+, hỗ trợ C++17
  - OS: Linux / macOS / Windows (MinGW hoặc WSL)
  - Python: 3.8+ (chỉ cần để chạy grader.py)
  - Không cần thư viện ngoài — chỉ dùng STL chuẩn


================================================================
2. BIÊN DỊCH (COMPILE)
================================================================

  >> Linux / macOS:
  ------------------
    g++ -std=c++17 -O2 -o game game.cpp

  >> Windows (MinGW):
  -------------------
    g++ -std=c++17 -O2 -o game.exe game.cpp

  Lưu ý: flag -O2 được khuyến nghị để bot HARD chạy nhanh hơn.
  Có thể thêm -Wall để xem cảnh báo:
    g++ -std=c++17 -O2 -Wall -o game game.cpp


================================================================
3. CHẠY GAME THƯỜNG (INTERACTIVE MODE)
================================================================

  >> Linux / macOS:
    ./game

  >> Windows:
    game.exe

  Khi chạy interactive, game sẽ hỏi lần lượt:
    [1] Kích thước bàn N (3 ≤ N ≤ 12)
    [2] Số quân thắng goal (3 ≤ goal ≤ N)
    [3] End Rule:
          (1) NONE      — thắng khi đủ goal quân liên tiếp
          (2) OPEN_ONE  — cần ít nhất 1 đầu hở
          (3) OPEN_TWO  — cần 2 đầu hở (mặc định)
    [4] Game mode: PVP / PVE / EVE
    [5] Chọn ký hiệu X hoặc O (chỉ PVP/PVE)
    [6] Độ khó bot: EASY / MEDIUM / HARD

  Trong lúc chơi (Human):
    - Nhập: <row> <col>      — đánh quân vào ô (row, col), 0-indexed
    - Nhập: 99 99            — Undo nước cuối (giới hạn 1 lần/lượt)
    - Nhập: 100 100          — Surrender (thua luôn)


================================================================
4. JUDGE MODE & CỜ DÒNG LỆNH 
================================================================

  Judge Mode được dùng để chạy game tự động từ file input,
  không hiển thị UI, phù hợp với grader.

  Cú pháp:
    ./game -j -i <đường_dẫn_input>

  Các cờ (flags):
  ----------------
  -j, --judge       Bật judge mode:
                      - Tắt UI (clearScreen, displayBoard, menu)
                      - Tắt delay bot (sleep_for)
                      - Tắt countdown timer
                      - Tắt màu ANSI trong log
                      - Chỉ in ra stdout: kết quả game

  -i, --input <file>  Đường dẫn file input để redirect cin
                      Mỗi dòng là 1 lần nhập từ người dùng/bot

  -l, --log <file>    Đường dẫn file log (mặc định: log.txt)
                      Dùng -l "" để tắt ghi file log

  -h, --help          Hiển thị help

  Ví dụ:
    ./game -j -i testcase/input01.txt
    ./game -j -i testcase/input01.txt -l mylog.txt
    ./src/game --judge --input testcase/input1.txt

================================================================
5. CHẠY GRADER.PY (TỰ ĐỘNG TEST)
================================================================

  grader.py chạy game trên tất cả testcase trong một thư mục,
  so sánh stdout của game với file output mẫu, và báo cáo kết quả.

  Yêu cầu:
    - game đã được compile sẵn (game hoặc game.exe cùng thư mục)
    - thư mục testcase chứa các cặp input*/output*

  Cú pháp:
    python grader.py --target src/game --testcase_dir testcase

  Tham số:
  ---------
  --target <tên>      Tên file thực thi (không đuôi .exe)
                      Mặc định: "game"
                      Ví dụ: --target mygame

  --testcase_dir <đường_dẫn>
                      Đường dẫn thư mục chứa testcase
                      Mặc định: "../testcase/"

  Ví dụ chạy:
  ------------
    # Cấu trúc thư mục mặc định:
    python grader.py

    # Chỉ định file và thư mục cụ thể:
    python grader.py --target game --testcase_dir ./testcase/

    # Nếu thư mục testcase ngang hàng với grader.py:
    python grader.py --testcase_dir testcase/

  Output mẫu của grader:
  -----------------------
    Grading start for game...
    --------------------------------------------------
    [1/5] Testing: input01.txt... PASSED
    [2/5] Testing: input02.txt... PASSED
    [3/5] Testing: input03.txt... FAILED (Wrong Answer)
    --- DIFF (input03.txt) ---
    Expected: Draw...
    Actual:   Player 1 wins...
    ------------------------------
    --------------------------------------------------
    GRADING OVERALL:
    - Total testcase: 5
    - Corrects: 2
    - Accuracy:  40.00%
    - Incorrects: input03.txt

  Hành vi của grader:
    - Dừng lại ngay khi gặp testcase đầu tiên FAILED
    - Timeout mỗi testcase: 10 giây (tránh vòng lặp vô hạn)
    - Game được chạy với cờ: -j -i <input_file>
    - So sánh stdout của game (sau khi .strip()) với nội dung output file


================================================================
6. CẤU TRÚC TESTCASE (đã chỉnh sửa để khớp với code chạy,thêm chọn end_rule)
================================================================

  Mỗi testcase gồm 2 file:
    inputXX.txt   — chuỗi các lần nhập (1 giá trị/dòng)
    outputXX.txt  — kết quả mong đợi (stdout của game)

  Format file input:
  -------------------
  Mỗi dòng tương ứng 1 lần đọc stdin của game, theo thứ tự:

    Dòng 1: size bàn (VD: 3)
    Dòng 2: goal     (VD: 3)
    Dòng 3: end rule (VD: 1)    ← 1=NONE, 2=OPEN_ONE, 3=OPEN_TWO
    Dòng 4: game mode (VD: 2)   ← 1=PVP, 2=PVE, 3=EVE
    Dòng 5: bot level (VD: 1)   ← 1=EASY, 2=MEDIUM, 3=HARD
    Dòng 6+: các nước đi "row col" (VD: 0 0)

  Ví dụ input01.txt (PVE, bàn 3x3, goal 3, NONE, EASY):
  -------------------------------------------------------
    3
    3
    1
    2
    1
    0 0
    1 1
    0 1
    2 2
    0 2

  Lưu ý:
    - Trong PVP: cả 2 người dùng đều nhập luân phiên
    - Trong PVE: người dùng nhập, bot tự tính (không cần dòng bot)
    - Trong EVE: cả 2 bot tự tính, không cần dòng nào ngoài config
    - Nếu input hết trước khi game xong → game trả Draw, exit 0


================================================================
7. OUTPUT FILES
================================================================

  Sau mỗi lần chạy game tạo ra 2 file:

  results.txt:
    - Toàn bộ output của game (menu, bàn cờ, kết quả)
    - Dùng để debug, xem lại nước đi
    - Bị ghi đè mỗi lần chạy mới

  log.txt:
    - Log nội bộ theo timestamp
    - Ghi thời gian thực thi botMove()
    - Ghi sự kiện khởi tạo, kết quả game
    - Bị ghi đè mỗi lần chạy mới
    - Có thể đổi tên bằng: -l <tên_khác>

  Trong judge mode (-j):
    - stdout của game = nội dung kết quả (grader đọc từ đây)
    - Không in màu ANSI, không in log ra terminal

Chú ý : Các file trong tệp log và results là mẫu của vài lần chạy game

================================================================
8. CÁC TÍNH NĂNG GAME
================================================================

  Game mode:
    PVP  — 2 người chơi luân phiên
    PVE  — 1 người vs 1 bot
    EVE  — bot vs bot (dùng để benchmark)

  Độ khó bot:
    EASY   — đánh ngẫu nhiên (random_pick)
    MEDIUM — heuristic đơn (evaluatePosition, attack×12 + defend×10)
    HARD   — Minimax + Alpha-Beta Pruning + Transposition Table (Zobrist)
              depth: 8 (size≤5), 6 (size≤8), 4 (size>8)
              top-12 nước candidate, move ordering theo score

  End Rule:
    NONE      — thắng khi đủ goal quân liên tiếp (bất kể đầu hở)
    OPEN_ONE  — cần ít nhất 1 đầu không bị chặn
    OPEN_TWO  — cần 2 đầu không bị chặn (Gomoku tiêu chuẩn)

  Special moves (Human):
    99 99   — Undo (xóa 2 nước gần nhất, giới hạn 1 lần/lượt,
              xóa luôn Transposition Table)
    100 100 — Surrender (thua ngay, đối phương thắng)

  Timeout:
    PVE (EASY):   15 giây/lượt
    PVE (MEDIUM): 10 giây/lượt
    PVE (HARD):    5 giây/lượt
    PVP:          10 giây/lượt → quá giờ đánh random


