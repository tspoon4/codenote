# Vim

## Modes
* **i** Insert mode, for typing before character
* **a** Insert mode, for typing after character
* **v** Visual mode, for selecting

## Files
* **:ls** List open files
* **:e path** Edit file
* **:b id** Switch to buffer id
* **:w** Write file

## Navigation
* **h, j, k, l** Move
* **w** Move to the next word
* **b** Move to previous word
* **^** Move to the beginning of the line
* **$** Move to the end of the line
* **%** Move to the matching bracket
* __\*__ Find forward
* __\#__ Find backward

## Copy-paste
* **yy** Copy the current line
* **3yy** Copy 3 lines
* **yiw** Copy from the current word
* **y$** Copy from the cursor to the end of the line
* **y^** Copy from the cursor to the beginning of the line
* **y%** Copy to the matching character (e.g. braces)
* **dd** Delete the current line
* **3dd** Delete 3 lines
* **d$** Delete from the cursor to the end of the line
* **p** Paste after the cursor
* **P** Paste before the cursor

## Undo-redo
* **:u** Undo
* **:undolist** List the operations
* **u** Undo
* **2u** Undo
* **Ctrl-r** Redo

## Find-replace
* **/foo** Find each foo occurrences
* **:s/foo/bar/g** Find each foo occurrence **in current line** and replace
* **:%s/foo/bar/g** Find each foo occurrence **in all lines** and replace
* **:%s/foo/bar/gci** Find each foo occurrence **in all lines** with confirmation and case insensitive
* **:%s/\<foo\>/bar/gcI** Find each whole word foo occurrence **in all lines** with confirmation and case sensitive

