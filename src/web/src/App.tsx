import { useState, useEffect } from "react";
import { ModeSelector } from "./components/ModeSelector";
import { Heatmap } from "./components/Heatmap";
import { Metrics } from "./components/Metrics";
import { JobStatusDisplay } from "./components/JobStatus";
import { ThemeToggle } from "./components/ThemeToggle";
import { CardPicker } from "./components/CardPicker";
import { WinMethodChart } from "./components/WinMethodChart";
import { EquityCategoryChart } from "./components/EquityCategoryChart";
import { SimulationCountSelector } from "./components/SimulationCountSelector";
import { useJob } from "./hooks/useJob";
import { EngineMode, Card as CardType } from "./types";
import { Button } from "./components/ui/button";
import { Card, CardContent, CardHeader, CardTitle } from "./components/ui/card";

function App() {
  const [mode, setMode] = useState<EngineMode>("base_python");
  const [numWorkers, setNumWorkers] = useState(4);
  const [numSimulations, setNumSimulations] = useState(100000);
  const [card1, setCard1] = useState<CardType | null>(null);
  const [card2, setCard2] = useState<CardType | null>(null);
  const {
    jobId,
    submitJob,
    resetJob,
    loading,
    error,
    telemetry,
    connected,
    telemetryConnected,
  } = useJob();

  // Classify user's cards into standard notation (AA, AKs, AKo, etc.)
  const classifyHoleCards = (c1: CardType, c2: CardType): string => {
    const rankToChar = (rank: number) => {
      if (rank <= 10) return rank.toString();
      const map: Record<number, string> = {
        11: "J",
        12: "Q",
        13: "K",
        14: "A",
      };
      return map[rank] || rank.toString();
    };

    // Pocket pair
    if (c1.rank === c2.rank) {
      return `${rankToChar(c1.rank)}${rankToChar(c1.rank)}`;
    }

    // Sort by rank (higher first)
    const highCard = c1.rank > c2.rank ? c1 : c2;
    const lowCard = c1.rank > c2.rank ? c2 : c1;

    // Suited or offsuit
    const suited = c1.suit === c2.suit;
    const suffix = suited ? "s" : "o";

    return `${rankToChar(highCard.rank)}${rankToChar(lowCard.rank)}${suffix}`;
  };

  const handleSubmit = async () => {
    if (!card1 || !card2) {
      alert("Please select both hole cards");
      return;
    }

    // Generate hand name (e.g., "AsKh" for specific cards)
    const rankToChar = (rank: number) => {
      if (rank <= 10) return rank.toString();
      const map: Record<number, string> = {
        11: "J",
        12: "Q",
        13: "K",
        14: "A",
      };
      return map[rank] || rank.toString();
    };

    const suitToChar = (suit: number) => ["h", "d", "c", "s"][suit] || "?";

    const handName = `${rankToChar(card1.rank)}${suitToChar(card1.suit)}${rankToChar(card2.rank)}${suitToChar(card2.suit)}`;

    const rangeSpec = {
      [handName]: [card1, card2],
    };

    await submitJob({
      range_spec: rangeSpec,
      num_opponents: 1,
      num_simulations: numSimulations,
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
      console.log("[App] Telemetry received:", {
        status: telemetry.status,
        simsPerSec: telemetry.metrics.simulations_per_second,
        hasCurrentResults: Object.keys(telemetry.current_results).length > 0,
        hasSampleCounts: Object.keys(telemetry.sample_counts).length > 0,
        hasWinMethodMatrices:
          Object.keys(telemetry.win_method_matrices || {}).length > 0,
      });

      // Filter out first few data points if they're unreasonably high (warm-up artifacts)
      // Typical values should be in the 1k-100k range for most hardware
      const isReasonableValue =
        telemetry.metrics.simulations_per_second < 500000;

      if (isReasonableValue) {
        setMetricsHistory((prev) => [
          ...prev,
          {
            time: prev.length, // Sequential index for x-axis
            value: telemetry.metrics.simulations_per_second,
          },
        ]);
      }
    }
  }, [telemetry, telemetry?.timestamp]); // Trigger on new telemetry

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
            <div className="space-y-4">
              <div className="grid grid-cols-1 gap-4 md:grid-cols-2">
                <CardPicker
                  label="Card 1"
                  value={card1}
                  onChange={setCard1}
                  excludeCards={card2 ? [card2] : []}
                />
                <CardPicker
                  label="Card 2"
                  value={card2}
                  onChange={setCard2}
                  excludeCards={card1 ? [card1] : []}
                />
              </div>
            </div>

            <ModeSelector
              mode={mode}
              onChange={setMode}
              numWorkers={numWorkers}
              onWorkersChange={setNumWorkers}
              disabled={loading || !!jobId}
            />

            <SimulationCountSelector
              value={numSimulations}
              onChange={setNumSimulations}
              disabled={loading || !!jobId}
            />

            <div className="flex gap-2">
              <Button
                onClick={handleSubmit}
                disabled={loading || !!jobId || !card1 || !card2}
              >
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
                  sampleCounts={telemetry.sample_counts}
                  heroHand={
                    card1 && card2 ? classifyHoleCards(card1, card2) : undefined
                  }
                  isLoading={telemetry.status === "running"}
                />
              </CardContent>
            </Card>

            {telemetry.win_method_matrices &&
              Object.keys(telemetry.win_method_matrices).length > 0 && (
                <EquityCategoryChart
                  equityData={telemetry.current_results}
                  winMethodMatrices={telemetry.win_method_matrices}
                />
              )}

            {telemetry.win_method_matrices &&
            Object.keys(telemetry.win_method_matrices).length > 0 ? (
              <WinMethodChart
                winMethodMatrix={
                  Object.values(telemetry.win_method_matrices)[0]
                }
              />
            ) : (
              <div className="text-muted-foreground text-sm">
                No win method data yet (matrices:{" "}
                {JSON.stringify(
                  Object.keys(telemetry.win_method_matrices || {}),
                )}
                )
              </div>
            )}

            <Metrics metrics={telemetry.metrics} history={metricsHistory} />
          </>
        )}
      </main>
    </div>
  );
}

export default App;
