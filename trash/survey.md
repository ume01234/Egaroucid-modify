# Egaroucid 接戦AI改造プロジェクト - 初期調査
本格的に改造の実装に入る前にこのファイルを元にしつつ現状の把握と調査を実施すること。

## 背景
オセロAI「Egaroucid」のC++ソースコードを改造して、「接戦になるAI」を作りたい。
友人にリンクを送って遊んでもらうため、Web版（WASM）として公開する。
GitHub Pages でホスティング予定。

## 調査してほしいこと

### 1. Web版の構成把握
- Web版のソースコードはどのディレクトリにあるか？
- WASMビルドの手順は？（CMakeLists.txt、Makefile、ビルドスクリプトの有無）
- 依存関係は何か？（Emscripten以外に必要なもの）

### 2. 評価関数の特定
- AI の評価関数（evaluation function）はどのファイルのどの関数か？
- 探索部分（alpha-beta / negascout など）はどこか？
- 評価関数を差し替えるにはどこを修正すればよいか？

### 3. ディレクトリ構成の整理
今回の目的（Web版の改造・公開）に不要なコードを特定してほしい。

不要と思われるもの：
- Windows GUI版（Siv3D関連）
- コンソール版
- インストーラー関連
- Windows専用のビルド設定

### 4. 実行すべきタスク
調査後、以下を実行してほしい：
```bash
# プロジェクトルートに trash フォルダを作成
mkdir -p trash

# 不要なディレクトリ・ファイルを trash に移動
# （具体的なパスは調査結果に基づいて判断）
# 例: mv src/gui trash/
#     mv installer trash/
#     mv *.sln trash/
#     mv *.vcxproj* trash/
```

**注意**: 移動前に、Web版ビルドに本当に不要かを確認すること。
共有されているコード（src/engine など）は残すこと。

### 5. 出力してほしいもの
調査完了後、以下をまとめて報告：

1. **Web版ビルド手順**（MacBook Air + Emscripten で実行可能なコマンド）
2. **評価関数の場所**（ファイルパス、関数名、行番号）
3. **trashに移動したファイル一覧**
4. **残った必要なディレクトリ構成**（tree形式）
5. **次に自分がやるべきこと**（評価関数の改造方針）

## 環境
- macOS（MacBook Air）
- Cursor（VSCode）
- Homebrew インストール済み
- Git インストール済み

## 改造の方向性（参考情報）
通常の評価関数: 自分の有利さを最大化
改造後: 駒数差の絶対値を最小化（接戦狙い）
```cpp
// 改造イメージ
int evaluate_close_game(Board& board) {
    int diff = my_discs - opponent_discs;
    return -abs(diff);
}
```