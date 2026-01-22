# Egaroucid 接戦AI改造プロジェクト

**作成日**: 2026-01-22
**最終更新**: 2026-01-23
**ステータス**: 実装完了、デプロイ準備中

---

## プロジェクト概要

### 背景

オセロAI「Egaroucid」のC++ソースコードを改造して、「接戦になるAI」を作成する。友人にリンクを送って遊んでもらうため、Web版（WASM）として公開する。GitHub Pages でホスティング予定。

### 目標

- **通常のAI**: 自分の有利さを最大化 → 大差で勝つ
- **改造版AI**: 相手の手の評価に合わせる → 接戦を狙う

### 環境

- **OS**: macOS（MacBook Air）
- **エディタ**: Cursor（VSCode）
- **ツール**: Homebrew、Git、Emscripten（バージョン3.1.20）

---

## 1. 実装した接戦AI（案C: ミラーリング + 累積評価補正）

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

### 累積評価補正の効果

- 毎回のわずかな評価値の差が累積してスコアが離れる問題を解決
- 累積差分を次の手で補正することで、最終スコアが接戦に収束

### 実装ファイル

| ファイル | 変更内容 |
|---------|---------|
| `src/Egaroucid_for_Web.cpp` | グローバル変数（累積評価）、`ai_mirror_js()`、`calc_opponent_eval_js()`、`reset_cumulative()` |
| `src/web/ai.hpp` | `ai_mirror()` 関数 |
| `docs/ja/web/script.js` | 評価値計算・AI呼び出し、ゲーム開始時のリセット |

### 検証結果

history.mdに詳細な対戦データあり。

| シナリオ | 結果 |
|---------|------|
| 黒(先攻)人間 vs 白(後攻)AI | 44:20, 36:28, **32:32** (引き分け達成) |
| 黒(先攻)AI vs 白(後攻)人間 | 31:33, 31:33, 33:31 (接戦) |

---

## 2. ビルド手順

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

## 3. ディレクトリ構成

```
Egaroucid-modify/
├── .git/
├── project_reference.md      # 本ドキュメント
├── history.md                # 検証結果
│
├── src/
│   ├── Egaroucid_for_Web.cpp # Web版メインファイル（改造済み）
│   └── web/                  # Web版コア実装
│       ├── ai.hpp            # AIインターフェース（ai_mirror追加）
│       ├── evaluate.hpp      # 評価関数
│       ├── midsearch.hpp     # 中盤探索
│       ├── endsearch.hpp     # 終盤探索
│       └── ...
│
├── docs/                     # GitHub Pages配信先
│   └── ja/web/               # Web版アプリ
│       ├── ai.js             # ビルド済みWASMラッパー
│       ├── ai.wasm           # ビルド済みWASMバイナリ
│       ├── index.html        # UI
│       └── script.js         # フロントエンド（改造済み）
│
└── trash/                    # 不要ファイル保管（復元可能）
```

---

## 4. GitHub Pagesでの公開手順

### 公開手順

1. **リモートリポジトリにプッシュ**
   ```bash
   git add .
   git commit -m "接戦AI完成: 案C（ミラーリング + 累積評価補正）"
   git push origin main
   ```

2. **GitHub Pages設定**
   - GitHubリポジトリの Settings > Pages
   - Source: `Deploy from a branch`
   - Branch: `main`, Folder: `/docs`

3. **確認**
   - 数分後に公開URLにアクセス
   - 動作確認

---

## 5. 完了した作業

- ✅ プロジェクト構造の把握
- ✅ Web版ビルド手順の特定
- ✅ 不要なファイルのtrashへの移動（210MB）
- ✅ Web版ページの編集（不要な要素の削除）
- ✅ 案A（固定目標0）の実装と検証
- ✅ 案C（ミラーリング）の実装と検証
- ✅ 案C改良版（累積評価補正）の実装と検証
- ✅ 接戦AI完成

---

## 6. 残りの作業（デプロイ・微調整）

- [ ] UIの最終調整（必要に応じて）
- [ ] GitHub Pagesへデプロイ
- [ ] 友人への共有

---

**最終更新**: 2026-01-23
