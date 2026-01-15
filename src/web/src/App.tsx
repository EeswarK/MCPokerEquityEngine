import { useState, useEffect } from "react";
import { ModeSelector } from "./components/ModeSelector";
import { Heatmap } from "./components/Heatmap";
import { Metrics } from "./components/Metrics";
import { JobStatusDisplay } from "./components/JobStatus";
import { ThemeToggle } from "./components/ThemeToggle";
import { useJob } from "./hooks/useJob";
import { EngineMode } from "./types";
import { Button } from "./components/ui/button";
import { Card, CardContent, CardHeader, CardTitle } from "./components/ui/card";

function App() {
  const [mode, setMode] = useState<EngineMode>("base_python");
  const [numWorkers, setNumWorkers] = useState(4);
  const { jobId, submitJob, resetJob, loading, error, telemetry, connected, telemetryConnected } =
    useJob();

  const handleSubmit = async () => {
    // Mock range spec for now
    const rangeSpec = {
      AA: [
        { rank: 14, suit: 0 },
        { rank: 14, suit: 1 },
      ],
      KK: [
        { rank: 13, suit: 0 },
        { rank: 13, suit: 1 },
      ],
      QQ: [
        { rank: 12, suit: 0 },
        { rank: 12, suit: 1 },
      ],
      // Add more hands as needed
    };

    await submitJob({
      range_spec: rangeSpec,
      num_opponents: 1,
      num_simulations: 100000,
      mode,
      num_workers:
        mode === "multiprocessing" || mode === "cpp_threaded"
          ? numWorkers
          : undefined,
    });
  };

  // State for history accumulation
  const [metricsHistory, setMetricsHistory] = useState<
    Array<{ time: number; value: number }>
  >([]);

  // Accumulate history when telemetry updates
  useEffect(() => {
    if (telemetry && telemetry.metrics.simulations_per_second > 0) {
      setMetricsHistory((prev) => [
        ...prev,
        {
          time: prev.length,  // Sequential index for x-axis
          value: telemetry.metrics.simulations_per_second,
        },
      ]);
    }
  }, [telemetry?.timestamp]);  // Trigger on new telemetry

  // Reset history when job resets
  useEffect(() => {
    if (!jobId) {
      setMetricsHistory([]);
    }
  }, [jobId]);

  return (
    <div className="mx-auto max-w-[1400px] p-8">
      <ThemeToggle />
      <header className="mb-8 text-center">
        <h1 className="mb-2 text-3xl font-bold">Poker Equity Engine</h1>
        <p className="text-muted-foreground">
          Compare performance across different implementation strategies
        </p>
      </header>

      <main className="space-y-6">
        <Card>
          <CardHeader>
            <CardTitle>Simulation Controls</CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <ModeSelector
              mode={mode}
              onChange={setMode}
              numWorkers={numWorkers}
              onWorkersChange={setNumWorkers}
              disabled={loading || !!jobId}
            />

            <div className="flex gap-2">
              <Button onClick={handleSubmit} disabled={loading || !!jobId}>
                {loading ? "Starting..." : "Run Simulation"}
              </Button>
              {jobId && (
                <Button onClick={resetJob} disabled={loading} variant="outline">
                  Cancel
                </Button>
              )}
            </div>

            {jobId && (
              <JobStatusDisplay
                status={telemetry?.status || "pending"}
                progress={telemetry?.progress || 0}
                error={
                  error || telemetry?.status === "failed"
                    ? "Job failed"
                    : undefined
                }
              />
            )}

            {connected && (
              <div className="text-sm text-green-600 dark:text-green-400">
                Connected (Job Management)
              </div>
            )}
            {telemetryConnected && (
              <div className="text-sm text-green-600 dark:text-green-400">
                Connected (Telemetry)
              </div>
            )}
            {!telemetryConnected && jobId && (
              <div className="text-sm text-yellow-600 dark:text-yellow-400">
                Telemetry connection pending...
              </div>
            )}
          </CardContent>
        </Card>

        {telemetry && (
          <>
            <Card>
              <CardHeader>
                <CardTitle>Equity Heatmap</CardTitle>
              </CardHeader>
              <CardContent>
                <Heatmap
                  data={telemetry.current_results}
                  isLoading={telemetry.status === "running"}
                />
              </CardContent>
            </Card>

            <Metrics metrics={telemetry.metrics} history={metricsHistory} />
          </>
        )}
      </main>
    </div>
  );
}

export default App;
