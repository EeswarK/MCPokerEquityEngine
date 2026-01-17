import itertools
from typing import Sequence, Iterator, List, Dict
from .card import Card


class LookupTable:
    """
    Pre-computed lookup tables for hand evaluation.

    Maps 5-card hand prime products to ranks (1-7462):
    - 1 = Royal Flush (best)
    - 7462 = 7-high (worst)

    Two tables:
    - flush_lookup: suited hands (flushes, straight flushes)
    - unsuited_lookup: non-flush hands (pairs, straights, etc.)
    """

    MAX_ROYAL_FLUSH: int = 1
    MAX_STRAIGHT_FLUSH: int = 10
    MAX_FOUR_OF_A_KIND: int = 166
    MAX_FULL_HOUSE: int = 322
    MAX_FLUSH: int = 1599
    MAX_STRAIGHT: int = 1609
    MAX_THREE_OF_A_KIND: int = 2467
    MAX_TWO_PAIR: int = 3325
    MAX_PAIR: int = 6185
    MAX_HIGH_CARD: int = 7462

    MAX_TO_RANK_CLASS: dict[int, int] = {
        MAX_ROYAL_FLUSH: 0,
        MAX_STRAIGHT_FLUSH: 1,
        MAX_FOUR_OF_A_KIND: 2,
        MAX_FULL_HOUSE: 3,
        MAX_FLUSH: 4,
        MAX_STRAIGHT: 5,
        MAX_THREE_OF_A_KIND: 6,
        MAX_TWO_PAIR: 7,
        MAX_PAIR: 8,
        MAX_HIGH_CARD: 9
    }

    RANK_CLASS_TO_STRING: dict[int, str] = {
        0: "Royal Flush",
        1: "Straight Flush",
        2: "Four of a Kind",
        3: "Full House",
        4: "Flush",
        5: "Straight",
        6: "Three of a Kind",
        7: "Two Pair",
        8: "Pair",
        9: "High Card"
    }

    def __init__(self) -> None:
        """Initialize lookup tables (~0.5s one-time cost)."""
        self.flush_lookup: dict[int, int] = {}
        self.unsuited_lookup: dict[int, int] = {}

        self.flushes()
        self.multiples()

    def flushes(self) -> None:
        """
        Straight flushes and flushes.
        
        Lookup is done on 5-card prime product.
        Rank bit patterns are used to generate prime products.
        """
        
        # Straight flushes
        # 0x1F00 (AKQJT) down to 0x1F (65432) ?? No
        # Rank bits: A=bit12, 2=bit0.
        # AKQJT = 11111 0000 0000 => 0x1F00 ? No 
        # A=12, K=11, Q=10, J=9, T=8. Bits 8-12. 
        # (1<<12)|(1<<11)|(1<<10)|(1<<9)|(1<<8) = 0x1F00. Correct.
        # Low straight: 5432A. A=bit12. 5432=bits3-0.
        # (1<<12)|(1<<3)|(1<<2)|(1<<1)|(1<<0) = 0x100F.
        
        straight_flushes = [
            0x1F00, # Royal
            0xF80,  # KQJT9
            0x7C0,  # QJT98
            0x3E0,  # JT987
            0x1F0,  # T9876
            0xF8,   # 98765
            0x7C,   # 87654
            0x3E,   # 76543
            0x1F,   # 65432
            0x100F  # 5432A
        ]
        
        # Map straight flushes
        rank = 1
        for sf in straight_flushes:
            prime_product = Card.prime_product_from_rankbits(sf)
            self.flush_lookup[prime_product] = rank
            rank += 1
            
        # Flushes (excluding straight flushes)
        rank = self.MAX_FULL_HOUSE + 1 # Start after Full House
        
        # Generate all 13 choose 5 bit patterns
        # We iterate backwards to assign better ranks to better hands
        # Using lexigraphic combinations of rank indices
        # reversed(list(itertools.combinations(range(13), 5))) gives:
        # (12, 11, 10, 9, 8) -> AKQJT
        # ...
        # (4, 3, 2, 1, 0) -> 65432
        
        for bits in self.get_lexographically_next_bit_sequence(list(range(13))):
            prime_product = Card.prime_product_from_rankbits(bits)
            
            # If not already in lookup (i.e. not a straight flush)
            if prime_product not in self.flush_lookup:
                self.flush_lookup[prime_product] = rank
                rank += 1

    def multiples(self) -> None:
        """
        Pairs, Trips, Quads, Full Houses, High Cards.
        
        Lookup is done on 5-card prime product.
        """
        # 1. Four of a Kind
        rank = self.MAX_STRAIGHT_FLUSH + 1
        
        # For each rank (quads)
        for i in reversed(range(13)):
            # For each kicker
            for j in reversed(range(13)):
                if i == j: continue
                product = (Card.PRIMES[i] ** 4) * Card.PRIMES[j]
                self.unsuited_lookup[product] = rank
                rank += 1
                
        # 2. Full House
        rank = self.MAX_FOUR_OF_A_KIND + 1
        for i in reversed(range(13)): # Trips part
            for j in reversed(range(13)): # Pair part
                if i == j: continue
                product = (Card.PRIMES[i] ** 3) * (Card.PRIMES[j] ** 2)
                self.unsuited_lookup[product] = rank
                rank += 1
                
        # 3. Straight
        rank = self.MAX_FLUSH + 1
        
        straight_flushes = [
            0x1F00, # Royal
            0xF80,  # KQJT9
            0x7C0,  # QJT98
            0x3E0,  # JT987
            0x1F0,  # T9876
            0xF8,   # 98765
            0x7C,   # 87654
            0x3E,   # 76543
            0x1F,   # 65432
            0x100F  # 5432A
        ]
        
        for sf in straight_flushes:
            product = Card.prime_product_from_rankbits(sf)
            self.unsuited_lookup[product] = rank
            rank += 1
            
        # 4. Three of a Kind
        rank = self.MAX_STRAIGHT + 1
        for i in reversed(range(13)):
            for j in reversed(range(13)):
                for k in reversed(range(j)):
                    if i == j or i == k: continue
                    product = (Card.PRIMES[i] ** 3) * Card.PRIMES[j] * Card.PRIMES[k]
                    self.unsuited_lookup[product] = rank
                    rank += 1
                    
        # 5. Two Pair
        rank = self.MAX_THREE_OF_A_KIND + 1
        for i in reversed(range(13)): # Top pair
            for j in reversed(range(i)): # Bottom pair
                for k in reversed(range(13)): # Kicker
                    if k == i or k == j: continue
                    product = (Card.PRIMES[i] ** 2) * (Card.PRIMES[j] ** 2) * Card.PRIMES[k]
                    self.unsuited_lookup[product] = rank
                    rank += 1
        
        # 6. One Pair
        rank = self.MAX_TWO_PAIR + 1
        for i in reversed(range(13)): # Pair
            for j in reversed(range(13)):
                for k in reversed(range(j)):
                    for l in reversed(range(k)):
                        if i == j or i == k or i == l: continue
                        product = (Card.PRIMES[i] ** 2) * Card.PRIMES[j] * Card.PRIMES[k] * Card.PRIMES[l]
                        self.unsuited_lookup[product] = rank
                        rank += 1
                        
        # 7. High Card
        rank = self.MAX_PAIR + 1
        for bits in self.get_lexographically_next_bit_sequence(list(range(13))):
            product = Card.prime_product_from_rankbits(bits)
            # If not already in lookup (i.e. not a straight)
            if product not in self.unsuited_lookup:
                self.unsuited_lookup[product] = rank
                rank += 1

    def get_lexographically_next_bit_sequence(self, bits: List[int]) -> Iterator[int]:
        """
        Generator for bit sequences of 5 set bits.
        Equivalent to iterating combinations of ranks.
        """
        for combo in itertools.combinations(reversed(bits), 5):
            rankbits = 0
            for bit in combo:
                rankbits |= (1 << bit)
            yield rankbits
