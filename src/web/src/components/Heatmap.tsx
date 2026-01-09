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
  isLoading?: boolean;
}

const RANKS = ["2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"];

export const Heatmap: React.FC<HeatmapProps> = ({
  data,
  isLoading = false,
}) => {
  const getEquity = (rank1: string, rank2: string, suited: boolean): number => {
    const handName =
      rank1 === rank2
        ? `${rank1}${rank1}`
        : `${rank1}${rank2}${suited ? "s" : "o"}`;
    return data[handName] ?? 0;
  };

  const getColor = (equity: number): string => {
    if (equity === 0) return "#f0f0f0";
    const intensity = Math.floor(equity * 255);
    return `rgb(${255 - intensity}, ${intensity}, 0)`;
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
                const equity = isPair
                  ? getEquity(rank1, rank2, false)
                  : i > j
                    ? getEquity(rank1, rank2, true) // suited
                    : getEquity(rank2, rank1, false); // offsuit

                return (
                  <TableCell
                    key={`${rank1}-${rank2}`}
                    className="text-center"
                    style={{
                      backgroundColor: getColor(equity),
                      color: equity > 0.5 ? "#fff" : "#000",
                    }}
                    title={`${rank1}${rank2}${
                      isPair ? "" : i > j ? "s" : "o"
                    }: ${(equity * 100).toFixed(1)}%`}
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
