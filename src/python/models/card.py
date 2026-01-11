from dataclasses import dataclass
from typing import Dict, List


@dataclass
class Card:
    rank: int
    suit: int

    def __post_init__(self):
        if not (2 <= self.rank <= 14):
            raise ValueError(f"Rank must be between 2 and 14, got {self.rank}")
        if not (0 <= self.suit <= 3):
            raise ValueError(f"Suit must be between 0 and 3, got {self.suit}")

    def to_dict(self) -> Dict[str, int]:
        return {"rank": self.rank, "suit": self.suit}

    @classmethod
    def from_dict(cls, data: Dict[str, int]) -> "Card":
        return cls(rank=data["rank"], suit=data["suit"])


HandRange = Dict[str, List[Card]]
