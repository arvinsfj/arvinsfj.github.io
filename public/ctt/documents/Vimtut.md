## the vim cmd

level 1
=====================================
* i -> Insert Mode
* ESC -> Normal Mode
* x -> delete a char
* :wq -> save and quit
* dd -> delete a line and save to shear plate
* p -> paste from shear plate
* hjkl -> move cursor
* :help <command> -> show the help of the command
 
level 2
=====================================
* a -> append insert
* o -> insert a line behind the current line
* O -> insert a line before the current line
* cw -> replace a word
* 
* 0 -> move to the head of a line 
* $ -> move to the end of a line
* /pattern -> search pattern string
* 
* p -> paste
* yy -> copy the current line like ddP
* 
* u -> undo
* <C-r> -> redo
* 
* :e <file path> -> open and edit a new file
* :w -> save file
* :saveas <file> -> save as a new file
* :x,ZZ,:wq -> save and quit
* :!q -> quit and not save
* :qa! -> quit all
* :bp,:bn -> jump to pre file , jump to next file

level 3
=====================================
* . -> repeat pre command
* N<command> -> repeat N times the command
* 2dd -> delete 2 lines
* 3p -> paste 3 times
* 3. -> 3 times pre command
* 
* NG -> move to N line
* gg -> move to the first line
* G -> move to the end line
* w -> move to the begin char of the next word
* e -> move to the end char of the next word
* W,E -> ignore blank char like w,e
* % -> match (,[,{ and move to ),],}
* *,# -> match the current word and move to next or pre word
* 
* <start position><command><end position>
* 
* 0y$ -> copy a line frome the begine to the end 
* ye -> copy sub string from the current char to the end
* y2/foo -> copy a sub string between the 2 foo
* 
* d -> delete
* v -> view the selected text
* gU -> change to Upper 
* gu -> change to lower

level 4
=====================================
* 0 ^ $ f F t T , ; -> move in line
* dt" -> delete all text until "
* <action>a<object>
* <action>i<object>
* action: command like: d y v
* object: w W s p " ' ) ] }
* 
* <C-v>
* 0 <C-v><C-d> I-- [ESC]
* 
* <C-n> <C-p> -> auto complete
* 
* qa, @a, @@ -> micro record and replay
* 
* qaYp<C-a>q
* @a
* @@
* 
* v, V, <C-v>
* J -> connect all lines
* <, > -> left or right indent
* = -> auto indent
* A -> append string when visual selected
* 
* :split, vsplit -> split screen h or v
* <C-w><dir> -> <C-w>hjkl change window
* <C-w>_or| -> max window
* <C-w>+or- -> change window size

* s/*/* 
* s/*/* /g
* %s/*/* 
* %s/*/* /g 
