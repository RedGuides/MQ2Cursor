---
tags:
  - command
---

# /cursor

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/cursor [option] [value]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Controls handling of items on cursor, as well as current status
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `on|off|auto` | Turn Plugin On/Off or Toggle |
| `silent on|off|auto` | Turn Quiet Mode On/Off or Toggle |
| `help` | displays help text |
| `list [item name]` | lists items currently handled by the plugin. Will display any matching name provided, or all if no name is provided. |
| `load|save` | load a list from the .ini file, or save the current list to the .ini |
| `remove|delete [id]` | Remove itemID or the current item on cursor from the list. |
| `all` | Always keep the item on cursor |
| `*#* [stacks]` | Keep quantity (Item On Cursor). |
| `random #` | Random Humanish delay in ms 0=off. |

## Examples

- /cursor 2 stacks
- /cursor rem 1685
- /cursor silent on
- /cursor list Water