# 接戦オセロAI（Egaroucid改造版）

**人間と接戦になるように調整されたオセロAI**  
**授業用成果物で、商用目的等ではありません。**  
情報メディア実験B秋の成果物  
Webブラウザで遊べます。 

---

## 遊び方

1. [Web版](https://ume01234.github.io/Egaroucid-modify/ja/web/)にアクセス
2. 先攻/後攻を選んで「対局開始」

---

## 特徴

- **接戦を演出**: AIが相手の実力に合わせて手を調整
- **累積評価補正**: ゲーム全体を通してスコアが拮抗するよう自動調整
- **ブラウザで動作**: インストール不要、WebAssemblyで高速動作

---

## 技術的な仕組み

「ミラーリング + 累積評価補正」方式を採用：

1. 人間が打った手の評価値を計算
2. これまでの累積評価の差分を計算
3. AIは累積差分を補正したターゲットに最も近い手を選択

詳細は [project_reference.md](project_reference.md) を参照。

---

## ビルド方法

```bash
cd src
em++ Egaroucid_for_Web.cpp -o ai.js -s WASM=1 \
  -s "EXPORTED_FUNCTIONS=['_init_ai', '_ai_js', '_ai_mirror_js', '_calc_opponent_eval_js', '_calc_value', '_stop', '_resume', '_reset_cumulative', '_malloc', '_free']" \
  -s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']" \
  -O3 -s TOTAL_MEMORY=629145600 -s ALLOW_MEMORY_GROWTH=1

cp ai.js ai.wasm ../docs/ja/web/
```

要件: Emscripten 3.1.20以降

---

## ライセンス

GNU General Public License v3.0 or later

本プロジェクトは [Egaroucid](https://github.com/Nyanyan/Egaroucid) をフォーク・改造したものです。

---

## クレジット

### 原作者

[Takuto Yamana (a.k.a Nyanyan)](https://nyanyan.dev/en/)

Egaroucid - One of the strongest Othello AI in the world
https://www.egaroucid.nyanyan.dev/

---

## 注意事項

オセロ・Othelloは登録商標です。 TM&(C) Othello,Co. and MegaHouse
