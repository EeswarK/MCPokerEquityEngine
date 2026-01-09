import { useEffect, useState } from "react";
import { Moon, Sun, Menu } from "lucide-react";
import { Switch } from "./ui/switch";
import { Button } from "./ui/button";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuLabel,
  DropdownMenuTrigger,
} from "./ui/dropdown-menu";

export function ThemeToggle() {
  const [isDark, setIsDark] = useState(false);
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    setMounted(true);
    const isDarkMode =
      document.documentElement.classList.contains("dark") ||
      localStorage.theme === "dark" ||
      (!("theme" in localStorage) &&
        window.matchMedia("(prefers-color-scheme: dark)").matches);
    setIsDark(isDarkMode);
    if (isDarkMode) {
      document.documentElement.classList.add("dark");
    }
  }, []);

  const toggleTheme = (checked: boolean) => {
    setIsDark(checked);
    if (checked) {
      document.documentElement.classList.add("dark");
      localStorage.theme = "dark";
    } else {
      document.documentElement.classList.remove("dark");
      localStorage.theme = "light";
    }
  };

  if (!mounted) {
    return null;
  }

  return (
    <>
      <div className="fixed top-4 right-4 z-50 hidden md:block">
        <div className="bg-background/80 flex items-center gap-2 rounded-lg border p-2 shadow-lg backdrop-blur-sm">
          <Sun className="h-4 w-4" />
          <Switch checked={isDark} onCheckedChange={toggleTheme} />
          <Moon className="h-4 w-4" />
        </div>
      </div>

      <div className="fixed top-4 right-4 z-50 md:hidden">
        <DropdownMenu>
          <DropdownMenuTrigger asChild>
            <Button variant="outline" size="icon" className="rounded-full">
              <Menu className="h-5 w-5" />
            </Button>
          </DropdownMenuTrigger>
          <DropdownMenuContent
            align="end"
            className="z-[100] w-56"
            onCloseAutoFocus={(e) => e.preventDefault()}
          >
            <DropdownMenuLabel className="flex items-center justify-between px-2 py-3">
              <div className="flex items-center gap-2">
                <Sun className="h-4 w-4" />
                <span>Theme</span>
              </div>
              <div
                className="flex items-center gap-2"
                onClick={(e) => e.stopPropagation()}
              >
                <Switch checked={isDark} onCheckedChange={toggleTheme} />
                <Moon className="h-4 w-4" />
              </div>
            </DropdownMenuLabel>
          </DropdownMenuContent>
        </DropdownMenu>
      </div>
    </>
  );
}
