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
              <p className="text-sm text-muted-foreground">Simulations/sec:</p>
              <p className="text-sm font-medium">
                {Math.floor(metrics.simulations_per_second).toLocaleString()}
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
                <XAxis dataKey="time" />
                <YAxis />
                <Tooltip />
                <Legend />
                <Line
                  type="monotone"
                  dataKey="value"
                  stroke="#8884d8"
                  name="Simulations/sec"
                />
              </LineChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>
      )}
    </div>
  );
};
