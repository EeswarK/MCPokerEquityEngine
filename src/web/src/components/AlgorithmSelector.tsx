import { AlgorithmType, ImplementationType } from "../types";
import { RadioGroup, RadioGroupItem } from "./ui/radio-group";
import { Label } from "./ui/label";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "./ui/tooltip";
import { Badge } from "./ui/badge";

interface AlgorithmSelectorProps {
  algorithm: AlgorithmType;
  onChange: (algorithm: AlgorithmType) => void;
  implementation: ImplementationType;
  disabled?: boolean;
}

interface AlgorithmInfo {
  id: AlgorithmType;
  name: string;
  description: string;
  status: "stable" | "experimental" | "beta";
  performanceLevel: "baseline" | "fast" | "very-fast";
  supportedIn: ImplementationType[];
}

const ALGORITHMS: AlgorithmInfo[] = [
  {
    id: "naive",
    name: "Naive",
    description: "Brute-force evaluation checking all 5-card combinations. Slowest but most straightforward implementation. Good for testing and verification.",
    status: "stable",
    performanceLevel: "baseline",
    supportedIn: ["python", "cpp"],
  },
  {
    id: "cactus_kev",
    name: "Cactus Kev",
    description: "Fast evaluation using bitwise operations and perfect hash lookup table by Paul Senzee. Excellent balance of speed and memory usage.",
    status: "stable",
    performanceLevel: "fast",
    supportedIn: ["python", "cpp"],
  },
  {
    id: "ph_evaluator",
    name: "Perfect Hash",
    description: "Uses perfect hashing for fast lookup-based evaluation. Optimized for minimal collisions and cache-friendly access patterns.",
    status: "beta",
    performanceLevel: "fast",
    supportedIn: ["cpp"],
  },
  {
    id: "two_plus_two",
    name: "Two Plus Two",
    description: "Classic algorithm with large precomputed lookup table (~128MB). Very fast evaluation at the cost of memory footprint.",
    status: "stable",
    performanceLevel: "very-fast",
    supportedIn: ["cpp"],
  },
  {
    id: "omp_eval",
    name: "OMP Eval",
    description: "Optimized for modern hardware with SIMD support. Best performance when combined with SIMD optimization on AVX2-capable CPUs.",
    status: "experimental",
    performanceLevel: "very-fast",
    supportedIn: ["cpp"],
  },
];

const getStatusColor = (status: AlgorithmInfo["status"]) => {
  switch (status) {
    case "stable":
      return "default";
    case "beta":
      return "secondary";
    case "experimental":
      return "outline";
  }
};

const getPerformanceBadge = (level: AlgorithmInfo["performanceLevel"]) => {
  switch (level) {
    case "baseline":
      return <Badge variant="outline">Baseline</Badge>;
    case "fast":
      return <Badge variant="default">Fast</Badge>;
    case "very-fast":
      return <Badge variant="default">Very Fast</Badge>;
  }
};

export const AlgorithmSelector: React.FC<AlgorithmSelectorProps> = ({
  algorithm,
  onChange,
  implementation,
  disabled = false,
}) => {
  // Filter algorithms by implementation support
  const availableAlgorithms = ALGORITHMS.filter((algo) =>
    algo.supportedIn.includes(implementation)
  );

  return (
    <div className="algorithm-selector space-y-4">
      <fieldset disabled={disabled} className="space-y-4">
        <legend className="text-sm font-medium mb-2">
          Core Algorithm ({implementation === "python" ? "Python" : "C++"})
        </legend>
        <RadioGroup
          value={algorithm}
          onValueChange={(value) => onChange(value as AlgorithmType)}
        >
          <div className="space-y-3">
            {availableAlgorithms.map((algo) => {
              const isExperimentalOrBeta = algo.status !== "stable";

              return (
                <TooltipProvider key={algo.id}>
                  <Tooltip>
                    <TooltipTrigger asChild>
                      <div className="flex items-center justify-between space-x-3 p-3 rounded-lg border hover:bg-accent transition-colors">
                        <div className="flex items-center space-x-3 flex-1">
                          <RadioGroupItem
                            value={algo.id}
                            id={algo.id}
                            disabled={disabled || isExperimentalOrBeta}
                          />
                          <div className="flex flex-col">
                            <Label
                              htmlFor={algo.id}
                              className={
                                isExperimentalOrBeta
                                  ? "cursor-not-allowed opacity-50"
                                  : "cursor-pointer"
                              }
                            >
                              {algo.name}
                            </Label>
                            {isExperimentalOrBeta && (
                              <span className="text-xs text-muted-foreground">
                                Coming soon
                              </span>
                            )}
                          </div>
                        </div>
                        <div className="flex items-center gap-2">
                          {getPerformanceBadge(algo.performanceLevel)}
                          <Badge variant={getStatusColor(algo.status)}>
                            {algo.status}
                          </Badge>
                        </div>
                      </div>
                    </TooltipTrigger>
                    <TooltipContent side="right" className="max-w-sm">
                      <p>{algo.description}</p>
                    </TooltipContent>
                  </Tooltip>
                </TooltipProvider>
              );
            })}
          </div>
        </RadioGroup>
      </fieldset>
    </div>
  );
};
