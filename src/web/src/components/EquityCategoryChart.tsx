import { useState, useMemo } from "react";
import {
  BarChart,
  Bar,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip as RechartsTooltip,
  Legend,
  ResponsiveContainer,
  Cell,
} from "recharts";
import {
  Card,
  CardContent,
  CardHeader,
  CardTitle,
} from "./ui/card";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "./ui/tooltip";

interface EquityCategoryChartProps {
  equityData: Record<string, number>;
  winMethodMatrices: Record<string, number[][]>;
  lossMethodMatrices: Record<string, number[][]>;
}

const HAND_TYPE_NAMES = [
  "High Card",
  "One Pair",
  "Two Pair",
  "Three of a Kind",
  "Straight",
  "Flush",
  "Full House",
  "Four of a Kind",
  "Straight Flush",
  "Royal Flush",
];

// Standard poker abbreviations (fixes duplicate "Str" bug)
const HAND_TYPE_ABBREVIATIONS = [
  "HC",  // High Card
  "1P",  // One Pair
  "2P",  // Two Pair
  "3K",  // Three of a Kind
  "ST",  // Straight
  "FL",  // Flush
  "FH",  // Full House
  "4K",  // Four of a Kind
  "SF",  // Straight Flush
  "RF",  // Royal Flush
];

// Categorize hands
const categorizeHand = (handName: string): string => {
  if (handName.length === 2 && handName[0] === handName[1]) {
    return "Pocket Pairs";
  }

  const rank1 = handName[0];
  const rank2 = handName[1];
  const suited = handName.endsWith("s");

  const broadway = ["T", "J", "Q", "K", "A"];
  if (broadway.includes(rank1) && broadway.includes(rank2)) {
    return suited ? "Broadway Suited" : "Broadway Offsuit";
  }

  // Check for suited connectors (within 1-2 ranks)
  const rankValues: Record<string, number> = {
    "2": 2, "3": 3, "4": 4, "5": 5, "6": 6, "7": 7, "8": 8, "9": 9,
    "T": 10, "J": 11, "Q": 12, "K": 13, "A": 14,
  };
  const diff = Math.abs(rankValues[rank1] - rankValues[rank2]);
  if (suited && diff <= 2 && diff > 0) {
    return "Suited Connectors";
  }

  return "Other";
};

export const EquityCategoryChart: React.FC<EquityCategoryChartProps> = ({
  equityData,
  winMethodMatrices,
  lossMethodMatrices,
}) => {
  const [hoveredCategory, setHoveredCategory] = useState<string | null>(null);

  // Aggregate equity by category
  const chartData = useMemo(() => {
    const categories = new Map<string, {
      totalEquity: number;
      count: number;
      winMatrix: number[][];
      lossMatrix: number[][];
    }>();

    for (const [handName, equity] of Object.entries(equityData)) {
      const category = categorizeHand(handName);
      const winMatrix = winMethodMatrices[handName] || Array(10).fill(null).map(() => Array(10).fill(0));
      const lossMatrix = lossMethodMatrices[handName] || Array(10).fill(null).map(() => Array(10).fill(0));

      if (!categories.has(category)) {
        categories.set(category, {
          totalEquity: 0,
          count: 0,
          winMatrix: Array(10).fill(null).map(() => Array(10).fill(0)),
          lossMatrix: Array(10).fill(null).map(() => Array(10).fill(0)),
        });
      }

      const cat = categories.get(category)!;
      cat.totalEquity += equity;
      cat.count += 1;

      // Aggregate matrices
      for (let i = 0; i < 10; i++) {
        for (let j = 0; j < 10; j++) {
          cat.winMatrix[i][j] += winMatrix[i][j];
          cat.lossMatrix[i][j] += lossMatrix[i][j];
        }
      }
    }

    return Array.from(categories.entries()).map(([category, data]) => ({
      category,
      equity: (data.totalEquity / data.count) * 100,
      winMatrix: data.winMatrix,
      lossMatrix: data.lossMatrix,
    }));
  }, [equityData, winMethodMatrices, lossMethodMatrices]);

  const handleBarHover = (data: any) => {
    if (data && data.activePayload && data.activePayload[0]) {
      const payload = data.activePayload[0].payload;
      setHoveredCategory(payload.category);
    }
  };

  const handleBarLeave = () => {
    // Optional: clear on leave, but sometimes nice to keep last hovered
    // setHoveredCategory(null);
  };

  // Derive active data directly from chartData and hoveredCategory (no extra state/effect)
  const activeData = useMemo(() => {
    if (!hoveredCategory) return null;
    return chartData.find(d => d.category === hoveredCategory) || null;
  }, [hoveredCategory, chartData]);

  return (
    <Card>
      <CardHeader>
        <CardTitle className="flex items-center justify-between">
          <span>Equity by Opponent Category</span>
        </CardTitle>
      </CardHeader>
      <CardContent>
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
          <ResponsiveContainer width="100%" height={300}>
            <BarChart
              data={chartData}
              layout="vertical"
              onMouseMove={handleBarHover}
              onMouseLeave={handleBarLeave}
            >
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis type="number" domain={[0, 100]} />
              <YAxis dataKey="category" type="category" width={150} />
              <RechartsTooltip content={<CustomTooltip />} />
              <Legend />
              <Bar dataKey="equity" fill="#8884d8" name="Equity %">
                {chartData.map((entry, index) => (
                  <Cell
                    key={`cell-${index}`}
                    fill={hoveredCategory === entry.category ? "#ff7300" : "#8884d8"}
                  />
                ))}
              </Bar>
            </BarChart>
          </ResponsiveContainer>

          {activeData && (
            <div key={activeData.category}>
              <h3 className="text-sm font-medium mb-2 flex items-center gap-2">
                Detailed Matchup Matrix: {activeData.category}
                <span className="text-xs font-normal text-muted-foreground ml-auto">
                  Total Outcomes: {(activeData.winMatrix.flat().reduce((a, b) => a + b, 0) + activeData.lossMatrix.flat().reduce((a, b) => a + b, 0)).toLocaleString()}
                </span>
              </h3>
              <UnifiedMatrixHeatmap
                winMatrix={activeData.winMatrix}
                lossMatrix={activeData.lossMatrix}
                category={activeData.category}
              />
            </div>
          )}
        </div>
      </CardContent>
    </Card>
  );
};

const CustomTooltip = ({ active, payload }: any) => {
  if (active && payload && payload.length) {
    return (
      <div className="bg-background border border-border p-2 rounded shadow-lg">
        <p className="text-sm font-medium">{payload[0].payload.category}</p>
        <p className="text-sm">Equity: {payload[0].value.toFixed(1)}%</p>
        <p className="text-xs text-muted-foreground mt-1">Hover for detailed matchup matrix</p>
      </div>
    );
  }
  return null;
};

const UnifiedMatrixHeatmap: React.FC<{
  winMatrix: number[][];
  lossMatrix: number[][];
  category: string;
}> = ({ winMatrix, lossMatrix, category }) => {
  // Calculate total outcomes for the entire matrix to normalize percentages
  // Note: This ignores ties that are not spatially mapped in the matrix
  // If we wanted to be perfectly accurate to "Total Simulations", we'd need that number passed down
  // But calculating percentage of "Decisive Outcomes" (Wins+Losses) is a good proxy for visualization
  const totalCount = winMatrix.flat().reduce((sum, val) => sum + val, 0) + 
                     lossMatrix.flat().reduce((sum, val) => sum + val, 0);

  // Determine max cell value for color intensity scaling
  let maxCellValue = 0;
  for (let i = 0; i < 10; i++) {
    for (let j = 0; j < 10; j++) {
      // winMatrix[i][j] (We Win)
      // lossMatrix[j][i] (Opponent Wins, note index swap from parsing if needed, but in our parsing we did:
      // matrix[opp_type][our_type] = lossMatrixFlat...
      // So lossMatrix is currently [OpponentType][OurType]
      // To get losses where We have i and Opp has j, we need lossMatrix[j][i]
      
      const wins = winMatrix[i][j];
      const losses = lossMatrix[j][i]; 
      const cellTotal = wins + losses;
      if (cellTotal > maxCellValue) maxCellValue = cellTotal;
    }
  }

  return (
    <div className="overflow-auto border rounded-md p-2 bg-card">
      <table className="text-xs border-collapse w-full">
        <thead>
          <tr>
            <th className="border p-1 bg-muted/50"></th>
            {HAND_TYPE_ABBREVIATIONS.map((abbr, idx) => (
              <th key={idx} className="border p-1 text-center bg-muted/50" title={`Opponent: ${HAND_TYPE_NAMES[idx]}`}>
                {abbr}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {winMatrix.map((row, rowIdx) => (
            <tr key={rowIdx}>
              <td className="border p-1 font-medium bg-muted/50" title={`You: ${HAND_TYPE_NAMES[rowIdx]}`}>
                {HAND_TYPE_ABBREVIATIONS[rowIdx]}
              </td>
              {row.map((_, colIdx) => {
                // winMatrix[row][col] -> We have row, Opp has col. We Win.
                // lossMatrix[col][row] -> Opp has col, We have row. Opp Wins (We Lose).
                
                const wins = winMatrix[rowIdx][colIdx];
                const losses = lossMatrix[colIdx][rowIdx];
                const cellTotal = wins + losses;
                
                const percentage = totalCount > 0 ? (cellTotal / totalCount) * 100 : 0;
                const winPct = cellTotal > 0 ? (wins / cellTotal) * 100 : 0;
                const lossPct = cellTotal > 0 ? (losses / cellTotal) * 100 : 0;

                const intensity = maxCellValue > 0 ? cellTotal / maxCellValue : 0;
                const minAlpha = 0.1;
                const alpha = minAlpha + (intensity * (1 - minAlpha));

                let bgColor;
                
                // Color Logic:
                // Mostly Wins (>60%) -> Green
                // Mostly Losses (>60%) -> Red
                // Mixed (40-60%) -> Blue/Purple/Orange (Contested)
                
                if (cellTotal === 0) {
                    bgColor = `rgba(200, 200, 200, 0.05)`; // Empty
                } else if (winPct > 60) {
                    // Green scaled by intensity (how often this matchup happens)
                    bgColor = `rgba(34, 197, 94, ${alpha})`; 
                } else if (lossPct > 60) {
                    // Red scaled by intensity
                    bgColor = `rgba(239, 68, 68, ${alpha})`;
                } else {
                    // Mixed/Contested
                    bgColor = `rgba(59, 130, 246, ${alpha})`; // Blue
                }
                
                // Special border for diagonal
                const isDiagonal = rowIdx === colIdx;
                const borderStyle = isDiagonal ? "2px solid #f59e0b" : undefined; // Orange border for kicker battles

                return (
                  <TooltipProvider key={colIdx}>
                    <Tooltip>
                      <TooltipTrigger asChild>
                        <td
                          className="border p-1 text-center transition-colors hover:ring-2 hover:ring-primary z-10 relative cursor-help"
                          style={{
                            backgroundColor: bgColor,
                            border: borderStyle
                          }}
                        >
                          {percentage >= 1.0 ? `${percentage.toFixed(0)}%` : (percentage > 0 ? "<1%" : "")}
                        </td>
                      </TooltipTrigger>
                      <TooltipContent>
                        <div className="text-sm">
                          <p className="font-semibold mb-1">Matchup Analysis</p>
                          <p>You: <span className="font-medium">{HAND_TYPE_NAMES[rowIdx]}</span></p>
                          <p>Opponent: <span className="font-medium">{HAND_TYPE_NAMES[colIdx]}</span></p>
                          <div className="my-2 border-t border-border/50" />
                          <p>Frequency: <span className="font-medium">{percentage.toFixed(2)}%</span></p>
                          <p>Wins: <span className="text-green-500 font-medium">{wins.toLocaleString()}</span> ({winPct.toFixed(1)}%)</p>
                          <p>Losses: <span className="text-red-500 font-medium">{losses.toLocaleString()}</span> ({lossPct.toFixed(1)}%)</p>
                        </div>
                      </TooltipContent>
                    </Tooltip>
                  </TooltipProvider>
                );
              })}
            </tr>
          ))}
        </tbody>
      </table>
      <div className="mt-2 text-xs text-muted-foreground space-y-1">
        <p className="font-medium">Rows: Your Hand | Columns: Opponent Hand</p>
        <div className="flex flex-wrap gap-2">
           <span className="flex items-center"><span className="w-3 h-3 bg-green-500 mr-1 rounded-sm"></span>You Win (&gt;60%)</span>
           <span className="flex items-center"><span className="w-3 h-3 bg-red-500 mr-1 rounded-sm"></span>Opponent Wins (&gt;60%)</span>
           <span className="flex items-center"><span className="w-3 h-3 bg-blue-500 mr-1 rounded-sm"></span>Contested (~50/50)</span>
           <span className="flex items-center"><span className="w-3 h-3 border-2 border-orange-500 mr-1 rounded-sm"></span>Diagonal (Kicker Battles)</span>
        </div>
      </div>
    </div>
  );
};
