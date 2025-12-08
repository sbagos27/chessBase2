# chess-base-main
please dont break 
it can build now.

I tried my best to follow along with the lectures, I added and used Bitboard.h. I also added Magicbitboard from Monday and used it for king moves

I added screenshots in the Screenshots file.

I added all the generator methods to generate the moves for pawns kings and knights. For some reason knights and kings cant capture so thats something ill have to look into. May I need to validate or do a check but i can move all pieces to open spaces.

my pawns can capture funny

lets say 

0000000p
P0000000

p can capture P from across the board

AI Implementations:

describing your challenges
The AI can make weird moves, when testing I saw a rook disappear. Also i think it's looking at the bitboard diagnoly. 
Like A8 could move to H7 type of deal. 
With all the code provided and lectures it was easier to make the piece movements ofcourse. 
Also making sure the AI uses the "future-seeing" board instead of the actual board to make moves was crucial.
Just lots of moving parts, conceptually I think I understand it but implementing is very difficult.

depth achieved,
I kept it depth 5, and even then it still freezes a bit anything more it crashes. I notices depth 3 is kinda the same but dumber.

How well your AI plays
The AI reminds me of that one aggressive chess.com AI Nelson??? because it brings out the bishop and queen asap
But the AI is very aggressive but easy to counter 

Video Link:
https://www.youtube.com/watch?v=ehWrYshd4gc