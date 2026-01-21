# Egaroucid 接戦AI改造プロジェクト - 完全版ドキュメント

**作成日**: 2026-01-22
**ステータス**: 初期調査完了、実装準備中

---

## プロジェクト概要

### 背景

オセロAI「Egaroucid」のC++ソースコードを改造して、「接戦になるAI」を作成する。友人にリンクを送って遊んでもらうため、Web版（WASM）として公開する。GitHub Pages でホスティング予定。

### 目標

- **通常のAI**: 自分の有利さを最大化 → 大差で勝つ
- **改造版AI**: 駒数差の絶対値を最小化 → 接戦を狙う

### 環境

- **OS**: macOS（MacBook Air）
- **エディタ**: Cursor（VSCode）
- **ツール**: Homebrew、Git、Emscripten（バージョン3.1.20）

---

## 1. Web版ビルド手順

### MacBook Air + Emscripten で実行可能なコマンド

```bash
cd src
em++ Egaroucid_for_Web.cpp -o ai.js -s WASM=1 \
  -s "EXPORTED_FUNCTIONS=['_init_ai', '_ai_js', '_calc_value', '_stop', '_resume', '_malloc', '_free']" \
  -O3 -s TOTAL_MEMORY=629145600 -s ALLOW_MEMORY_GROWTH=1
```

### 必要な環境

- **Emscripten**: バージョン3.1.20で動作確認済み
  - インストール: `brew install emscripten`
- **macOS**: MacBook Air
- **Git**: インストール済み

### ビルド後の配置

- `ai.js` と `ai.wasm` が生成される
- これらを `docs/ja/web/` にコピーして GitHub Pages で配信

```bash
# ビルド後の配置コマンド
cp ai.js ai.wasm ../docs/ja/web/
```

### ローカルテスト方法

```bash
cd docs/ja/web/
python3 -m http.server 8000
# ブラウザで http://localhost:8000 にアクセス
```

---

## 2. 評価関数の場所

### 主要な評価関数

- **ファイル**: `src/web/evaluate.hpp`
- **関数名**: `mid_evaluate_diff(Search *search)`
- **行番号**: 427-433

```cpp
inline int mid_evaluate_diff(Search *search){
    int phase_idx = search->phase();
    int res = calc_pattern_diff(phase_idx, search);  // パターンベース評価
    res += res > 0 ? STEP_2 : (res < 0 ? -STEP_2 : 0);
    res /= STEP;
    return max(-SCORE_MAX, min(SCORE_MAX, res));
}
```

### 評価の仕組み

1. **パターンベース評価関数**を使用
2. `calc_pattern_diff()` (line 410-421) が実際の評価値を計算
3. 事前学習された `pattern_arr` テーブルから評価値を取得
4. **評価値の範囲**: -SCORE_MAX (=-64) ～ SCORE_MAX (=64)
5. **正の値**: 自分が有利
6. **負の値**: 相手が有利

### 補助的な評価関数

- **終盤評価**: `end_evaluate(Board *b)` (line 423-425)
  - 終局時の正確な駒数差を返す
  - `b->score_player()` を呼び出し

### 探索部分

#### 中盤探索 (`src/web/midsearch.hpp`)
- **nega_alpha()**: ネガアルファ探索（Alpha-Betaの変形）
- **nega_alpha_eval1()**: 深さ1の探索
- **nega_alpha_ordering_nomemo()**: 手順生成付き探索
- **first_nega_scout()**: 最初の探索（nega-scout法）

#### 終盤探索 (`src/web/endsearch.hpp`)
- 完全読み切りを実行

#### AIインターフェース (`src/web/ai.hpp`)
- 探索を統合して最善手を返す
- `ai()` 関数が探索のエントリーポイント

---

## 3. trashに移動したファイル一覧

**合計サイズ**: 約210MB

### 移動したディレクトリ

| ディレクトリ | 説明 |
|------------|------|
| `src/gui/` | GUI版（Siv3D関連） |
| `src/console/` | コンソール版 |
| `src/engine/` | デスクトップ版の本格エンジン |
| `src/tools/` | 開発ツール（評価関数学習など） |
| `bin/` | バイナリ・リソースファイル |
| `app_resources/` | デスクトップ版用リソース |
| `benchmark/` | ベンチマークテスト |
| `img/` | ドキュメント用画像 |
| `.vscode/` | VSCode設定 |
| `.github/` | GitHub Actions設定 |
| `Egaroucid.xcodeproj/` | Xcodeプロジェクト |
| `docs/ja/benchmarks/` | ベンチマークページ |
| `docs/ja/console/` | コンソール版ページ |
| `docs/ja/download/` | ダウンロードページ |
| `docs/ja/technology/` | 技術ページ |
| `docs/ja/usage/` | 使い方ページ |
| `docs/ja/index.html` | ホームページ |
| `docs/en/` | 英語版サイト全体 |

### 移動したファイル

| ファイル | 説明 |
|---------|------|
| `src/Egaroucid.cpp` | GUI版メインファイル |
| `src/Egaroucid_for_Console.cpp` | コンソール版メインファイル |
| `src/Egaroucid_light.cpp` | テスト用軽量版 |
| `src/stdafx.*` | プリコンパイル済みヘッダー |
| `Egaroucid.sln` | Visual Studioソリューション |
| `Egaroucid*.vcxproj*` | Visual Studioプロジェクト |
| `Egaroucid_for_Console.sln` | コンソール版ソリューション |
| `CMakeLists.txt` | CMake設定 |
| `Info.plist` | macOSアプリ用設定 |
| `icon.icns` | macOSアイコン |

**注意**: これらのファイルは削除されていません。`trash/` フォルダに移動しただけなので、必要に応じて復元可能です。

---

## 4. 残った必要なディレクトリ構成

```
Egaroucid-modify/
├── .git/                              # Git管理
├── .gitignore
├── LICENSE
├── README.md
├── project_reference.md               # 本ドキュメント（統合版）
│
├── src/
│   ├── Egaroucid_for_Web.cpp          # Web版メインファイル
│   ├── Egaroucid_for_Web_compile_cmd.txt  # ビルドコマンド記録
│   ├── README.md
│   └── web/                           # Web版コア実装（24ファイル）
│       ├── ai.hpp                     # AIインターフェース
│       ├── bit.hpp                    # ビット演算
│       ├── board.hpp                  # 盤面管理
│       ├── book.hpp                   # 定石
│       ├── book_const.hpp             # 定石データ
│       ├── bookrw.hpp                 # 定石読み書き
│       ├── common.hpp                 # 共通定義
│       ├── embed_book.cpp             # 定石埋め込み
│       ├── endsearch.hpp              # 終盤探索
│       ├── evaluate.hpp               # ★評価関数（改造対象）
│       ├── evaluate_const.hpp         # 評価関数パラメータ
│       ├── flip.hpp                   # 反転処理
│       ├── last_flip.hpp              # 最終反転
│       ├── level.hpp                  # レベル設定
│       ├── midsearch.hpp              # ★中盤探索（改造対象）
│       ├── mobility.hpp               # 着手可能数
│       ├── move_ordering.hpp          # 手順生成
│       ├── probcut.hpp                # ProbCut枝刈り
│       ├── search.hpp                 # 探索基盤
│       ├── setting.hpp                # 設定
│       ├── stability.hpp              # 確定石
│       ├── transpose_table.hpp        # 置換表
│       ├── util.hpp                   # ユーティリティ
│       └── util/                      # ユーティリティサブディレクトリ
│
├── web_resources/                     # Web開発用リソース
│   ├── generate_html.py               # HTML生成スクリプト
│   ├── copy_to_docs.py                # docs/へのコピースクリプト
│   ├── lang.txt
│   ├── main_page_url.txt
│   ├── en/                            # 英語版リソース
│   ├── ja/                            # 日本語版リソース
│   └── table_generator/               # テーブル生成ツール
│
├── docs/                              # GitHub Pages配信先
│   ├── index.html
│   ├── script.js
│   ├── version.txt
│   ├── 404.html
│   ├── CNAME
│   ├── img/
│   └── ja/                            # 日本語版サイト
│       ├── img/                       # 画像
│       ├── style.css                  # スタイル
│       └── web/                       # Web版アプリ（メイン）
│           ├── ai.js                  # ビルド済みWASMラッパー
│           ├── ai.wasm                # ビルド済みWASMバイナリ
│           ├── index.html             # Web版UI
│           ├── script.js              # フロントエンドロジック
│           ├── style.css
│           ├── style_web.css
│           └── img/
│
└── trash/                             # 不要ファイル保管（210MB）
    └── （上記参照）
```

---

## 5. 接戦AIへの改造方針

### 現状の評価関数の動作

- **目的**: 自分の有利さを最大化
- **方式**: パターンマッチングベース
- **評価値**: -64 ～ +64
  - 正の値 = 自分が有利
  - 負の値 = 相手が有利

### 接戦AIへの改造コンセプト

**目標**: 駒数差を小さくして接戦にする

**方針**: 評価関数を「駒数差の絶対値を最小化」するように変更

### 具体的な実装手順

#### Step 1: 新しい評価関数を追加

`src/web/evaluate.hpp` の `mid_evaluate_diff()` 関数の下に追加:

```cpp
// 接戦を狙う評価関数
inline int mid_evaluate_close_game(Search *search){
    // 通常の評価値を取得（盤面の良し悪しの判断用）
    int normal_eval = mid_evaluate_diff(search);

    // 現在の駒数を計算
    int my_discs = pop_count_ull(search->board.player);
    int opp_discs = pop_count_ull(search->board.opponent);
    int disc_diff = my_discs - opp_discs;

    // 駒数差の絶対値を最小化する評価値
    // 駒数差が小さいほど高評価
    int close_game_score = -abs(disc_diff) * 100;

    // 通常評価と接戦評価をブレンド
    // 重み付けは調整可能（現在は 2:8 = 通常20%, 接戦80%）
    return (normal_eval * 2 + close_game_score * 8) / 10;
}
```

#### Step 2: 評価関数の呼び出しを置き換え

`src/web/midsearch.hpp` 内の `mid_evaluate_diff(search)` を `mid_evaluate_close_game(search)` に置き換え。

**置き換え箇所** (約5箇所):

1. Line 48: `g = -mid_evaluate_diff(search);`
2. Line 67: `return mid_evaluate_diff(search);`
3. Line 107: `return mid_evaluate_diff(search);`
4. Line 192: `return mid_evaluate_diff(search);`
5. Line 292: `return mid_evaluate_diff(search);`

**置き換えコマンド例**:
```bash
cd src/web/
# バックアップ
cp midsearch.hpp midsearch.hpp.backup

# 一括置換（要確認）
sed -i '' 's/mid_evaluate_diff(search)/mid_evaluate_close_game(search)/g' midsearch.hpp
```

#### Step 3: ビルドとテスト

```bash
cd src/

# ビルド
em++ Egaroucid_for_Web.cpp -o ai.js -s WASM=1 \
  -s "EXPORTED_FUNCTIONS=['_init_ai', '_ai_js', '_calc_value', '_stop', '_resume', '_malloc', '_free']" \
  -O3 -s TOTAL_MEMORY=629145600 -s ALLOW_MEMORY_GROWTH=1

# 生成されたファイルをWeb版ディレクトリにコピー
cp ai.js ai.wasm ../docs/ja/web/

# ローカルサーバーで動作確認
cd ../docs/ja/web/
python3 -m http.server 8000
# ブラウザで http://localhost:8000 にアクセスしてテスト
```

#### Step 4: パラメータ調整

接戦の度合いを調整したい場合、以下のパラメータを変更:

```cpp
// より接戦を重視する場合
return (normal_eval * 1 + close_game_score * 9) / 10;  // 10%, 90%

// バランス型
return (normal_eval * 3 + close_game_score * 7) / 10;  // 30%, 70%

// 駒数差のペナルティを変更
int close_game_score = -abs(disc_diff) * 50;  // より緩やか
int close_game_score = -abs(disc_diff) * 200; // より厳しく
```

### 注意事項

1. **終盤では接戦評価を弱める**
   - ゲーム終盤（残り手数が少ない）では、通常評価の比重を上げる
   - 最終的に負けないようにする

2. **定石の扱い**
   - 序盤の定石データ (`book_const.hpp`) はそのまま使用
   - 定石を無視したい場合は `book_init()` を呼ばないように変更

3. **デバッグ方法**
   - `cerr` で評価値をログ出力
   - ブラウザの開発者ツールでコンソールを確認

---

## 6. GitHub Pagesでの公開方法

### 現在の設定

- **リポジトリ**: `Egaroucid-modify`
- **配信ディレクトリ**: `docs/`
- **URL**: `https://<username>.github.io/Egaroucid-modify/ja/web/`

### 公開手順

1. **リモートリポジトリにプッシュ**
   ```bash
   git add .
   git commit -m "接戦AI実装: 評価関数を駒数差最小化に変更"
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

## 7. Web版ページの編集内容

### 実施した変更（2026-01-22）

1. **不要なページをtrashに移動**
   - ホーム、ダウンロード、使い方、コンソール版、技術ページ
   - 英語版サイト全体（docs/en/）

2. **Web版ページ（docs/ja/web/index.html）の編集**
   - Xポストボタン削除
   - 英語リンク削除
   - メニューバー削除
   - タイトル変更: "Egaroucid for Web" → "改造版Egaroucid"
   - "これはダウンロード版よりも弱い簡易バージョンです"の文章削除
   - "使い方"セクションをコメントアウト
   - "AIの強さ"セクションをコメントアウト
   - "最終更新: 2025/02/05 評価関数のアップデート"削除

3. **script.jsの編集**
   - "AI読み込み完了！"メッセージを空文字列に変更

---

## まとめ

### 完了した作業

- ✅ プロジェクト構造の把握
- ✅ Web版ビルド手順の特定
- ✅ 評価関数・探索部分のコード特定
- ✅ 不要なファイルのtrashへの移動（210MB）
- ✅ 必要なファイルの整理
- ✅ Web版ページの編集（不要な要素の削除）
- ✅ ドキュメントの統合

### 次のステップ

1. ✅ Emscriptenのインストール（完了）
2. 評価関数の改造実装
3. ビルドと動作確認
4. パラメータ調整
5. GitHub Pagesで公開

### 改造のポイント

- **メインファイル**: `src/web/evaluate.hpp` (評価関数追加)
- **サブファイル**: `src/web/midsearch.hpp` (呼び出し変更)
- **改造の難易度**: 低〜中

---

**最終更新**: 2026-01-22
