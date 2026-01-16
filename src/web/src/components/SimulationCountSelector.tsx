import { useState, useEffect } from "react";
import { Slider } from "./ui/slider";
import { Input } from "./ui/input";
import { Label } from "./ui/label";

interface SimulationCountSelectorProps {
  value: number;
  onChange: (value: number) => void;
  disabled?: boolean;
}

const MIN_SIMULATIONS = 10000;
const MAX_SIMULATIONS = 100000000;

const formatNumber = (num: number): string => {
  if (num >= 1000000) {
    return `${(num / 1000000).toFixed(1)}M`;
  }
  if (num >= 1000) {
    return `${(num / 1000).toFixed(1)}K`;
  }
  return num.toString();
};

export const SimulationCountSelector: React.FC<SimulationCountSelectorProps> = ({
  value,
  onChange,
  disabled = false,
}) => {
  const [inputValue, setInputValue] = useState(value.toString());
  const [isValid, setIsValid] = useState(true);

  useEffect(() => {
    setInputValue(value.toString());
  }, [value]);

  const handleSliderChange = (newValue: number[]) => {
    const val = newValue[0];
    onChange(val);
    setInputValue(val.toString());
    setIsValid(true);
  };

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const inputVal = e.target.value;
    setInputValue(inputVal);

    if (inputVal === "") {
      setIsValid(false);
      return;
    }

    const numValue = parseInt(inputVal.replace(/,/g, ""), 10);

    if (isNaN(numValue)) {
      setIsValid(false);
      return;
    }

    if (numValue < MIN_SIMULATIONS || numValue > MAX_SIMULATIONS) {
      setIsValid(false);
      return;
    }

    setIsValid(true);
    onChange(numValue);
  };

  const handleInputBlur = () => {
    if (!isValid || inputValue === "") {
      setInputValue(value.toString());
      setIsValid(true);
      return;
    }

    const numValue = parseInt(inputValue.replace(/,/g, ""), 10);
    if (!isNaN(numValue)) {
      const clampedValue = Math.max(
        MIN_SIMULATIONS,
        Math.min(MAX_SIMULATIONS, numValue)
      );
      onChange(clampedValue);
      setInputValue(clampedValue.toString());
    }
  };

  const sliderValue = Math.max(
    MIN_SIMULATIONS,
    Math.min(MAX_SIMULATIONS, value)
  );

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between">
        <Label htmlFor="simulations">Number of Simulations</Label>
        <span className="text-sm text-muted-foreground">
          {formatNumber(value)}
        </span>
      </div>
      <div className="space-y-2">
        <Slider
          value={[sliderValue]}
          onValueChange={handleSliderChange}
          min={MIN_SIMULATIONS}
          max={MAX_SIMULATIONS}
          step={10000}
          disabled={disabled}
          className="w-full"
        />
        <div className="flex items-center gap-2">
          <Input
            id="simulations"
            type="text"
            value={inputValue}
            onChange={handleInputChange}
            onBlur={handleInputBlur}
            disabled={disabled}
            aria-invalid={!isValid}
            className="w-32"
            placeholder="100000"
          />
          <span className="text-xs text-muted-foreground">
            ({MIN_SIMULATIONS.toLocaleString()} - {MAX_SIMULATIONS.toLocaleString()})
          </span>
          {!isValid && (
            <span className="text-xs text-destructive">
              Invalid range
            </span>
          )}
        </div>
      </div>
    </div>
  );
};
