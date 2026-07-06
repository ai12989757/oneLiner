# OneLiner

OneLiner is a lightweight renaming plugin for Maya. The UI stays intentionally minimal, but the current version includes live tree preview, batch replacement, wildcard selection, hierarchy/type filters, and a small tools menu for common actions.

## It has been integrated into [OneMaya](https://github.com/ai12989757/OneMaya.git). The oneLiner library will no longer receive standalone updates. For the new link, visit [OneLiner](https://github.com/ai12989757/OneMaya/tree/main/Plugin/Common/OneLiner)

## English | [简体中文](README_zh.md)

## UI

![image.png](images/10.png)

## Requirements

- Windows
- Maya 2017+

## Current Features

- Single-line rename workflow with instant execution on Enter
- Live preview panel with tree structure for hierarchy mode
- Stable preview for duplicated Maya names
- Old-name reuse, numeric tokens, alphabetic tokens
- Trim, replace, and ordered multi-replace rules
- Wildcard-based selection mode using Maya native matching
- `-h`, `-s`, and `-type` filters
- Right-click tools for preview toggle, auto-close, wildcard shape inclusion, `pasted__` cleanup, and help
- Full-width punctuation normalization for common rule characters when switching input methods

## Installation

### Standard Install

1. Keep the repository path simple if possible.
2. If the plugin binary has not been built yet, run `build.bat` in the repository root first.
3. Drag `install.mel` into the Maya viewport.
4. The installer loads `bin/<maya version>/oneLiner.mll` and registers image/icon search paths automatically.

![11.gif](images/11.gif)

### Build From Source

Run this in the repository root:

```bat
build.bat
```

The compiled plugin is written to `bin/<MayaVersion>/oneLiner.mll`.

## Command-line / Maya `oneLiner` Command

`oneLiner` can also be invoked from Maya as a command. Example usages:

- Open the UI:

```mel
oneLiner -showWindow;
```

- Get a preview for a rule (no renaming):

```mel
oneLiner -rule "ctrl_##/3" -preview;
```

- Execute a rule immediately from script:

```mel
oneLiner -rule "L_>R_" -execute;
```

Flags:

- `-showWindow / -sw` — 打开插件 UI
- `-rule / -r <text>` — 传入规则字符串
- `-preview / -p` — 返回预览列表（不执行重命名）
- `-execute / -e` — 直接执行重命名
- `-mode / -m <s|h|a>` — 强制作用域（`s`=selected, `h`=hierarchy, `a`=all）
- `-clearPasted / -cp` — 清理 `pasted__` 前缀
- `-help / -h` — 输出命令帮助文本（包含中/英两种说明）

## Usage

### Basic Flow

1. Select objects in Maya.
2. Open OneLiner.
3. Type a rule in the input field.
4. Review the live preview.
5. Press Enter to execute.

![01.gif](images/01.gif)

### Rule Quick Reference

#### `!` `#` `@`

- **`!`**: reuse the original object name, e.g. `side_!` → `side_box` (keeps part of the old name).
- **`#`**: numeric sequence. Use multiple `#` for zero-padding: `ctrl_##` → `01, 02, ...`.
  - Set a numeric start by appending `/N` after the rule: `ctrl_##/3` → `03, 04, ...`. `\` is also supported, e.g. `ctrl_##\3`.
- **`@`**: alphabetic sequence. Two modes:
  - **Single `@` (expandable carry):** `@` produces `A, B, ..., Z, AA, AB, ...` (carry increments leftward).
  - **Multiple `@` (fixed-width):** `@@` / `@@@` reserve exactly that many letters. `@@` produces `AA, AB, ..., AZ, BA, ...`.
  - Provide an optional start template after a `/` to control starting letters and case: `@@/Aa` starts at `Aa` and preserves case per template (`A` = upper, `a` = lower). If omitted, fixed-width starts from `A...A`. `\` is also supported, e.g. `@@\Aa`.

Notes:

- Full-width punctuation commonly produced by IMEs (e.g. `！ ？ ＃ ＠ ＊`) are normalized to their ASCII counterparts, so rules still work when typing in different input modes.
- If a start template is malformed, the engine falls back to treating the marker as a literal character rather than producing unexpected prefixes.

![02.gif](images/02.gif)
![03.gif](images/03.gif)
![04.gif](images/04.gif)

#### Replace Rules

- `old>new`: single replacement pair
- `Mesh A>Joint B`: ordered multi-replace
- `Mesh,A>Joint,B`: comma-separated multi-replace
- `Mesh，A>Joint，B`: Chinese comma is also supported

Examples:

- `L_>R_`
- `Mesh A>Joint B`
- `Ctrl,FK>Drv,IK`

#### Trim Rules

- `+number`: remove characters from the beginning
- `-number`: remove characters from the end
- `--number`: keep only the first N characters

Examples:

- `+3`
- `-2`
- `--6`

![06.gif](images/06.gif)

#### Hierarchy and Type Filters

- `-h`: include selected objects and all descendants
- `-h -s`: include shapes in hierarchy mode
- `-type joint blendShape`: filter by Maya node type

Notes:

- Selected mode is the default
- If `-type` finds nothing in the current candidate set, the engine falls back to a global type lookup

![05.gif](images/05.gif)

#### Wildcard Selection

- Typing `*` or `?` switches to Maya native wildcard selection
- Pressing Enter updates selection instead of renaming
- The “include shape” toggle in the tools menu only affects wildcard matching

Examples:

- `ctrl_*`
- `L_arm_??`

#### Preview and Input History

- The preview updates as you type
- Hierarchy candidates are shown in a tree with node icons
- Double-clicking a preview item writes its raw text back into the input field
- Up and Down arrow keys browse rule history for the current Maya session

## Right-Click Menu

The input field opens a tools menu on right click. Current entries are:

- Enable Preview
- Auto Close
- Include Shape Objects In Wildcard Search
- Clear `pasted__` Prefix
- Help

![08.png](images/08.png)

## Repository Layout

- `script/`: Maya plugin logic and UI
- `OneQtC++/`: OneQt widgets and tests used by the plugin
- `images/`: README assets, help gifs, and icons
- `install.mel`: Maya installation entry point
- `build.bat`: root build script

## Credits

- Original Author: Fauzan Syabana
- Email: zansyabana@gmail.com
- Original page: <https://www.highend3d.com/maya/script/oneliner-simple-renamer-tool-for-maya>

The original renaming idea comes from the original oneLiner script. This version extends it with a compiled Maya plugin, refreshed UI, tree preview, and expanded rule handling.

![09.png](images/09.png)

## License

[MIT](LICENSE)
