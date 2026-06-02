/* -------------------- [IMPORTING] -------------------- */
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <unordered_map>
#include <cstdint>

/* ------------------------------------------------------------ */
/* -------------------- [GLOBAL VARIABLES] -------------------- */
/* ------------------------------------------------------------ */


std::ofstream output_file;
bool is_mid = true;
bool undo_used_this_turn = false;

const std::string VERSION = "0.4.25022808";

const bool ALGORITHM_FLAG = true; 
const bool TIME_ENABLED   = true;
const int  RANDOM_SEED    = 2013;

const int  BOARD_N_MAX    = 12;
const int  SLEEP_TIME     = 1500;
constexpr int DRAW_RESULT = -1;

std::mt19937 generator(RANDOM_SEED);


enum class BotLevel { EASY, MEDIUM, HARD, INVALID_LV };
enum class GameMode  { PVP, PVE, EVE, INVALID_MODE };




enum class SelectType {
    TITLE_UI, SIZE_UI, GOAL_UI, GAME_MODE_UI,
    BOT_LEVEL_UI, PLAYER_UI, MUL_BOT_LEVEL_UI, END_RULE_UI, INVALID_UI
}; 

enum class EndRule { NONE, OPEN_ONE, OPEN_TWO };

typedef std::pair<int, int> pII;

struct MoveHistory {
    int row;
    int col;
    char symbol;
    int player;
};
std::vector<MoveHistory> move_history;



struct RunConfig {
    bool        interactive  = true;
    bool        judge_mode   = false;  
    std::string input_file;
    bool        to_file      = true;
    std::string log_file     = "log.txt";

    bool        nonInteractive = false; 
    bool        loggingEnabled = true;
    std::string results_file = "results.txt";
    int         timeLimitSecs = 10;
};

struct GameSetup {
    std::vector<std::vector<char>> board;
    int      size;
    int      goal;
    GameMode mode;
    BotLevel levels[2];

    int      startingPlayer = 0; 
    char     symbols[2] = {'X', 'O'};

    EndRule  endRule = EndRule::OPEN_TWO;
};


struct GameResult {
    int  winner = 0;
    bool isBot  = false; 
    int  turns  = 0;
};


const BotLevel HUMAN_LEVEL = BotLevel::INVALID_LV;
const GameMode PVE_MODE    = GameMode::PVE;
const GameMode EVE_MODE    = GameMode::EVE;



namespace Zobrist {
    std::vector<std::vector<uint64_t>> table;
    uint64_t sideToMove;
    std::unordered_map<uint64_t, int> transpositionTable;
    
    void init(int size) {
        std::mt19937_64 rng(RANDOM_SEED);
        table.assign(size, std::vector<uint64_t>(size, 0));
        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++)
                table[i][j] = rng();
        sideToMove = rng();
    }
    
    uint64_t hash(const std::vector<std::vector<char>>& board, int currentPlayer) {
        uint64_t h = 0;
        int size = (int)board.size();
        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++)
                if (board[i][j] != '-')
                    h ^= table[i][j] * (board[i][j] == 'X' ? 1ULL : 2ULL);
        if (currentPlayer == 1) h ^= sideToMove;
        return h;
    }
}

/* -------------------------------------------------------- */
/* -------------------- [DECLARATIONS] -------------------- */
/* -------------------------------------------------------- */


// ── GameLogger ────────────────────────────────────────────
namespace GameLogger {

enum class Level { DEBUG, INFO, WARNING, ERROR, MSG };  



std::string levelToString(Level level);
std::string getColor(Level level);

inline static Level min_level = Level::DEBUG;

const std::string RESET  = "\033[0m";
const std::string RED    = "\033[31m"; 
const std::string GREEN  = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE   = "\033[34m";
const std::string CYAN   = "\033[36m";

inline static std::ofstream log_file;
inline static bool write_to_file  = false;
inline static bool is_judge_mode  = false;


void init(bool judge_mode, bool to_file = true, const std::string& path = "log.txt");
void log(const std::string& msg, Level level = Level::INFO);
void close();


void logGameResult(int winner, int turns, bool isBot);
}  

std::string GameLogger::levelToString(Level level) {
    switch (level) {
        case Level::DEBUG:   return "DEBUG";
        case Level::INFO:    return "INFO";
        case Level::WARNING: return "WARN";
        case Level::ERROR:   return "ERROR";
        case Level::MSG:     return "";
        default:             return "UNKNOWN";
    }
}

std::string GameLogger::getColor(Level level) {
    switch (level) {
        case Level::DEBUG:   return BLUE;
        case Level::INFO:    return GREEN;
        case Level::WARNING: return YELLOW;
        case Level::ERROR:   return RED;
        default:             return RESET;
    }
}

RunConfig  parseArgs(int argc, char* argv[]);



// ── GameInteraction ───────────────────────────────────────


namespace GameInteraction {
    inline static std::ifstream global_file_in;
}

std::streambuf* initInteraction(const RunConfig& config);

void closeInteraction(std::streambuf* cin_backup);
bool validateInput(const std::string& input);

bool getInput(int* val);
bool selectSize(int* size);
bool selectGoal(int* goal, const int size);
bool selectGameMode(GameMode* mode);
bool selectBotLevel(BotLevel* levels, const int index);
bool getPlayerMove(int* row, int* col);


bool selectLevel(BotLevel* level);

bool selectEndRule(EndRule* rule);
bool selectPlayerSymbol(GameSetup* gameSetup);



// ── Renderer ──────────────────────────────────────────────
void clearScreen();


void showSelectMenu(SelectType selectType, const std::string& version);
void displayBoard(const std::vector<std::vector<char>>& board, const int size, const GameSetup* gameSetup = nullptr);
void showMove(const int row, const int col, bool beforeBoard = false);
void showInvalidMove();
void showPlayer(const int player, const bool is_bot);
void showResult(const int winner, const bool is_bot);
void printResult(const GameResult& gameResult);


void printSymbol(const char symbol);
void showCountdown(int secondsLeft);
void printGameInfoRightSide(const GameSetup* gameSetup, int termWidth, int boardWidth);
bool undoLastMove(std::vector<std::vector<char>>& board, int& currentPlayer, int& turns, char symbols[2]);
bool surrender(int surrenderingPlayer, int& winner, bool isBot);




void       startGame(const RunConfig& config, GameSetup& gameSetup);
GameResult playGame (const RunConfig& config, GameSetup& gameSetup);
void       endGame  (const RunConfig& config, GameSetup& gameSetup, GameResult& gameResult);


enum class HumanMoveResult { OK, UNDO, SURRENDER, SKIP, END_INPUT };
static HumanMoveResult getHumanMove(
        const RunConfig& config, GameSetup& gameSetup,
        int currentPlayer, pII* move);



void initBoard(std::vector<std::vector<char>>& board, const int size);
bool isValidMove(const std::vector<std::vector<char>>& board, const int size, const int row, const int col);
void makeMove(std::vector<std::vector<char>>& board, const int row, const int col, const char symbol);
bool isEmptyHead(const std::vector<std::vector<char>>& board, int size, int x, int y, const char symbol);


bool checkWin(const std::vector<std::vector<char>>& board, const int size, const char symbol,
              const int goal, EndRule rule = EndRule::OPEN_TWO, int lastRow = -1, int lastCol = -1);
bool checkDraw(const std::vector<std::vector<char>>& board, const int size);


std::vector<pII> findWinningLine(const std::vector<std::vector<char>>& board, const int size, 
                                  const char symbol, const int goal,
                                  EndRule rule = EndRule::OPEN_TWO);



// ── Bot ───────────────────────────────────────────────────
pII botMove(std::vector<std::vector<char>>& board, const int size, const int goal,
            const char symbol, const BotLevel level, EndRule endRule = EndRule::OPEN_TWO);
pII random_pick(std::vector<std::vector<char>>& board, const int size);
pII simple_heuristic(std::vector<std::vector<char>>& board, const int size, const int goal,
                     const char botSymbol, const char playerSymbol);
pII hard_level(std::vector<std::vector<char>>& board, const int size, const int goal,
               const char botSymbol, const char playerSymbol, EndRule endRule = EndRule::OPEN_TWO);

static std::vector<pII> getCandidateMoves(std::vector<std::vector<char>>& board, int size);



// ── Helper template ───────────────────────────────────────
template <typename Function>
auto measureExecutionTime(const std::string& label, Function func, bool enabled)
    -> std::invoke_result_t<Function>; 

int getPVETimeLimit(BotLevel level);



/* ------------------------------------------------------- */
/* -------------------- [DEFINITIONS] -------------------- */
/* ------------------------------------------------------- */

/* ============================================================
 * MODULE 1: LOGGER (Given — không sửa)
 * ============================================================ */

void GameLogger::init(bool judge_mode, bool to_file, const std::string& path) {
    write_to_file  = to_file;
    is_judge_mode  = judge_mode;

    if (write_to_file) {
        log_file.open(path, std::ios::out | std::ios::trunc);
        if (!log_file.is_open()) {
            std::cerr << "[Logger] Cannot open log file: " << path
                      << ". Falling back to console only." << std::endl;
            write_to_file = false;
        }
    }

    std::string header = "Tic-tac-toe Game (Version: " + std::string(VERSION) + ")\n";
    header += std::string(48, '-');

    if (write_to_file) log_file << header << std::endl;
    if (!is_judge_mode) std::cout << header << std::endl;
}

void GameLogger::log(const std::string& msg, Level level) {
    if (static_cast<int>(level) < static_cast<int>(min_level)) return;

    auto now = std::time(nullptr);
    auto tm  = *std::localtime(&now);

    std::stringstream ss_lv;
    std::string formatted_lv;
    if (level != Level::MSG) {
        ss_lv << "[" << levelToString(level) << "]";
        formatted_lv = ss_lv.str();
    }

    std::stringstream ss_msg;
    ss_msg << (formatted_lv.empty() ? "" : " - ")
           << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
           << msg;
    std::string formatted_msg = ss_msg.str();

    if (write_to_file) {
        log_file << formatted_lv << formatted_msg << std::endl;
        log_file.flush();
    }
    if (!is_judge_mode) {
        std::cout << getColor(level) << formatted_lv
                  << getColor(Level::MSG) << formatted_msg << RESET << std::endl;
    }
}



void GameLogger::logGameResult(int winner, int turns, bool isBot) {
    std::stringstream ss;
    ss << "Game over — ";
    if (winner == 0) ss << "Draw";
    else if (isBot)  ss << "Bot wins";
    else             ss << "Player " << winner << " wins";
    ss << " | Turns: " << turns;
    log(ss.str(), Level::INFO);
}



void GameLogger::close() {
    if (log_file.is_open()) log_file.close();
}


/* ============================================================
 * MODULE 2: ARGUMENT PARSER (Given — không sửa)
 * ============================================================ */

RunConfig parseArgs(int argc, char* argv[]) {
    RunConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-j" || arg == "--judge") {
            config.judge_mode  = true;
            config.interactive = false; 
            config.nonInteractive = true;  
        } 
        else if ((arg == "-i" || arg == "--input") && (i + 1 < argc)) {
            config.input_file = argv[++i];
        } 
        else if ((arg == "-l" || arg == "--log") && (i + 1 < argc)) {
            config.log_file = argv[++i];
            config.to_file = !config.log_file.empty();
        } 
        else if (arg == "-h" || arg == "--help") {
            std::cout << "Tic-tac-toe Game (Version: " << VERSION << " by baluong.87)\n"
                      << "Usage:\n"
                      << "  -j, --judge   Enable judge mode (no UI)\n"
                      << "  -i, --input   Path to input file\n"
                      << "  -l, --log     Path to log file (default: log.txt)\n";
            exit(0);
        }
    }
    return config;
}

/* ============================================================
 * MODULE 3: INTERACTION
 * ============================================================ */

std::streambuf* initInteraction(const RunConfig& config) {
    std::streambuf* cin_backup = nullptr;
    if (!config.interactive && !config.input_file.empty()) {
        GameInteraction::global_file_in.open(config.input_file);
        if (GameInteraction::global_file_in.is_open()) {
            cin_backup = std::cin.rdbuf();
            std::cin.rdbuf(GameInteraction::global_file_in.rdbuf());
            std::stringstream ss;
            ss << "redirected cin to: " << config.input_file;
            GameLogger::log(ss.str());
        } else {
            GameLogger::log("failed to open input file, using console.", GameLogger::Level::ERROR);
        }
    }
    return cin_backup;
}



void closeInteraction(std::streambuf* cin_backup) {
    if (cin_backup) {
        std::cin.rdbuf(cin_backup);
        GameLogger::log("fallback using std::cin input stream.");
    }
    if (GameInteraction::global_file_in.is_open())
        GameInteraction::global_file_in.close();
}



bool validateInput(const std::string& input) {
    if (input.empty()) return false;
    for (char c : input)
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

bool getInput(int* val) {
    if (val == nullptr) return false;
    std::string input;
    
    if (!std::getline(std::cin, input)) return false;
    
    input.erase(0, input.find_first_not_of(" \t\n\r\f\v"));
    input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1);
    
    if (!validateInput(input)) {
        std::cout << "Invalid input! Please enter a valid integer.\n";
        if (output_file.is_open()) output_file << "Invalid input! Please enter a valid integer.\n";
        return false;
    }
    try {
        *val = std::stoi(input);
    } catch (...) {
        std::cout << "Invalid input! Please enter a valid integer.\n";
        if (output_file.is_open()) output_file << "Invalid input! Please enter a valid integer.\n";
        return false;
    }
    return true;
}

bool parseSpecialMove(const std::string& input, int& row, int& col) {
    std::stringstream ss(input);
    std::string first, second;
    ss >> first >> second;
    
    if (first == "99" && second == "99") {
        row = 99;
        col = 99;
        return true;
    }
    if (first == "100" && second == "100") {
        row = 100;
        col = 100;
        return true;
    }
    return false;
}

bool getPlayerMove(int* row, int* col) {
    if (row == nullptr || col == nullptr) return false;
    std::string input;
    if (!std::getline(std::cin, input)) return false;
    
    int specialRow, specialCol;
    if (parseSpecialMove(input, specialRow, specialCol)) {
        *row = specialRow;
        *col = specialCol;
        return true;
    }
    
    std::stringstream ss(input);
    std::string first, second;
    ss >> first >> second;
    
    if (!validateInput(first) || !validateInput(second)) {
        return false;
    }
    
    try {
        *row = std::stoi(first);
        *col = std::stoi(second);
    } catch (...) {
        return false;
    }
    return true;
} 


bool selectEndRule(EndRule* rule) {
    if (rule == nullptr) return false;
    int choice;
    if (!getInput(&choice)) return false;
    switch (choice) {
        case 1: *rule = EndRule::NONE;      break;
        case 2: *rule = EndRule::OPEN_ONE;  break;
        case 3: *rule = EndRule::OPEN_TWO;  break;
        default:
            std::cout << "Invalid choice. Please try again.\n";
            if (output_file.is_open()) output_file << "Invalid choice. Please try again.\n";
            return false;
    }
    return true;
}

bool selectPlayerSymbol(GameSetup* gameSetup) {
    if (gameSetup == nullptr) return false;
    
    std::cout << "\nChoose your symbol:\n"
              << "  (1) X (goes first)\n"
              << "  (2) O (goes second)\n"
              << "Choice: \n";
    if (output_file.is_open()) output_file << "\nChoose your symbol:\n"
              << "  (1) X (goes first)\n"
              << "  (2) O (goes second)\n"
              << "Choice: \n";
    int choice;
    if (!getInput(&choice)) return false;
    
    if (choice == 1) {
        gameSetup->symbols[0] = 'X';
        gameSetup->symbols[1] = 'O';
        gameSetup->startingPlayer = 0;
    } else if (choice == 2) {
        gameSetup->symbols[0] = 'O';
        gameSetup->symbols[1] = 'X';
        gameSetup->startingPlayer = 1;
    } else {
        std::cout << "Invalid choice. Please try again.\n";
        if (output_file.is_open()) output_file << "Invalid choice. Please try again.\n";
        return false;
    }
    
    std::cout << "Player 1 symbol: " << gameSetup->symbols[0] 
              << " | Player 2 symbol: " << gameSetup->symbols[1] 
              << " | Starting: Player " << (gameSetup->startingPlayer + 1) << "\n\n";
    if (output_file.is_open()) output_file << "Player 1 symbol: " << gameSetup->symbols[0] 
              << " | Player 2 symbol: " << gameSetup->symbols[1] 
              << " | Starting: Player " << (gameSetup->startingPlayer + 1) << "\n\n";
    
    return true;
}

bool selectSize(int* size) {
    if (size == nullptr) return false;
    int temp;  
    if (!getInput(&temp)) return false;
    if (temp < 3 || temp > BOARD_N_MAX) {
        std::cout << "Size must be in range [3, " << BOARD_N_MAX << "]. Please try again.\n";
        if (output_file.is_open()) output_file << "Size must be in range [3, " << BOARD_N_MAX << "]. Please try again.\n";
        return false;
    }
    *size = temp;
    return true;
}

bool selectGoal(int* goal, const int size) {
    if (goal == nullptr) return false;
    int temp;
    if (!getInput(&temp)) return false;
    if (temp < 3 || temp > size) {
        std::cout << "Goal must be in range [3, " << size << "]. Please try again.\n";
        if (output_file.is_open()) output_file << "Goal must be in range [3, " << size << "]. Please try again.\n";
        return false;
    }
    *goal = temp;
    return true;
}

bool selectGameMode(GameMode* mode) {
    if (mode == nullptr) return false;
    int choice;
    if (!getInput(&choice)) return false;
    switch (choice) {
        case 1: *mode = GameMode::PVP; break;
        case 2: *mode = GameMode::PVE; break;
        case 3: *mode = GameMode::EVE; break;
        default:
            std::cout << "Invalid choice. Please try again.\n";
            if (output_file.is_open()) output_file << "Invalid choice. Please try again.\n";
            return false;
    }
    return true;
}

bool selectBotLevel(BotLevel* levels, const int index) {
    if (levels == nullptr) return false;
    int choice;
    if (!getInput(&choice)) return false;
    switch (choice) {
        case 1: levels[index] = BotLevel::EASY;   break;
        case 2: levels[index] = BotLevel::MEDIUM; break;
        case 3: levels[index] = BotLevel::HARD;   break;
        default:
            std::cout << "Invalid choice. Please try again.\n";
            if (output_file.is_open()) output_file << "Invalid choice. Please try again.\n";
            return false;
    }
    return true;
}

bool selectLevel(BotLevel* level) {
    if (level == nullptr) return false;
    BotLevel arr[1];
    if (!selectBotLevel(arr, 0)) return false;
    *level = arr[0];
    return true;
}

int getPVETimeLimit(BotLevel level) {
    switch (level) {
        case BotLevel::EASY:   return 15;
        case BotLevel::MEDIUM: return 10;
        case BotLevel::HARD:   return 5;
        default:               return 10;
    }
}

/* ============================================================
 * MODULE 4: RENDERER
 * ============================================================ */
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\x1B[2J\x1B[H" << std::flush;
#endif
}


void showSelectMenu(SelectType selectType,const std::string& version) {
    switch (selectType) {
        case SelectType::TITLE_UI:
            std::cout << ">----- Tic-tac-toe [Console v" << version << "] -----<\n\n";
            if (output_file.is_open()) output_file << ">----- Tic-tac-toe [Console v" << version << "] -----<\n\n";
            break;

        case SelectType::SIZE_UI:
            std::cout << "Enter board size NxN (3 <= N <= " << BOARD_N_MAX << "): ";
            if (output_file.is_open()) output_file << "Enter board size NxN (3 <= N <= " << BOARD_N_MAX << "): ";
            break;

        case SelectType::GOAL_UI:
            std::cout << "Enter consecutive pieces to win (3 <= goal <= size): ";
            if (output_file.is_open()) output_file << "Enter consecutive pieces to win (3 <= goal <= size): ";
            break;

        case SelectType::GAME_MODE_UI:
            std::cout << "Select game mode:\n"
                      << "  (1) PvP  - Player vs Player\n"
                      << "  (2) PvE  - Player vs Bot\n"
                      << "  (3) EvE  - Bot vs Bot\n"
                      << "Choice: \n";
            if (output_file.is_open()) output_file << "Select game mode:\n"
                      << "  (1) PvP  - Player vs Player\n"
                      << "  (2) PvE  - Player vs Bot\n"
                      << "  (3) EvE  - Bot vs Bot\n"
                      << "Choice: \n";
            break;

        case SelectType::BOT_LEVEL_UI:
            std::cout << "Select bot difficulty:\n"
                      << "  (1) EASY\n"
                      << "  (2) MEDIUM\n"
                      << "  (3) HARD\n"
                      << "Choice: \n";  
            if (output_file.is_open()) output_file << "Select bot difficulty:\n"
                      << "  (1) EASY\n"
                      << "  (2) MEDIUM\n"
                      << "  (3) HARD\n"
                      << "Choice: \n";
            break;

        case SelectType::PLAYER_UI:
            std::cout << "Enter move (row col) - '99 99' to undo, '100 100' to surrender: ";
            if (output_file.is_open()) output_file << "Enter move (row col) - '99 99' to undo, '100 100' to surrender: ";
            break;

        case SelectType::MUL_BOT_LEVEL_UI:
            std::cout << "Select difficulty:\n"
                      << "  (1) EASY  (2) MEDIUM  (3) HARD\n"
                      << "Choice: \n";
            if (output_file.is_open()) output_file << "Select difficulty:\n"
                      << "  (1) EASY  (2) MEDIUM  (3) HARD\n"
                      << "Choice: \n";
            break;

        case SelectType::END_RULE_UI:
            std::cout << "Select end rule:\n"
                      << "  (1) NONE - Any " << BOARD_N_MAX << " in a row\n"
                      << "  (2) OPEN_ONE - Requires at least one end open\n"
                      << "  (3) OPEN_TWO - Requires both ends open (Gomoku standard)\n"
                      << "Choice: \n";
            if (output_file.is_open()) output_file << "Select end rule:\n"
                      << "  (1) NONE - Any " << BOARD_N_MAX << " in a row\n"
                      << "  (2) OPEN_ONE - Requires at least one end open\n"
                      << "  (3) OPEN_TWO - Requires both ends open (Gomoku standard)\n"
                      << "Choice: \n";
            break;

        default:
            break;
    }
}


void printGameInfoRightSide(const GameSetup* gameSetup, int termWidth, int boardWidth) {
    if (!gameSetup) return;
    
    int infoStartCol = boardWidth + 4;
    if (!is_mid) infoStartCol = 0;
    std::string infoPad(infoStartCol, ' ');
    
    std::vector<std::string> infoLines;
    infoLines.push_back("Board size : " + std::to_string(gameSetup->size));
    infoLines.push_back("Goal : " + std::to_string(gameSetup->goal));
    
    std::string endRuleStr;
    switch (gameSetup->endRule) {
        case EndRule::NONE: endRuleStr = "NONE"; break;
        case EndRule::OPEN_ONE: endRuleStr = "OPEN_ONE"; break;
        case EndRule::OPEN_TWO: endRuleStr = "OPEN_TWO"; break;
    }
    infoLines.push_back("End Rule : " + endRuleStr);
    
    std::string modeStr;
    switch (gameSetup->mode) {
        case GameMode::PVP: modeStr = "PvP"; break;
        case GameMode::PVE: modeStr = "PvE"; break;
        case GameMode::EVE: modeStr = "EvE"; break;
        default: modeStr = "Unknown"; break;
    }
    infoLines.push_back("Game Mode : " + modeStr);
    
    if (gameSetup->mode == GameMode::PVE) {
        std::string botLevelStr;
        switch (gameSetup->levels[1]) {
            case BotLevel::EASY: botLevelStr = "EASY"; break;
            case BotLevel::MEDIUM: botLevelStr = "MEDIUM"; break;
            case BotLevel::HARD: botLevelStr = "HARD"; break;
            default: botLevelStr = "UNKNOWN"; break;
        }
        infoLines.push_back("Bot : " + botLevelStr);
    } else if (gameSetup->mode == GameMode::EVE) {
        std::string bot1LevelStr, bot2LevelStr;
        switch (gameSetup->levels[0]) {
            case BotLevel::EASY: bot1LevelStr = "EASY"; break;
            case BotLevel::MEDIUM: bot1LevelStr = "MEDIUM"; break;
            case BotLevel::HARD: bot1LevelStr = "HARD"; break;
            default: bot1LevelStr = "UNKNOWN"; break;
        }
        switch (gameSetup->levels[1]) {
            case BotLevel::EASY: bot2LevelStr = "EASY"; break;
            case BotLevel::MEDIUM: bot2LevelStr = "MEDIUM"; break;
            case BotLevel::HARD: bot2LevelStr = "HARD"; break;
            default: bot2LevelStr = "UNKNOWN"; break;
        }
        infoLines.push_back("Bot 1 : " + bot1LevelStr);
        infoLines.push_back("Bot 2 : " + bot2LevelStr);
    }
    
    for (const auto& line : infoLines) {
        std::cout << infoPad << line << "\n";
        if (output_file.is_open()) output_file << infoPad << line << "\n";
    }
}


void displayBoard(const std::vector<std::vector<char>>& board, const int size, const GameSetup* gameSetup) {
    int boardWidth = 4 + size * 3;
    int termWidth  = 80;
    int indent     = is_mid ? std::max(0, (termWidth - boardWidth) / 2) : 0;
    std::string pad(indent, ' ');

    std::cout << pad << "    ";
    if (output_file.is_open()) output_file << pad << "    ";
    for (int col = 0; col < size; col++) {
        std::cout << std::setw(3) << col;
        if (output_file.is_open()) output_file << std::setw(3) << col;
    }
    std::cout << "\n" << pad << "    ";
    if (output_file.is_open()) output_file << "\n" << pad << "    ";
    for (int col = 0; col < size; col++) {
        std::cout << "---";
        if (output_file.is_open()) output_file << "---";
    }
    std::cout << "\n";
    if (output_file.is_open()) output_file << "\n";

    for (int row = 0; row < size; row++) {
        std::cout << pad << std::setw(2) << row << " | ";
        if (output_file.is_open()) output_file << pad << std::setw(2) << row << " | ";
        for (int col = 0; col < size; col++) {
            std::cout << "  ";
            if (output_file.is_open()) output_file << "  ";
            printSymbol(board[row][col]);
            if (output_file.is_open()) output_file << board[row][col];
        }
        std::cout << "\n";
        if (output_file.is_open()) output_file << "\n";
    }
    std::cout << "\n";
    if (output_file.is_open()) output_file << "\n";
    
    if (gameSetup != nullptr) {
        printGameInfoRightSide(gameSetup, termWidth, boardWidth);
    }
}


void showMove(const int row, const int col, bool beforeBoard) {
    if (beforeBoard) {
        std::cout << "Move placed at (" << row << ", " << col << ")\n";
        if (output_file.is_open()) output_file << "Move placed at (" << row << ", " << col << ")\n";
    }
}


void showInvalidMove() {
    std::cout << "Invalid move! Please try again.\n";
    if (output_file.is_open()) output_file << "Invalid move! Please try again.\n";
}

void showPlayer(const int player, const bool is_bot) {
    if (is_bot) {
        std::cout << "Bot (Player " << player << ") is thinking...\n";
        if (output_file.is_open()) output_file << "Bot (Player " << player << ") is thinking...\n";
    } else {
        std::cout << "Player " << player << " turn\n";
        if (output_file.is_open()) output_file << "Player " << player << " turn\n";
    }
}

void showResult(const int winner, const bool is_bot) {
    if (winner == 0) {
        std::cout << "Draw!\n";
        if (output_file.is_open()) output_file << "Draw!\n";
    } else if (is_bot) {
        std::cout << "Bot (Player " << winner << ") wins!\n";
        if (output_file.is_open()) output_file << "Bot (Player " << winner << ") wins!\n";
    } else {
        std::cout << "Player " << winner << " wins!\n";
        if (output_file.is_open()) output_file << "Player " << winner << " wins!\n";
    }
}

void printResult(const GameResult& gameResult) {
    std::cout << gameResult.winner << " " << gameResult.turns << "\n";
    if (output_file.is_open()) output_file << gameResult.winner << " " << gameResult.turns << "\n";
}

void printSymbol(const char symbol) {
    if (symbol == 'X')
        std::cout << GameLogger::CYAN << symbol << GameLogger::RESET;
    else if (symbol == 'O')
        std::cout << GameLogger::YELLOW << symbol << GameLogger::RESET;
    else
        std::cout << symbol;
}

void showCountdown(int secondsLeft) {
    std::cout << "\rTime left: " << std::setw(2) << secondsLeft << "s  " << std::flush;
    // Không ghi vào output_file vì \r không có ý nghĩa trong file
}

void displayBoardWithWinLine(const std::vector<std::vector<char>>& board, const int size, 
                              const std::vector<pII>& winLine, const GameSetup* gameSetup) {
    int boardWidth = 4 + size * 3;
    int termWidth  = 80;
    int indent     = is_mid ? std::max(0, (termWidth - boardWidth) / 2) : 0;
    std::string pad(indent, ' ');

    std::cout << pad << "    ";
    if (output_file.is_open()) output_file << pad << "    ";
    for (int col = 0; col < size; col++) {
        std::cout << std::setw(3) << col;
        if (output_file.is_open()) output_file << std::setw(3) << col;
    }
    std::cout << "\n" << pad << "    ";
    if (output_file.is_open()) output_file << "\n" << pad << "    ";
    for (int col = 0; col < size; col++) {
        std::cout << "---";
        if (output_file.is_open()) output_file << "---";
    }
    std::cout << "\n";
    if (output_file.is_open()) output_file << "\n";

    for (int row = 0; row < size; row++) {
        std::cout << pad << std::setw(2) << row << " | ";
        if (output_file.is_open()) output_file << pad << std::setw(2) << row << " | ";
        for (int col = 0; col < size; col++) {
            bool isWinPos = false;
            for (const auto& pos : winLine) {
                if (pos.first == row && pos.second == col) {
                    isWinPos = true;
                    break;
                }
            }
            
            if (isWinPos) {
                std::cout << GameLogger::RED << std::setw(3) << board[row][col] << GameLogger::RESET;
                if (output_file.is_open()) output_file << std::setw(3) << board[row][col];
            } else {
                std::cout << "  ";
                if (output_file.is_open()) output_file << "  ";
                printSymbol(board[row][col]);
                if (output_file.is_open()) output_file << board[row][col];
            }
        }
        std::cout << "\n";
        if (output_file.is_open()) output_file << "\n";
    }
    std::cout << "\n";
    if (output_file.is_open()) output_file << "\n";
  
    if (gameSetup != nullptr) {
        printGameInfoRightSide(gameSetup, termWidth, boardWidth);
    }
}

bool undoLastMove(std::vector<std::vector<char>>& board, int& currentPlayer, int& turns, char symbols[2]) {
    if (move_history.empty()) {
        std::cout << "No moves to undo!\n";
        if (output_file.is_open()) output_file << "No moves to undo!\n";
        return false;
    }
    
    int movesToUndo = std::min(2, (int)move_history.size());
    
    for (int i = 0; i < movesToUndo; i++) {
        MoveHistory last = move_history.back();
        board[last.row][last.col] = '-';
        move_history.pop_back();
        turns--;
    }

    Zobrist::transpositionTable.clear();

    if (movesToUndo == 2) {
        currentPlayer = move_history.empty() ? currentPlayer : (move_history.back().player == 1 ? 1 : 0);
    } else {
        currentPlayer = 1 - currentPlayer;
    }
    
    std::cout << "Undo successful! " << movesToUndo << " move(s) undone.\n";
    if (output_file.is_open()) output_file << "Undo successful! " << movesToUndo << " move(s) undone.\n";
    
    return true;
}

bool surrender(int surrenderingPlayer, int& winner, bool isBot) {
    winner = (surrenderingPlayer == 1) ? 2 : 1;
    std::cout << "Player " << surrenderingPlayer << " surrenders! Player " << winner << " wins.\n";
    if (output_file.is_open()) output_file << "Player " << surrenderingPlayer << " surrenders! Player " << winner << " wins.\n";
    return true;
}

/* ============================================================
 * MODULE 5: ENGINE
 * ============================================================ */

void startGame(const RunConfig& config, GameSetup& gameSetup) {
    if (config.interactive) clearScreen();
    if (config.interactive) showSelectMenu(SelectType::TITLE_UI, VERSION);
    do {
       if (config.interactive) showSelectMenu(SelectType::SIZE_UI, VERSION);
    } while (!selectSize(&gameSetup.size));

    Zobrist::init(gameSetup.size);

    do {
        if (config.interactive) showSelectMenu(SelectType::GOAL_UI, VERSION);
    } while (!selectGoal(&gameSetup.goal, gameSetup.size));

    do {
        if (config.interactive) showSelectMenu(SelectType::END_RULE_UI, VERSION);
    } while (!selectEndRule(&gameSetup.endRule));

    do {
        if (config.interactive) showSelectMenu(SelectType::GAME_MODE_UI, VERSION);
    } while (!selectGameMode(&gameSetup.mode));

    if (config.interactive && gameSetup.mode != GameMode::EVE) {
        do {
            if (!selectPlayerSymbol(&gameSetup)) {
                std::cout << "Please try again.\n";
                if (output_file.is_open()) output_file << "Please try again.\n";
            } else {
                break;
            }
        } while (true);
    }

    if (gameSetup.mode == PVE_MODE) {
        gameSetup.levels[0] = HUMAN_LEVEL;
        do {
            showSelectMenu(SelectType::BOT_LEVEL_UI, VERSION);
        } while (!selectLevel(&gameSetup.levels[1]));
    } else if (gameSetup.mode == EVE_MODE) {
        std::cout << "Select difficulty for Bot 1:\n";
        if (output_file.is_open()) output_file << "Select difficulty for Bot 1:\n";
        do {
            showSelectMenu(SelectType::BOT_LEVEL_UI, VERSION);
        } while (!selectLevel(&gameSetup.levels[0]));
        std::cout << "Select difficulty for Bot 2:\n";
        if (output_file.is_open()) output_file << "Select difficulty for Bot 2:\n";
        do {
            showSelectMenu(SelectType::BOT_LEVEL_UI, VERSION);
        } while (!selectLevel(&gameSetup.levels[1]));
    } else {
        gameSetup.levels[0] = HUMAN_LEVEL;
        gameSetup.levels[1] = HUMAN_LEVEL;
    }

    initBoard(gameSetup.board, gameSetup.size);
    Zobrist::transpositionTable.clear();
    GameLogger::log("startGame: board initialized, size=" + std::to_string(gameSetup.size)
                    + " goal=" + std::to_string(gameSetup.goal));
}

int getBotSleepTime(BotLevel level) {
    switch (level) {
        case BotLevel::EASY:   return 2000;  
        case BotLevel::MEDIUM: return 3000; 
        case BotLevel::HARD:   return 4000;  
        default:               return 2000;
    }
}

static HumanMoveResult getHumanMove(
        const RunConfig& config, GameSetup& gameSetup,
        int currentPlayer, pII* move) {

    int row = -1, col = -1;
    bool timedOut = false;

    int currentTimeLimit = config.timeLimitSecs;
    if (gameSetup.mode == GameMode::PVE && gameSetup.levels[1] != HUMAN_LEVEL) {
        currentTimeLimit = getPVETimeLimit(gameSetup.levels[1]);
    }

    if (config.interactive && currentTimeLimit > 0 && gameSetup.mode != GameMode::EVE) {
        auto deadline = std::chrono::steady_clock::now()
                        + std::chrono::seconds(currentTimeLimit);

        std::atomic<bool> inputDone{false};
        std::thread countdownThread([&]() {
            while (!inputDone.load()) {
                auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
                    deadline - std::chrono::steady_clock::now()).count();
                if (remaining < 0) remaining = 0;
                showCountdown(static_cast<int>(remaining));
                if (remaining == 0) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            std::cout << "\n";
        });

        bool inputOk = getPlayerMove(&row, &col);
        inputDone.store(true);
        countdownThread.join();

        if (!inputOk || std::chrono::steady_clock::now() > deadline) {
            if (gameSetup.mode == GameMode::PVP) {
                std::cout << "Time's up! Making random move...\n";
                if (output_file.is_open()) output_file << "Time's up! Making random move...\n";
                pII randomMove = random_pick(gameSetup.board, gameSetup.size);
                if (randomMove.first != -1 && randomMove.second != -1) {
                    *move = randomMove;
                    return HumanMoveResult::OK;
                }
            }
            timedOut = true;
        }
    } else {
        if (!getPlayerMove(&row, &col)) {
            if (!std::cin.good()) return HumanMoveResult::END_INPUT;
            showInvalidMove();
            return HumanMoveResult::SKIP;
        }
    }

    if (row == 99 && col == 99)  return HumanMoveResult::UNDO;
    if (row == 100 && col == 100) return HumanMoveResult::SURRENDER;

    if (timedOut) return HumanMoveResult::SKIP;

    if (!std::cin.good()) return HumanMoveResult::END_INPUT;

    *move = {row, col};
    return HumanMoveResult::OK;
}

GameResult playGame(const RunConfig& config, GameSetup& gameSetup) {
    GameResult result;
    int currentPlayer = gameSetup.startingPlayer;
    int turns = 0;
    int lastMoveRow = -1, lastMoveCol = -1;
    move_history.clear(); 

    while (true) {
        if (!config.nonInteractive) clearScreen();

        undo_used_this_turn = false;

        bool isBot = (gameSetup.levels[currentPlayer] != HUMAN_LEVEL);

        if (!config.nonInteractive && isBot) {
            showPlayer(currentPlayer + 1, true);
        }
        
        if (!config.nonInteractive)
            displayBoard(gameSetup.board, gameSetup.size, &gameSetup);

        if (!config.nonInteractive && !isBot) {
            showPlayer(currentPlayer + 1, false);
        }

        pII move;
        if (isBot) {
            move = measureExecutionTime(
                "botMove",
                [&]() {
                    return botMove(
                        gameSetup.board,
                        gameSetup.size,
                        gameSetup.goal,
                        gameSetup.symbols[currentPlayer],
                        gameSetup.levels[currentPlayer],
                        gameSetup.endRule
                    );
                },
                TIME_ENABLED
            );
            std::this_thread::sleep_for(std::chrono::milliseconds(getBotSleepTime(gameSetup.levels[currentPlayer])));

        } else {
            HumanMoveResult hmr = getHumanMove(config, gameSetup, currentPlayer, &move);

            if (hmr == HumanMoveResult::END_INPUT) {
                result.winner = 0; result.turns = turns; result.isBot = false;
                return result;
            }
            if (hmr == HumanMoveResult::SKIP) {
                continue;
            }
            if (hmr == HumanMoveResult::SURRENDER) {
                surrender(currentPlayer + 1, result.winner, false);
                result.turns = turns;
                result.isBot = false;
                return result;
            }
            if (hmr == HumanMoveResult::UNDO) {
                if (undo_used_this_turn) {
                    std::cout << "Undo already used this turn!\n";
                    if (output_file.is_open()) output_file << "Undo already used this turn!\n";
                } else {
                    if (undoLastMove(gameSetup.board, currentPlayer, turns, gameSetup.symbols)) {
                        lastMoveRow = -1;
                        lastMoveCol = -1;
                        undo_used_this_turn = true;
                    }
                }
                continue;
            }
        }

        if (!isValidMove(gameSetup.board, gameSetup.size, move.first, move.second)) {
            showInvalidMove();
            continue;
        }

        makeMove(gameSetup.board, move.first, move.second, gameSetup.symbols[currentPlayer]);
        
        MoveHistory hist;
        hist.row = move.first;
        hist.col = move.second;
        hist.symbol = gameSetup.symbols[currentPlayer];
        hist.player = currentPlayer + 1;
        move_history.push_back(hist);
        
        turns++;
        lastMoveRow = move.first;
        lastMoveCol = move.second;

        if (!config.nonInteractive) {
            showMove(move.first, move.second, true);
        }

        if (checkWin(gameSetup.board, gameSetup.size, gameSetup.symbols[currentPlayer],
                     gameSetup.goal, gameSetup.endRule, lastMoveRow, lastMoveCol)) {
            result.winner = currentPlayer + 1;
            result.turns  = turns;
            result.isBot  = isBot;
            return result;
        }

        if (checkDraw(gameSetup.board, gameSetup.size)) {
            result.winner = 0;
            result.turns  = turns;
            result.isBot  = false;
            return result;
        }

        currentPlayer = 1 - currentPlayer;
    }
}

void endGame(const RunConfig& config, GameSetup& gameSetup, GameResult& gameResult) {
    if (config.interactive) {
        clearScreen(); 
        if (gameResult.winner != 0) {
            std::vector<pII> winLine = findWinningLine(
                gameSetup.board, gameSetup.size, 
                gameSetup.symbols[gameResult.winner - 1],
                gameSetup.goal, gameSetup.endRule
            );
            displayBoardWithWinLine(gameSetup.board, gameSetup.size, winLine, &gameSetup);
        } else {
            displayBoard(gameSetup.board, gameSetup.size, &gameSetup);
        }
        showResult(gameResult.winner, gameResult.isBot);
    } else {
        printResult(gameResult);
    }

    if (config.loggingEnabled) {
        GameLogger::logGameResult(gameResult.winner, gameResult.turns, gameResult.isBot);
    }
}


/* ============================================================
 * MODULE 6: GAME LOGIC
 * ============================================================ */


void initBoard(std::vector<std::vector<char>>& board, const int size) {
    board.assign(size, std::vector<char>(size, '-'));
}


bool isValidMove(const std::vector<std::vector<char>>& board, const int size,
                 const int row, const int col) {
    if (row < 0 || row >= size || col < 0 || col >= size) return false;
    if (board[row][col] != '-') return false;
    return true;
}


void makeMove(std::vector<std::vector<char>>& board, const int row, const int col, const char symbol) {
    board[row][col] = symbol;
}


bool isEmptyHead(const std::vector<std::vector<char>>& board, int size, int x, int y, const char symbol) {
    if (x < 0 || x >= size || y < 0 || y >= size) return true;
    if (board[x][y] == '-')      return true;
    if (board[x][y] == symbol)   return true;
    return false;
}

static bool checkLineDirection(const std::vector<std::vector<char>>& board, int size, char symbol,
                                int goal, EndRule rule, int row, int col, int dr, int dc) {
    int count = 1;
    for (int step = 1; step < goal; step++) {
        int nr = row + step * dr, nc = col + step * dc;
        if (nr < 0 || nr >= size || nc < 0 || nc >= size) break;
        if (board[nr][nc] == symbol) count++;
        else break;
    }
    for (int step = 1; step < goal; step++) {
        int nr = row - step * dr, nc = col - step * dc;
        if (nr < 0 || nr >= size || nc < 0 || nc >= size) break;
        if (board[nr][nc] == symbol) count++;
        else break;
    }
    
    if (count < goal) return false;
    
    if (rule == EndRule::NONE) return true;
    
    int startRow = row, startCol = col;
    while (true) {
        int nr = startRow - dr, nc = startCol - dc;
        if (nr < 0 || nr >= size || nc < 0 || nc >= size) break;
        if (board[nr][nc] == symbol) {
            startRow = nr;
            startCol = nc;
        } else break;
    }
    
    int endRow = startRow, endCol = startCol;
    int length = 0;
    while (length < goal) {
        if (endRow < 0 || endRow >= size || endCol < 0 || endCol >= size) break;
        if (board[endRow][endCol] == symbol) {
            length++;
            if (length == goal) break;
            endRow += dr;
            endCol += dc;
        } else break;
    }
    
    int headR = startRow - dr, headC = startCol - dc;
    int tailR = endRow + dr, tailC = endCol + dc;
    
    bool headOpen = isEmptyHead(board, size, headR, headC, symbol);
    bool tailOpen = isEmptyHead(board, size, tailR, tailC, symbol);
    
    if (rule == EndRule::OPEN_ONE && (headOpen || tailOpen)) return true;
    if (rule == EndRule::OPEN_TWO && (headOpen && tailOpen)) return true;
    
    return false;
}

static bool checkLine(const std::vector<std::vector<char>>& board, int size, char symbol,
                      int goal, EndRule rule,
                      int startRow, int startCol, int dr, int dc) {
    for (int r = startRow, c = startCol, k = 0; k <= size - goal; r += dr, c += dc, k++) {
        bool ok = true;
        for (int step = 0; step < goal; step++) {
            int nr = r + step * dr, nc = c + step * dc;
            if (nr < 0 || nr >= size || nc < 0 || nc >= size) { ok = false; break; }
            if (board[nr][nc] != symbol) { ok = false; break; }
        }
        if (!ok) continue;

        if (rule == EndRule::NONE) return true;

        int tailR = r + goal * dr, tailC = c + goal * dc;
        int headR = r - dr,        headC = c - dc;

        bool headOpen = isEmptyHead(board, size, headR, headC, symbol);
        bool tailOpen = isEmptyHead(board, size, tailR, tailC, symbol);

        if (rule == EndRule::OPEN_ONE && (headOpen || tailOpen)) return true;
        if (rule == EndRule::OPEN_TWO && (headOpen && tailOpen)) return true;
    }
    return false;
}

bool checkWin(const std::vector<std::vector<char>>& board, const int size, const char symbol,
              const int goal, EndRule rule, int lastRow, int lastCol) {
    if (lastRow == -1 || lastCol == -1) {
        for (int r = 0; r < size; r++)
            if (checkLine(board, size, symbol, goal, rule, r, 0, 0, 1)) return true;
        for (int c = 0; c < size; c++)
            if (checkLine(board, size, symbol, goal, rule, 0, c, 1, 0)) return true;
        for (int r = 0; r < size; r++)
            if (checkLine(board, size, symbol, goal, rule, r, 0, 1, 1)) return true;
        for (int c = 1; c < size; c++)
            if (checkLine(board, size, symbol, goal, rule, 0, c, 1, 1)) return true;
        for (int r = 0; r < size; r++)
            if (checkLine(board, size, symbol, goal, rule, r, size - 1, 1, -1)) return true;
        for (int c = 0; c < size - 1; c++)
            if (checkLine(board, size, symbol, goal, rule, 0, c, 1, -1)) return true;
        return false;
    }
    
    if (checkLineDirection(board, size, symbol, goal, rule, lastRow, lastCol, 0, 1)) return true;
    if (checkLineDirection(board, size, symbol, goal, rule, lastRow, lastCol, 1, 0)) return true;
    if (checkLineDirection(board, size, symbol, goal, rule, lastRow, lastCol, 1, 1)) return true;
    if (checkLineDirection(board, size, symbol, goal, rule, lastRow, lastCol, 1, -1)) return true;
    
    return false;
}


bool checkDraw(const std::vector<std::vector<char>>& board, const int size) {
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            if (board[i][j] == '-') return false;
    return true;
}

static std::vector<pII> findLinePositions(const std::vector<std::vector<char>>& board, int size, 
                                          char symbol, int goal, EndRule rule,
                                          int startRow, int startCol, int dr, int dc) {
    std::vector<pII> result;
    for (int r = startRow, c = startCol, k = 0; k <= size - goal; r += dr, c += dc, k++) {
        bool ok = true;
        for (int step = 0; step < goal; step++) {
            int nr = r + step * dr, nc = c + step * dc;
            if (nr < 0 || nr >= size || nc < 0 || nc >= size) { ok = false; break; }
            if (board[nr][nc] != symbol) { ok = false; break; }
        }
        if (!ok) continue;
        for (int step = 0; step < goal; step++)
            result.push_back({r + step * dr, c + step * dc});
        return result;
    }
    return result;
}

std::vector<pII> findWinningLine(const std::vector<std::vector<char>>& board, const int size, 
                                  const char symbol, const int goal,
                                  EndRule rule) {
    std::vector<pII> winPositions;
    
    for (int r = 0; r < size; r++) {
        winPositions = findLinePositions(board, size, symbol, goal, rule, r, 0, 0, 1);
        if (!winPositions.empty()) return winPositions;
    }
    for (int c = 0; c < size; c++) {
        winPositions = findLinePositions(board, size, symbol, goal, rule, 0, c, 1, 0);
        if (!winPositions.empty()) return winPositions;
    }
    for (int r = 0; r < size; r++) {
        winPositions = findLinePositions(board, size, symbol, goal, rule, r, 0, 1, 1);
        if (!winPositions.empty()) return winPositions;
    }
    for (int c = 1; c < size; c++) {
        winPositions = findLinePositions(board, size, symbol, goal, rule, 0, c, 1, 1);
        if (!winPositions.empty()) return winPositions;
    }
    for (int r = 0; r < size; r++) {
        winPositions = findLinePositions(board, size, symbol, goal, rule, r, size - 1, 1, -1);
        if (!winPositions.empty()) return winPositions;
    }
    for (int c = 0; c < size - 1; c++) {
        winPositions = findLinePositions(board, size, symbol, goal, rule, 0, c, 1, -1);
        if (!winPositions.empty()) return winPositions;
    }
    
    return winPositions;
}

/* ============================================================
 * MODULE 7: BOT LOGIC (NÂNG CẤP)
 * ============================================================ */

pII random_pick(std::vector<std::vector<char>>& board, const int size) {
    std::vector<pII> emptyCells;
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            if (board[i][j] == '-')
                emptyCells.push_back({i, j});
    if (emptyCells.empty()) return {-1, -1};
    int idx = static_cast<int>(generator() % emptyCells.size());
    return emptyCells[idx];
}

static int countLine(std::vector<std::vector<char>>& board, int size,
                     int row, int col, int dx, int dy, char symbol) {
    int count = 1;
    for (int r = row + dx, c = col + dy;
         r >= 0 && r < size && c >= 0 && c < size && board[r][c] == symbol;
         r += dx, c += dy) count++;
    for (int r = row - dx, c = col - dy;
         r >= 0 && r < size && c >= 0 && c < size && board[r][c] == symbol;
         r -= dx, c -= dy) count++;
    return count;
}

static int longestLine(std::vector<std::vector<char>>& board, int size,
                       int row, int col, char symbol) {
    return std::max({
        countLine(board, size, row, col, 0,  1, symbol),
        countLine(board, size, row, col, 1,  0, symbol),
        countLine(board, size, row, col, 1,  1, symbol),
        countLine(board, size, row, col, 1, -1, symbol)
    });
}

static int evaluatePattern(int length, bool openLeft, bool openRight, bool isOwn) {
    int baseScore = 0;
    switch (length) {
        case 5: baseScore = 100000; break;
        case 4: baseScore = 10000; break;
        case 3: baseScore = 1000; break;
        case 2: baseScore = 100; break;
        case 1: baseScore = 10; break;
        default: baseScore = 0; break;
    }
    
    if (!isOwn) {
        if (length >= 4 && (openLeft || openRight)) baseScore *= 2;
        if (length >= 3 && openLeft && openRight) baseScore *= 3;
    } else {
        if (openLeft && openRight) baseScore *= 2;
        else if (openLeft || openRight) baseScore *= 1;
        else baseScore /= 2;
    }
    
    return baseScore;
}

static int evaluatePosition(std::vector<std::vector<char>>& board, int size, int row, int col, char symbol) {
    if (board[row][col] != '-') return 0;
    
    int totalScore = 0;
    int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0], dy = dirs[d][1];
        
        int posCount = 1;
        int posOpenLeft = 0, posOpenRight = 0;
        
        for (int step = 1; step <= 5; step++) {
            int nr = row + step * dx, nc = col + step * dy;
            if (nr < 0 || nr >= size || nc < 0 || nc >= size) {
                posOpenRight = 0;
                break;
            }
            if (board[nr][nc] == symbol) {
                posCount++;
            } else if (board[nr][nc] == '-') {
                posOpenRight = 1;
                break;
            } else {
                posOpenRight = 0;
                break;
            }
        }
        
        for (int step = 1; step <= 5; step++) {
            int nr = row - step * dx, nc = col - step * dy;
            if (nr < 0 || nr >= size || nc < 0 || nc >= size) {
                posOpenLeft = 0;
                break;
            }
            if (board[nr][nc] == symbol) {
                posCount++;
            } else if (board[nr][nc] == '-') {
                posOpenLeft = 1;
                break;
            } else {
                posOpenLeft = 0;
                break;
            }
        }
        
        if (posCount >= 5) {
            totalScore += evaluatePattern(5, posOpenLeft > 0, posOpenRight > 0, true);
        } else if (posCount == 4) {
            totalScore += evaluatePattern(4, posOpenLeft > 0, posOpenRight > 0, true);
        } else if (posCount == 3) {
            totalScore += evaluatePattern(3, posOpenLeft > 0, posOpenRight > 0, true);
        } else if (posCount == 2) {
            totalScore += evaluatePattern(2, posOpenLeft > 0, posOpenRight > 0, true);
        } else if (posCount == 1) {
            totalScore += evaluatePattern(1, posOpenLeft > 0, posOpenRight > 0, true);
        }
    }
    
    return totalScore;
}

static std::vector<pII> getCandidateMovesAdvanced(std::vector<std::vector<char>>& board, int size) {
    std::vector<std::vector<bool>> seen(size, std::vector<bool>(size, false));
    std::vector<pII> candidates;
    int radius = (size <= 8) ? 1 : 2;

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            if (board[i][j] == '-') continue;
            for (int di = -radius; di <= radius; di++)
                for (int dj = -radius; dj <= radius; dj++) {
                    int ni = i + di, nj = j + dj;
                    if (ni < 0 || ni >= size || nj < 0 || nj >= size) continue;
                    if (board[ni][nj] != '-' || seen[ni][nj]) continue;
                    seen[ni][nj] = true;
                    candidates.push_back({ni, nj});
                }
        }

    if (candidates.empty())
        candidates.push_back({size / 2, size / 2});
    return candidates;
}

static std::vector<pII> getCandidateMoves(std::vector<std::vector<char>>& board, int size) {
    return getCandidateMovesAdvanced(board, size);
}

pII simple_heuristic(std::vector<std::vector<char>>& board, const int size, const int goal,
                     const char botSymbol, const char playerSymbol) {
    int bestScore = -1;
    pII bestMove = {-1, -1};
    auto candidates = getCandidateMovesAdvanced(board, size);

    for (auto [i, j] : candidates) {
        int attackScore = evaluatePosition(board, size, i, j, botSymbol);
        int defenseScore = evaluatePosition(board, size, i, j, playerSymbol);
        int score = attackScore * 12 + defenseScore * 10;

        if (score > bestScore) {
            bestScore = score;
            bestMove = {i, j};
        }
    }

    if (bestMove.first != -1) return bestMove;
    return random_pick(board, size);
}

static int evaluateBoardAdvanced(std::vector<std::vector<char>>& board, int size, 
                                  char botSymbol, char playerSymbol) {
    int botScore = 0, playerScore = 0;
    
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (board[i][j] == botSymbol) {
                botScore += evaluatePosition(board, size, i, j, botSymbol);
            } else if (board[i][j] == playerSymbol) {
                playerScore += evaluatePosition(board, size, i, j, playerSymbol);
            }
        }
    }
    
    return botScore - playerScore * 2;
}

static int minimaxWithTTAdvanced(std::vector<std::vector<char>>& board, int size, int goal,
                         char botSymbol, char oppSymbol,
                         int depth, int alpha, int beta, bool isMaximizing,
                         EndRule endRule, uint64_t hash) {
    auto it = Zobrist::transpositionTable.find(hash);
    if (it != Zobrist::transpositionTable.end() && it->second != 0) {
        return it->second;
    }
    
    if (checkWin(board, size, botSymbol, goal, endRule))  return  100000 + depth;
    if (checkWin(board, size, oppSymbol, goal, endRule))  return -100000 - depth;
    if (checkDraw(board, size))                           return 0;
    
    if (depth == 0) {
        int score = evaluateBoardAdvanced(board, size, botSymbol, oppSymbol);
        Zobrist::transpositionTable[hash] = score;
        return score;
    }

    auto candidates = getCandidateMovesAdvanced(board, size);
    
    std::vector<std::pair<int, pII>> scoredCandidates;
    for (auto [r, c] : candidates) {
        int priority = evaluatePosition(board, size, r, c, isMaximizing ? botSymbol : oppSymbol);
        scoredCandidates.push_back({priority, {r, c}});
    }
    std::sort(scoredCandidates.begin(), scoredCandidates.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    char currentSym = isMaximizing ? botSymbol : oppSymbol;

    if (isMaximizing) {
        int best = -9999999;
        for (auto [priority, pos] : scoredCandidates) {
            int r = pos.first, c = pos.second;
            board[r][c] = currentSym;
            uint64_t newHash = Zobrist::hash(board, isMaximizing ? 1 : 0);
            int val = minimaxWithTTAdvanced(board, size, goal, botSymbol, oppSymbol,
                                    depth - 1, alpha, beta, false, endRule, newHash);
            board[r][c] = '-';
            best = std::max(best, val);
            alpha = std::max(alpha, best);
            if (alpha >= beta) break;
        }
        Zobrist::transpositionTable[hash] = best;
        return best;
    } else {
        int best = 9999999;
        for (auto [priority, pos] : scoredCandidates) {
            int r = pos.first, c = pos.second;
            board[r][c] = currentSym;
            uint64_t newHash = Zobrist::hash(board, isMaximizing ? 1 : 0);
            int val = minimaxWithTTAdvanced(board, size, goal, botSymbol, oppSymbol,
                                    depth - 1, alpha, beta, true, endRule, newHash);
            board[r][c] = '-';
            best = std::min(best, val);
            beta = std::min(beta, best);
            if (alpha >= beta) break;
        }
        Zobrist::transpositionTable[hash] = best;
        return best;
    }
}

pII hard_level(std::vector<std::vector<char>>& board, const int size, const int goal,
               const char botSymbol, const char playerSymbol, EndRule endRule) {
    auto candidates = getCandidateMovesAdvanced(board, size);

    for (auto [r, c] : candidates) {
        board[r][c] = botSymbol;
        if (checkWin(board, size, botSymbol, goal, endRule))
            { board[r][c] = '-'; return {r, c}; }
        board[r][c] = '-';
    }

    for (auto [r, c] : candidates) {
        board[r][c] = playerSymbol;
        if (checkWin(board, size, playerSymbol, goal, endRule))
            { board[r][c] = '-'; return {r, c}; }
        board[r][c] = '-';
    }

    std::vector<std::pair<int, pII>> scored;
    for (auto [r, c] : candidates) {
        int attackScore = evaluatePosition(board, size, r, c, botSymbol);
        int defenseScore = evaluatePosition(board, size, r, c, playerSymbol);
        int score = attackScore * 10 + defenseScore * 15;
        scored.emplace_back(score, pII{r, c});
    }
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    int depth;
    if (size <= 5) depth = 8;
    else if (size <= 8) depth = 6;
    else depth = 4;

    int topN = std::min<int>((int)scored.size(), 12);
    int bestScore = -9999999;
    pII bestMove = scored[0].second;

    for (int i = 0; i < topN; i++) {
        auto [r, c] = scored[i].second;
        board[r][c] = botSymbol;
        uint64_t hash = Zobrist::hash(board, 1);
        int val = minimaxWithTTAdvanced(board, size, goal, botSymbol, playerSymbol,
                                depth, -9999999, 9999999, false, endRule, hash);
        board[r][c] = '-';
        if (val > bestScore) { 
            bestScore = val; 
            bestMove = {r, c}; 
        }
    }

    return bestMove;
}

pII botMove(std::vector<std::vector<char>>& board, const int size, const int goal,
            const char symbol, const BotLevel level, EndRule endRule) {
    char opponent = (symbol == 'X') ? 'O' : 'X';
    switch (level) {
        case BotLevel::EASY:   return random_pick(board, size);
        case BotLevel::MEDIUM: return simple_heuristic(board, size, goal, symbol, opponent);
        case BotLevel::HARD:   return hard_level(board, size, goal, symbol, opponent, endRule);
        default:               return random_pick(board, size);
    }
}

/* ============================================================
 * MODULE 8: HELPER TEMPLATE
 * ============================================================ */

template <typename Function>
auto measureExecutionTime(const std::string& label, Function func, bool enabled)
    -> std::invoke_result_t<Function>
{
    using ReturnT = std::invoke_result_t<Function>;
    auto start = std::chrono::high_resolution_clock::now();

    if constexpr (std::is_void_v<ReturnT>) {
        func();
        auto end = std::chrono::high_resolution_clock::now();
        if (enabled) {
            std::chrono::duration<double> duration = end - start;
            std::stringstream msg;
            msg << "execution time of [" << label << "()] = " << duration.count() << "s";
            GameLogger::log(msg.str(), GameLogger::Level::DEBUG);
        }
        return;
    } else {
        ReturnT result = func();
        auto end = std::chrono::high_resolution_clock::now();
        if (enabled) {
            std::chrono::duration<double> duration = end - start;
            std::stringstream msg;
            msg << "execution time of [" << label << "()] = " << duration.count() << "s";
            GameLogger::log(msg.str(), GameLogger::Level::DEBUG);
        }
        return result;
    }
}

/* ----------------------------------------------------- */
/* -------------------- [MAIN GAME] -------------------- */
/* ----------------------------------------------------- */

int main(int argc, char* argv[]) {
    RunConfig config = parseArgs(argc, argv);

    output_file.open(config.results_file, std::ios::out | std::ios::trunc);
    if (!output_file.is_open()) {
        std::cerr << "Warning: Cannot open results file " << config.results_file << std::endl;
    }

    GameLogger::init(config.judge_mode, config.to_file, config.log_file);
    GameLogger::log("GameLogger initialized!");

    std::streambuf* cin_backup = initInteraction(config);
    GameLogger::log("GameInteraction initialized!");

    GameSetup gameSetup;
    startGame(config, gameSetup);
    GameLogger::log("GameEngine initialized!");

    GameResult gameResult = playGame(config, gameSetup);
    GameLogger::log("GameEngine playing done!");

    endGame(config, gameSetup, gameResult);
    GameLogger::log("GameEngine show endgame done!");

    closeInteraction(cin_backup);
    GameLogger::log("GameInteraction closed!");

    GameLogger::log("GameLogger closing . . .");
    GameLogger::close();

    if (output_file.is_open()) {
        output_file.close();
    }

    return 0;
}
