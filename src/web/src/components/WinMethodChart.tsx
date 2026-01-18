import {
  BarChart,
  Bar,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from "recharts";
import {
  Card,
  CardContent,
  CardHeader,
  CardTitle,
} from "./ui/card";

interface WinMethodChartProps {
  winMethodMatrix: number[][]; // 10x10 matrix [our_type][opp_type]
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

const HAND_TYPE_SHORT = [
  "High",
  "Pair",
  "2 Pair",
  "Trips",
  "Str",
  "Flush",
  "Full",
  "Quads",
  "SF",
  "Royal",
];

export const WinMethodChart: React.FC<WinMethodChartProps> = ({
  winMethodMatrix,
}) => {
  // Aggregate wins by hand type (sum across all opponent types)
  const aggregatedData = HAND_TYPE_NAMES.map((name, ourType) => {
    const totalWins = winMethodMatrix[ourType].reduce(
      (sum, count) => sum + count,
      0
    );
    return {
      handType: HAND_TYPE_SHORT[ourType],
      wins: totalWins,
      fullName: name,
    };
  }).filter((d) => d.wins > 0); // Only show hand types we actually made

  if (aggregatedData.length === 0) {
    return (
      <Card>
        <CardHeader>
          <CardTitle>Win Method Breakdown</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="text-center py-8 text-muted-foreground">
            No win data available yet...
          </div>
        </CardContent>
      </Card>
    );
  }

  return (
    <Card>
      <CardHeader>
        <CardTitle>Win Method Breakdown</CardTitle>
        <p className="text-sm text-muted-foreground">
          Frequency of hand types when winning
        </p>
      </CardHeader>
      <CardContent>
        <ResponsiveContainer width="100%" height={300}>
          <BarChart data={aggregatedData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis
              dataKey="handType"
              angle={-45}
              textAnchor="end"
              height={80}
            />
            <YAxis label={{ value: "Win Count", angle: -90, position: "insideLeft" }} />
            <Tooltip
              content={({ active, payload }) => {
                if (active && payload && payload.length) {
                  const data = payload[0].payload;
                  return (
                    <div className="bg-background border border-border p-2 rounded shadow-lg">
                      <p className="font-semibold">{data.fullName}</p>
                      <p className="text-sm">Wins: {data.wins.toLocaleString()}</p>
                    </div>
                  );
                }
                return null;
              }}
            />
            <Legend />
            <Bar dataKey="wins" fill="#8884d8" name="Win Count" />
          </BarChart>
        </ResponsiveContainer>
      </CardContent>
    </Card>
  );
};
