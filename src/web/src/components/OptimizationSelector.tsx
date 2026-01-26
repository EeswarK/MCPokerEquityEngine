import { AlgorithmType, OptimizationType } from "../types";
import { Checkbox } from "./ui/checkbox";
import { Label } from "./ui/label";
import { Input } from "./ui/input";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "./ui/tooltip";
import { Badge } from "./ui/badge";

interface OptimizationSelectorProps {
  algorithm: AlgorithmType;
  optimizations: OptimizationType[];
  onChange: (optimizations: OptimizationType[]) => void;
  numWorkers?: number;
  onWorkersChange?: (workers: number) => void;
  disabled?: boolean;
}

interface OptimizationInfo {
  id: OptimizationType;
  name: string;
  description: string;
  estimatedGain: string;
  requiresWorkerCount?: boolean;
}

const OPTIMIZATIONS: OptimizationInfo[] = [
  {
    id: "multithreading",
    name: "Multithreading",
    description:
      "Parallel processing across multiple CPU cores. Scales linearly with core count. Compatible with all algorithms.",
    estimatedGain: "2-8x",
    requiresWorkerCount: true,
  },
  {
    id: "simd",
    name: "SIMD (AVX2)",
    description:
      "Single Instruction Multiple Data vectorization using AVX2 instructions. Processes multiple hands simultaneously. Best with OMP_EVAL algorithm.",
    estimatedGain: "2-4x",
  },
  {
    id: "perfect_hash",
    name: "Perfect Hash",
    description:
      "Built-in perfect hash table for collision-free lookups. Automatically included in Cactus Kev and PH Evaluator algorithms.",
    estimatedGain: "Built-in",
  },
  {
    id: "prefetching",
    name: "Memory Prefetching",
    description:
      "CPU cache prefetch hints to reduce memory latency. Most effective with large lookup tables like Two Plus Two.",
    estimatedGain: "10-30%",
  },
];

// Compatibility matrix: which optimizations work with which algorithms
const OPTIMIZATION_COMPATIBILITY: Record<
  AlgorithmType,
  OptimizationType[]
> = {
  naive: ["multithreading"],
  cactus_kev: ["multithreading", "perfect_hash", "prefetching"],
  ph_evaluator: ["multithreading", "perfect_hash"],
  two_plus_two: ["multithreading", "prefetching"],
  omp_eval: ["multithreading", "simd"],
};

// Mutual exclusivity: SIMD and PERFECT_HASH cannot be used together
const MUTUALLY_EXCLUSIVE_GROUPS: OptimizationType[][] = [
  ["simd", "perfect_hash"],
];

export const OptimizationSelector: React.FC<OptimizationSelectorProps> = ({
  algorithm,
  optimizations,
  onChange,
  numWorkers,
  onWorkersChange,
  disabled = false,
}) => {
  const compatibleOptimizations = OPTIMIZATION_COMPATIBILITY[algorithm] || [];

  const isCompatible = (opt: OptimizationType): boolean => {
    return compatibleOptimizations.includes(opt);
  };

  const isBuiltIn = (opt: OptimizationType): boolean => {
    // PERFECT_HASH is built into CACTUS_KEV and PH_EVALUATOR
    return opt === "perfect_hash" &&
           (algorithm === "cactus_kev" || algorithm === "ph_evaluator");
  };

  const isMutuallyExclusive = (opt: OptimizationType): boolean => {
    // Check if this optimization conflicts with any currently selected ones
    for (const group of MUTUALLY_EXCLUSIVE_GROUPS) {
      if (group.includes(opt)) {
        // If any other option in this group is selected, this is mutually exclusive
        const otherSelected = group.find(
          (o) => o !== opt && optimizations.includes(o)
        );
        if (otherSelected) return true;
      }
    }
    return false;
  };

  const handleToggle = (opt: OptimizationType, checked: boolean) => {
    if (checked) {
      // When enabling, remove any mutually exclusive options
      const newOptimizations = optimizations.filter((o) => {
        for (const group of MUTUALLY_EXCLUSIVE_GROUPS) {
          if (group.includes(opt) && group.includes(o) && o !== opt) {
            return false; // Remove conflicting option
          }
        }
        return true;
      });
      onChange([...newOptimizations, opt]);
    } else {
      onChange(optimizations.filter((o) => o !== opt));
    }
  };

  const showMultithreadingWorkers =
    optimizations.includes("multithreading") &&
    numWorkers !== undefined &&
    onWorkersChange;

  return (
    <div className="optimization-selector space-y-4">
      <fieldset disabled={disabled} className="space-y-4">
        <legend className="text-sm font-medium mb-2">
          Optimizations
          <span className="text-xs text-muted-foreground ml-2">
            (Available for {algorithm})
          </span>
        </legend>

        <div className="space-y-3">
          {OPTIMIZATIONS.map((opt) => {
            const compatible = isCompatible(opt.id);
            const builtIn = isBuiltIn(opt.id);
            const mutuallyExclusive = isMutuallyExclusive(opt.id);
            const isChecked = optimizations.includes(opt.id) || builtIn;
            const isDisabled =
              disabled || !compatible || mutuallyExclusive || builtIn;

            return (
              <TooltipProvider key={opt.id}>
                <Tooltip>
                  <TooltipTrigger asChild>
                    <div className="flex items-center justify-between space-x-3 p-3 rounded-lg border hover:bg-accent transition-colors">
                      <div className="flex items-center space-x-3 flex-1">
                        <Checkbox
                          id={opt.id}
                          checked={isChecked}
                          onCheckedChange={(checked) =>
                            handleToggle(opt.id, checked as boolean)
                          }
                          disabled={isDisabled}
                        />
                        <div className="flex flex-col">
                          <Label
                            htmlFor={opt.id}
                            className={
                              isDisabled && !builtIn
                                ? "cursor-not-allowed opacity-50"
                                : "cursor-pointer"
                            }
                          >
                            {opt.name}
                          </Label>
                          {!compatible && (
                            <span className="text-xs text-muted-foreground">
                              Not compatible with {algorithm}
                            </span>
                          )}
                          {builtIn && (
                            <span className="text-xs text-green-600 dark:text-green-400">
                              Built-in to this algorithm
                            </span>
                          )}
                          {mutuallyExclusive && compatible && (
                            <span className="text-xs text-yellow-600 dark:text-yellow-400">
                              Conflicts with selected optimization
                            </span>
                          )}
                        </div>
                      </div>
                      <Badge variant="secondary">{opt.estimatedGain}</Badge>
                    </div>
                  </TooltipTrigger>
                  <TooltipContent side="right" className="max-w-sm">
                    <p>{opt.description}</p>
                  </TooltipContent>
                </Tooltip>
              </TooltipProvider>
            );
          })}
        </div>

        {showMultithreadingWorkers && (
          <div className="workers-input space-y-2 pl-9">
            <Label htmlFor="workers" className="text-sm">
              Number of Worker Threads
            </Label>
            <Input
              id="workers"
              type="number"
              min="1"
              max="32"
              value={numWorkers}
              onChange={(e) =>
                onWorkersChange!(parseInt(e.target.value) || 1)
              }
              disabled={disabled}
              className="w-32"
            />
            <p className="text-xs text-muted-foreground">
              Recommended: {navigator.hardwareConcurrency || 4} threads (based on your CPU)
            </p>
          </div>
        )}
      </fieldset>
    </div>
  );
};
