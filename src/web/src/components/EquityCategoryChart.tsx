import { useState } from "react";
import {
  BarChart,
  Bar,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
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

interface EquityCategoryChartProps {
  equityData: Record<string, number>;
  winMethodMatrices: Record<string, number[][]>;
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
}) => {
  const [hoveredCategory, setHoveredCategory] = useState<string | null>(null);
  const [hoveredMatrix, setHoveredMatrix] = useState<number[][] | null>(null);

  // Aggregate equity by category
  const categories = new Map<string, { totalEquity: number; count: number; matrix: number[][] }>();

  for (const [handName, equity] of Object.entries(equityData)) {
    const category = categorizeHand(handName);
    const matrix = winMethodMatrices[handName] || Array(10).fill(null).map(() => Array(10).fill(0));

    if (!categories.has(category)) {
      categories.set(category, {
        totalEquity: 0,
        count: 0,
        matrix: Array(10).fill(null).map(() => Array(10).fill(0)),
      });
    }

    const cat = categories.get(category)!;
    cat.totalEquity += equity;
    cat.count += 1;

    // Aggregate matrices
    for (let i = 0; i < 10; i++) {
      for (let j = 0; j < 10; j++) {
        cat.matrix[i][j] += matrix[i][j];
      }
    }
  }

  const chartData = Array.from(categories.entries()).map(([category, data]) => ({
    category,
    equity: (data.totalEquity / data.count) * 100,
    matrix: data.matrix,
  }));

  const handleBarHover = (data: any) => {
    if (data && data.activePayload && data.activePayload[0]) {
      const payload = data.activePayload[0].payload;
      console.log("[EquityCategoryChart] Bar hover:", payload.category, "Matrix sum:", payload.matrix.flat().reduce((a: number, b: number) => a + b, 0));
      setHoveredCategory(payload.category);
      setHoveredMatrix(payload.matrix);
    }
  };

  const handleBarLeave = () => {
    console.log("[EquityCategoryChart] Bar leave");
    setHoveredCategory(null);
    setHoveredMatrix(null);
  };

  return (
    <Card>
      <CardHeader>
        <CardTitle>Equity by Opponent Category</CardTitle>
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
              <Tooltip content={<CustomTooltip />} />
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

          {hoveredMatrix && hoveredCategory && (
            <div key={hoveredCategory}>
              <h3 className="text-sm font-medium mb-2">
                Win-Method Matrix: {hoveredCategory}
              </h3>
              <WinMethodMatrixHeatmap
                matrix={hoveredMatrix}
                category={hoveredCategory}
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
        <p className="text-xs text-muted-foreground mt-1">Hover for win-method details</p>
      </div>
    );
  }
  return null;
};

const WinMethodMatrixHeatmap: React.FC<{
  matrix: number[][];
  category: string;
}> = ({ matrix, category }) => {
  console.log("[WinMethodMatrixHeatmap] Rendering for category:", category, "Total count:", matrix.flat().reduce((sum, val) => sum + val, 0));

  const maxValue = Math.max(...matrix.flat());
  const totalCount = matrix.flat().reduce((sum, val) => sum + val, 0);

  return (
    <div className="overflow-auto">
      <table className="text-xs border-collapse">
        <thead>
          <tr>
            <th className="border p-1"></th>
            {HAND_TYPE_ABBREVIATIONS.map((abbr, idx) => (
              <th key={idx} className="border p-1 text-center" title={HAND_TYPE_NAMES[idx]}>
                {abbr}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {matrix.map((row, ourType) => (
            <tr key={ourType}>
              <td className="border p-1 font-medium" title={HAND_TYPE_NAMES[ourType]}>
                {HAND_TYPE_ABBREVIATIONS[ourType]}
              </td>
              {row.map((count, oppType) => {
                const intensity = maxValue > 0 ? count / maxValue : 0;
                const percentage = totalCount > 0 ? (count / totalCount) * 100 : 0;

                // Determine cell styling based on diagonal position
                const isAboveDiagonal = ourType < oppType; // Opponent rank is higher (losses)
                const isOnDiagonal = ourType === oppType;  // Same rank (kicker battles)
                const isBelowDiagonal = ourType > oppType; // Your rank is higher (clear wins)

                // Apply heatmap color (green for clear wins, blue for kicker battles)
                const bgColor = isBelowDiagonal
                  ? `rgba(34, 197, 94, ${intensity})` // Green for clear wins
                  : isOnDiagonal
                    ? `rgba(59, 130, 246, ${intensity})` // Blue for kicker battles
                    : `rgba(200, 200, 200, 0.2)`; // Light gray for empty cells (no data)

                // Create narrative tooltip
                const narrative = count > 0
                  ? `You win ${percentage.toFixed(1)}% of the time by making ${HAND_TYPE_NAMES[ourType]} against their ${HAND_TYPE_NAMES[oppType]}`
                  : isAboveDiagonal
                    ? `Cannot win with ${HAND_TYPE_NAMES[ourType]} against opponent's ${HAND_TYPE_NAMES[oppType]} (only tracks wins)`
                    : "No outcomes in this category";

                return (
                  <td
                    key={oppType}
                    className={`border p-1 text-center ${
                      isAboveDiagonal ? "opacity-60" : ""
                    }`}
                    style={{
                      backgroundColor: bgColor,
                      borderRight: oppType === ourType - 1 ? "2px solid #6b7280" : undefined,
                      borderBottom: ourType === oppType ? "2px solid #6b7280" : undefined,
                    }}
                    title={narrative}
                  >
                    {count > 0 ? `${percentage.toFixed(1)}%` : ""}
                  </td>
                );
              })}
            </tr>
          ))}
        </tbody>
      </table>
      <div className="mt-2 text-xs text-muted-foreground space-y-1">
        <p className="font-medium">Rows: Your final hand | Columns: Opponent final hand</p>
        <p>
          <span className="inline-block w-3 h-3 bg-green-500 mr-1"></span>
          Clear Wins (your rank &gt; opponent) |
          <span className="inline-block w-3 h-3 bg-blue-500 mx-1"></span>
          Kicker Battles (same rank)
        </p>
        <p className="italic mt-1">
          Note: This matrix only tracks hands where you WIN. Cells above the diagonal
          (opponent rank &gt; yours) are empty because you cannot win with a lower-ranked hand.
          Tracking loss details would require backend changes to capture a loss_method_matrix.
        </p>
      </div>
    </div>
  );
};
