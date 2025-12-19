# ChessEngine

This is my first project in c++ and my first big coding project in general. I'm pretty satisfied with the playing strength it achieved and that's why I decided to stop making improvements and optimizations (for now). If you want to play against my engine, go to the release page, I've provided instructions there.

## Brief description

The engine is UCI compatible. On the "go ..." command, negaMax is called (I will not explain these algorithms in depth, if you are interested, the site I used to learn and implement them is chessprogramming.org), which is a recursive algorithm that calls itself with decrementing depth and alternating side to move. When depth 0 is reached, quiescence is called.

Quiescence is a massive improvement in playing strength for chess engines. It keeps analyzing the position until it "calms down", meaning that no more captures can be made from either side. Without this, the engine can miss important tactics.
When quiescence finishes, it returns the best evaluation found.

Both of these functions use a function to generate pseudo legal moves, meaning it doesn't check if the king is left in check and a make and unmake move function. To filter out illegal moves, the engige first makes the move, and if the king is in check, it doesn't analyze it and unmakes it.

This keeps happening until the engine analyzes all the moves for both sides on a given depth. Then, it returns the root move that produced the best score, while playing the best moves for both sides, and the score (evaluation) itself. Then it sends that move to the GUI. NegaMax also implements pruning; if a move is good enough to refute the opponent's previous move, it stops searching remaining moves, since the opponent won't allow this position anyway. This is called alpha beta pruning and it saves a lot of time.

Another big improvement includes transposition tables with zobrist hashing. The implementation is quite complicated, refer to the chessprogramming.org for full analysis. In a few words, it saves positions analyzed, along with the depth and score found and a few other key variables. In negaMax, the engine checks if the current position has been analyzed already at an equal or greater depth than what we would be using on this iteration, in order to save time by retrieving the result of the previous search and not searching the same position multiple times.

The function that calculates the evaluation is pretty simple. It includes material count, some king safety, knight and pawn positioning, piece mobility and a few other checks that penalize unorthodox behaviour (e.g. queen out very early).
After generating moves, a function is called to order them based on captures, promotions and MVV-LVA (most valuable victim - least valuable attacker).

The board is represented using bitboards of type unsigned long long (64 bits, 1 for each square). The main bitboards include 2 for each color of piece type, 12 in total, and 3 occupancy bitboards (white, black and both).

Lastly, I found that for my current setup, depth 6 + quiescence is a good balance between speed and accuracy. Ideally it should be 10, but since the number of moves analyzed increases exponentially with each ply added, 6 is the current ceiling for my engine to play at a reasonable pace. I have tested it with 8, it is indeed better but games take a long time. It has been studied that depth 10+ has deminishing returns, so there is really no point in striving for such result for a basic engine. Maybe in the future i'll try to further optimize the search and evaluation functions to achieve depth 10.

This is essentially the logic of most of the big functions/alogrithms used in my engine.

## RESOURCES

I used this great video as a template and base for my engine: https://www.youtube.com/watch?v=o-ySJ2EBarY

As mentioned before, the site I used to learn and implement these algorithms is: https://www.chessprogramming.org/Main_Page
