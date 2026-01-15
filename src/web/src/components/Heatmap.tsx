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
  isLoading?: boolean;
}

const RANKS = ["2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"];

export const Heatmap: React.FC<HeatmapProps> = ({
  data,
  sampleCounts,
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

    // Blue → Green → Orange/Yellow gradient
    let r, g, b;

    if (equity < 0.5) {
      // 0.0 to 0.5: Blue to Green
      const t = equity * 2; // normalize to 0-1
      r = Math.floor(0 + t * 0); // 0 to 0
      g = Math.floor(100 + t * 155); // 100 to 255
      b = Math.floor(255 - t * 255); // 255 to 0
    } else {
      // 0.5 to 1.0: Green to Orange/Yellow
      const t = (equity - 0.5) * 2; // normalize to 0-1
      r = Math.floor(0 + t * 255); // 0 to 255
      g = Math.floor(255 - t * 100); // 255 to 155
      b = 0;
    }

    // Calculate opacity based on sample count (burn-in effect)
    const targetSampleSize = 1000;
    const opacity = Math.min(sampleCount / targetSampleSize, 1.0);

    return `rgba(${r}, ${g}, ${b}, ${opacity})`;
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
                  : i > j
                    ? getHandData(rank1, rank2, true) // suited
                    : getHandData(rank2, rank1, false); // offsuit

                const { equity, sampleCount, handName } = handData;

                return (
                  <TableCell
                    key={`${rank1}-${rank2}`}
                    className="text-center"
                    style={{
                      backgroundColor: getColor(equity, sampleCount),
                      color: equity > 0.5 ? "#fff" : "#000",
                    }}
                    title={`${handName}: ${(equity * 100).toFixed(1)}% (${sampleCount} sims)`}
                  >
                    {equity > 0 ? `${(equity * 100).toFixed(0)}%` : "-"}
                  </TableCell>
                );
              })}
            </TableRow>
          ))}
        </TableBody>
      </Table>
    </div>
  );
};
