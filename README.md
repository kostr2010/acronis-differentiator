# ABOUT

This simple programm gets mathematic formula from input file, specifiedin main and gets it's derivative (only one variable is supported for now).

# METHOD

Concept is rather simple - we build a calculation tree using recursive descent parser. Base is a simple tree (which i for no reason at all decided to make compact (inside continuous array)), each node of which contains information about his type (e.g. operator / number / variable / function) and value (enum according to the type). then, we make several passes through this tree and trey to find some patterns that we can simplify (i did it in a very naive way, by just adding rules for each of the possible nodes, e.g. '+' node can only be simplified if one of the children is 0, or both are numbers, etc).
