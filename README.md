# rssb
An [RSSB](http://esolangs.org/wiki/RSSB) assembler and VM in C

This is my first successful attempt at writing an assembler.  It uses Edward Dijkstra's [Shunting Yard Algorithm](http://en.wikipedia.org/wiki/Shunting_yard_algorithm).  It targets the rssb one instruction computer.

Partial EBNF grammar:
```
line := [label] ["rssb" expression] [";" comment] "\n"

expression := label
            | number
            | "(" expression ")"
            | "-" expression
            | expression "+" expression
            | expression "-" expression
            | expression "*" expression
            | expression "/" expression
```
Specify an assembly file as the first command-line argument.  Example:
`rssb helloworld.rssb`

There are probably many fun bugs that will manifest themselves if you feed the assembler invalid source.  
