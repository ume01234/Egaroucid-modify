# Egaroucid 接戦AI改造ドキュメント

## 1. Egaroucid本来の手の選択ロジック

### アーキテクチャ概要

```
┌─────────────────────────────────────────────────────────────┐
│                    探索アルゴリズム                          │
│              NegaScout / Alpha-Beta探索                     │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ 最適化技術:                                          │   │
│  │ - 置換表 (Transposition Table)                      │   │
│  │ - Move Ordering (良い手から先に探索)                 │   │
│  │ - MPC (Multi-Prob Cut)                              │   │
│  │ - Stability Cut (確定石による枝刈り)                 │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      評価関数                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ パターン評価 + 小規模ニューラルネットワーク           │   │
│  │                                                      │   │
│  │ 盤面 → 46パターン分解 → 各パターンをNNで評価 → 合計  │   │
│  │                                                      │   │
│  │ NN構造: Dense0 → LeakyReLU → Dense1 → Dense2 → tanh │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### フェーズ別の処理

| フェーズ | 残り手数 | 処理内容 | 評価関数 |
|---------|---------|---------|---------|
| 序盤〜中盤 | 多い | NegaScout探索（深さ制限あり） | `mid_evaluate_diff()` - パターン評価 |
| 中盤→終盤移行 | 約20手 | `MID_TO_END_DEPTH`で切り替え | パターン評価 → 駒数評価 |
| 終盤 | 少ない | 完全読み（全手読み切り） | `end_evaluate()` = `score_player()` |

### 評価関数の詳細

```cpp
// 中盤評価: パターンベースNN
inline int mid_evaluate_diff(Search *search) {
    int phase_idx = search->phase();
    int res = calc_pattern_diff(phase_idx, search);  // 46パターンの評価合計
    res /= STEP;
    return max(-SCORE_MAX, min(SCORE_MAX, res));     // -64 〜 +64 にクリップ
}

// 終盤評価: 実際の駒数差
inline int end_evaluate(Board *b) {
    return b->score_player();  // 自分の石 - 相手の石
}
```

### 探索の流れ（簡略化）

```
first_nega_scout()
    ├── 中盤: nega_scout() / nega_alpha_ordering()
    │       └── 評価: mid_evaluate_diff() [パターンNN]
    │
    └── 終盤 (depth <= MID_TO_END_DEPTH):
            └── nega_alpha_end() / nega_alpha_end_fast()
                    └── 評価: end_evaluate() [駒数差]
```

---

## 2. 現在の接戦AI実装

### 変更方針

**目標**: 最終スコアが0（引き分け）に近くなるようにプレイする

**手法**: 評価関数の出力を `-abs(score)` に変換

### 変更した評価関数

```cpp
// 接戦AI用の中盤評価関数
inline int mid_evaluate_close_game(Search *search) {
    int raw_eval = mid_evaluate_diff(search);  // 元のパターン評価
    return -abs(raw_eval);                      // 0に近いほど高評価
}

// 接戦AI用の終盤評価関数
inline int end_evaluate_close_game(Board *b) {
    int score = b->score_player();             // 実際の駒数差
    return -abs(score);                         // 0に近いほど高評価
}
```

### 変更箇所一覧

| ファイル | 変更内容 |
|---------|---------|
| `evaluate.hpp` | `mid_evaluate_close_game()`, `end_evaluate_close_game()` を追加 |
| `midsearch.hpp` | 6箇所で `end_evaluate` → `end_evaluate_close_game` |
| `endsearch.hpp` | 5箇所で `end_evaluate` → `end_evaluate_close_game` |
| `move_ordering.hpp` | move ordering評価を `mid_evaluate_close_game` に変更 |
| `probcut.hpp` | probcut評価を `mid_evaluate_close_game` に変更 |

### 現在の動作

```
┌─────────────────────────────────────────────────────────────┐
│ 全フェーズ共通:                                             │
│   評価値 = -abs(元の評価値)                                 │
│                                                             │
│   → 「0に近い局面」が最も高く評価される                     │
│   → Negamax探索で「両者が0を目指す」ゲームとして解く        │
└─────────────────────────────────────────────────────────────┘
```

### 現在のアプローチの限界

1. **Negamaxの前提とのミスマッチ**
   - Negamaxは「両プレイヤーが最善を尽くす」前提
   - 実際は「AIだけが接戦を目指し、人間は勝ちを目指す」

2. **評価関数の学習目的の違い**
   - 元のNN: 「勝てる局面か」を予測
   - 必要なもの: 「接戦になりやすい局面か」を予測

3. **協力ゲームのモデル化**
   - `-abs()` は「両者が0を目指す」協力ゲームをモデル化
   - 現実の対戦は非対称（AIだけが接戦を目指す）

---

## 3. 接戦AI精度向上のための改善案

### 案A: 探索後の手選択調整（固定目標）

**概念**: 通常の評価で探索し、最善手ではなく「結果が目標（0）に近い手」を選ぶ

```cpp
// 疑似コード
int select_close_game_move(Search *search, int target_score = 0) {
    vector<pair<int, int>> move_scores;  // (手, 予測スコア)

    for (auto move : legal_moves) {
        // 通常の評価関数で探索
        int score = normal_search(search, move);
        move_scores.push_back({move, score});
    }

    // 目標スコアに最も近い手を選択
    return argmin(move_scores, |score - target_score|);
}
```

**メリット**:
- 実装が比較的簡単
- 元の探索精度を維持

**デメリット**:
- 相手の実力を考慮しない（固定目標）
- 探索木の中で「相手が最善を尽くす」前提は変わらない

---

### 案B: 評価関数の再学習

**概念**: 「接戦になりやすい局面」を高く評価するNNを学習

```
学習データ:
  - 接戦で終わった棋譜の局面 → 高評価
  - 大差で終わった棋譜の局面 → 低評価

損失関数:
  L = |predicted_closeness - actual_closeness|
  where actual_closeness = -abs(final_score)
```

**メリット**:
- 理論的に最も正しいアプローチ
- 「接戦になりやすさ」を直接学習

**デメリット**:
- 大量の学習データが必要
- 学習インフラの構築が必要

---

### 案C: 相手の手の評価に合わせる（ミラーリング）【推奨】

**概念**: 相手が打った手の評価値を計算し、AIも同程度の評価の手を選ぶ

```
相手の手の評価値 → AIも同じ評価値の手を選ぶ → 実力が拮抗 → 接戦
```

#### 動作フロー

```
┌─────────────────────────────────────────────────────────────┐
│ 1. 相手（人間）が手を打つ                                   │
│ 2. その手の評価値を計算: opponent_eval                      │
│ 3. AIの合法手それぞれの評価値を計算                         │
│ 4. |my_eval - opponent_eval| が最小の手を選択               │
└─────────────────────────────────────────────────────────────┘

特殊ケース:
- 初手（AIが先手）: ランダム選択（初期局面は評価0）
- 相手が大悪手: AIも同様に悪い手を返す（意図的）
```

#### 実装

Egaroucidは本来、常に探索（NegaScout）を行って手を選択する設計のため、探索後の評価を使用する。

```cpp
// 相手の手の評価を記録（探索による評価）
// ※相手が打った後、AIが内部で「相手の手の価値」を探索で評価
int opponent_eval = nega_scout(search, -INF, INF, depth, ...);

int select_matching_move(Search *search, int opponent_eval, int depth) {
    uint64_t legal = search->board.get_legal();
    vector<pair<int, int>> move_evals;  // (手, 評価値)

    for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)) {
        Flip flip;
        calc_flip(&flip, &search->board, cell);
        search->move(&flip);

        // 通常の探索で評価（Egaroucid本来の探索を使用）
        int my_eval = -nega_scout(search, -INF, INF, depth - 1, ...);
        move_evals.push_back({cell, my_eval});

        search->undo(&flip);
    }

    // opponent_evalに最も近い手を選択
    int best_move = -1;
    int min_diff = INF;
    for (auto [move, eval] : move_evals) {
        int diff = abs(eval - opponent_eval);
        if (diff < min_diff) {
            min_diff = diff;
            best_move = move;
        }
    }
    return best_move;
}
```

#### 設計方針

| 状況 | 動作 |
|-----|------|
| 初手（AIが先手） | ランダム選択 |
| 相手が好手 | AIも好手を返す |
| 相手が悪手 | AIも悪手を返す（意図的に合わせる） |
| 相手が大悪手 | AIも大悪手を返す（接戦維持のため） |

#### メリット

1. **動的な難易度調整**: 相手の実力に自動で合わせる
2. **自然な接戦感**: 常に拮抗した展開になる
3. **実装が比較的簡単**: 評価値の比較だけ
4. **相手の体験向上**: 「いい勝負だった」と感じやすい
5. **相手依存である**: AIの手の質が相手に左右される → **接戦AIの目的そのもの**
6. **相手が完璧ならAIも完璧になる**: 強い相手には強く返す → **正しい動作**

#### 注意点

1. **序盤の不確定性**: 初手がランダム（初期局面は評価0のため）

#### 検証項目

- [ ] 最終スコアの分布（目標: 平均石差 < 10）
- [ ] 人間プレイヤーの体感（「いい勝負だった」感）
- [ ] 探索深度による挙動の違い

---

### 推奨アプローチ

| 段階 | アプローチ | 理由 |
|-----|-----------|------|
| 短期 | 現状の `-abs()` で検証 | まず効果を確認 |
| 中期〜長期 | **案C（ミラーリング）** | 相手の実力に動的対応、接戦AIの本質に合致 |
| 代替案 | 案A（固定目標0） | シンプルだが相手の実力を考慮しない |
| 将来検討 | 案B（再学習） | 実装コスト大だが理論的には最善 |

---

## 4. 検証方法

1. 10回以上対局し、最終スコアの分布を確認
2. 目標: 平均石差 < 10
3. グラフで予想スコアが0付近を推移しているか確認

---

## 5. 検証の進め方

### Step 1: 案A, Cの実装

- 案A（固定目標0）と案C（ミラーリング）をそれぞれ単独で実装
- 別ファイル/フォルダ/コンポーネントとして管理し、切り替え可能にする
- 案B（再学習）は実装コストが高いため一旦保留（最終手段）

### Step 2: ベンチマーク対戦

各実装をEgaroucidレベル1, 7, 15と10戦ずつ対戦させる。

**記録する指標**:

| 指標 | 説明 |
|-----|------|
| 勝敗 | AI勝ち / 負け / 引き分け |
| 平均石差 | 目標: < 10 |
| 石差の標準偏差 | 安定性の評価 |
| 最大石差 | 極端なケースがないか確認 |

### Step 3: アンサンブル戦略の設計

Step 2の結果を踏まえ、フェーズごとに戦略を組み合わせる。

**検討するアンサンブル例**:
- 序盤: 案C（相手に合わせる）
- 中盤: 案A（目標0に誘導）
- 終盤: 案C（接戦維持）

### Step 4: 案Bの検討（最終手段）

案A, C、およびそのアンサンブルで十分な効果が得られない場合のみ、案B（評価関数の再学習）を検討する。

