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
      setHoveredCategory(payload.category);
      setHoveredMatrix(payload.matrix);
    }
  };

  const handleBarLeave = () => {
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

          {hoveredMatrix && (
            <div>
              <h3 className="text-sm font-medium mb-2">
                Win-Method Matrix: {hoveredCategory}
              </h3>
              <WinMethodMatrixHeatmap matrix={hoveredMatrix} />
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

const WinMethodMatrixHeatmap: React.FC<{ matrix: number[][] }> = ({ matrix }) => {
  const maxValue = Math.max(...matrix.flat());

  return (
    <div className="overflow-auto">
      <table className="text-xs border-collapse">
        <thead>
          <tr>
            <th className="border p-1"></th>
            {HAND_TYPE_NAMES.map((name, idx) => (
              <th key={idx} className="border p-1 text-center" title={name}>
                {name.split(" ")[0].slice(0, 3)}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {matrix.map((row, ourType) => (
            <tr key={ourType}>
              <td className="border p-1 font-medium" title={HAND_TYPE_NAMES[ourType]}>
                {HAND_TYPE_NAMES[ourType].split(" ")[0].slice(0, 3)}
              </td>
              {row.map((count, oppType) => {
                const intensity = maxValue > 0 ? count / maxValue : 0;
                const bgColor = `rgba(59, 130, 246, ${intensity})`;
                return (
                  <td
                    key={oppType}
                    className="border p-1 text-center"
                    style={{ backgroundColor: bgColor }}
                    title={`Our: ${HAND_TYPE_NAMES[ourType]} vs Opp: ${HAND_TYPE_NAMES[oppType]}: ${count}`}
                  >
                    {count > 0 ? count : ""}
                  </td>
                );
              })}
            </tr>
          ))}
        </tbody>
      </table>
      <div className="mt-2 text-xs text-muted-foreground">
        <p>Rows: Your final hand | Columns: Opponent final hand</p>
      </div>
    </div>
  );
};
