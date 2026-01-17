from typing import Sequence

class Card:
    """
    32-bit integer card representation for fast evaluation.

    Card structure:
                          bitrank     suit rank   prime
                    +--------+--------+--------+--------+
                    |xxxbbbbb|bbbbbbbb|cdhsrrrr|xxpppppp|
                    +--------+--------+--------+--------+

    - p: prime number of rank (2→2, 3→3, ..., A→41)
    - r: rank index (2→0, 3→1, ..., A→12)
    - cdhs: suit bitfield (one bit per suit)
    - b: rank bitfield (one bit per rank for flush/straight detection)
    - x: unused
    """

    STR_RANKS: str = '23456789TJQKA'
    STR_SUITS: str = 'shdc'
    INT_RANKS: range = range(13)
    PRIMES: list[int] = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41]

    # Suit mapping: s=1(spades), h=2(hearts), d=4(diamonds), c=8(clubs)
    CHAR_SUIT_TO_INT_SUIT: dict[str, int] = {
        's': 1, 'h': 2, 'd': 4, 'c': 8,
    }
    INT_SUIT_TO_CHAR_SUIT: str = 'xshxxdxxc'  # Crude mapping, or just use dict

    @staticmethod
    def new(string: str) -> int:
        """Convert card string (e.g., 'As', 'Kh') to integer."""
        rank_char = string[0]
        suit_char = string[1]
        rank_int = Card.STR_RANKS.index(rank_char)
        suit_int = Card.CHAR_SUIT_TO_INT_SUIT[suit_char]
        rank_prime = Card.PRIMES[rank_int]

        bitrank = 1 << rank_int << 16
        suit = suit_int << 12
        rank = rank_int << 8

        return bitrank | suit | rank | rank_prime

    @staticmethod
    def prime_product_from_hand(card_ints: Sequence[int]) -> int:
        """Calculate prime product of hand for lookup."""
        product = 1
        for c in card_ints:
            product *= (c & 0xFF)
        return product

    @staticmethod
    def prime_product_from_rankbits(rankbits: int) -> int:
        """Calculate prime product from rank bitfield."""
        product = 1
        for i in Card.INT_RANKS:
            if rankbits & (1 << i):
                product *= Card.PRIMES[i]
        return product
    
    @staticmethod
    def get_rank_int(card_int: int) -> int:
        """Get rank index (0-12) from card integer."""
        return (card_int >> 8) & 0xF

    @staticmethod
    def get_suit_int(card_int: int) -> int:
        """Get suit int (1, 2, 4, 8) from card integer."""
        return (card_int >> 12) & 0xF
    
    @staticmethod
    def get_bitrank_int(card_int: int) -> int:
        """Get bitrank from card integer."""
        return (card_int >> 16) & 0x1FFF
    
    @staticmethod
    def get_prime(card_int: int) -> int:
        """Get prime number from card integer."""
        return card_int & 0x3F

    @staticmethod
    def hand_to_binary(card_strs: list[str]) -> list[int]:
        """Convert list of card strings to list of integers."""
        return [Card.new(c) for c in card_strs]

    @staticmethod
    def int_to_str(card_int: int) -> str:
        """Convert integer to card string."""
        rank_int = Card.get_rank_int(card_int)
        suit_int = Card.get_suit_int(card_int)
        
        suit_char = ''
        if suit_int == 1: suit_char = 's'
        elif suit_int == 2: suit_char = 'h'
        elif suit_int == 4: suit_char = 'd'
        elif suit_int == 8: suit_char = 'c'
        
        return Card.STR_RANKS[rank_int] + suit_char