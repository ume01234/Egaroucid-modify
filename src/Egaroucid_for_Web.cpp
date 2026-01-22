/*
    Egaroucid Project

    @file Egaroucid_web.cpp
        Main file for Egaroucid for Web

    @date 2021-2026
    @author Takuto Yamana
    @license GPL-3.0-or-later
*/

#include <iostream>
#include "web/ai.hpp"

// 案C用グローバル変数
static int g_opponent_eval = 0;      // 相手の現在の手の評価値
static int g_human_cumulative = 0;   // 人間の累積評価
static int g_ai_cumulative = 0;      // AIの累積評価

inline void init(int *percentage) {
    *percentage = 1;
    board_init();
    mobility_init();
    stability_init();
    parent_transpose_table.first_init();
    child_transpose_table.first_init();
    *percentage = 80;
    std::cerr << "eval init" << std::endl;
    evaluate_init();
    book_init();
    *percentage = 100;
}

inline int input_board(Board *bd, const int *arr, const int ai_player) {
    int i, j;
    uint64_t b = 0ULL, w = 0ULL;
    int elem;
    int n_stones = 0;
    for (i = 0; i < HW; ++i) {
        for (j = 0; j < HW; ++j) {
            elem = arr[i * HW + j];
            if (elem != -1) {
                b |= (uint64_t)(elem == 0) << (HW2_M1 - i * HW - j);
                w |= (uint64_t)(elem == 1) << (HW2_M1 - i * HW - j);
                ++n_stones;
            }
        }
    }
    if (ai_player == 0) {
        bd->player = b;
        bd->opponent = w;
    } else{
        bd->player = w;
        bd->opponent = b;
    }
    return n_stones;
}

inline double calc_result_value(int v) {
    return (double)v;
}

inline void print_result(int policy, int value) {
    cout << policy / HW << " " << policy % HW << " " << calc_result_value(value) << endl;
}

inline void print_result(Search_result result) {
    cout << idx_to_coord(result.policy) << " " << calc_result_value(result.value) << endl;
}

inline int output_coord(int policy, int raw_val) {
    return 1000 * (HW2_M1 - policy) + 100 + raw_val;
}

extern "C" int init_ai(int *percentage) {
    cout << "initializing AI" << endl;
    init(percentage);
    cout << "AI iniitialized" << endl;
    return 0;
}

extern "C" int ai_js(int *arr_board, int level, int ai_player) {
    cout << "start AI" << endl;
    int i, n_stones, policy;
    Board b;
    Search_result result;
    cout << endl;
    n_stones = input_board(&b, arr_board, ai_player);
    b.print();
    cout << "ply " << n_stones - 3 << endl;
    result = ai(b, level, true, false, true);
    cout << "searched policy " << idx_to_coord(result.policy) << " value " << result.value << " nps " << result.nps << endl;
    int res = output_coord(result.policy, result.value);
    cout << "res " << res << endl;
    return res;
}

extern "C" int calc_opponent_eval_js(int *arr_board, int level, int ai_player, int move_y, int move_x) {
    Board b;
    input_board(&b, arr_board, 1 - ai_player);  // 人間視点

    int move_cell = HW2_M1 - (move_y * HW + move_x);
    Flip flip;
    calc_flip(&flip, &b, move_cell);
    b.move_board(&flip);

    // 探索で評価値を計算
    Search_result result = ai(b, level, true, false, false);
    g_opponent_eval = -result.value;
    g_human_cumulative += g_opponent_eval;  // 累積に加算

    cerr << "opponent eval: " << g_opponent_eval << " human_cum: " << g_human_cumulative << " ai_cum: " << g_ai_cumulative << endl;
    return g_opponent_eval;
}

extern "C" int ai_mirror_js(int *arr_board, int level, int ai_player) {
    Board b;
    int n_stones = input_board(&b, arr_board, ai_player);

    // 初手はランダム（ただし評価値は累積に加算）
    if (n_stones == 4) {
        uint64_t legal = b.get_legal();
        vector<int> moves;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal))
            moves.push_back(cell);
        int policy = moves[myrandrange(0, (int)moves.size())];

        // ランダムに選んだ手の評価値を計算して累積に加算
        Flip flip;
        calc_flip(&flip, &b, policy);
        Board child = b.copy();
        child.move_board(&flip);
        Search_result child_result = ai(child, level, true, false, false);
        int eval = -child_result.value;
        g_ai_cumulative += eval;

        cerr << "mirror AI first move (random): " << idx_to_coord(policy) << " eval=" << eval << " ai_cum=" << g_ai_cumulative << endl;
        return output_coord(policy, eval);
    }

    // 累積差分を補正したターゲットを計算
    int adjusted_target = g_human_cumulative - g_ai_cumulative;
    cerr << "mirror AI: human_cum=" << g_human_cumulative << " ai_cum=" << g_ai_cumulative << " adjusted_target=" << adjusted_target << endl;

    // 補正ターゲットに最も近い手を選択
    Search_result result = ai_mirror(b, level, adjusted_target);
    g_ai_cumulative += result.value;  // 累積に加算

    cerr << "mirror AI: selected=" << idx_to_coord(result.policy) << " value=" << result.value << " new_ai_cum=" << g_ai_cumulative << endl;
    return output_coord(result.policy, result.value);
}

extern "C" void calc_value(int *arr_board, int *res, int level, int ai_player) {
    int i, n_stones, policy;
    Board b;
    Search_result result;
    cerr << "AI player " << ai_player << " level " << level << endl;
    n_stones = input_board(&b, arr_board, 1 - ai_player);
    b.print();
    cout << "ply " << n_stones - 3 << endl;
    int tmp_res[HW2];
    for (i = 0; i < HW2; ++i)
        tmp_res[i] = -1;
    uint64_t legal = b.get_legal();
    cerr << pop_count_ull(legal) << " legal moves found" << endl;
    Flip flip;
    uint64_t searched_nodes = 0ULL;
    for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)) {
        calc_flip(&flip, &b, cell);
        b.move_board(&flip);
            tmp_res[cell] = -ai(b, level, true, false, false).value;
        b.undo_board(&flip);
        cerr << idx_to_coord(cell) << " value " << tmp_res[cell] << endl;
    }
    for (i = 0; i < HW2; ++i)
        res[10 + HW2_M1 - i] = tmp_res[i];
    for (int y = 0; y < HW; ++y) {
        for (int x = 0; x < HW; ++x)
            cout << tmp_res[HW2_M1 - y * HW - x] << " ";
        cout << endl;
    }
}

extern "C" void stop() {
    global_searching = false;
}

extern "C" void resume() {
    global_searching = true;
}

extern "C" void reset_cumulative() {
    g_opponent_eval = 0;
    g_human_cumulative = 0;
    g_ai_cumulative = 0;
    cerr << "cumulative values reset" << endl;
}