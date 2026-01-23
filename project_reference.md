# Egaroucid 接戦AI改造プロジェクト
**ステータス**: デプロイ完了
**公開URL**: https://ume01234.github.io/Egaroucid-modify/ja/web/

---

## プロジェクト概要

### 背景

オセロAI「Egaroucid」のC++ソースコードを改造して、「接戦になるAI」を作成する。友人にリンクを送って遊んでもらうため、Web版（WASM）として公開する。

### 目標

- **通常のAI**: 自分の有利さを最大化 → 大差で勝つ
- **改造版AI**: 相手の手の評価に合わせる → 接戦を狙う

### 環境

- **OS**: macOS（MacBook Air）
- **エディタ**: Cursor（VSCode）
- **ツール**: Homebrew、Git、Emscripten（バージョン3.1.20）

---

## 1. オリジナルEgaroucidの処理

### 手の選択フロー

```
ai() 関数
  ↓
1. 定石（book）があれば使用
2. get_level() で探索パラメータを決定
3. tree_search() で探索実行
4. 最も評価値が高い手を返す
```

### 序盤・中盤・終盤の判定 (level.hpp)

```cpp
int n_empties = 60 - n_moves;  // 空きマス数

if (n_empties > level_status.complete0) {
    // 中盤探索：評価関数を使用
    *is_mid_search = true;
    *depth = level_status.mid_lookahead;
} else {
    // 終盤探索：完全読み
    *is_mid_search = false;
    *depth = n_empties;  // 残り全手を読む
}
```

---

## 2. 案A（固定目標0）- 失敗

### 概念

評価関数を変更し、「石差0（同点）に近い手」を選ぶようにする。

### 実装

`src/web/evaluate.hpp` の評価関数を変更:

```cpp
// 通常の評価関数
inline int mid_evaluate(Board *b) {
    int raw_eval = calc_pattern_eval(...);
    return raw_eval;  // 自分が有利なほど高い値
}

// 案A: 接戦用評価関数
inline int mid_evaluate_close_game(Board *b) {
    int raw_eval = calc_pattern_eval(...);
    return -abs(raw_eval);  // 0に近いほど高い値
}

inline int end_evaluate_close_game(Board *b) {
    int score = b->score_player();
    return -abs(score);  // 石差0に近いほど高い値
}
```

### 失敗した理由

| 対戦 | 結果 |
|------|------|
| 人間先攻 vs AI後攻 | 21:43, 19:45, 17:47 (AI圧勝) |
| AI先攻 vs 人間後攻 | 58:6, 46:18, 57:6 (AI圧勝) |

**原因**:
1. 「同点を目指す」≠「接戦になる」- 相手の実力を考慮していない
2. 人間の悪手による差が累積し、最終的に大差になる
3. 固定目標（0）は動的な調整ができない

---

## 3. 案C（ミラーリング + 累積評価補正）- 成功

### 概念

相手（人間）が打った手の評価値を計算し、AIは**累積評価の差分を補正したターゲット**に最も近い手を選択する。

```
┌─────────────────────────────────────────────────────────────┐
│ 1. 相手（人間）が手を打つ                                   │
│ 2. その手の評価値を計算し、人間の累積評価に加算             │
│ 3. adjusted_target = 人間の累積評価 - AIの累積評価          │
│ 4. AIの合法手それぞれの評価値を計算                         │
│ 5. |my_eval - adjusted_target| が最小の手を選択             │
│ 6. 選んだ手の評価値をAIの累積評価に加算                     │
└─────────────────────────────────────────────────────────────┘
```

### 実装詳細

#### 3.1 グローバル変数の追加 (Egaroucid_for_Web.cpp:15-18)

```cpp
// 案C用グローバル変数
static int g_opponent_eval = 0;      // 相手の現在の手の評価値
static int g_human_cumulative = 0;   // 人間の累積評価
static int g_ai_cumulative = 0;      // AIの累積評価
```

#### 3.2 人間の手の評価値計算 (Egaroucid_for_Web.cpp:98-114)

```cpp
extern "C" int calc_opponent_eval_js(int *arr_board, int level, int ai_player, int move_y, int move_x) {
    Board b;
    input_board(&b, arr_board, 1 - ai_player);  // 人間視点

    // 人間が打った手を盤面に反映
    int move_cell = HW2_M1 - (move_y * HW + move_x);
    Flip flip;
    calc_flip(&flip, &b, move_cell);
    b.move_board(&flip);

    // 探索で評価値を計算
    Search_result result = ai(b, level, true, false, false);
    g_opponent_eval = -result.value;
    g_human_cumulative += g_opponent_eval;  // 累積に加算

    return g_opponent_eval;
}
```

#### 3.3 ミラーリングAIエントリーポイント (Egaroucid_for_Web.cpp:116-151)

```cpp
extern "C" int ai_mirror_js(int *arr_board, int level, int ai_player) {
    Board b;
    int n_stones = input_board(&b, arr_board, ai_player);

    // 初手はランダム（評価値は累積に加算）
    if (n_stones == 4) {
        // ランダムに手を選び、その評価値を累積に加算
        ...
        g_ai_cumulative += eval;
        return output_coord(policy, eval);
    }

    // 累積差分を補正したターゲットを計算
    int adjusted_target = g_human_cumulative - g_ai_cumulative;

    // 補正ターゲットに最も近い手を選択
    Search_result result = ai_mirror(b, level, adjusted_target);
    g_ai_cumulative += result.value;  // 累積に加算

    return output_coord(result.policy, result.value);
}
```

#### 3.4 ミラーリングAI本体 (web/ai.hpp:325-364)

```cpp
Search_result ai_mirror(Board board, int level, int target_eval) {
    Search_result res;

    // パスチェック
    if (board.get_legal() == 0ULL) { ... }

    uint64_t legal = board.get_legal();
    int best_policy = -1;
    int best_value = 0;
    int min_diff = INF;

    // 全合法手について評価値を計算
    Flip flip;
    for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)) {
        calc_flip(&flip, &board, cell);
        Board child = board.copy();
        child.move_board(&flip);

        // オリジナルのai()を使って評価
        Search_result child_result = ai(child, level, true, false, false);
        int eval = -child_result.value;
        int diff = abs(eval - target_eval);

        // ターゲットに最も近い手を記録
        if (diff < min_diff) {
            min_diff = diff;
            best_policy = cell;
            best_value = eval;
        }
    }

    res.policy = best_policy;
    res.value = best_value;
    return res;
}
```

#### 3.5 累積値リセット (Egaroucid_for_Web.cpp:192-197)

```cpp
extern "C" void reset_cumulative() {
    g_opponent_eval = 0;
    g_human_cumulative = 0;
    g_ai_cumulative = 0;
}
```

#### 3.6 JavaScript側の変更 (docs/ja/web/script.js)

```javascript
// モード切り替え変数
var use_mirror_mode = true;  // true: 案C, false: 案A

// 人間が手を打った後、評価値を計算
function calc_opponent_eval(y, x) {
    // 盤面データをC++に送信
    _calc_opponent_eval_js(pointer, level_idx, ai_player, y, x);
}

// AI呼び出し
function ai() {
    if (use_mirror_mode) {
        val = _ai_mirror_js(pointer, level_idx, ai_player);
    } else {
        val = _ai_js(pointer, level_idx, ai_player);
    }
}

// ゲーム開始時に累積値をリセット
function start() {
    _reset_cumulative();
    ...
}
```

### 検証結果

| シナリオ | 結果 |
|---------|------|
| 黒(先攻)人間 vs 白(後攻)AI | 44:20, 36:28, **32:32** (引き分け達成) |
| 黒(先攻)AI vs 白(後攻)人間 | 31:33, 31:33, 33:31 (接戦) |

---

## 4. オリジナルと案Cの処理比較

| フェーズ | オリジナル | 案C |
|---------|-----------|-----|
| **序盤** | 定石（book）を使用 | 定石は使わない（ミラーリング） |
| **中盤** | 評価関数で探索 → **最善手**を選択 | 評価関数で探索 → **ターゲットに近い手**を選択 |
| **終盤** | 完全読み → **最善手**を選択 | 完全読み → **ターゲットに近い手**を選択 |

**重要**: 案Cは探索エンジン自体は変更せず、「手の選択ロジック」のみを追加。

---

## 5. ビルド手順

### ビルドコマンド

```bash
cd /Users/hashizumerikuto/dev/Egaroucid-modify/Egaroucid-modify/src

em++ Egaroucid_for_Web.cpp -o ai.js -s WASM=1 \
  -s "EXPORTED_FUNCTIONS=['_init_ai', '_ai_js', '_ai_mirror_js', '_calc_opponent_eval_js', '_calc_value', '_stop', '_resume', '_reset_cumulative', '_malloc', '_free']" \
  -s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']" \
  -O3 -s TOTAL_MEMORY=629145600 -s ALLOW_MEMORY_GROWTH=1
```

### ビルド後の配置

```bash
cp ai.js ai.wasm ../docs/ja/web/
```

### ローカルテスト

```bash
cd docs/ja/web/
python3 -m http.server 8000
# ブラウザで http://localhost:8000 にアクセス
```

---

## 6. ディレクトリ構成

```
Egaroucid-modify/
├── .git/
├── LICENSE                   # GPL-3.0ライセンス
├── README.md                 # プロジェクト説明
├── project_reference.md      # 本ドキュメント
├── history.md                # 検証結果
│
├── src/
│   ├── Egaroucid_for_Web.cpp # Web版メインファイル（改造済み）
│   └── web/                  # Web版コア実装
│       ├── ai.hpp            # AIインターフェース（ai_mirror追加）
│       ├── evaluate.hpp      # 評価関数（案A用の変更あり）
│       ├── level.hpp         # レベル設定・探索パラメータ
│       ├── midsearch.hpp     # 中盤探索
│       ├── endsearch.hpp     # 終盤探索
│       └── ...
│
├── docs/                     # GitHub Pages配信先
│   └── ja/web/               # Web版アプリ
│       ├── ai.js             # ビルド済みWASMラッパー
│       ├── ai.wasm           # ビルド済みWASMバイナリ
│       ├── index.html        # UI
│       ├── style_web.css     # スタイル
│       └── script.js         # フロントエンド（改造済み）
│
└── trash/                    # 不要ファイル保管（復元可能）
```

---

## 7. ライセンス

GPL-3.0-or-later

本プロジェクトは [Egaroucid](https://github.com/Nyanyan/Egaroucid) をフォーク・改造したもの。

---

