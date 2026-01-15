import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from "recharts";
import { PerformanceMetrics } from "../api/contract";
import {
  Card,
  CardContent,
  CardHeader,
  CardTitle,
} from "./ui/card";

interface MetricsProps {
  metrics: PerformanceMetrics;
  history: Array<{ time: number; value: number }>;
}

export const Metrics: React.FC<MetricsProps> = ({ metrics, history }) => {
  const peakSimsPerSec = history.length > 0
    ? Math.max(...history.map((h) => h.value))
    : metrics.simulations_per_second;

  const avgSimsPerSec = history.length > 0
    ? history.reduce((sum, h) => sum + h.value, 0) / history.length
    : metrics.simulations_per_second;

  return (
    <div className="space-y-4">
      <Card>
        <CardHeader>
          <CardTitle>Performance Metrics</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="grid grid-cols-2 md:grid-cols-3 gap-4">
            <div className="space-y-1">
              <p className="text-sm text-muted-foreground">Mode:</p>
              <p className="text-sm font-medium">{metrics.mode.replace("_", " ")}</p>
            </div>
            <div className="space-y-1">
              <p className="text-sm text-muted-foreground">Duration:</p>
              <p className="text-sm font-medium">{metrics.duration_seconds.toFixed(2)}s</p>
            </div>
            <div className="space-y-1">
              <p className="text-sm text-muted-foreground">Current Sims/sec:</p>
              <p className="text-sm font-medium">
                {Math.floor(metrics.simulations_per_second).toLocaleString()}
              </p>
            </div>
            <div className="space-y-1">
              <p className="text-sm text-muted-foreground">Peak Sims/sec:</p>
              <p className="text-sm font-medium">
                {Math.floor(peakSimsPerSec).toLocaleString()}
              </p>
            </div>
            <div className="space-y-1">
              <p className="text-sm text-muted-foreground">Avg Sims/sec:</p>
              <p className="text-sm font-medium">
                {Math.floor(avgSimsPerSec).toLocaleString()}
              </p>
            </div>
            {metrics.cpu_percent !== undefined && (
              <div className="space-y-1">
                <p className="text-sm text-muted-foreground">CPU:</p>
                <p className="text-sm font-medium">{metrics.cpu_percent.toFixed(1)}%</p>
              </div>
            )}
            {metrics.memory_mb !== undefined && (
              <div className="space-y-1">
                <p className="text-sm text-muted-foreground">Memory:</p>
                <p className="text-sm font-medium">{metrics.memory_mb.toFixed(0)} MB</p>
              </div>
            )}
            {metrics.num_workers !== undefined && (
              <div className="space-y-1">
                <p className="text-sm text-muted-foreground">Workers:</p>
                <p className="text-sm font-medium">{metrics.num_workers}</p>
              </div>
            )}
            {metrics.cpu_cycles !== undefined && metrics.cpu_cycles > 0 && (
              <div className="space-y-1">
                <p className="text-sm text-muted-foreground">CPU Cycles:</p>
                <p className="text-sm font-medium">
                  {metrics.cpu_cycles.toLocaleString()}
                </p>
              </div>
            )}
          </div>
        </CardContent>
      </Card>

      {history.length > 0 && (
        <Card>
          <CardHeader>
            <CardTitle>Simulations per Second Over Time</CardTitle>
          </CardHeader>
          <CardContent>
            <ResponsiveContainer width="100%" height={200}>
              <LineChart data={history}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis
                  dataKey="time"
                  label={{ value: "Sample #", position: "insideBottom", offset: -5 }}
                  interval="preserveStartEnd"
                  tickCount={10}
                />
                <YAxis
                  label={{ value: "Sims/sec", angle: -90, position: "insideLeft" }}
                />
                <Tooltip />
                <Legend />
                <Line
                  type="monotone"
                  dataKey="value"
                  stroke="#8884d8"
                  name="Simulations/sec"
                  dot={false}
                />
              </LineChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>
      )}
    </div>
  );
};
