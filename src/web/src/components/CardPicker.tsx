import { useState } from "react";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "./ui/select";
import { Card } from "../types";

interface CardPickerProps {
  value: Card | null;
  onChange: (card: Card) => void;
  excludeCards: Card[];
  label: string;
}

const RANKS = [
  { label: "2", value: 2 },
  { label: "3", value: 3 },
  { label: "4", value: 4 },
  { label: "5", value: 5 },
  { label: "6", value: 6 },
  { label: "7", value: 7 },
  { label: "8", value: 8 },
  { label: "9", value: 9 },
  { label: "T", value: 10 },
  { label: "J", value: 11 },
  { label: "Q", value: 12 },
  { label: "K", value: 13 },
  { label: "A", value: 14 },
];

const SUITS = [
  { label: "♥", value: 0 },
  { label: "♦", value: 1 },
  { label: "♣", value: 2 },
  { label: "♠", value: 3 },
];

export const CardPicker: React.FC<CardPickerProps> = ({
  value,
  onChange,
  excludeCards,
  label,
}) => {
  const [rank, setRank] = useState<number | null>(value?.rank || null);
  const [suit, setSuit] = useState<number | null>(value?.suit || null);

  const handleRankChange = (newRank: string) => {
    const rankValue = parseInt(newRank);
    setRank(rankValue);
    if (suit !== null) {
      onChange({ rank: rankValue, suit });
    }
  };

  const handleSuitChange = (newSuit: string) => {
    const suitValue = parseInt(newSuit);
    setSuit(suitValue);
    if (rank !== null) {
      onChange({ rank, suit: suitValue });
    }
  };

  const isCardExcluded = (r: number, s: number) =>
    excludeCards.some((c) => c.rank === r && c.suit === s);

  // Generate card image path
  const getCardImagePath = (r: number, s: number): string => {
    const rankNames: Record<number, string> = {
      2: "two", 3: "three", 4: "four", 5: "five", 6: "six",
      7: "seven", 8: "eight", 9: "nine", 10: "ten",
      11: "jack", 12: "queen", 13: "king", 14: "ace"
    };
    const suitNames = ["hearts", "diamonds", "clubs", "spades"];
    return `/playing-cards/${rankNames[r]}-of-${suitNames[s]}.png`;
  };

  return (
    <div className="space-y-2">
      <label className="text-sm font-medium">{label}</label>
      <div className="flex gap-2 items-center">
        <Select
          value={rank?.toString()}
          onValueChange={handleRankChange}
        >
          <SelectTrigger className="w-20">
            <SelectValue placeholder="Rank" />
          </SelectTrigger>
          <SelectContent>
            {RANKS.map((r) => (
              <SelectItem key={r.value} value={r.value.toString()}>
                {r.label}
              </SelectItem>
            ))}
          </SelectContent>
        </Select>

        <Select
          value={suit?.toString()}
          onValueChange={handleSuitChange}
        >
          <SelectTrigger className="w-20">
            <SelectValue placeholder="Suit" />
          </SelectTrigger>
          <SelectContent>
            {SUITS.map((s) => (
              <SelectItem
                key={s.value}
                value={s.value.toString()}
                disabled={rank !== null && isCardExcluded(rank, s.value)}
              >
                {s.label}
              </SelectItem>
            ))}
          </SelectContent>
        </Select>

        {/* Card image */}
        {rank !== null && suit !== null && (
          <img
            src={getCardImagePath(rank, suit)}
            alt={`${RANKS.find((r) => r.value === rank)?.label}${SUITS.find((s) => s.value === suit)?.label}`}
            className="h-20 w-auto rounded shadow-md"
          />
        )}
      </div>
    </div>
  );
};
