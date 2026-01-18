import { EngineMode } from "../types";
import { RadioGroup, RadioGroupItem } from "./ui/radio-group";
import { Label } from "./ui/label";
import { Input } from "./ui/input";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "./ui/tooltip";

interface ModeSelectorProps {
  mode: EngineMode;
  onChange: (mode: EngineMode) => void;
  numWorkers?: number;
  onWorkersChange?: (workers: number) => void;
  disabled?: boolean;
}

export const ModeSelector: React.FC<ModeSelectorProps> = ({
  mode,
  onChange,
  numWorkers,
  onWorkersChange,
  disabled = false,
}) => {
  const pythonModes: EngineMode[] = ["base_python", "senzee", "numpy", "multiprocessing"];
  const cppModes: EngineMode[] = ["cpp_base", "cpp_simd", "cpp_threaded"];

  const formatModeName = (m: string): string => {
    // Special cases for display names
    if (m === "base_python") return "Naive";
    if (m === "senzee") return "Cactus Kev's";
    if (m === "numpy") return "Numpy";
    if (m === "multiprocessing") return "Multiprocessing";

    // Fallback to title case
    return m.replace("_", " ").replace(/\b\w/g, (l) => l.toUpperCase());
  };

  const getModeTooltip = (m: string): string | null => {
    if (m === "senzee") {
      return "Fast evaluation using bitwise operations and perfect hash lookup table (Paul Senzee)";
    }
    return null;
  };

  return (
    <div className="mode-selector space-y-4">
      <fieldset disabled={disabled} className="space-y-4">
        <legend className="text-sm font-medium mb-4">Engine Mode</legend>

        <div className="mode-group space-y-3">
          <h3 className="text-sm font-semibold">Python Modes</h3>
          <RadioGroup value={mode} onValueChange={(value) => onChange(value as EngineMode)}>
            {pythonModes.map((m) => {
              const tooltip = getModeTooltip(m);
              const radioButton = (
                <div key={m} className="flex items-center space-x-2">
                  <RadioGroupItem value={m} id={m} disabled={disabled} />
                  <Label htmlFor={m} className="cursor-pointer">
                    {formatModeName(m)}
                  </Label>
                </div>
              );

              if (tooltip) {
                return (
                  <TooltipProvider key={m}>
                    <Tooltip>
                      <TooltipTrigger asChild>{radioButton}</TooltipTrigger>
                      <TooltipContent>
                        <p className="max-w-xs">{tooltip}</p>
                      </TooltipContent>
                    </Tooltip>
                  </TooltipProvider>
                );
              }

              return radioButton;
            })}
          </RadioGroup>
        </div>

        <div className="mode-group space-y-3">
          <h3 className="text-sm font-semibold">C++ Modes (Future)</h3>
          <RadioGroup value={mode} onValueChange={(value) => onChange(value as EngineMode)}>
            {cppModes.map((m) => (
              <div key={m} className="flex items-center space-x-2">
                <RadioGroupItem value={m} id={m} disabled />
                <Label htmlFor={m} className="cursor-not-allowed opacity-50">
                  {formatModeName(m.replace("cpp_", ""))}
                </Label>
              </div>
            ))}
          </RadioGroup>
        </div>

        {(mode === "multiprocessing" || mode === "cpp_threaded") &&
          numWorkers !== undefined &&
          onWorkersChange && (
            <div className="workers-input space-y-2">
              <Label htmlFor="workers">Number of Workers:</Label>
              <Input
                id="workers"
                type="number"
                min="1"
                max="32"
                value={numWorkers}
                onChange={(e) =>
                  onWorkersChange(parseInt(e.target.value) || 1)
                }
                disabled={disabled}
              />
            </div>
          )}
      </fieldset>
    </div>
  );
};