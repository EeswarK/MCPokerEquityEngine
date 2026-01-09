import { JobStatus } from "../types";
import { Badge } from "./ui/badge";
import { Progress } from "./ui/progress";
import { Alert, AlertDescription, AlertTitle } from "./ui/alert";

interface JobStatusProps {
  status: JobStatus;
  progress: number;
  error?: string;
}

export const JobStatusDisplay: React.FC<JobStatusProps> = ({
  status,
  progress,
  error,
}) => {
  const getStatusVariant = (): "default" | "secondary" | "destructive" => {
    switch (status) {
      case "pending":
        return "secondary";
      case "running":
        return "default";
      case "completed":
        return "default";
      case "failed":
        return "destructive";
      default:
        return "secondary";
    }
  };

  return (
    <div className="job-status space-y-2">
      <div className="status-header flex items-center gap-2">
        <Badge variant={getStatusVariant()}>
          {status.toUpperCase()}
        </Badge>
        {status === "running" && (
          <span className="text-sm text-muted-foreground">
            {Math.floor(progress * 100)}%
          </span>
        )}
      </div>

      {status === "running" && (
        <Progress value={progress * 100} className="w-full" />
      )}

      {error && (
        <Alert variant="destructive">
          <AlertTitle>Error</AlertTitle>
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}
    </div>
  );
};
