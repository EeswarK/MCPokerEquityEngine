import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "./ui/table";

interface HeatmapProps {
  data: Record<string, number>; // hand_name -> equity (0.0 to 1.0)
  sampleCounts: Record<string, number>; // hand_name -> simulation count
  heroHand?: string; // Hero's hand classification (e.g., "TT", "AKs")
  isLoading?: boolean;
}

// Reversed: Aces at top (index 0), Deuces at bottom (index 12)
const RANKS = ["A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"];

export const Heatmap: React.FC<HeatmapProps> = ({
  data,
  sampleCounts,
  heroHand,
  isLoading = false,
}) => {
  const getHandData = (
    rank1: string,
    rank2: string,
    suited: boolean
  ): { equity: number; sampleCount: number; handName: string } => {
    const handName =
      rank1 === rank2
        ? `${rank1}${rank1}`
        : `${rank1}${rank2}${suited ? "s" : "o"}`;
    return {
      equity: data[handName] ?? 0,
      sampleCount: sampleCounts[handName] ?? 0,
      handName,
    };
  };

  const getColor = (equity: number, sampleCount: number): string => {
    if (equity === 0) return "#f0f0f0";

    // Diverging palette: Blue (0%) → Gray (50%) → Green (100%)
    let r, g, b;

    if (equity < 0.5) {
      // 0.0 to 0.5: Blue to Gray
      const t = equity * 2; // normalize to 0-1
      r = Math.floor(33 + t * (128 - 33));    // 33 (dark blue) to 128 (gray)
      g = Math.floor(102 + t * (128 - 102));  // 102 to 128
      b = Math.floor(172 + t * (128 - 172));  // 172 to 128
    } else {
      // 0.5 to 1.0: Gray to Green
      const t = (equity - 0.5) * 2; // normalize to 0-1
      r = Math.floor(128 - t * (128 - 34));   // 128 to 34 (dark green)
      g = Math.floor(128 + t * (139 - 128));  // 128 to 139
      b = Math.floor(128 - t * (128 - 34));   // 128 to 34
    }

    // Calculate opacity based on sample count (burn-in effect)
    const targetSampleSize = 1000;
    const opacity = Math.min(sampleCount / targetSampleSize, 1.0);

    return `rgba(${r}, ${g}, ${b}, ${opacity})`;
  };

  // Calculate standard error for a proportion
  const getStandardError = (equity: number, sampleCount: number): number => {
    if (sampleCount === 0) return 0;
    // SE = sqrt(p * (1 - p) / n)
    return Math.sqrt((equity * (1 - equity)) / sampleCount);
  };

  if (isLoading && Object.keys(data).length === 0) {
    return <div className="text-center py-8 text-muted-foreground">Loading heatmap...</div>;
  }

  return (
    <div className="overflow-x-auto">
      <Table>
        <TableHeader>
          <TableRow>
            <TableHead></TableHead>
            {RANKS.map((rank) => (
              <TableHead key={rank} className="text-center">
                {rank}
              </TableHead>
            ))}
          </TableRow>
        </TableHeader>
        <TableBody>
          {RANKS.map((rank1, i) => (
            <TableRow key={rank1}>
              <TableHead className="font-medium">{rank1}</TableHead>
              {RANKS.map((rank2, j) => {
                const isPair = i === j;
                const handData = isPair
                  ? getHandData(rank1, rank2, false)
                  : i < j
                    ? getHandData(rank1, rank2, true) // suited (above diagonal)
                    : getHandData(rank2, rank1, false); // offsuit (below diagonal)

                const { equity, sampleCount, handName } = handData;
                const isHeroCell = heroHand && handName === heroHand;
                const standardError = getStandardError(equity, sampleCount);
                const marginOfError = standardError * 1.96 * 100; // 95% confidence interval

                // Enhanced tooltip
                const tooltipText = equity > 0
                  ? `${handName}: ${(equity * 100).toFixed(2)}% equity\n` +
                    `Simulations: ${sampleCount.toLocaleString()}\n` +
                    `Margin of Error: ±${marginOfError.toFixed(2)}%`
                  : `${handName}: No data`;

                return (
                  <TableCell
                    key={`${rank1}-${rank2}`}
                    className="text-center relative"
                    style={{
                      backgroundColor: getColor(equity, sampleCount),
                      color: equity > 0.5 ? "#fff" : "#000",
                      border: isHeroCell ? "3px solid #ff6b00" : undefined,
                      boxShadow: isHeroCell ? "0 0 8px rgba(255, 107, 0, 0.6)" : undefined,
                    }}
                    title={tooltipText}
                  >
                    {equity > 0 ? `${(equity * 100).toFixed(0)}%` : "-"}
                  </TableCell>
                );
              })}
            </TableRow>
          ))}
        </TableBody>
      </Table>

      {/* Color Scale Legend */}
      <div className="mt-4 space-y-3">
        <div>
          <div className="text-sm font-medium mb-2">Equity Scale</div>
          <div className="flex items-center gap-2">
            <span className="text-xs">0%</span>
            <div className="flex-1 h-6 rounded" style={{
              background: 'linear-gradient(to right, rgb(33, 102, 172), rgb(128, 128, 128), rgb(34, 139, 34))'
            }}>
            </div>
            <span className="text-xs">100%</span>
          </div>
          <div className="text-xs text-muted-foreground mt-1">
            Opacity indicates confidence (darker = more simulations)
          </div>
        </div>

        {/* Suited/Offsuit Legend */}
        <div className="text-xs text-muted-foreground">
          <div className="font-medium mb-1">Hand Types:</div>
          <div className="space-y-0.5">
            <div>• <span className="font-medium">Above diagonal</span> (upper-right): Suited hands (e.g., AKs)</div>
            <div>• <span className="font-medium">On diagonal</span>: Pocket pairs (e.g., AA, KK)</div>
            <div>• <span className="font-medium">Below diagonal</span> (lower-left): Offsuit hands (e.g., AKo)</div>
          </div>
        </div>
      </div>
    </div>
  );
};
